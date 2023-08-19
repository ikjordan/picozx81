#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tusb.h"

#include "emuapi.h"
#include "emuvideo.h"
#include "zx81rom.h"

#include "hid_usb.h"
#include "display.h"
#include "ff.h"
#include "menu.h"

typedef enum
{
    PINCF6 = 2,
    PTOPF6 = 8,
    PSYNC = PTOPF6,
    PNTSC = PSYNC + PINCF6,
    PCENTRE = PNTSC + PINCF6,
    PVTOL = PCENTRE + PINCF6,
    PWRX = PVTOL + PINCF6,
    PSOUNDTYPE = PWRX + PINCF6,
    PSTEREOACB = PSOUNDTYPE + PINCF6,
    PBOTTOMF6 = PSTEREOACB
} PositionF6_T;

typedef enum
{
    PINCF7 = 2,
    PTOPF7 = 12,
    PCOMPUTER = PTOPF7,
    PMSIZE = PCOMPUTER + PINCF7,
    PLOWRAM = PMSIZE + PINCF7,
    PM1NOT = PLOWRAM + PINCF7,
    PQSUDG = PM1NOT + PINCF7,
    PCHR128 = PQSUDG + PINCF7,
    PBOTTOMF7 = PCHR128
} PositionF7_T;

typedef struct
{
    FrameSync_T fsync;
    bool        ntsc;
    bool        centre;
    uint16_t    vTol;
    bool        wrx;
    uint16_t    sound;
    bool        stereo;
} ModifyF6_T;

typedef struct
{
    ComputerType_T computer;
    uint16_t    msize;
    bool        lowRAM;
    bool        m1not;
    bool        qsudg;
    bool        chr128;
} RestartF7_T;

static bool buildMenu(bool clone);
static void endMenu(bool blank);
static void delay(void);
static void debounceEnter(void);

static void xorRow(uint row);
static void xorChar(char c, uint col, uint row);
static void writeChar(char c, uint col, uint row);
static void writeString(const char* s, uint col, uint row);
static void writeInvertString(const char* s, uint col, uint row, bool invert);

static int populateFiles(const char* path, uint first);
static bool getFile(char* inout, uint index, bool* direct);

static void showModify(PositionF6_T pos, ModifyF6_T* modify);
static void showRestart(PositionF7_T pos, RestartF7_T* restart);
static void showReboot(FiveSevenSix_T mode);

static bool wasBlank = false;
static bool wasBlack = false;
static uint8_t* currBuff = 0;

static uint8_t* menuscreen = 0;

static bool allfiles = false;

/* 
 * Public interface
 */

// Displays files on screen and allows user to select which file
// to load. Exits with ENTER (load) or ESC (do not load) (f2)
bool loadMenu(void)
{
    char working[MAX_DIRECTORY_LEN];
    char newdir[MAX_DIRECTORY_LEN];
    uint8_t key = 0;
    uint8_t detected = 0;
    uint row = 0;
    bool debounce = true;
    bool exit = false;
    uint fullrow = (uint)((disp.height>>3)-2);

    if (!buildMenu(false))
        return false;

    allfiles = emu_AllFilesRequested();

    strcpy(working, emu_GetDirectory());
    strcpy(newdir,working);
    uint entries = populateFiles(working, 0);
    uint maxrow = (entries < fullrow) ? entries : fullrow;
    uint offset = 0;

    // Highlight first line
    xorRow(row + 1);

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
                        xorRow(row + 1);
                        xorRow(++row + 1);
                    }
                    else
                    {
                        if ((((row + 1 + offset) < entries) && (key == HID_KEY_ARROW_DOWN)) ||
                            (((offset + fullrow) < entries) && (key == HID_KEY_ARROW_RIGHT)))
                        {
                            // Move to next page
                            offset += fullrow;
                            memset(menuscreen, 0x00, disp.stride_byte * disp.height);
                            populateFiles(working, offset);
                            row = 0;
                            maxrow = ((entries - offset) < fullrow) ? entries - offset : fullrow;
                            xorRow(row + 1);
                        }
                    }
                break;

                case HID_KEY_ARROW_UP:
                case HID_KEY_ARROW_LEFT:
                    if (row && (key == HID_KEY_ARROW_UP))
                    {
                        xorRow(row + 1);
                        xorRow(--row + 1);
                    }
                    else
                    {
                        if (offset)
                        {
                            // Move to previous page
                            offset -= fullrow;
                            memset(menuscreen, 0x00, disp.stride_byte * disp.height);
                            populateFiles(working, offset);
                            row = fullrow - 1;
                            maxrow = fullrow;
                            xorRow(row + 1);
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
                            memset(menuscreen, 0x00, disp.stride_byte * disp.height);
                            entries = populateFiles(working, 0);
                            row = 0;
                            maxrow = (entries < fullrow) ? entries : fullrow;
                            xorRow(row + 1);

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

                case HID_KEY_A:
                    if (!allfiles)
                    {
                        allfiles = true;
                        offset = 0;
                        row = 0;
                        memset(menuscreen, 0x00, disp.stride_byte * disp.height);
                        entries = populateFiles(working, 0);
                        maxrow = (entries < fullrow) ? entries : fullrow;
                        xorRow(row + 1);
                    }
                break;

                case HID_KEY_P:
                    if (allfiles)
                    {
                        allfiles = false;
                        offset = 0;
                        row = 0;
                        memset(menuscreen, 0x00, disp.stride_byte * disp.height);
                        entries = populateFiles(working, 0);
                        maxrow = (entries < fullrow) ? entries : fullrow;
                        xorRow(row + 1);
                    }
                break;
            }
        }
        delay();
    } while (!exit && (key != HID_KEY_ESCAPE));

    bool change = (key == HID_KEY_ENTER);

    if (change)
        debounceEnter();

    endMenu(change);
    return (change);
}

// Displays the status of the emulator, pausing execution
// ESC to exit (f3)
bool statusMenu(void)
{
    uint8_t key = 0;
    uint lcount = (disp.height >> 4) - 13;

    char c[20];
    int lhs = (disp.width >> 4) - 10;
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
    writeString(emu_M1NOTRequested() ? "On" : "Off", rhs, lcount++);
    writeString("Extend File:",lhs, lcount);
    writeString(emu_ExtendFileRequested() ? "Yes" : "No", rhs, lcount++);

    writeString("Resolution:", lhs, ++lcount);
    writeString((emu_576Requested() == OFF) ? "640x480x60" : (emu_576Requested() == MATCH) ? "720x568x50.6" : "720x568x50", rhs, lcount++);
    writeString("Frame Sync:", lhs, lcount);
    writeString((emu_FrameSyncRequested() == SYNC_OFF) ? "Off" : (emu_FrameSyncRequested() == SYNC_ON) ? "On" : "On Int", rhs, lcount++);

    writeString("Em TV Type:", lhs, ++lcount);
    writeString(emu_NTSCRequested() ? "NTSC" : "PAL", rhs, lcount++);
    writeString("Centre:", lhs, lcount);
    writeString((emu_CentreY()!=0) ? "Yes" : "No", rhs, lcount++);
    writeString("Vert Tol:", lhs, lcount);
    sprintf(c,"%d lines\n",emu_VTol());
    writeString(c, rhs, lcount++);

    writeString("Sound:", lhs, ++lcount);
    switch (emu_SoundRequested())
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
    if (emu_SoundRequested() != AY_TYPE_NONE)
    {
        writeString("ACB Stereo:", lhs, lcount);
        writeString(emu_ACBRequested() ? "ON" : "OFF", rhs, lcount++);
    }

    writeString("CHAR$128:", lhs, ++lcount);
    writeString(emu_CHR128Requested() ? "Yes" : "No", rhs, lcount++);

    writeString("QS UDG:", lhs, lcount);
    writeString(emu_QSUDGRequested() ? "Yes" : "No", rhs, lcount++);
    if (emu_QSUDGRequested())
    {
        writeString("QS UDG On:", lhs, lcount);
        writeString(UDGEnabled ? "Yes" : "No", rhs, lcount++);
    }

    writeString("Directory:", lhs, ++lcount);
    writeString(emu_GetDirectory(), rhs, lcount++);

    writeString("All Files:", lhs, lcount);
    writeString(emu_AllFilesRequested() ? "Yes" : "No", rhs, lcount++);

    //writeString("Fn Key Map:", lhs, ++lcount);
    //writeString(emu_DoubleShiftRequested() ? "Yes" : "No", rhs, lcount++);

    do
    {
        tuh_task();
        hidNavigateMenu(&key);
        emu_WaitFor50HzTimer();
    } while (key != HID_KEY_ESCAPE);

    endMenu(false);

    return true;
}

// Pauses emulator execution, displays P in the 4 corners and waits for
// ESC to be pressed (f4)
void pauseMenu(void)
{
    uint8_t key = 0;

    if (!buildMenu(true))
        return;

    // XOR a P in each corner - allowing for over scan
    xorChar('P', 1, 1);
    xorChar('P', 1, (disp.height >> 3) - 2);
    xorChar('P', (disp.width >> 3) - 2, 1);
    xorChar('P', (disp.width >> 3) - 2, (disp.height >> 3) - 2);

    // Just wait for ESC to be pressed
    do
    {
        tuh_task();
        hidNavigateMenu(&key);
        emu_WaitFor50HzTimer();
    } while ((key != HID_KEY_ENTER) && (key != HID_KEY_ESCAPE));

    debounceEnter();
    endMenu(false);
}

// Entries that can be modified without rebooting the emulator (f6)
bool modifyMenu(void)
{
    uint8_t key = 0;
    uint8_t debounce = 0;
    PositionF6_T field = PTOPF6;
    ModifyF6_T modify;

    modify.fsync = emu_FrameSyncRequested();
    modify.ntsc = emu_NTSCRequested();
    modify.centre = emu_Centre();
    modify.vTol = emu_VTol();
    modify.wrx = emu_WRXRequested();
    modify.sound = emu_SoundRequested();
    modify.stereo = emu_ACBRequested();

    if (!buildMenu(false))
        return false;

    showModify(field, &modify);

    do
    {
        key = 0;
        tuh_task();
        hidNavigateMenu(&key);
        emu_WaitFor50HzTimer();

        if (debounce != key)
        {
            switch (key)
            {
                case HID_KEY_ARROW_UP:
                    if (field == PTOPF6)
                    {
                        field = PBOTTOMF6;
                    }
                    else
                    {
                        field = (PositionF6_T)((int)field - (int)PositionF6_T::PINCF6);
                    }
                    showModify(field, &modify);
                break;

                case HID_KEY_ARROW_DOWN:
                    if (field == PBOTTOMF6)
                    {
                        field = PTOPF6;
                    }
                    else
                    {
                        field = (PositionF6_T)((int)field + (int)PositionF6_T::PINCF6);
                    }
                    showModify(field, &modify);
                break;

                case HID_KEY_ARROW_RIGHT:
                    if (field == PSYNC)
                    {
                        if (modify.fsync == SYNC_ON_INTERLACED)
                        {
                            modify.fsync = SYNC_OFF;
                        }
                        else
                        {
                            modify.fsync = (FrameSync_T)(((int)modify.fsync) + 1);
                        }
                    }
                    else if (field == PNTSC)
                    {
                        modify.ntsc = ! modify.ntsc;
                    }
                    else if (field == PCENTRE)
                    {
                        modify.centre = ! modify.centre;
                    }
                    else if (field == PVTOL)
                    {
                        if (modify.vTol < 200)
                        {
                            modify.vTol +=5;
                        }
                    }
                    else if (field == PWRX)
                    {
                        modify.wrx = !modify.wrx;
                    }
                    else if (field == PSOUNDTYPE)
                        {
                        modify.sound = (modify.sound + 1) % 3;
                        }
                    else if (field == PSTEREOACB)
                        {
                        modify.stereo = ! modify.stereo;
                    }
                    showModify(field, &modify);
                break;

                case HID_KEY_ARROW_LEFT:
                    if (field == PSYNC)
                    {
                        if (modify.fsync == SYNC_OFF)
                        {
                            modify.fsync = SYNC_ON_INTERLACED;
                        }
                        else
                        {
                            modify.fsync = (FrameSync_T)(((int)modify.fsync) - 1);
                        }
                    }
                    else if (field == PNTSC)
                    {
                        modify.ntsc = ! modify.ntsc;
                    }
                    else if (field == PCENTRE)
                    {
                        modify.centre = ! modify.centre;
                    }
                    else if (field == PVTOL)
                    {
                        if (modify.vTol > 5)
                        {
                            modify.vTol -=5;
                        }
                    }
                    else if (field == PSOUNDTYPE)
                    {
                        modify.sound = (modify.sound + 2) % 3;
                        }
                    else if (field == PSTEREOACB)
                        {
                        modify.stereo = ! modify.stereo;
                    }
                    showModify(field, &modify);
                break;
            }
            debounce = key;

        }
    } while ((key != HID_KEY_ENTER) && (key != HID_KEY_ESCAPE));

    // Update values if requested
    bool update = (key == HID_KEY_ENTER);

    if (update)
    {
        emu_SetFrameSync(modify.fsync);
        emu_SetNTSC(modify.ntsc);
        emu_SetCentre(modify.centre);
        emu_SetVTol(modify.vTol);
        emu_SetWRX(modify.wrx);
        emu_SetSound(modify.sound);
        emu_SetACB(modify.stereo);

        // wait for Enter to be released
        debounceEnter();
    }
    endMenu(false);
    
    return update;
}

static const int allowedMem[] {1, 2, 3, 4, 16, 32, 48};

// Entries that can be modified without rebooting the emulator (f6)
bool restartMenu(void)
{
    uint8_t key = 0;
    uint8_t debounce = 0;
    PositionF7_T field = PTOPF7;
    RestartF7_T restart;

    restart.msize = emu_MemoryRequested();
    restart.lowRAM = emu_LowRAMRequested();
    restart.m1not = emu_M1NOTRequested();
    restart.computer = emu_ComputerRequested();
    restart.qsudg = emu_QSUDGRequested();
    restart.chr128 = emu_CHR128Requested();

    int memLength = (sizeof(allowedMem) / sizeof(int)) - 1;
    int memPos = 0;
    for (int i=0; i<=memLength; ++i)
    {
        if (allowedMem[i] == restart.msize)
        {
            memPos = i;
            break;
        }
    }

    if (!buildMenu(false))
        return false;

    showRestart(field, &restart);

    do
    {
        key = 0;
        tuh_task();
        hidNavigateMenu(&key);
        emu_WaitFor50HzTimer();

        if (debounce != key)
        {
            switch (key)
            {
                case HID_KEY_ARROW_UP:
                    if (field == PTOPF7)
                    {
                        field = PBOTTOMF7;
                    }
                    else
                    {
                        field = (PositionF7_T)((int)field - (int)PositionF7_T::PINCF7);
                    }
                    showRestart(field, &restart);
                break;

                case HID_KEY_ARROW_DOWN:
                    if (field == PBOTTOMF7)
                    {
                        field = PTOPF7;
                    }
                    else
                    {
                        field = (PositionF7_T)((int)field + (int)PositionF7_T::PINCF7);
                    }
                    showRestart(field, &restart);
                break;

                case HID_KEY_ARROW_RIGHT:
                    if (field == PCOMPUTER)
                    {
                        if (restart.computer == ZX81X2)
                        {
                            restart.computer = ZX80;
                        }
                        else
                        {
                            restart.computer = (ComputerType_T)(((int)restart.computer) + 1);
                        }
                    }
                    else if (field == PMSIZE)
                    {
                        if (memPos < memLength)
                        {
                            restart.msize = allowedMem[++memPos];
                        }
                        else
                        {
                            memPos = 0;
                            restart.msize = allowedMem[memPos];
                        }
                    }
                    else if (field == PLOWRAM)
                    {
                        restart.lowRAM = !restart.lowRAM;
                    }
                    else if (field == PM1NOT)
                    {
                        restart.m1not = !restart.m1not;
                    }
                    else if (field == PQSUDG)
                    {
                        restart.qsudg = !restart.qsudg;
                    }
                    else if (field == PCHR128)
                    {
                        restart.chr128 = !restart.chr128;
                    }
                    showRestart(field, &restart);
                break;

                case HID_KEY_ARROW_LEFT:
                    if (field == PCOMPUTER)
                    {
                        if (restart.computer == ZX80)
                        {
                            restart.computer = ZX81X2;
                        }
                        else
                        {
                            restart.computer = (ComputerType_T)(((int)restart.computer) - 1);
                        }
                    }
                    else if (field == PMSIZE)
                    {
                        if (memPos)
                        {
                            restart.msize = allowedMem[--memPos];
                        }
                        else
                        {
                            memPos = memLength;
                            restart.msize = allowedMem[memPos];
                        }
                    }
                    else if (field == PLOWRAM)
                    {
                        restart.lowRAM = !restart.lowRAM;
                    }
                    else if (field == PM1NOT)
                    {
                        restart.m1not = !restart.m1not;
                    }
                    else if (field == PQSUDG)
                    {
                        restart.qsudg = !restart.qsudg;
                    }
                    else if (field == PCHR128)
                    {
                        restart.chr128 = !restart.chr128;
                    }
                    showRestart(field, &restart);
                break;
            }
            debounce = key;

        }
    } while ((key != HID_KEY_ENTER) && (key != HID_KEY_ESCAPE));

    // Update values if requested
    bool update = (key == HID_KEY_ENTER);

    if (update)
    {
        // Validate the something was changed
        if ((restart.computer != emu_ComputerRequested()) ||
            (restart.m1not != emu_M1NOTRequested()) ||
            (restart.msize != emu_MemoryRequested()) ||
            (restart.lowRAM != emu_LowRAMRequested()) ||
            (restart.qsudg != emu_QSUDGRequested()) ||
            (restart.chr128 != emu_CHR128Requested()))
        {
            emu_SetComputer(restart.computer);
            emu_SetMemory(restart.msize);
            emu_SetLowRAM(restart.lowRAM);
            emu_SetM1NOT(restart.m1not);
            emu_SetQSUDG(restart.qsudg);
            emu_SetCHR128(restart.chr128);
        }
        else
        {
            update = false;
        }
        // wait for Enter to be released
        debounceEnter();
    }
    endMenu(false);

    return update;
}

// Entries that will trigger an emulator reboot (f8)
void rebootMenu(void)
{
    uint8_t key = 0;
    uint8_t debounce = 0;
    FiveSevenSix_T initialMode = emu_576Requested();
    FiveSevenSix_T mode = initialMode;

    if (!buildMenu(false))
        return;
    
    showReboot(mode);

    do
    {
        key = 0;
        tuh_task();
        hidNavigateMenu(&key);
        emu_WaitFor50HzTimer();

        if (debounce != key)
        {
            switch (key)
            {

                case HID_KEY_ARROW_RIGHT:
                    if (mode == MATCH)
                    {
                        mode = OFF;
                    }
                    else
                    {
                        mode = (FiveSevenSix_T)(((int)mode) + 1);
                    }
                    showReboot(mode);
                break;

                case HID_KEY_ARROW_LEFT:
                    if (mode == OFF)
                    {
                        mode = MATCH;
                    }
                    else
                    {
                        mode = (FiveSevenSix_T)(((int)mode) - 1);
                    }
                    showReboot(mode);
                break;
            }
            debounce = key;

        }
    } while ((key != HID_KEY_ENTER) && (key != HID_KEY_ESCAPE));
    
    endMenu(false);

    if (key == HID_KEY_ENTER)
    {
        if (initialMode != mode)
        {
            // Write file and reboot
            emu_SetRebootMode(mode);
        }
    }
}

/*
 * Private interface
 */
static bool buildMenu(bool clone)
{
    // Obtain a display buffer
    displayGetFreeBuffer(&menuscreen);

    // Store the current display
    wasBlank = displayIsBlank(&wasBlack);

    if (!wasBlank)
    {
        // Get the current displayed buffer
        displayGetCurrentBuffer(&currBuff);
    }

    if (clone)
    {
        if (wasBlank)
        {
            memset(menuscreen, wasBlack, disp.stride_byte * disp.height);
        }
        else
        {
            uint8_t* s = currBuff;
            uint8_t* d = menuscreen;
            for (int i = 0; i < disp.height; i++)
            {
                memcpy(d, s, disp.stride_byte);
                d += disp.stride_byte;
                s += disp.stride_byte;
            }
        }
    }
    else
    {
        memset(menuscreen, 0x00, disp.stride_byte * disp.height);
    }
    // Display 
    displayBuffer(menuscreen, false, false);
    return true;
}

static void endMenu(bool blank)
{
    // Put the old screen back
    if (wasBlank)
    {
        displayBlank(wasBlack);
    }
    else
    {
        if (blank)
        {
            memset(currBuff, 0x00, disp.stride_byte * disp.height);
        }
        displayBuffer(currBuff, false, true);
    }
}

static void showModify(PositionF6_T pos, ModifyF6_T* modify)
{ 
    uint lcount = (disp.height >> 4) - 13;

    int lhs = (disp.width >> 4) - 10;
    int rhs = lhs + 13;
    char c[20];

    writeString("Modify", lhs + 6, lcount);
    writeString("======", lhs + 6, lcount+1);

    writeInvertString("Frame Sync:", lhs, lcount + PositionF6_T::PSYNC, pos == PositionF6_T::PSYNC);
    writeString((modify->fsync == SYNC_OFF) ? "Off      " : (modify->fsync == SYNC_ON) ? "On       " : "Interlace", rhs , lcount + PositionF6_T::PSYNC);

    writeInvertString("Em TV Type:", lhs, lcount + PositionF6_T::PNTSC, pos == PositionF6_T::PNTSC);
    writeString(modify->ntsc ? "NTSC" : "PAL ", rhs, lcount + PositionF6_T::PNTSC);

    writeInvertString("Vert Tol:", lhs, lcount + PositionF6_T::PVTOL, pos == PositionF6_T::PVTOL);
    sprintf(c,"%d lines  \n",modify->vTol);
    writeString(c, rhs , lcount + PositionF6_T::PVTOL);

    writeInvertString("Centre:", lhs, lcount + PositionF6_T::PCENTRE, pos == PositionF6_T::PCENTRE);
    writeString(modify->centre ? "YES" : "NO ", rhs , lcount + PositionF6_T::PCENTRE);

    writeInvertString("WRX RAM:", lhs, lcount + PositionF6_T::PWRX, pos == PositionF6_T::PWRX);
    writeString(modify->wrx ? "YES" : "NO ", rhs , lcount + PositionF6_T::PWRX);

    writeInvertString("Sound:", lhs, lcount + PositionF6_T::PSOUNDTYPE, pos == PositionF6_T::PSOUNDTYPE);
    writeString((modify->sound == AY_TYPE_QUICKSILVA) ? "QUICKSILVA" : (modify->sound == AY_TYPE_ZONX) ? "ZonX      " : "None      ", rhs , lcount + PositionF6_T::PSOUNDTYPE);

    writeInvertString("Stereo:", lhs, lcount + PositionF6_T::PSTEREOACB, pos == PositionF6_T::PSTEREOACB);
    writeString(modify->stereo ? "ON-ACB" : "OFF   ", rhs , lcount + PositionF6_T::PSTEREOACB);
}

static void showRestart(PositionF7_T pos, RestartF7_T* restart)
{
    uint lcount = (disp.height >> 4) - 13;

    int lhs = (disp.width >> 4) - 10;
    int rhs = lhs + 13;
    char c[20];

    writeString("Restart", lhs + 6, lcount);
    writeString("=======", lhs + 6, lcount+1);

    writeString("Note: CHANGING VALUES", lhs - 1, lcount + 4);
    writeString("      WILL RESTART THE", lhs - 1, lcount + 5);
    writeString("      EMULATED MACHINE", lhs - 1, lcount + 6);

    writeInvertString("Computer:", lhs, lcount + PositionF7_T::PCOMPUTER, pos == PositionF7_T::PCOMPUTER);
    writeString((restart->computer == ZX80) ? "ZX80  " : (restart->computer == ZX81X2) ? "ZX81X2" : "ZX81  ", rhs , lcount + PositionF7_T::PCOMPUTER);

    writeInvertString("Memory:", lhs, lcount + PositionF7_T::PMSIZE, pos == PositionF7_T::PMSIZE);
    sprintf(c,"%0d KB\n",restart->msize);
    writeString(c, rhs , lcount + PositionF7_T::PMSIZE);

    writeInvertString("LOW RAM:", lhs, lcount + PositionF7_T::PLOWRAM, pos == PositionF7_T::PLOWRAM);
    writeString((restart->lowRAM) ? "On " : "Off", rhs , lcount + PositionF7_T::PLOWRAM);

    writeInvertString("MINOT:", lhs, lcount + PositionF7_T::PM1NOT, pos == PositionF7_T::PM1NOT);
    writeString((restart->m1not) ? "On " : "Off", rhs , lcount + PositionF7_T::PM1NOT);

    writeInvertString("CHAR$128:", lhs, lcount + PositionF7_T::PCHR128, pos == PositionF7_T::PCHR128);
    writeString((restart->chr128) ? "On " : "Off", rhs , lcount + PositionF7_T::PCHR128);

    writeInvertString("QS UDG:", lhs, lcount + PositionF7_T::PQSUDG, pos == PositionF7_T::PQSUDG);
    writeString((restart->qsudg) ? "On " : "Off", rhs , lcount + PositionF7_T::PQSUDG);
}

static void showReboot(FiveSevenSix_T mode)
{ 
    uint lcount = (disp.height >> 4) - 13;

    int lhs = (disp.width >> 4) - 10;
    int rhs = lhs + 13;

    writeString("Reboot", lhs + 6, lcount);
    writeString("======", lhs + 6, lcount+1);

    writeString("Note: RESOLUTION", lhs - 1, lcount + 4);
    writeString("      CHANGE WILL", lhs - 1, lcount + 5);
    writeString("      REBOOT THE PICO", lhs - 1, lcount + 6);

    writeInvertString("Resolution:", lhs, lcount + 12, true);
    writeString((mode == OFF) ? "640x480x60  " : (mode == MATCH) ? "720x568x50.6" : "720x568x50  ", rhs, lcount + 12);
}


// Waits for 12th of a second
static void delay(void)
{
    emu_WaitFor50HzTimer();
    emu_WaitFor50HzTimer();
    emu_WaitFor50HzTimer();
    emu_WaitFor50HzTimer();
}

static void debounceEnter(void)
{
    uint8_t key = 0;
    
    hidNavigateMenu(&key);

    while (key == HID_KEY_ENTER)
    {    
        tuh_task();
        emu_WaitFor50HzTimer();
        hidNavigateMenu(&key);
    }
}

// Inverts a row
static void xorRow(uint row)
{
    uint8_t* screen = menuscreen;
    screen += row * disp.stride_bit;
    for (uint i=0; i<disp.stride_bit; ++i)
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
    if ((col < (uint)(disp.width>>3)) && (row < (uint)(disp.height>>3)))
    {
        unsigned int len = strlen(s);

        len = len > ((disp.width>>3)-col-1) ? ((disp.width>>3)-col-1) : len;
        for (uint i=0; i<len; ++i)
        {
            writeChar(s[i], col+i, row);
        }
    }
}

// Write string to screen, terminate string at screen edge, optionally inverting characters
static void writeInvertString(const char* s, uint col, uint row, bool invert)
{
    if ((col < (uint)(disp.width>>3)) && (row < (uint)(disp.height>>3)))
    {
        unsigned int len = strlen(s);

        len = len > ((disp.width>>3)-col-1) ? ((disp.width>>3)-col-1) : len;
        for (uint i=0; i<len; ++i)
        {
            if (invert)
            {
                xorChar(s[i], col+i, row);
            }
            else
            {
                writeChar(s[i], col+i, row);
            }
        }
    }
}

// Write one character to screen
static void writeChar(char c, uint col, uint row)
{
    uint8_t* pos = menuscreen + row * disp.stride_bit + col;
    uint16_t offset = 0x1e00;   // Start of characters in ZX81 ROM

    // Convert from ascii to ZX
    if ((c >= 32) && (c < 128))
    {
        offset += (ascii2zx[c-32] << 3);
    }

    // Find the offset in the ROM
    for (uint i=0; i<8; ++i)
    {
        *pos = zx81rom[offset+i];
        pos += disp.stride_byte;
    }
}

static void xorChar(char c, uint col, uint row)
{
    uint8_t* pos = menuscreen + row * disp.stride_bit + col;
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
        pos += disp.stride_byte;
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
    uint fullrow = (uint)((disp.height>>3)-2);

    // Populate parent directory
    if (path[0])
    {
        if (!first)
        {
            writeString("<..>", 1, 1);
            ++i;
        }
        ++count;
    }

    res = f_opendir(&dir, path);                            /* Open the directory */
    if (res == FR_OK)
    {
        char name[(disp.width>>3) + 1];
        int len = 0;
        name[0] = '<';

        // Find all directories first
        while (true)
        {
            res = f_readdir(&dir, &fno);                    /* Read an entry */
            if (res != FR_OK || fno.fname[0] == 0) break;   /* Break on error or end of entries */

            if ((fno.fattrib & AM_DIR) &&
                ((strlen(path) + strlen(fno.fname) + 1) < MAX_DIRECTORY_LEN) &&
                (strcasecmp(fno.fname, "SYSTEM VOLUME INFORMATION")))
            {
                if ((count >= first) && (count < (first + fullrow)))
                {
                    strncpy(&name[1], fno.fname, sizeof(name)-1);

                    len = strlen(fno.fname) + 1; // Position of closing >
                    if (len > ((disp.width>>3) - 3))
                    {
                        name[((disp.width>>3) - 4)] = '+';
                        name[((disp.width>>3) - 3)] = '>';
                        name[((disp.width>>3) - 2)] = '0';
                    }
                    else
                    {
                        name[len] = '>';
                        name[len + 1] = 0;
                    }
                    writeString(name, 1, i+1);
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

            if ((!(fno.fattrib & AM_DIR) && (strlen(fno.fname) < MAX_FILENAME_LEN)) &&
                ((emu_EndsWith(fno.fname, ".o") || emu_EndsWith(fno.fname, ".p") ||
                  emu_EndsWith(fno.fname, ".80") || emu_EndsWith(fno.fname, ".81") ||
                  allfiles)))
            {
                if ((count >= first) && (count < (first + fullrow)))
                {
                    // Terminate long file names with a +
                    if (strlen(fno.fname) > (uint)((disp.width>>3) - 2))
                    {
                        // Copy and terminate with a +
                        strncpy(name, fno.fname, sizeof(name));
                        name[((disp.width>>3) - 3)] = '+';
                        name[((disp.width>>3) - 2)] = 0;
                        writeString(name, 1, i+1);
                    }
                    else
                    {
                        writeString(fno.fname, 1, i+1);
                    }
                    ++i;
                }
                ++count;
            }
        }
        f_closedir(&dir);
    }
    return count;
}

// Directory path passed in inout
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
    uint count = 0;

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
                    ((emu_EndsWith(fno.fname, ".o") || emu_EndsWith(fno.fname, ".p") ||
                      emu_EndsWith(fno.fname, ".80") || emu_EndsWith(fno.fname, ".81")) ||
                      allfiles))
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

