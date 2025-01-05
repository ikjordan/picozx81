#include <string.h>
#include <stdlib.h>
#include "z80.h"
#include "zx80rom.h"
#include "zx81x2rom.h"
#include "zx81rom.h"
#include "zx8x.h"
#include "emuapi.h"
#include "emusound.h"
#include "emuvideo.h"
#include "emukeyboard.h"
#include "common.h"
#include "hid_app.h"
#include "hid_usb.h"
#include "menu.h"
#include "display.h"
#include "loadp.h"

char *strzx80_to_ascii(int memaddr);
bool parseNumber(const char* input,
                 unsigned int min,
                 unsigned int max,
                 char term,
                 unsigned int* out);

#define ERROR_D() mem[16384] = 12;
#define ERROR_INV1() mem[16384] = 128;
#define ERROR_INV2() mem[16384] = 129;
#define ERROR_INV3() mem[16384] = 130;

byte mem[MEMORYRAM_SIZE];
unsigned char *memptr[64];
int memattr[64];
int ramsize=16;

/* the keyboard state and other */
static uint8_t keyboard[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
int zx80=0;
int rom4k=0;
int autoload=1;
int chromamode=0;
#ifdef SUPPORT_CHROMA
unsigned char bordercolour=0x0f;
unsigned char bordercolournew=0x0f;
unsigned char fullcolour=0xff;
unsigned char chroma_set=0;
#endif

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

static char tapename[64]={0};

static void set_mem_attribute_and_ptr(void);

unsigned int __not_in_flash_func(in)(int h, int l)
{
  if ((h == 0x7f) && (l == 0xef))
  {
#ifdef SUPPORT_CHROMA
    if ((ramsize < 48) || (!LowRAM))
    {
#ifdef DEBUG_CHROMA
      printf("Insufficient RAM Size for Chroma!\n");
#endif
      return 0xff;
    }
    else if (!emu_chromaSupported())
    {
#ifdef DEBUG_CHROMA
      printf("Display does not support Chroma \n");
#endif
      return 0xff;
    }
    return 0x02; /* chroma 80 and 81 available*/
#else
    return 0xff;
#endif
  }

  if (!(l&1))
  {
    int data=0x80;
    LastInstruction=LASTINSTINFE;

    if (((sound_type == SOUND_TYPE_VSYNC) || ((sound_type == SOUND_TYPE_CHROMA) && frameNotSync)))
    {
        sound_beeper(0);
    }

#ifdef LOAD_AND_SAVE
    if (running_rom)
    {
      data = useNTSC ? 0x40 : 0;
      data |= loadPGetBit() ? 0x0 : 0x80; // Reversed as use xor below
    }
#endif

    switch(h)
    {
      case 0xfe:  return(keyboard[0] ^ data);
      case 0xfd:  return(keyboard[1] ^ data);
      case 0xfb:  return(keyboard[2] ^ data);
      case 0xf7:  return(keyboard[3] ^ data);
      case 0xef:  return(keyboard[4] ^ data);
      case 0xdf:  return(keyboard[5] ^ data);
      case 0xbf:  return(keyboard[6] ^ data);
      case 0x7f:  return(keyboard[7] ^ data);

      default:
      {
        int i,mask,retval=0xff;
        /* some games (e.g. ZX Galaxians) do smart-arse things
          * like zero more than one bit. What we have to do to
          * support this is AND together any for which the corresponding
          * bit is zero.
          */
        for(i = 0, mask = 1; i < 8; i++, mask <<= 1)
        {
          if(!(h & mask)) retval &= keyboard[i];
        }
        return(retval ^ data);
      }
    }
  }
  return 0xff;
}

void __not_in_flash_func(out)(int h, int l, int a)
{
  if ((sound_type == SOUND_TYPE_VSYNC) || ((sound_type == SOUND_TYPE_CHROMA) && frameNotSync))
    sound_beeper(1);

#ifdef SUPPORT_CHROMA
  if ((h == 0x7f) && (l == 0xef))
  { /* chroma */
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
          chroma_set = 0;
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
    LastInstruction = LASTINSTOUTFF;
    return;
  }
#endif

  switch(l)
  {
    case 0x0f:
    case 0x1f:
      if(sound_type == SOUND_TYPE_ZONX)
        sound_ay_write(ay_reg,a);
    break;

    case 0xbf:
    case 0xcf:
    case 0xdf:
      if(sound_type == SOUND_TYPE_ZONX)
        ay_reg=(a&15);
    break;

    case 0xfd:
      LastInstruction = LASTINSTOUTFD;
    break;

    case 0xfe:
      LastInstruction = LASTINSTOUTFE;
    break;

    default:
      LastInstruction = LASTINSTOUTFF;
    break;
  }
}

static char fname[256];

char* z8x_getFilenameDirectory(void)
{
    strcpy(fname, emu_GetDirectory());
    return fname;
}

bool load_p(int name_addr, bool defer_rom)
{
  int max_read;
  char* ptr=(char*)mem + (name_addr & 0x7fff);
  char* dptr=fname;
  char* extend = 0;
  int start = -1;
  int offset = 0;
  bool from_menu = false;

  bool defer = false;

  memset(fname, 0, sizeof(fname));

  strcpy(fname, emu_GetDirectory());

  if ((name_addr < 0x8000) && (!rom4k))
  {
    // Try to open the name provided
    dptr += strlen(fname);

    do
    {
      *dptr++=zx2ascii[(*ptr)&63];
    }
    while((*ptr++) < 128 && dptr< (fname + sizeof(fname) - 3));

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
        if (!parseNumber(extend, 0, 0xffff, 0, (unsigned int*)&start))
        {
          printf("Mem load address parse error, generating error 1\n");
          ERROR_INV1();
          return defer;
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
      emu_sndSilence();
      displayHideKeyboard();

      if (loadMenu())
      {
        strcpy(fname, emu_GetDirectory());
        strcat(fname, emu_GetLoadName());
        from_menu = true;
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
    if (!rom4k)
    {
      ERROR_D();
    }
    EMU_UNLOCK_SDCARD
    return defer;
  }
  else if (size <= offset)
  {
    // All of the load would either be in ROM or non existent RAM, so report error 3
    printf("No data to write to RAM, generating error 3\n");
    ERROR_INV3();
    EMU_UNLOCK_SDCARD
    return defer;
  }

  if ((!autoload) && (start < 0)) /* if start address is given then don't search for settings */
  {
    // Already know that the file exists, but user may have specified a different directory
    // so adjust, if file was not picked from menu
    if (!from_menu)
    {
      char* nameStrt = strrchr(fname, '/');
      if (nameStrt)  *nameStrt = 0;
      emu_SetDirectory(nameStrt ? fname : "/");
      if (nameStrt) *nameStrt = '/';
    }
    // Load the settings for this file
    emu_ReadSpecificValues(fname);

    if (emu_ResetNeeded())
    {
      // Have to schedule an autoload, which will trigger the reset

      // Extract the file name from the full path
      char* nameStrt = strrchr(fname, '/');
      nameStrt = (nameStrt == NULL) ? fname : nameStrt + 1;

      emu_SetLoadName(nameStrt);
      z8x_Start(emu_GetLoadName());
      resetRequired = true;
      EMU_UNLOCK_SDCARD
      return defer;
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
        return defer;
      }
    }
    else
    {
      // A plain load
      defer = defer_rom;

      if (rom4k)
      {
        start = 0x4000;
      }
      else
      {
        start = 0x4009;
        max_read -= 9;
      }
    }

    // An 8k ROM loading a .p81 file should skip the filename
    if (!rom4k)
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
    if (!defer)
    {
      emu_FileRead(mem + start, size, offset);
      emu_FileClose();
    }
    else
    {
      // Close the open file and defer loading to ROM routine
      emu_FileClose();
      loadPInitialise(fname, name_addr, rom4k);
    }
  }
  else
  {
    // Report error D
    printf("File open failed, generating error D\n");
    if (!rom4k)
    {
      ERROR_D();
    }
    EMU_UNLOCK_SDCARD
    return defer;
  }
  EMU_UNLOCK_SDCARD
  return defer;
}

bool save_p(int name_addr, bool defer_rom)
{
  char* ptr=(char*)(mem+name_addr);
  char* dptr=fname;
  char* extend = 0;
  char* comma = 0;
  int start = 0;
  int length = 0;
  bool found = false;
  bool defer = false;

  memset(fname,0,sizeof(fname));
  strcat(fname, emu_GetDirectory());

  if(rom4k)
  {
    int index = 0x4028;                         /* Start of user program area */
    int vars = mem[0x4009] << 8 | mem[0x4008];  /* VARS */
    int idxend;

    while (index < vars)
    {
      /* Look for REM SAVE " */
      if (mem[index] == 0xfe && mem[index + 1] == 0xea &&
          mem[index + 2] == 0x01)
      {
        idxend = index = index + 3;             /* Position on first char */

        while (mem[idxend] != 0x01 &&
               mem[idxend] < 0x80 &&
               mem[idxend] != 0x76 &&
               idxend < vars)
        {
          idxend++;
        }

        if (index < idxend)
        {
          mem[--idxend] |= 0x80;                /* +80h marks last char */
          strcat(fname, strzx80_to_ascii(index));
          mem[idxend] &= ~0x80;                 /* Remove +80h */
          index = vars;                         /* Exit loop */
        }
      }
      index++;
    }

    if (index == vars)
    {
      // Stop any sound
      emu_sndSilence();
      bool displayed = displayShowKeyboard(false);

      // Display the ZX80 save screen
      uint length = strlen(fname);
      if (!saveMenu(&fname[length], sizeof(fname) - length - 1, true))
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
            return defer;
          }

          if (!parseNumber(comma, 1, 0x10000, 0, (unsigned int*)&length))
          {
            printf("Illegal length, generating error 2\n");
            ERROR_INV2();
            return defer;
          }

          // Check that the end address is within 64kB
          if (start + length > 0x10000)
          {
            printf("Start %i + length %i too large, generating error 3\n", start, length);
            ERROR_INV3();
            return defer;
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
      strcat(fname, rom4k ? ".o" : ".p");
    }

    // Set start and length
    if(rom4k)
    {
      start = 0x4000;
      length = fetch2(16394)-0x4000;
    }
    else
    {
      start = 0x4009;
      length = fetch2(16404)-0x4009;
    }
    defer = defer_rom;
  }

  EMU_LOCK_SDCARD

  if (!emu_SaveFile(fname, &mem[start], length))
  {
    if (!rom4k)
    {
      ERROR_D();
    }
    defer = false;
  }
  EMU_UNLOCK_SDCARD
  return defer;
}

#ifdef LOAD_AND_SAVE
RomPatches_T rom_patches;
#endif

void rom8kPatches()
{
#ifdef LOAD_AND_SAVE
  rom_patches.save.start = SAVE_START_8K;
  rom_patches.save.use_rom = emu_saveUsingROMRequested();
  rom_patches.load.start = LOAD_START_8K;
  rom_patches.load.use_rom = emu_loadUsingROMRequested();
  rom_patches.retAddr = LOAD_SAVE_RET_8K;
  rom_patches.rstrtAddr = LOAD_SAVE_RSTRT_8K;
#else
  /* patch save routine */
  mem[0x2fc]=0xed; mem[0x2fd]=0xfd;
  mem[0x2fe]=0xc3; mem[0x2ff]=0x07; mem[0x300]=0x02;

  /* patch load routine */
  mem[0x347]=0xeb;
  mem[0x348]=0xed; mem[0x349]=0xfc;
  mem[0x34a]=0xc3; mem[0x34b]=0x07; mem[0x34c]=0x02;
#endif
}

void rom4kPatches()
{
#ifdef LOAD_AND_SAVE
  rom_patches.save.start = SAVE_START_4K;
  rom_patches.save.use_rom = emu_saveUsingROMRequested();
  rom_patches.load.start = LOAD_START_4K;
  rom_patches.load.use_rom = emu_loadUsingROMRequested();
  rom_patches.retAddr = LOAD_SAVE_RET_4K;
  rom_patches.rstrtAddr = LOAD_SAVE_RSTRT_4K;
#else
  /* patch save routine */
  mem[0x1b6]=0xed; mem[0x1b7]=0xfd;
  mem[0x1b8]=0xc3; mem[0x1b9]=0x83; mem[0x1ba]=0x02;

  /* patch load routine */
  mem[0x206]=0xed; mem[0x207]=0xfc;
  mem[0x208]=0xc3; mem[0x209]=0x83; mem[0x20a]=0x02;
#endif
}

static void initmem(void)
{
  if(rom4k)
  {
    memset(mem+0x1000,0,0xf000);
  }
  else
  {
    memset(mem+0x2000,0,0xe000);
  }
  set_mem_attribute_and_ptr();
}

static void set_mem_attribute_and_ptr(void)
{
  int f;
  int ramtmp = ramsize;
  bool odd = false;
  int count = 0;

  /* ROM setup */
  for(f=0;f<16;f++)
  {
    memattr[f]=memattr[32+f]=0;
    memptr[f]=memptr[32+f]=mem+1024*count;
    count++;
    if(count>=(rom4k?4:8)) count=0;
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

  if(rom4k)
    rom4kPatches();
  else
    rom8kPatches();
}

#define ZX80_PIXEL_OFFSET       6

void z8x_Init(void)
{
  // Get machine type and memory
  zx80 = emu_ZX80Requested();
  rom4k = emu_ROM4KRequested();
  ramsize = emu_MemoryRequested();
  sound_type = emu_SoundRequested();
  m1not = emu_M1NOTRequested();
  chr128 = emu_CHR128Requested();
  LowRAM = emu_LowRAMRequested();
  useQSUDG = emu_QSUDGRequested();
  useWRX = emu_WRXRequested();
  useNTSC = emu_NTSCRequested();
  frameSync = (emu_FrameSyncRequested() != SYNC_OFF);
  UDGEnabled = false;

  setEmulatedTV(!useNTSC, emu_VTol());
  setDisplayBoundaries();
  emu_KeyboardInitialise(keyboard);
  emu_JoystickInitialiseNinePin();

  /* load rom with ghosting at 0x2000 */
  int siz=(rom4k?4096:8192);
  if(rom4k)
  {
    memcpy( mem + 0x0000, zx80rom, siz );
  }
  else
  {
    if (emu_ComputerRequested() == ZX81X2)
    {
      memcpy( mem + 0x0000, zx81x2rom, siz );
    }
    else
    {
      memcpy( mem + 0x0000, zx81rom, siz );
    }
  }
  memcpy(mem+siz,mem,siz);
  if(rom4k)
    memcpy(mem+siz*2,mem,siz*2);

  initmem();

  /* reset the keyboard state */
  memset( keyboard, 255, sizeof( keyboard ) );

  resetZ80();
  emu_sndInit(sound_type != SOUND_TYPE_NONE, true);
}

void z8x_updateValues(void)
{
  sound_type = emu_SoundRequested();
  emu_sndInit(sound_type != SOUND_TYPE_NONE, false);

  useWRX = emu_WRXRequested();
  useNTSC = emu_NTSCRequested();
  frameSync = (emu_FrameSyncRequested() != SYNC_OFF);
  setEmulatedTV(!useNTSC, emu_VTol());
  setDisplayBoundaries();
  emu_VideoSetInterlace();
}

bool z8x_Step(void)
{
  zx80 ? execZX80() : execZX81();

  if (resetRequired)
  {
    resetRequired = false;
    return false;
  }

  if (sound_type != SOUND_TYPE_NONE)
  {
    emu_sndGenerateSamples();
  }
  return true;
}

// Set the tape name and validate it
bool z8x_Start(const char * filename)
{
  bool reset = true;
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

    if (emu_EndsWith(tapename, ".s"))
    {
      emu_loadSnapshot(fname);
      reset = false;
    }
    else
    {
      EMU_LOCK_SDCARD

      if (emu_FileOpen(fname, "r+b"))
      {
        int fsize = emu_FileRead(&c, 1, 0);
        if (!fsize)
        {
          printf("Read %s failed. No autoload\n", filename);
        }
        else
        {
          autoload = 1;

          // Read the new specific values
          emu_ReadSpecificValues(fname);

          // Determine the computer type from the ending - do not change for .p81
          if (!emu_EndsWith(fname, ".p81"))
          {
            emu_SetRom4K(emu_EndsWith(fname, ".o") || emu_EndsWith(fname, ".80"));
          }
        }
        emu_FileClose();
        EMU_UNLOCK_SDCARD
      }
    }
  }
  return reset;
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
bool save_snap_zx8x(void)
{
  if (!emu_FileWriteBytes(mem, sizeof(byte) * MEMORYRAM_SIZE)) return false;
  // memptr and memattr recreated when loaded

  if (!emu_FileWriteBytes(keyboard, sizeof(uint8_t) * 8)) return false;
  if (!emu_FileWriteBytes(&ramsize, sizeof(ramsize))) return false;
  if (!emu_FileWriteBytes(&zx80, sizeof(zx80))) return false;
  if (!emu_FileWriteBytes(&rom4k, sizeof(rom4k))) return false;
  if (!emu_FileWriteBytes(&autoload, sizeof(autoload))) return false;
  if (!emu_FileWriteBytes(&chromamode, sizeof(chromamode))) return false;
#ifdef SUPPORT_CHROMA
  if (!emu_FileWriteBytes(&bordercolour, sizeof(bordercolour))) return false;
  if (!emu_FileWriteBytes(&bordercolournew, sizeof(bordercolournew))) return false;
  if (!emu_FileWriteBytes(&fullcolour, sizeof(fullcolour))) return false;
  if (!emu_FileWriteBytes(&chroma_set, sizeof(chroma_set))) return false;
#endif
  if (!emu_FileWriteBytes(&resetRequired, sizeof(resetRequired))) return false;

  return true;
}

bool load_snap_zx8x(void)
{
  if (!emu_FileReadBytes(mem, sizeof(byte) * MEMORYRAM_SIZE)) return false;
  // memptr and memattr recreated when loaded

  if (!emu_FileReadBytes(keyboard, sizeof(uint8_t) * 8)) return false;
  if (!emu_FileReadBytes(&ramsize, sizeof(ramsize))) return false;
  if (!emu_FileReadBytes(&zx80, sizeof(zx80))) return false;
  if (!emu_FileReadBytes(&rom4k, sizeof(rom4k))) return false;
  if (!emu_FileReadBytes(&autoload, sizeof(autoload))) return false;
  if (!emu_FileReadBytes(&chromamode, sizeof(chromamode))) return false;
#ifdef SUPPORT_CHROMA
  if (!emu_FileReadBytes(&bordercolour, sizeof(bordercolour))) return false;
  if (!emu_FileReadBytes(&bordercolournew, sizeof(bordercolournew))) return false;
  if (!emu_FileReadBytes(&fullcolour, sizeof(fullcolour))) return false;
  if (!emu_FileReadBytes(&chroma_set, sizeof(chroma_set))) return false;
#endif
  if (!emu_FileReadBytes(&resetRequired, sizeof(resetRequired))) return false;

  set_mem_attribute_and_ptr();
  return true;
}
