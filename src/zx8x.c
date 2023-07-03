#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "Z80.h"
#include "zx80rom.h"
#include "zx81x2rom.h"
#include "zx81rom.h"
#include "zx8x.h"
#include "emuapi.h"
#include "emusound.h"
#include "common.h"
#include "hid_app.h"
#include "hid_usb.h"
#include "menu.h"

#define ERROR_B() mem[16384] = 10;
#define ERROR_D() mem[16384] = 12;

byte mem[MEMORYRAM_SIZE];
unsigned char *memptr[64];
unsigned char font[1024];
int memattr[64];
int nmigen=0,hsyncgen=0,vsync=0;
int vsync_visuals=1;
int signal_int_flag=0;
int ramsize=16;

/* the keyboard state and other */
static byte keyboard[ 8 ] = {0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff};;
int zx80=0;
int autoload=1;
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

unsigned int out(int h, int l,int a)
{
  switch(l)
  {
    case 0x0f:
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


void bitbufBlit(unsigned char * buf)
{
  emu_DisplayFrame(buf);
}

void blankScreen()
{
  emu_BlankScreen();
}

static unsigned char fname[256];

void load_p(int a)
{
  int max_read;
  unsigned char *ptr=mem+(a&32767),*dptr=fname;
  unsigned char *extend = 0;
  int start = 0;

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
        long res=strtol(extend, NULL, 10);

        if ((res == LONG_MIN || res == LONG_MAX  || res <= 0 || res > 0xffff))
        {
          // Default to 0 if entry is not convertible
          res = 0;
          printf("Illegal memory load address\n");
        }

        start = (int)res;

        // Series of checks to ensure start is in the right area
        if (start)
        {
          printf("Start Requested %i\n", start);

          // Must not be in the ROM
          if (start < 0x2000)
          {
            start = 0;
            printf("Requested memory load in ROM\n");
          }
          else if ((start < 0x4000) && !LowRAM)
          {
            // No low memory
            start = 0;
            printf("Requested memory load in low memory - no low memory enabled\n");
          }
        }

        // Read a start address, but failed a check, so have an error
        if (!start)
        {
          printf("Mem parse error, generating error B\n");
          ERROR_B();
          return;
        }
      }
    }

    /* add '.p' if no extension given */
    if(!strrchr(fname, '.') && !start)
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
  int size = emu_FileSize(fname);
  if (!size)
  {
    // Report error D
    printf("Zero length, generating error D\n");
    ERROR_D();
    return;
  }

  if ((!autoload) && (start == 0)) /* if start address is given then don't search for settings */
  {
    // Load the settings for this file
    emu_ReadSpecificValues(fname);

    if (emu_resetNeeded())
    {
      // Have to schedule an autoload, which will trigger the reset
      emu_SetLoadName(&fname[nameSrt]);
      z8x_Start(emu_GetLoadName());
      resetRequired = true;
      return;
    }
  }

  // Got this far then all good
  autoload=0;

  int f = emu_FileOpen(fname, "r+b");

  // This really should not fail!
  if (f)
  {
    /* Make sure we do not read in too much for the RAM we have */
    max_read = ramsize * 1024;

    if (start)
    {
      if ((start >= 0x2000) && (start < 0x4000))
      {
        max_read = 0x4000 - start;
      }
      else
      {
        max_read -= (start - 0x4000);
        if (max_read <= 0)
        {
          printf("Memory load address %i out of range, generating error B\n", start);
          ERROR_B();
          return;
        }
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

    // Finally load the file
    size  = (size < max_read) ? size : max_read;
    emu_FileRead(mem + start, size, f);
  }
  else
  {
    // Report error D
    printf("File read failed, generating error D\n");
    ERROR_D();
    return;
  }
  emu_FileClose(f);
}

void save_p(int a)
{
  unsigned char *ptr=mem+a,*dptr=fname;
  FILE *out;
  unsigned char *extend = 0;
  unsigned char *comma = 0;
  int start = 0;
  int length = 0;

  memset(fname,0,sizeof(fname));
  strcat(fname, emu_GetDirectory());

  if(zx80)
  {
      strcat(fname,"zx80prog.o");
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
    while((*ptr++)<128 && dptr<fname+sizeof(fname)-3);

    if (emu_ExtendFileRequested())
    {
      // Search for a separator that indicates request to save memory
      extend = strrchr(fname, ',');
      if (extend)
      {
        comma = extend; // Need to store this in case length not found
        *extend++ = '\0';

        // Attempt to read the length
        long res=strtol(extend, NULL, 10);

        if ((res == LONG_MIN || res == LONG_MAX  || res < 0 || res > 0xbfff))
        {
          // Default to 0 if entry is not convertible
          res = 0;
          printf("Illegal memory length, generating error B\n");
          ERROR_B();
          return;
        }

        length = (int)res;

        // With a valid length, we can find the start address
        if (length)
        {
          extend = strrchr(fname, ';');

          if (extend)
          {
            *extend++ = '\0';

            // Attempt to read the start address
            long res=strtol(extend, NULL, 10);

            if ((res == LONG_MIN || res == LONG_MAX  || res < 0 || res > 0xbfff))
            {
              // Default to -1 if entry is not convertible
              res = -1;
              printf("Illegal start address, generating error B\n");
              ERROR_B();
              return;
            }

            start = (int)res;

            // Attempt to validate both
            if (length && start >= 0)
            {
              // Check for saving ROM
              if (start < 0x2000)
              {
                // Validate that length does not exceed 8kB boundary
                if (start + length >= 0x2000)
                {
                  printf("Saving past end of ROM area. Start %i, Length %i\n", start, length);
                  start = -1;
                  length = 0;
                }
                else
                {
                  printf("Saving ROM. Start %i, Length %i\n", start, length);
                }
              }
              // If in low memory, ensure it exists, and do not stray outside
              else if ((start < 0x4000) && ((!LowRAM) || (start + length >= 0x4000)))
              {
                printf("Either no low RAM, or out of range. Start %i, Length %i\n", start, length);
                start = -1;
                length = 0;
              }
              // If in normal memory ensure all of range exists
              else if ((start + length) >= (0x4000 + (ramsize * 1024)))
              {
                printf("Range does not fit available RAM. Start %i, Length %i\n", start, length);
                start = -1;
                length = 0;
              }
              else
              {
                // Success!!!
                printf("Filename: %s, start address %i, length %i\n", fname, start, length);
              }
            }

            if (start == -1)
            {
              printf("Gnerating error B\n");
              ERROR_B();
              return;
            }
          }
          else
          {
            // If found length, but no comma, then revert to full address
            printf("Found memory length, but no start address. Length %i\n", length);
            length = 0;
            start = 0;
            *comma = ',';
          }
        }
        else
        {
          // length was not valid, so restore comma
          *comma = ',';
        }
      }
    }

    if(!strrchr(fname, '.') && (!length && start >= 0))
    {
      strcat(fname,".p");
    }
  }
  /* work out how much to write, and write it.
  * we need to write from 0x4009 (0x4000 on ZX80) to E_LINE inclusive.
  */
  if(zx80)
  {
    if (!start)
    {
        start = 0x4000;
    }
    if (!length)
    {
      length = fetch2(16394)-0x4000;
    }
  }
  else
  {
    if (!start)
    {
        start = 0x4009;
    }
    if (!length)
    {
      length = fetch2(16404)-0x4009;
    }
  }
  emu_SaveFile(fname, &mem[start], length);
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
  sound_ay = emu_soundRequested();
  m1not = emu_M1NOTRequested();
  LowRAM = emu_LowRAMRequested();
  useQSUDG = emu_QSUDGRequested();
  useWRX = emu_WRXRequested();
  useNTSC = emu_NTSCRequested();
  adjustStartX=emu_CentreX();
  adjustStartY=emu_CentreY();

  UDGEnabled = false;

  setTVRange(!useNTSC, emu_VTol());

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
  emu_sndInit(sound_ay != AY_TYPE_NONE);
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
    int f = emu_FileOpen(fname, "r+b");
    if ( f )
    {
      int fsize = emu_FileRead(&c, 1, f);
      if ( fsize == 0)
      {
        printf("z81 Open %s failed. No autoload\n", filename);
      }
      else
      {
        autoload = 1;

        // Read the new specific values
        emu_ReadSpecificValues(fname);

        // Determine the computer type from the ending
        emu_setZX80(emu_endsWith(fname, ".o") || emu_endsWith(fname, ".80"));
      }
      emu_FileClose(f);
    }
  }
}
