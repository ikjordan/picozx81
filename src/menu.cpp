#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tusb.h"

#include "emuapi.h"
#include "zx81rom.h"
#include "common.h"

#include "hid_usb.h"
#include "display.h"
#include "ff.h"
#include "menu.h"

static bool buildMenu(bool clone);
static void endMenu(void);
static void delay(void);

static void xorRow(uint row);
static void xorChar(char c, uint col, uint row);
static void writeChar(char c, uint col, uint row);
static void writeString(const char* s, uint col, uint row);
static int populateFiles(const char* path, uint first);
static bool getFile(char* inout, uint index, bool* direct);

static bool wasBlank = false;
static bool wasBlack = false;
static uint8_t* currBuff = 0;
static uint16_t curr_x_off = 0;
static uint16_t curr_y_off = 0;
static uint16_t curr_stride = 0;

static uint8_t* menu = 0;

const static uint16_t MENU_X = DISPLAY_WIDTH;
const static uint16_t MENU_Y = DISPLAY_HEIGHT;
static uint8_t* menuscreen = 0;


static bool buildMenu(bool clone)
{
    // Create a display buffer
    menuscreen = getMenuBuffer();

    // Store the current display
    wasBlank = displayIsBlank(&wasBlack);

    if (!wasBlank)
    {
        // Needed as menu does not have a leading blanking word
        displayGetCurrent(&currBuff, &curr_x_off, &curr_y_off, &curr_stride);
    }

    if (clone)
    {
        if (wasBlank)
        {
            int fill = wasBlack ? 0xff : 0;
            memset(menuscreen, wasBlack, DISPLAY_STRIDE_BYTE * MENU_Y);
        }
        else
        {
            uint8_t* s = currBuff + curr_y_off * curr_stride + curr_x_off;
            uint8_t* d = menuscreen;
            for (int i = 0; i < MENU_Y; i++)
            {
                memcpy(d, s, DISPLAY_STRIDE_BYTE);
                d += DISPLAY_STRIDE_BYTE;
                s += (curr_stride);
            }
        }
    }
    else
    {
        memset(menuscreen, 0x00, DISPLAY_STRIDE_BYTE * MENU_Y);
    }
    displaySetBuffer(menuscreen, 0, 0, DISPLAY_STRIDE_BYTE);
    return true;
}

static void endMenu(void)
{
    // Put the old screen back
    if (wasBlank)
    {
        displayBlank(wasBlack);
    }
    else
    {
        displaySetBuffer(currBuff, curr_x_off, curr_y_off, curr_stride);
    }
}

// Pauses emulator execution, displays P in the 4 corners and waits for
// ESC to be pressed
void pauseMenu(void)
{
    uint8_t key = 0;

    if (!buildMenu(true))
        return;

    // XOR a P in each corner
    xorChar('P', 0, 0);
    xorChar('P', 0, 29);
    xorChar('P', 39, 0);
    xorChar('P', 39, 29);

    // Just wait for ESC to be pressed
    do
    {
        tuh_task();
        hidNavigateMenu(&key);
        emu_WaitFor50HzTimer();
    } while ((key != HID_KEY_ENTER) && (key != HID_KEY_ESCAPE));

    endMenu();
}

// Displays the status of the emulator, pausing execution
// ESC to exit
bool statusMenu(void)
{
    uint8_t key = 0;
    uint lcount = 5;

    char c[20];
    int lhs = 11;
    int rhs = lhs + 13;

    if (!buildMenu(false))
        return false;

    writeString("Status", lhs + 6, lcount++);
    writeString("======", lhs + 6, lcount++);

    writeString("Computer:", lhs, ++lcount);
    writeString((emu_ComputerRequested() == ZX80) ? "ZX80" : (emu_ComputerRequested() == ZX81X2) ? "ZX81X2" : "ZX81", rhs, lcount++);
    writeString("Memory:", lhs, lcount);
    sprintf(c,"%0d KB\n",emu_MemoryRequested());
    writeString(c, rhs, lcount++);
    writeString("WRX RAM:", lhs, lcount);
    writeString(emu_WRXRequested() ? "Yes" : "No", rhs, lcount++);
    writeString("8K-16K RAM:", lhs, lcount);
    writeString(emu_LowRAMRequested() ? "Yes" : "No", rhs, lcount++);
    writeString("M1NOT:", lhs, lcount);
    writeString(emu_M1NOTRequested() ? "ON" : "OFF", rhs, lcount++);
    writeString("Extend File:",lhs, lcount);
    writeString(emu_ExtendFileRequested() ? "Yes" : "No", rhs, lcount++);

    writeString("TV Type:", lhs, ++lcount);
    writeString(emu_NTSCRequested() ? "NTSC" : "PAL", rhs, lcount++);
    writeString("Centre:", lhs, lcount);
    writeString((emu_CentreY()!=0) ? "Yes" : "No", rhs, lcount++);
    writeString("Vert Tol:", lhs, lcount);
    sprintf(c,"%d lines\n",emu_VTol());
    writeString(c, rhs, lcount++);

    writeString("Sound:", lhs, ++lcount);
    switch (emu_soundRequested())
    {
        case AY_TYPE_QUICKSILVA:
            strcpy(c,"QUICKSILVA");
        break;

        case AY_TYPE_ZONX:
            strcpy(c,"ZonX");
        break;

        default:
            strcpy(c,"NONE");
        break;
    }
    writeString(c, rhs, lcount++);
    if (emu_soundRequested() != AY_TYPE_NONE)
    {
        writeString("ACB Stereo:", lhs, lcount);
        writeString(emu_ACBRequested() ? "ON" : "OFF", rhs, lcount++);
    }

    writeString("QS UDG:", lhs, ++lcount);
    writeString(emu_QSUDGRequested() ? "Yes" : "No", rhs, lcount++);
    if (emu_QSUDGRequested())
    {
        writeString("QS UDG On:", lhs, lcount);
        writeString(UDGEnabled ? "Yes" : "No", rhs, lcount++);
    }

    writeString("Directory:", lhs, ++lcount);
    writeString(emu_GetDirectory(), rhs, lcount++);

    writeString("Fn Key Map:", lhs, lcount);
    writeString(emu_DoubleShiftRequested() ? "Yes" : "No", rhs, lcount++);

    do
    {
        tuh_task();
        hidNavigateMenu(&key);
        emu_WaitFor50HzTimer();
    } while (key != HID_KEY_ESCAPE);

    endMenu();

    return true;
}

// Displays files on screen and allows user to select which file
// to load. Exits with ENTER (load) or ESC (do not load)
bool loadMenu(void)
{
    char working[MAX_DIRECTORY_LEN];
    char newdir[MAX_DIRECTORY_LEN];
    uint8_t key = 0;
    uint8_t detected = 0;
    uint row = 0;
    bool debounce = true;
    bool exit = false;

    if (!buildMenu(false))
        return false;

    strcpy(working, emu_GetDirectory());
    strcpy(newdir,working);
    uint entries = populateFiles(working, 0);
    uint maxrow = (entries < MENU_Y>>3) ? entries : MENU_Y>>3;
    uint offset = 0;

    // Highlight first line
    xorRow(row);

    do
    {
        tuh_task();
        detected = hidNavigateMenu(&key);

        // Debounce the enter key, for case when called through LOAD
        if (detected && (key == HID_KEY_ENTER))
        {
            if (debounce) detected = false;
        }
        else
        {
            debounce = false;
        }

        if (detected)
        {
            switch (key)
            {
                case HID_KEY_ARROW_DOWN:
                case HID_KEY_ARROW_RIGHT:
                    if (((row + 1) < maxrow) && (key == HID_KEY_ARROW_DOWN))
                    {
                        xorRow(row++);
                        xorRow(row);
                    }
                    else
                    {
                        if ((((row + 1 + offset) < entries) && (key == HID_KEY_ARROW_DOWN)) ||
                            ((offset + (MENU_Y>>3)) < entries) && (key == HID_KEY_ARROW_RIGHT))
                        {
                            // Move to next page
                            offset += MENU_Y>>3;
                            memset(menuscreen, 0x00, DISPLAY_STRIDE_BYTE * MENU_Y);
                            populateFiles(working, offset);
                            row = 0;
                            maxrow = ((entries - offset) < (MENU_Y>>3)) ? entries - offset : (MENU_Y>>3);
                            xorRow(row);
                        }
                    }
                break;

                case HID_KEY_ARROW_UP:
                case HID_KEY_ARROW_LEFT:
                    if (row && (key == HID_KEY_ARROW_UP))
                    {
                        xorRow(row--);
                        xorRow(row);
                    }
                    else
                    {
                        if (offset)
                        {
                            // Move to previous page
                            offset -= MENU_Y>>3;
                            memset(menuscreen, 0x00, DISPLAY_STRIDE_BYTE * MENU_Y);
                            populateFiles(working, offset);
                            row = (MENU_Y>>3) - 1;
                            maxrow = MENU_Y>>3;
                            xorRow(row);
                        }
                    }
                break;

                case HID_KEY_ENTER:
                    bool direct;
                    if (getFile(working, row + offset, &direct))
                    {
                        if (direct)
                        {
                            // Move to the new directory
                            offset = 0;
                            strcpy(newdir,working);
                            memset(menuscreen, 0x00, DISPLAY_STRIDE_BYTE * MENU_Y);
                            entries = populateFiles(working, 0);
                            row = 0;
                            maxrow = (entries < MENU_Y>>3) ? entries : MENU_Y>>3;
                            xorRow(row);

                            // Need to debounce the enter key again
                            debounce = true;
                        }
                        else
                        {
                            // Load the name file and (possibly) new directory
                            emu_SetDirectory(newdir);
                            emu_SetLoadName(working);
                            exit = true;
                        }
                    }
                    else
                    {
                        // If getFile failed menu will exit with previous settings
                        key = HID_KEY_ESCAPE;
                    }
                break;
            }
        }
        delay();
    } while (!exit && (key != HID_KEY_ESCAPE));

    endMenu();
    return (key == HID_KEY_ENTER);
}

// Waits for 12th of a second
static void delay(void)
{
    emu_WaitFor50HzTimer();
    emu_WaitFor50HzTimer();
    emu_WaitFor50HzTimer();
    emu_WaitFor50HzTimer();
}

// Inverts a row
static void xorRow(uint row)
{
    uint8_t* screen = menuscreen;
    screen += row * DISPLAY_STRIDE_BIT;
    for (uint i=0; i<DISPLAY_STRIDE_BIT; ++i)
    {
        *screen++ ^= 0xff;
    }
}

static char ascii2zx[96]=
  {
   0, 0,11,12,13, 0, 0,11,16,17,23,21,26,22,27,24,
  28,29,30,31,32,33,34,35,36,37,14,25,19,20,18,15,
  23,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,
  53,54,55,56,57,58,59,60,61,62,63,16,24,17,11,22,
  11,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,
  53,54,55,56,57,58,59,60,61,62,63,16,24,17,11, 0
  };

// Write string to screen, terminate string at screen edge
static void writeString(const char* s, uint col, uint row)
{
    if ((col < (MENU_X>>3)) && row < ((MENU_Y>>3)))
    {
        int len = strlen(s);

        len = len > ((MENU_X>>3)-col-1) ? ((MENU_X>>3)-col-1) : len;
        for (uint i=0; i<len; ++i)
        {
            writeChar(s[i], col+i, row);
        }
    }
}

// Write one character to screen
static void writeChar(char c, uint col, uint row)
{
    uint8_t* pos = menuscreen + row * DISPLAY_STRIDE_BIT + col;
    uint16_t offset = 0x1e00;

    // Convert from ascii to ZX
    if ((c >= 32) && (c < 128))
    {
        offset += (ascii2zx[c-32] << 3);
    }

    // Find the offset in the ROM
    for (uint i=0; i<8; ++i)
    {
        *pos = zx81rom[offset+i];
        pos += DISPLAY_STRIDE_BYTE;
    }
}

static void xorChar(char c, uint col, uint row)
{
    uint8_t* pos = menuscreen + row * DISPLAY_STRIDE_BIT + col;
    uint16_t offset = 0x1e00;

    // Convert from ascii to ZX
    if ((c >= 32) && (c < 128))
    {
        offset += (ascii2zx[c-32] << 3);
    }

    // Find the offset in the ROM
    for (uint i=0; i<8; ++i)
    {
        *pos = (zx81rom[offset+i] ^ 0xff);
        pos += DISPLAY_STRIDE_BYTE;
    }
}

// Returns the total number of eligible files and directories.
// Populate screen with files / directories starting at first
static int populateFiles(const char* path, uint first)
{
    FRESULT res;
    DIR dir;
    uint i = 0;
    uint count = 0;
    FILINFO fno;

    // Populate parent directory
    if (path[0])
    {
        if (!first)
        {
            writeString("..", 1, 0);
            ++i;
        }
        ++count;
    }

    res = f_opendir(&dir, path);                            /* Open the directory */
    if (res == FR_OK)
    {
        // Find all directories first
        while (true)
        {
            res = f_readdir(&dir, &fno);                    /* Read an entry */
            if (res != FR_OK || fno.fname[0] == 0) break;   /* Break on error or end of entries */

            if ((fno.fattrib & AM_DIR) &&
                ((strlen(path) + strlen(fno.fname) + 1) < MAX_DIRECTORY_LEN) &&
                (strcasecmp(fno.fname, "SYSTEM VOLUME INFORMATION")))
            {
                if ((count >= first) && (count < (first + (MENU_Y>>3))))
                {
                    writeString(fno.fname, 1, i);
                    ++i;
                }
                ++count;
            }
        }

        // Close the directory and re-read for the files
        f_closedir(&dir);
        f_opendir(&dir, path);

        while(true)
        {
            res = f_readdir(&dir, &fno);                    /* Read an entry */
            if (res != FR_OK || fno.fname[0] == 0) break;   /* Break on error or end of entries */

            if (((!(fno.fattrib & AM_DIR) && (strlen(fno.fname) < MAX_FILENAME_LEN)) &&
                 (emu_endsWith(fno.fname, ".o") || emu_endsWith(fno.fname, ".p") ||
                  emu_endsWith(fno.fname, ".80") || emu_endsWith(fno.fname, ".81"))))
            {
                if ((count >= first) && (count < (first + (MENU_Y>>3))))
                {
                    writeString(fno.fname, 1, i);
                    ++i;
                }
                ++count;
            }
        }
        f_closedir(&dir);
    }
    return count;
}

// Directory path passes in in inout
// inout set to the name of the file / directory matching the index
// direct set true for a directory, false for a file
// Returns true for success, false for failure
// Failure modes should be limited to a failing card, or
// card removal. File /directory names that are too long
// will not have been put in the list
static bool getFile(char* inout, uint index, bool* direct)
{
    FRESULT res;
    DIR dir;
    FILINFO fno;
    bool ret = false;
    int count = 0;

    if (inout[0])
    {
        if (!index)
        {
            // return directory above
            char* term = strrchr(inout, '/');
            *term = 0;
            term = strrchr(inout, '/');
            term ? *++term = 0 : *inout = 0;
            *direct = true;
            return true;
        }
        ++count;
    }

    res = f_opendir(&dir, inout);                            /* Open the directory */
    if (res == FR_OK)
    {
        // find the directories first
        do
        {
            res = f_readdir(&dir, &fno);                    /* Read an entry */
            if (res != FR_OK || fno.fname[0] == 0) break;   /* Break on error or end of entries */
            if ((fno.fattrib & AM_DIR) &&
                ((strlen(inout) + strlen(fno.fname) + 1) < MAX_DIRECTORY_LEN) &&
                (strcasecmp(fno.fname, "SYSTEM VOLUME INFORMATION")))
            {
                ++count;
            }
        }
        while (index >= count);

        f_closedir(&dir);

        // Did we find it?
        if (res == FR_OK && fno.fname[0] != 0)
        {
            // Already know directory will fit
            strcat(inout, fno.fname);
            strcat(inout,"/");
            *direct = true;
            return true;
        }

        // Now find the files
        f_opendir(&dir, inout);                             /* Open the directory */

        do
        {
            res = f_readdir(&dir, &fno);                    /* Read an entry */
            if (res != FR_OK || fno.fname[0] == 0) break;   /* Break on error or end of entries */
            if (!(fno.fattrib & AM_DIR))
            {
                if ((strlen(fno.fname) < MAX_FILENAME_LEN) &&
                    (emu_endsWith(fno.fname, ".o") || emu_endsWith(fno.fname, ".p")||
                        emu_endsWith(fno.fname, ".80") || emu_endsWith(fno.fname, ".81")))
                {
                    ++count;
                }
            }
        }
        while (index >= count);

        // Already know name will fit
        strcpy(inout, fno.fname);
        *direct = false;
        ret = true;
    }
    f_closedir(&dir);
    return ret;
}

