#include <string.h>
#include <stdlib.h>
#include "Z80.h"
#include "zx80rom.h"
#include "zx81x2rom.h"
#include "zx81rom.h"
#include "zx8x.h"
#include "emuapi.h"
#include "emusound.h"
#include "emuvideo.h"
#include "common.h"
#include "hid_app.h"
#include "hid_usb.h"
#include "menu.h"
#include "display.h"

char *strzx80_to_ascii(int memaddr);
bool parseNumber(const char* input,
                 unsigned int min,
                 unsigned int max,
                 char term,
                 unsigned int* out);

static void adjustChroma(bool start);

#define ERROR_D() mem[16384] = 12;
#define ERROR_INV1() mem[16384] = 128;
#define ERROR_INV2() mem[16384] = 129;
#define ERROR_INV3() mem[16384] = 130;

byte mem[MEMORYRAM_SIZE];
unsigned char *memptr[64];
int memattr[64];
int nmigen=0,hsyncgen=0,vsync=0;
int vsync_visuals=1;
int signal_int_flag=0;
int ramsize=16;

/* the keyboard state and other */
static byte keyboard[ 8 ] = {0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff};;
int zx80=0;
int autoload=1;
int chromamode=0;
unsigned char bordercolour=0x0f;
unsigned char bordercolournew=0x0f;

static bool resetRequired = false;

/* the ZX81 char is used to index into this, to give the ascii.
 * awkward chars are mapped to '_' (underscore), and the ZX81's
 * all-caps is converted to lowercase.
 * The mapping is also valid for the ZX80 for alphanumerics.
 * WARNING: this only covers 0<=char<=63!
 */
static char zx2ascii[64]={
/*  0- 9 */ ' ', '_', '_', '_', '_', '_', '_', '_', '_', '_',
/* 10-19 */ '_', '\'','#', '$', ':', '?', '(', ')', '>', '<',
/* 20-29 */ '=', '+', '-', '*', '/', ';', ',', '.', '0', '1',
/* 30-39 */ '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b',
/* 40-49 */ 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
/* 50-59 */ 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
/* 60-63 */ 'w', 'x', 'y', 'z'
};


#if 0
const byte map_qw[8][5] = {
    {25, 6,27,29,224},// vcxz<caps shift=Lshift>
    {10, 9, 7,22, 4}, // gfdsa
    {23,21, 8,26,20}, // trewq
    {34,33,32,31,30}, // 54321
    {35,36,37,38,39}, // 67890
    {28,24,12,18,19}, // yuiop
    {11,13,14,15,40}, // hjkl<enter>
    { 5,17,16,1,44},  // bnm. <space>
};
static const int kBuf[]={13,25,19,25,19,40}; //,21,40}; // LOAD "" (J shift p shift p, R ENTER)
static const int tBuf[]={2,0,2,0,2,2};//200!,2,2};
static int kcount=0;
static int timeout=100;
#endif

static char tapename[64]={0};

unsigned int in(int h, int l)
{
  int ts=0;               /* additional cycles*256 */
  static int tapemask=0;
  int data=0;             /* = 0x80 if no tape noise (?) */

  tapemask++;
  data |= (tapemask & 0x0100) ? 0x80 : 0;
  if (useNTSC) data|=64;

  if ((h == 0x7f) && (l == 0xef))
  {
    if ((ramsize < 48) || (!LowRAM))
    {
#ifdef DEBUG_CHROMA
      printf("Insufficient RAM Size for Chroma!\n");
#endif
      return 0xFF;
    }
    else if (!emu_chromaSupported())
    {
#ifdef DEBUG_CHROMA
      printf("Display does not support Chroma \n");
#endif
      return 0xFF;
    }
    return 0x02; /* chroma 80 and 81 available*/
  }

  if (!(l&1))
  {
    LastInstruction=LASTINSTINFE;

    if (l==0x7e) return 0; // for Lambda

    switch(h)
    {
      case 0xfe:        return(ts|(keyboard[0]^data));
      case 0xfd:        return(ts|(keyboard[1]^data));
      case 0xfb:        return(ts|(keyboard[2]^data));
      case 0xf7:        return(ts|(keyboard[3]^data));
      case 0xef:        return(ts|(keyboard[4]^data));
      case 0xdf:        return(ts|(keyboard[5]^data));
      case 0xbf:        return(ts|(keyboard[6]^data));
      case 0x7f:        return(ts|(keyboard[7]^data));

      default:
      {
        int i,mask,retval=0xff;
        /* some games (e.g. ZX Galaxians) do smart-arse things
          * like zero more than one bit. What we have to do to
          * support this is AND together any for which the corresponding
          * bit is zero.
          */
        for(i=0,mask=1;i<8;i++,mask<<=1)
          if(!(h&mask))
            retval&=keyboard[i];
        return(ts|(retval^data));
      }
    }
  }
  return(255);
}

unsigned int out(int h,int l,int a)
{
  if ((h==0x7f) && (l==0xef))
  {	/* chroma */
    if (emu_chromaSupported())
    {
      chromamode = a&0x30;
      if (chromamode)
      {
    #ifdef DEBUG_CHROMA
        printf("Selecting Chroma mode 0x%x.\n",a);
    #endif
        if ((ramsize < 48) || (!LowRAM))
        {
          chromamode = 0;
          printf("Insufficient RAM Size for Chroma!\n");
        }
        else
        {
          adjustChroma(true);
          bordercolournew = a & 0x0f;
        }
      }
      else
      {
    #ifdef DEBUG_CHROMA
        printf("Selecting B/W mode.\n");
    #endif
        adjustChroma(false);
      }
    }
    else
    {
#ifdef DEBUG_CHROMA
      printf("Chroma requested, but not supported.\n");
#endif
    }
    LastInstruction=LASTINSTOUTFF;
    return 0;
  }

  switch(l)
  {
    case 0x0f:
    case 0x1f:
      if(sound_ay == AY_TYPE_ZONX)
        sound_ay_write(ay_reg,a);
    break;

    case 0xbf:
    case 0xcf:
    case 0xdf:
      if(sound_ay == AY_TYPE_ZONX)
        ay_reg=(a&15);
    break;

    case 0xfd:
      if (zx80) break;
      LastInstruction = LASTINSTOUTFD;
    break;

    case 0xfe:
      if (zx80) break;
      LastInstruction = LASTINSTOUTFE;
    break;

    case 0xff: // default out handled below
    break;

    default:
    break;
  }
  if (LastInstruction == LASTINSTNONE)
    LastInstruction=LASTINSTOUTFF;

  return 0; // No additional tstates
}

static char fname[256];

void load_p(int a)
{
  int max_read;
  char *ptr=(char*)mem+(a&32767),*dptr=fname;
  char *extend = 0;
  int start = -1;
  int offset = 0;

  memset(fname, 0, sizeof(fname));

  strcpy(fname, emu_GetDirectory());
  int nameSrt = strlen(fname);

  if ((a<32768) && (!zx80))
  {
    // Try to open the name provided
    dptr += strlen(fname);

    do
    {
      *dptr++=zx2ascii[(*ptr)&63];
    }
    while((*ptr++)<128 && dptr<fname+sizeof(fname)-3);

    *dptr = 0;

    if (emu_ExtendFileRequested())
    {
      // Search for a separator that indicates request to load memory
      extend = strrchr(fname, ';');
      if (extend)
      {
        // Terminate the file name
        *extend++ = '\0';

        // Attempt to read the start address
        if (!parseNumber(extend, 0, 65535, 0, (unsigned int*)&start))
        {
          printf("Mem load address parse error, generating error 1\n");
          ERROR_INV1();
          return;
        }

        // Series of checks to ensure start is in the right area
        printf("Start Requested %i\n", start);

        // Check if in ROM - not illegal, but cannot overwrite ROM!
        if (start < 0x2000)
        {
          // Calculate the offset to the start of available memory
          if (LowRAM)
          {
            offset = 0x2000 - start;
          }
          else
          {
            offset = 0x4000 - start;
          }
          printf("Requested memory load in ROM %i Offset %i\n", start, offset);
        }
        else if ((start < 0x4000) && !LowRAM)
        {
          // No low memory
          offset = 0x4000 - start;
          printf("Requested load in low memory %i Not enabled, offset %i\n", start, offset);
        }
      }
    }

    /* add '.p' if no extension given */
    if(!strrchr(fname, '.') && (start < 0))
    {
      strcat(fname,".p");
    }
  }
  else
  {
    if (autoload)
    {
      // Use value from menu or config
      strcat(fname, tapename);
    }
    else
    {
      emu_silenceSound();
      displayHideKeyboard();

      if (loadMenu())
      {
          strcpy(fname, emu_GetDirectory());
          strcat(fname, emu_GetLoadName());
      }
    }
  }

  // Have checked file exists and loaded specific values for
  // autoload, now do similar for non autoload and check if
  // a reset is required
  EMU_LOCK_SDCARD
  int size = emu_FileSize(fname);
  if (!size)
  {
    // Report error D
    printf("File does not exist, or zero length. Generating error D\n");
    if (!zx80)
    {
      ERROR_D();
    }
    EMU_UNLOCK_SDCARD
    return;
  }
  else if (size <= offset)
  {
    // All of the load would either be in ROM or non existent RAM, so report error 3
    printf("No data to write to RAM, generating error 3\n");
    ERROR_INV3();
    EMU_UNLOCK_SDCARD
    return;
  }

  if ((!autoload) && start >= 0) /* if start address is given then don't search for settings */
  {
    // Load the settings for this file
    emu_ReadSpecificValues(fname);

    if (emu_ResetNeeded())
    {
      // Have to schedule an autoload, which will trigger the reset
      emu_SetLoadName(&fname[nameSrt]);
      z8x_Start(emu_GetLoadName());
      resetRequired = true;
      EMU_UNLOCK_SDCARD
      return;
    }
  }

  // Got this far then all good
  autoload=0;

  // This really should not fail!
  if (emu_FileOpen(fname, "r+b"))
  {
    /* Make sure we do not read in too much for the RAM we have */
    max_read = ramsize * 1024;

    if (start > 0)
    {
      if (start < 0x4000)
      {
        // adjust maximum size to read dependent on LowRAM
        if (LowRAM)
        {
          max_read += 0x2000;
        }
        // Make sure to read into existing RAM
        start += offset;
      }
      else
      {
        // Prevent loading past the end of normal memory, have already handled
        // start in ROM and no low RAM through offset
        max_read -= (start - 0x4000);
      }

      // Check some memory will still be loaded
      if (max_read <= 0)
      {
        printf("No data can be written to RAM: %i out of range, generating error 3\n", max_read);
        ERROR_INV3();
        emu_FileClose();
        EMU_UNLOCK_SDCARD
        return;
      }
    }
    else
    {
      if (zx80)
      {
        start = 0x4000;
      }
      else
      {
        start = 0x4009;
        max_read -= 9;
      }
    }

    // A ZX81 loading a .p81 file should skip the filename
    if (!zx80)
    {
      if (emu_EndsWith(fname, ".p81"))
      {
        char temp[1];
        int pos = 0;
        do
        {
          emu_FileRead(temp, 1, pos++);
        } while (!(temp[0] & 0x80));    // Last character has bit 7 set

        offset += pos;
      }
    }

    // Finally load the file
    size  = (size < max_read) ? size : max_read;
    printf("start=%i size=%i\n", start, size);
    emu_FileRead(mem + start, size, offset);
    emu_FileClose();
  }
  else
  {
    // Report error D
    printf("File open failed, generating error D\n");
    if (!zx80)
    {
      ERROR_D();
    }
    EMU_UNLOCK_SDCARD
    return;
  }
  EMU_UNLOCK_SDCARD
}

void save_p(int a)
{
  char *ptr=(char*)(mem+a),*dptr=fname;
  char *extend = 0;
  char *comma = 0;
  int start = 0;
  int length = 0;
  bool found = false;

  memset(fname,0,sizeof(fname));
  strcat(fname, emu_GetDirectory());

  if(zx80)
  {
    int index = 0x4028;	/* Start of user program area */
    int vars = mem[0x4009] << 8 | mem[0x4008];	/* VARS */
    int idxend;

    while (index < vars)
    {
      /* Look for REM SAVE " */
      if (mem[index] == 0xfe && mem[index + 1] == 0xea &&
          mem[index + 2] == 0x01)
      {
        idxend = index = index + 3; /* Position on first char */

        while (mem[idxend] != 0x01 &&
               mem[idxend] < 0x80 &&
               mem[idxend] != 0x76 &&
               idxend < vars)
        {
          idxend++;
        }

        if (index < idxend)
        {
          mem[--idxend] |= 0x80;  /* +80h marks last char */
          strcat(fname, strzx80_to_ascii(index));
          mem[idxend] &= ~0x80;   /* Remove +80h */
          index = vars;           /* Exit loop */
        }
      }
      index++;
		}

    if (index == vars)
    {
      // Stop any sound
      emu_silenceSound();
      bool displayed = displayShowKeyboard(false);

      // Display the ZX80 save screen
      uint length = strlen(fname);
      if (!saveMenu((uint8_t*)(&fname[length]), sizeof(fname) - length - 1))
      {
        // A default name
        strcat(fname,"zx80prog.o");
      }

      if (!displayed)
      {
        displayHideKeyboard();
      }
    }
  }
  else
  {
    /* so the filename is at ptr, in the ZX81 char set, with the last char
    * having bit 7 set. First, get that name in ASCII.
    */
    dptr += strlen(fname);

    do
    {
      *dptr++=zx2ascii[(*ptr)&63];
    }
    while((*ptr++)<128 && dptr<(fname+sizeof(fname)-3));

    if (emu_ExtendFileRequested())
    {
      // find last ;
      extend = strrchr(fname, ';');

      if (extend)
      {
        // verify , after last ;
        ++extend;
        comma = strrchr(extend, ',');

        if (comma)
        {
          found = true; // Have found ; and ,
          ++comma;

          if (!parseNumber(extend, 0, 0xffff, ',', (unsigned int*)&start))
          {
            printf("Illegal start address, generating error 1\n");
            ERROR_INV1();
            return;
          }

          if (!parseNumber(comma, 1, 0x10000, 0, (unsigned int*)&length))
          {
            printf("Illegal length, generating error 2\n");
            ERROR_INV2();
            return;
          }

          // Check that the end address is within 64kB
          if (start + length > 0x10000)
          {
            printf("Start %i + length %i too large, generating error 3\n", start, length);
            ERROR_INV3();
            return;
          }
        }
      }
    }
  }

  if (found)
  {
    // Saving a memory block, fix the file name
    --extend;
    *extend = 0;
  }
  else
  {
    // Saving a program, append suffix if needed
    if(!strrchr(fname, '.'))
    {
      strcat(fname, zx80 ? ".o" : ".p");
    }

    // Set start and length
    if(zx80)
    {
      start = 0x4000;
      length = fetch2(16394)-0x4000;
    }
    else
    {
      start = 0x4009;
      length = fetch2(16404)-0x4009;
    }
  }

  EMU_LOCK_SDCARD

  if (!emu_SaveFile(fname, &mem[start], length))
  {
    if (!zx80)
    {
      ERROR_D();
    }
  }
  EMU_UNLOCK_SDCARD
}

void zx81hacks()
{
  /* patch save routine */
  mem[0x2fc]=0xed; mem[0x2fd]=0xfd;
  mem[0x2fe]=0xc3; mem[0x2ff]=0x07; mem[0x300]=0x02;

  /* patch load routine */
  mem[0x347]=0xeb;
  mem[0x348]=0xed; mem[0x349]=0xfc;
  mem[0x34a]=0xc3; mem[0x34b]=0x07; mem[0x34c]=0x02;
}

void zx80hacks()
{
  /* patch save routine */
  mem[0x1b6]=0xed; mem[0x1b7]=0xfd;
  mem[0x1b8]=0xc3; mem[0x1b9]=0x83; mem[0x1ba]=0x02;

  /* patch load routine */
  mem[0x206]=0xed; mem[0x207]=0xfc;
  mem[0x208]=0xc3; mem[0x209]=0x83; mem[0x20a]=0x02;
}

static void initmem()
{
  int f;
  int count;
  int ramtmp = ramsize;
  bool odd=false;

  if(zx80)
  {
    memset(mem+0x1000,0,0xf000);
  }
  else
  {
    memset(mem+0x2000,0,0xe000);
  }

  /* ROM setup */
  count=0;
  for(f=0;f<16;f++)
  {
    memattr[f]=memattr[32+f]=0;
    memptr[f]=memptr[32+f]=mem+1024*count;
    count++;
    if(count>=(zx80?4:8)) count=0;
  }

  // Handle the 3K case
  if (ramtmp==3)
  {
    ramtmp=4;
    odd=true;
  }
  /* RAM setup */
  count=0;
  for(f=16;f<32;f++)
  {
    memattr[f]=memattr[32+f]=1;
    memptr[f]=memptr[32+f]=mem+1024*(16+((odd && count==3)?2:count));
    count++;
    if(count>=ramtmp) count=0;
  }


/* z81's ROM and RAM initialisation code is OK for <= 16K RAM but beyond
 * that it requires a little tweaking.
 *
 * The following diagram shows the ZX81 + 8K ROM. The ZX80 version is
 * the same except that each 8K ROM region will contain two copies of
 * the 4K ROM.
 *
 * RAM less than 16K is mirrored throughout the 16K region.
 *
 * The ROM will only detect up to 8000h when setting RAMTOP, therefore
 * having more than 16K RAM will require RAMTOP to be set by the user
 * (or user program) to either 49152 for 32K or 65535 for 48/56K.
 *
 *           1K to 16K       32K           48K           56K      Extra Info.
 *
 *  65535  +----------+  +----------+  +----------+  +----------+
 * (FFFFh) | 16K RAM  |  | 16K RAM  |  | 16K RAM  |  | 16K RAM  | DFILE can be
 *         | mirrored |  | mirrored |  |          |  |          | wholly here.
 *         |          |  |          |  |          |  |          |
 *         |          |  |          |  |          |  |          | BASIC variables
 *         |          |  |          |  |          |  |          | can go here.
 *  49152  +----------+  +----------+  +----------+  +----------+
 * (C000h) | 8K ROM   |  | 16K RAM  |  | 16K RAM  |  | 16K RAM  | BASIC program
 *         | mirrored |  |          |  |          |  |          | is restricted
 *  40960  +----------+  |          |  |          |  |          | to here.
 * (A000h) | 8K ROM   |  |          |  |          |  |          |
 *         | mirrored |  |          |  |          |  |          |
 *  32768  +----------+  +----------+  +----------+  +----------+
 * (8000h) | 16K RAM  |  | 16K RAM  |  | 16K RAM  |  | 16K RAM  | No machine code
 *         |          |  |          |  |          |  |          | beyond here.
 *         |          |  |          |  |          |  |          |
 *         |          |  |          |  |          |  |          | DFILE can be
 *         |          |  |          |  |          |  |          | wholly here.
 *  16384  +----------+  +----------+  +----------+  +----------+
 * (4000h) | 8K ROM   |  | 8K ROM   |  | 8K ROM   |  | 8K RAM   |
 *         | mirrored |  | mirrored |  | mirrored |  |          |
 *   8192  +----------+  +----------+  +----------+  +----------+
 * (2000h) | 8K ROM   |  | 8K ROM   |  | 8K ROM   |  | 8K ROM   |
 *         |          |  |          |  |          |  |          |
 *      0  +----------+  +----------+  +----------+  +----------+
 */

  switch(ramsize)
  {
    case 48:
      for(f=48;f<64;f++)
      {
        memattr[f]=1;
        memptr[f]=mem+1024*f;
      }
      // fall through
    case 32:
      for(f=32;f<48;f++)
      {
        memattr[f]=1;
        memptr[f]=mem+1024*f;
      }
      break;
  }

  if (LowRAM)
  {
      for(f=8;f<16;f++)
      {
        memattr[f]=1;         /* It's now writable */
        memptr[f]=mem+1024*f;
      }
  }

  if(zx80)
    zx80hacks();
  else
    zx81hacks();
}

void z8x_Init(void)
{
  // Get machine type and memory
  zx80 = emu_ZX80Requested();
  ramsize = emu_MemoryRequested();
  sound_ay = emu_SoundRequested();
  m1not = emu_M1NOTRequested();
  chr128 = emu_CHR128Requested();
  LowRAM = emu_LowRAMRequested();
  useQSUDG = emu_QSUDGRequested();
  useWRX = emu_WRXRequested();
  useNTSC = emu_NTSCRequested();
  adjustStartX = emu_CentreX();
  adjustStartY = emu_CentreY();
  frameSync = (emu_FrameSyncRequested() != SYNC_OFF);
  UDGEnabled = false;

  setEmulatedTV(!useNTSC, emu_VTol());
  hidInitialise(keyboard);

  /* load rom with ghosting at 0x2000 */
  int siz=(zx80?4096:8192);
  if(zx80)
  {
    memcpy( mem + 0x0000, zx80rom, siz );
  }
  else
  {
    if (emu_ComputerRequested() == ZX81)
    {
      memcpy( mem + 0x0000, zx81rom, siz );
    }
    else
    {
      memcpy( mem + 0x0000, zx81x2rom, siz );
    }
  }
  memcpy(mem+siz,mem,siz);
  if(zx80)
    memcpy(mem+siz*2,mem,siz*2);

  initmem();

  /* reset the keyboard state */
  memset( keyboard, 255, sizeof( keyboard ) );

  ResetZ80();
  emu_sndInit(sound_ay != AY_TYPE_NONE, true);
}

void z8x_updateValues(void)
{
  sound_ay = emu_SoundRequested();
  emu_sndInit(sound_ay != AY_TYPE_NONE, false);
  useWRX = emu_WRXRequested();
  useNTSC = emu_NTSCRequested();
  adjustStartX = emu_CentreX();
  adjustStartY = emu_CentreY();
  frameSync = (emu_FrameSyncRequested() != SYNC_OFF);
  setEmulatedTV(!useNTSC, emu_VTol());
  emu_VideoSetInterlace();
}

bool z8x_Step(void)
{
  ExecZ80();
  if (resetRequired)
  {
    resetRequired = false;
    return false;
  }

  if (sound_ay != AY_TYPE_NONE)
  {
    emu_generateSoundSamples();
  }
  return true;
}

// Set the tape name and validate it
void z8x_Start(const char * filename)
{
  char c;

  autoload = 0;

  if (filename)
  {
    // Store the name
    strcpy(tapename, filename);

    // build the full name with directory, so we
    // can check that it exists
    strcpy(fname, emu_GetDirectory());
    strcat(fname, tapename);

    EMU_LOCK_SDCARD

    if (emu_FileOpen(fname, "r+b"))
    {
      int fsize = emu_FileRead(&c, 1, 0);
      if (!fsize)
      {
        printf("z81 Open %s failed. No autoload\n", filename);
      }
      else
      {
        autoload = 1;

        // Read the new specific values
        emu_ReadSpecificValues(fname);

        // Determine the computer type from the ending - do not change for .p81
        if (!emu_EndsWith(fname, ".p81"))
        {
          emu_SetZX80(emu_EndsWith(fname, ".o") || emu_EndsWith(fname, ".80"));
        }
      }
      emu_FileClose();
      EMU_UNLOCK_SDCARD
    }
  }
}

// input: string to be parsed for an unsigned number
// Min: Minimum allowed value
// Max: Maximum allowed value
// Term: Required terminating character
// Out: Contains value read if successful
// Return value: True if value parsed successfully, false otherwise
bool parseNumber(const char* input,
                 unsigned int min,
                 unsigned int max,
                 char term,
                 unsigned int* out)
{
  *out = 0;

  while ((*input >= '0') && (*input <= '9') && (*out <= max))
  {
    *out = *out * 10 + *input++ - '0';
  }
  return ((*input == term) && (*out >= min) && (*out <= max));
}

/***************************************************************************
 * Sinclair ZX80 String to ASCII String                                    *
 ***************************************************************************/
/* This will translate a ZX80 string of characters into an ASCII equivalent.
 * All alphabetical characters are converted to lowercase.
 * Those characters that won't translate are replaced with underscores.
 *
 * On entry: int memaddr = the string's address within mem[]
 *  On exit: returns a pointer to the translated string */

char *strzx80_to_ascii(int memaddr)
{
  static unsigned char zx81table[16] = {'_', '$', ':', '?', '(', ')',
    '-', '+', '*', '/', '=', '>', '<', ';', ',', '.'};
  static char translated[256];
  unsigned char sinchar;
  char asciichar;
  int index = 0;

  do
  {
    sinchar = mem[memaddr] & 0x7f;
    if (sinchar == 0x00)
    {
      asciichar = ' ';
    } else if (sinchar == 0x01)
    {
      asciichar = '\"';
    } else if (sinchar >= 0x0c && sinchar <= 0x1b)
    {
      asciichar = zx81table[sinchar - 0x0c];
    } else if (sinchar >= 0x1c && sinchar <= 0x25)
    {
      asciichar = '0' + sinchar - 0x1c;
    } else if (sinchar >= 0x26 && sinchar <= 0x3f)
    {
      asciichar = 'a' + sinchar - 0x26;
    } else
    {
      asciichar = '_';
    }
    translated[index] = asciichar;
    translated[index++ + 1] = 0;
  } while (mem[memaddr++] < 0x80);

  return translated;
}

/* Ensure that chroma and pixels are byte aligned */
static void adjustChroma(bool start)
{
    adjustStartX = start ? disp.adjust_x : emu_CentreX();
}
