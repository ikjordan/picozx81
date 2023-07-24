#include "pico.h"
#include <stdlib.h>
#include <limits.h>
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "tusb.h"
#include "hid_usb.h"
#include "display.h"

#include "emuapi.h"
#include "emupriv.h"

#include "ini.h"
#include "iopins.h"

/********************************
 * Menu file loader UI
 ********************************/
#include "ff.h"
static FATFS fatfs;
static FIL file;

/********************************
 * File IO
 ********************************/
bool emu_FileOpen(const char *filepath, const char *mode)
{
  (void)mode;   // Ignore mode, always open as read only
  bool retval = false;

  printf("FileOpen %s\n", filepath);
  FRESULT res = f_open(&file, filepath, FA_READ);
  if (res == FR_OK)
  {
    retval = true;
  }
  else
  {
    printf("FileOpen failed, reason %i\n", res);
  }
  return (retval);
}

int emu_FileRead(void *buf, int size, int offset)
{
  unsigned int read = 0;

  if (size > 0)
  {
    if (offset)
    {
      if (f_lseek(&file, offset) != FR_OK)
      {
        printf("emu_FileRead seek failed\n");
        return 0;
      }
    }

    if (f_read(&file, (void *)buf, size, &read) != FR_OK)
    {
      printf("emu_FileRead read failed\n");
      read = 0;
    }
  }
  else
  {
    printf("emu_FileRead negative size %i\n", size);
  }
  return read;
}

void emu_FileClose(void)
{
  f_close(&file);
}

unsigned int emu_FileSize(const char *filepath)
{
  int filesize = 0;
  FILINFO entry;
  if (!f_stat(filepath, &entry))
  {
    filesize = entry.fsize;
  }
  printf("FileSize of %s is %i\n", filepath, filesize);
  return (filesize);
}

bool emu_SaveFile(const char *filepath, void *buf, int size)
{
  printf("SaveFile %s\n", filepath);
  if (!(f_open(&file, filepath, FA_CREATE_ALWAYS | FA_WRITE)))
  {
    unsigned int retval = 0;
    if ((f_write(&file, buf, size, &retval)))
    {
      printf("file write failed\n");
    }
    f_close(&file);
  }
  else
  {
    printf("file open failed\n");
    return false;
  }
  return true;
}

/********************************
 * Initialization
 ********************************/
void emu_init(void)
{
  FRESULT fr = f_mount(&fatfs, "0:", 1);

  if (fr != FR_OK)
  {
    printf("SDCard mount failed. Error %i\n", fr);
  }
  else
  {
    printf("SDCard mounted\n");
  }
  emu_ReadDefaultValues();

  // The config in the root directory can set the initial
  // directory, so need to read that directory too
  if (emu_GetDirectory()[0])
  {
    emu_ReadDefaultValues();
  }
}

/********************************
 * Configuration File
 ********************************/
#define CONFIG_FILE "config.ini"

typedef struct
{
  char up;
  char down;
  char left;
  char right;
  char button;
  int memory;
  int sound;
  ComputerType computer;
  bool NTSC;
  uint16_t VTol;
  bool centre;
  bool WRX;
  bool QSUDG;
  bool M1NOT;
  bool LowRAM;
  bool acb;
  bool doubleshift;
  bool extendfile;
  bool allfiles;
} configuration_t;

typedef struct
{
  const char *section;
  configuration_t *conf;
} conf_handler_t;

static char selection[MAX_FILENAME_LEN] = "";
static char dirPath[MAX_DIRECTORY_LEN] = "";

static configuration_t specific;    // The specific settings for the file to be loaded
static configuration_t general;     // The settings specified in the default section
static configuration_t root_config; // The settings specified in the default section

static bool resetNeeded = false;

static bool setDirectory(const char* dir);
static char convert(const char *val);
static bool isEnabled(const char* val);
static int handler(void *user, const char *section, const char *name,
                   const char *value);
static int ini_parse_fatfs(const char *filename, ini_handler handler, void *user);

int emu_soundRequested(void)
{
  return specific.sound;
}

bool emu_ACBRequested(void)
{
  // Do not allow stereo on a mono board
#ifdef I2S
  return specific.acb;
#else
  return specific.acb && (AUDIO_PIN_L != AUDIO_PIN_R);
#endif
}

bool emu_ZX80Requested(void)
{
  return (specific.computer == ZX80);
}

ComputerType emu_ComputerRequested(void)
{
  return specific.computer;
}

int emu_MemoryRequested(void)
{
  // Do not allow more than 16k with QSUDG graphics
  return ((specific.QSUDG) && (specific.memory>16)) ? 16 : specific.memory;
}

bool emu_NTSCRequested(void)
{
  return specific.NTSC;
}

int emu_CentreX(void)
{
  if (specific.centre)
    return DISPLAY_ADJUST_X;
  else
    return 0;
}

int emu_CentreY(void)
{
  if (specific.centre)
    return specific.NTSC ? 16 : -8;
  else
    return 0;
}

uint16_t emu_VTol(void)
{
  return specific.VTol;
}

bool emu_WRXRequested(void)
{
  // WRX always enabled if no memory expansion
  return (specific.WRX || (specific.memory <= 2));
}

bool emu_M1NOTRequested(void)
{
  return (specific.M1NOT && (specific.memory>=32) && (!specific.QSUDG));
}

bool emu_LowRAMRequested(void)
{
  return specific.LowRAM;
}

bool emu_QSUDGRequested(void)
{
  return specific.QSUDG;
}

bool emu_DoubleShiftRequested(void)
{
  return specific.doubleshift;
}

bool emu_ExtendFileRequested(void)
{
  return specific.extendfile;
}

bool emu_AllFilesRequested(void)
{
  return specific.allfiles;
}

bool emu_resetNeeded(void)
{
  return resetNeeded;
}

const char* emu_GetDirectory(void)
{
  return dirPath;
}

const char* emu_GetLoadName(void)
{
  if (selection[0])
    return selection;
  else
    return NULL;
}

void emu_SetLoadName(const char* name)
{
  if (name!=NULL && name[0])
  {
    strncpy(selection, name, MAX_FILENAME_LEN-1);
    selection[MAX_FILENAME_LEN-1] = 0;
  }
  else
  {
    selection[0] = 0;
  }
}

void emu_SetDirectory(const char* dir)
{
  if (setDirectory(dir))
  {
    emu_ReadDefaultValues();
  }
}

static bool setDirectory(const char* dir)
{
  bool retVal = false;

  if (dir!=NULL && dir[0])
  {
    if (dir[strlen(dir) - 1] != '/')
    {
      if (strncasecmp(dirPath, dir, strlen(dir) - 1) ||
          (strlen(dir) != (strlen(dirPath) - 1)))
      {
        strncpy(dirPath, dir, MAX_DIRECTORY_LEN-2);
        dirPath[MAX_DIRECTORY_LEN-2] = 0;
        strcat(dirPath, "/");
        retVal = true;
      }
    }
    else
    {
      if (strncasecmp(dirPath, dir, strlen(dir)) ||
          (strlen(dir) != strlen(dirPath)))
      {
        strncpy(dirPath, dir, MAX_DIRECTORY_LEN-1);
        dirPath[MAX_DIRECTORY_LEN-1] = 0;
        retVal = true;
      }
    }
  }
  return retVal;
}

void emu_setZX80(bool zx80)
{
  if (zx80)
  {
    general.computer = ZX80;
  }
  else if (general.computer == ZX80)
  {
      general.computer = ZX81;
  }
}

static char convert(const char *val)
{
  if (!strcasecmp(val, "ENTER"))
  {
    return 0xd;
  }
  else if (!strcasecmp(val, "SPACE"))
  {
    return ' ';
  }
  else
  {
    return val[0];
  }
}

static bool isEnabled(const char* val)
{
  return ((strcasecmp(val, "OFF") != 0) && (strcasecmp(val, "0") != 0));
}

static int handler(void *user, const char *section, const char *name,
                   const char *value)
{
  conf_handler_t *c = (conf_handler_t *)user;
  if (!strcasecmp(section, c->section))
  {
    if (!strcasecmp(name, "UP"))
    {
      c->conf->up = convert(value);
    }
    else if (!strcasecmp(name, "DOWN"))
    {
      c->conf->down = convert(value);
    }
    else if (!strcasecmp(name, "LEFT"))
    {
      c->conf->left = convert(value);
    }
    else if (!strcasecmp(name, "RIGHT"))
    {
      c->conf->right = convert(value);
    }
    else if (!strcasecmp(name, "BUTTON"))
    {
      c->conf->button = convert(value);
    }
    else if (!strcasecmp(name, "SOUND"))
    {
      // Sound enabled if entry exists and is not "OFF" or 0
      if ((strcasecmp(value, "OFF") == 0) || (strcasecmp(value, "0") == 0))
      {
        c->conf->sound = AY_TYPE_NONE;
      }
      else if ((strcasecmp(value, "QUICKSILVA") == 0))
      {
        c->conf->sound = AY_TYPE_QUICKSILVA;
      }
      else
      {
        c->conf->sound = AY_TYPE_ZONX;
      }
    }
    else if (!strcasecmp(name, "ACB"))
    {
      // ACB stereo enabled if entry exists and is not "OFF" or 0
      c->conf->acb = isEnabled(value);
    }
    else if (!strcasecmp(name, "COMPUTER"))
    {
      // ZX81 enabled unless entry exists and is set to "ZX80" or "ZX81x2"
      if (strcasecmp(value, "ZX80") == 0)
      {
        c->conf->computer = ZX80;
      }
      else if (strcasecmp(value, "ZX81x2") == 0)
      {
        c->conf->computer = ZX81X2;
      }
      else
      {
        c->conf->computer = ZX81;
      }
    }
    else if (!strcasecmp(name, "M1NOT"))
    {
      // M1NOT enabled if entry exists and is not "OFF" or 0
      c->conf->M1NOT = isEnabled(value);
    }
    else if (!strcasecmp(name, "WRX"))
    {
      // WRX enabled if entry exists and is not "OFF" or 0
      c->conf->WRX = isEnabled(value);
    }
    else if (!strcasecmp(name, "LowRAM"))
    {
      // RAM expansion in 0x2000 to 0x3FFF range enabled
      // if entry exists and is not "OFF" or 0
      c->conf->LowRAM = isEnabled(value);
    }
    else if (!strcasecmp(name, "QSUDG"))
    {
      // QSUDG at 0x8400 to 0x87FF enabled
      // if entry exists and is not "OFF" or 0
      c->conf->QSUDG = isEnabled(value);
    }
    else if (!strcasecmp(name, "NTSC"))
    {
      // Defaults to off
      c->conf->NTSC = isEnabled(value);
    }
    else if (!strcasecmp(name, "VTOL"))
    {
      // If it is not a positive number in range 1 to 200 then use default
      long res=strtol(value, NULL, 10);
      if ((res == LONG_MIN || res == LONG_MAX  || res <= 0 || res > 200))
      {
        res = VTOL;
      }
      c->conf->VTol = (uint16_t)res;
    }
    else if (!strcasecmp(name, "Centre"))
    {
      // Defaults to on, set to Off or 0 to turn off
      c->conf->centre = isEnabled(value);
    }
    else if (!strcasecmp(name, "MEMORY"))
    {
      long res=strtol(value, NULL, 10);

      if ((res == LONG_MIN || res == LONG_MAX  || res <= 0))
      {
        // Default to 16 if entry is not convertible
        res = 16;
      }
      else if (res >= 48)
      {
        // Never allow more than 48k (0x2000 to 0x3FFFF specified
        // through WRX flag)
        res = 48;
      }
      else if (res >=32)
      {
        res = 32;
      }
      else if (res >= 16)
      {
        res = 16;
      }
      else if (res >= 4)
      {
        res = 4;
      }
      c->conf->memory = (int)res;
    }
    else if (!strcasecmp(name, "ExtendFile"))
    {
        // Defaults to off
        c->conf->extendfile = isEnabled(value);
    }
    else if ((!strcasecmp(section, "default")))
    {
      // Following only allowed in default section
      if (!strcasecmp(name, "Load"))
      {
        strcpy(selection, value);
      }
      else if (!strcasecmp(name, "Dir"))
      {
        setDirectory(value);
      }
      else if (!strcasecmp(name, "DoubleShift"))
      {
        // Defaults to off
        c->conf->doubleshift = isEnabled(value);
      }
      else if (!strcasecmp(name, "AllFiles"))
      {
        // Defaults to off
        c->conf->allfiles = isEnabled(value);
      }
      else
      {
        return 0;
      }
    }
    else
    {
      return 0;
    }
  }
  else
  {
    return 0;
  }
  return 1;
}

static int ini_parse_fatfs(const char *filename, ini_handler handler, void *user)
{
  FRESULT result;
  FIL file;
  int error;

  result = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
  if (result != FR_OK)
    return -1;
  error = ini_parse_stream((ini_reader)f_gets, &file, handler, user);
  f_close(&file);
  return error;
}

void emu_ReadDefaultValues(void)
{
  conf_handler_t hand;
  char name[] = "default";
  static bool first = true;

  if (first)
  {
    // Set values in case ini file cannot be read
    general.up = '7';
    general.down = '6';
    general.left = '5';
    general.right = '8';
    general.button = '0';
    general.sound = AY_TYPE_NONE;
    general.acb = false;
    general.computer = ZX81;
    general.M1NOT = false;
    general.WRX = false;
    general.QSUDG = false;
    general.LowRAM = false;
    general.memory = 16;
    general.NTSC = false;
    general.VTol = VTOL;
    general.centre = true;
    general.doubleshift = false;
    general.extendfile = false;
    general.allfiles = false;
    selection[0] = 0;
  }
  else
  {
    memcpy(&general, &root_config, sizeof(general));
  }

  hand.conf = &general;
  hand.section = name;

  // Read the config file from the current directory as the file
  char config[MAX_FULLPATH_LEN];
  strcpy(config, dirPath);
  strcat(config, CONFIG_FILE);

  ini_parse_fatfs(config, handler, &hand);

  if (first)
  {
    memcpy(&root_config, &general, sizeof(general));
  }

  // Set the specific values in the first instance, in case a file is not loaded
  if (first)
  {
    memcpy(&specific, &general, sizeof(specific));
    first = false;
  }
}

// Note this should be called after the filename has been validated
void emu_ReadSpecificValues(const char *filename)
{
  conf_handler_t hand;
  configuration_t used;

  // Store the settings we have before
  memcpy(&used, &specific, (sizeof(specific)));

  // Reset settings to the defaults before attempting to specialise
  memcpy(&specific, &general, (sizeof(specific)));
  resetNeeded = false;

  if (filename)
  {
    hand.conf = &specific;

    // Read the config file from the same directory as the file
    hand.section = &filename[strlen(dirPath)];
    char config[MAX_FULLPATH_LEN];
    strcpy(config, dirPath);
    strcat(config, CONFIG_FILE);

    ini_parse_fatfs(config, handler, &hand);
    // determine whether a reset is required
    resetNeeded =  ((specific.M1NOT != used.M1NOT) ||
                    (specific.memory != used.memory) ||
                    (specific.computer != used.computer) ||
                    (specific.WRX != used.WRX) ||
                    (specific.QSUDG != used.QSUDG) ||
                    (specific.LowRAM != used.LowRAM));
  }
}

/********************************
 * Input and keyboard
 ********************************/
bool emu_UpdateKeyboard(uint8_t* special)
{
  bool ret = hidReadUsbKeyboard(special, specific.doubleshift);

  if (!ret)
  {
    hidJoystickToKeyboard(1,
                          specific.up,
                          specific.down,
                          specific.left,
                          specific.right,
                          specific.button);
  }
  return ret;
}

bool emu_endsWith(const char * s, const char * suffix)
{
  bool retval = false;
  int len = strlen(s);
  int slen = strlen(suffix);
  if (len > slen ) {
    if (!strcasecmp(&s[len-slen], suffix)) {
      retval = true;
    }
  }
   return (retval);
}


/********************************
 * Video
 ********************************/
void emu_DisplayFrame(unsigned char *buf)
{
  displaySetBuffer(buf, DISPLAY_BLANK_BYTE, 0, DISPLAY_STRIDE_BYTE);
}

void emu_BlankScreen(void)
{
  displayBlank(true);
}

/********************************
 * Clock
 ********************************/
extern semaphore_t timer_sem;

void emu_WaitFor50HzTimer(void)
{
#ifdef TIME_SPARE
  static uint32_t count = 0;
  static uint64_t total_time;
  static uint32_t underrun;
  static int32_t  sound_prev = 0;
  static int64_t  int_prev = 0;

  uint64_t start = time_us_64();
#endif
  // Wait for the fifty Hz timer to fire
  sem_acquire_blocking(&timer_sem);

#ifdef TIME_SPARE
  uint64_t taken = (time_us_64() - start);
  if (taken < 100)
    underrun++;
  total_time += taken;

  if (++count == 500)
  {
    count = 0;
    int64_t ints = int_count + int_prev ;
    int_prev = -int_count;
    int32_t sound = sound_count + sound_prev;
    sound_prev = -sound_count;

    printf("Spare ms in 10sec: %lld Under: %d\n", total_time / 1000, underrun);
    printf("int: %lld sound: %d\n", ints, sound);
    total_time = 0;
    underrun = 0;
  }
#endif
}
