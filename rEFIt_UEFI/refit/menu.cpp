/*
 * refit/menu.c
 * Menu functions
 *
 * Copyright (c) 2006 Christoph Pfisterer
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *  * Neither the name of Christoph Pfisterer nor the names of the
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//#include "Platform.h"
#include "libegint.h"   //this includes platform.h
//#include "../include/scroll_images.h"

#include "Version.h"
//#include "colors.h"

#include "nanosvg.h"
#include "FloatLib.h"
#include "HdaCodecDump.h"
#include "menu.h"
#include "screen.h"
//#include "../Platform/Pointer.h"

#ifndef DEBUG_ALL
#define DEBUG_MENU 1
#else
#define DEBUG_MENU DEBUG_ALL
#endif

#if DEBUG_MENU == 0
#define DBG(...)
#else
#define DBG(...) DebugLog(DEBUG_MENU, __VA_ARGS__)
#endif

//#define PREBOOT_LOG L"EFI\\CLOVER\\misc\\preboot.log"
//#define VBIOS_BIN L"EFI\\CLOVER\\misc\\c0000.bin"
CONST CHAR16 *VBIOS_BIN = L"EFI\\CLOVER\\misc\\c0000.bin";

//#define LSTR(s) L##s

// scrolling definitions
static INTN MaxItemOnScreen = -1;
REFIT_MENU_SCREEN OptionMenu(4, L"Options", NULL, /*InfoLineCount*/0, NULL, 0, NULL, NULL, FALSE, FALSE, 0, 0, 0, 0, {0, 0, 0, 0}, NULL );
extern REFIT_MENU_ITEM_RETURN MenuEntryReturn;
extern UINTN            ThemesNum;
extern CHAR16           *ThemesList[];
extern UINTN            ConfigsNum;
extern CHAR16           *ConfigsList[];
extern UINTN            DsdtsNum;
extern CHAR16           *DsdtsList[];
extern UINTN            AudioNum;
extern HDA_OUTPUTS      AudioList[20];
extern CONST CHAR8            *AudioOutputNames[];
extern CHAR8            NonDetected[];
extern BOOLEAN          GetLegacyLanAddress;
extern UINT8            gLanMac[4][6]; // their MAC addresses
extern EFI_AUDIO_IO_PROTOCOL *AudioIo;

INTN LayoutBannerOffset = 64;
INTN LayoutButtonOffset = 0;
INTN LayoutTextOffset = 0;
INTN LayoutMainMenuHeight = 376;
INTN LayoutAnimMoveForMenuX = 0;
BOOLEAN SavePreBootLog = FALSE;

#define SCROLL_LINE_UP        (0)
#define SCROLL_LINE_DOWN      (1)
#define SCROLL_PAGE_UP        (2)
#define SCROLL_PAGE_DOWN      (3)
#define SCROLL_FIRST          (4)
#define SCROLL_LAST           (5)
#define SCROLL_NONE           (6)
#define SCROLL_SCROLL_DOWN    (7)
#define SCROLL_SCROLL_UP      (8)
#define SCROLL_SCROLLBAR_MOVE (9)


#define TEXT_CORNER_REVISION  (1)
#define TEXT_CORNER_HELP      (2)
#define TEXT_CORNER_OPTIMUS   (3)

#define TITLE_MAX_LEN (SVALUE_MAX_SIZE / sizeof(CHAR16) + 128)

// other menu definitions

#define MENU_FUNCTION_INIT            (0)
#define MENU_FUNCTION_CLEANUP         (1)
#define MENU_FUNCTION_PAINT_ALL       (2)
#define MENU_FUNCTION_PAINT_SELECTION (3)
#define MENU_FUNCTION_PAINT_TIMEOUT   (4)



static CHAR16 ArrowUp[2]   = { ARROW_UP, 0 };
static CHAR16 ArrowDown[2] = { ARROW_DOWN, 0 };

BOOLEAN MainAnime = FALSE;

BOOLEAN ScrollEnabled = FALSE;
BOOLEAN IsDragging = FALSE;

INTN ScrollWidth = 16;
INTN ScrollButtonsHeight = 20;
INTN ScrollBarDecorationsHeight = 5;
INTN ScrollScrollDecorationsHeight = 7;
INTN ScrollbarYMovement;


//#define TextHeight (FONT_CELL_HEIGHT + TEXT_YMARGIN * 2)

// clovy - set row height based on text size
#define RowHeightFromTextHeight (1.35f)

#define TITLEICON_SPACING (16)

//#define ROW0__TILESIZE (144)
//#define ROW1_TILESIZE (64)
#define TILE1_XSPACING (8)
//#define TILE_YSPACING (24)
#define ROW0_SCROLLSIZE (100)
#define INDICATOR_SIZE (52)

//EG_IMAGE *SelectionImages[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
//EG_IMAGE *Buttons[4] = {NULL, NULL, NULL, NULL};
static EG_IMAGE *TextBuffer = NULL;

//EG_PIXEL SelectionBackgroundPixel = { 0xef, 0xef, 0xef, 0xff }; //non-trasparent

//INTN row0TileSize = 144;
//INTN row1TileSize = 64;

static INTN row0Count, row0PosX, row0PosXRunning;
static INTN row1Count, row1PosX, row1PosXRunning;
static INTN *itemPosX = NULL;
static INTN *itemPosY = NULL;
static INTN row0PosY, row1PosY, textPosY, FunctextPosY;
static EG_IMAGE* MainImage;
static INTN OldX = 0, OldY = 0;
static INTN OldTextWidth = 0;
static UINTN OldRow = 0;
static INTN OldTimeoutTextWidth = 0;
static INTN MenuWidth, TimeoutPosY;
static INTN EntriesPosX, EntriesPosY;
static INTN EntriesWidth, EntriesHeight, EntriesGap;
static EG_IMAGE* ScrollbarImage = NULL;
static EG_IMAGE* ScrollbarBackgroundImage = NULL;
static EG_IMAGE* UpButtonImage = NULL;
static EG_IMAGE* DownButtonImage = NULL;
static EG_IMAGE* BarStartImage = NULL;
static EG_IMAGE* BarEndImage = NULL;
static EG_IMAGE* ScrollStartImage = NULL;
static EG_IMAGE* ScrollEndImage = NULL;
static EG_RECT BarStart;
static EG_RECT BarEnd;
static EG_RECT ScrollStart;
static EG_RECT ScrollEnd;
static EG_RECT ScrollTotal;
EG_RECT UpButton;
EG_RECT DownButton;
EG_RECT ScrollbarBackground;
EG_RECT Scrollbar;
EG_RECT ScrollbarOldPointerPlace;
EG_RECT ScrollbarNewPointerPlace;


INPUT_ITEM *InputItems = NULL;
UINTN  InputItemsCount = 0;

INTN OldChosenTheme;
INTN OldChosenConfig;
INTN OldChosenDsdt;
UINTN OldChosenAudio;
UINT8 DefaultAudioVolume = 70;
//INTN NewChosenTheme;
INTN TextStyle;

BOOLEAN mGuiReady = FALSE;


//REFIT_MENU_ENTRY_OTHER MenuEntryOptions  = { L"Options", TAG_OPTIONS, 1, '\0', 'O',  NULL, NULL, NULL,
//  {0, 0, 0, 0}, ActionEnter, ActionEnter, ActionNone, ActionNone, NULL };
//REFIT_MENU_ENTRY_OTHER MenuEntryAbout    = { L"About Clover", TAG_ABOUT_OLD, 1, 0, 'A', NULL, NULL, NULL,
//  {0, 0, 0, 0}, ActionEnter, ActionEnter, ActionNone, ActionNone,  NULL };
//REFIT_MENU_ENTRY_OTHER MenuEntryReset    = { L"Restart Computer", TAG_RESET_OLD, 1, 0, 'R', NULL, NULL, NULL,
//  {0, 0, 0, 0}, ActionSelect, ActionEnter, ActionNone, ActionNone,  NULL };
//REFIT_MENU_ENTRY_OTHER MenuEntryShutdown = { L"Exit Clover", TAG_SHUTDOWN_OLD, 1, 0, 'U',  NULL, NULL, NULL,
//  {0, 0, 0, 0}, ActionSelect, ActionEnter, ActionNone, ActionNone,  NULL };
//REFIT_MENU_ENTRY_OTHER MenuEntryReturn   = { L"Return", TAG_RETURN_OLD, 0, 0, 0,  NULL, NULL, NULL,
//  {0, 0, 0, 0}, ActionEnter, ActionEnter, ActionNone, ActionNone,  NULL };


REFIT_MENU_ITEM_OPTIONS  MenuEntryOptions (L"Options", 1, '\0', 'O', ActionEnter);
REFIT_MENU_ITEM_ABOUT    MenuEntryAbout   (L"About Clover", 1, 0, 'A', ActionEnter);
REFIT_MENU_ITEM_RESET    MenuEntryReset   (L"Restart Computer", 1, 0, 'R', ActionSelect);
REFIT_MENU_ITEM_SHUTDOWN MenuEntryShutdown(L"Exit Clover", 1, 0, 'U', ActionSelect);
REFIT_MENU_ITEM_RETURN   MenuEntryReturn  (L"Return", 0, 0, 0, ActionEnter);



REFIT_MENU_SCREEN MainMenu    = {1, L"Main Menu", NULL, 0, NULL, 0, L"Automatic boot", NULL, FALSE, FALSE, 0, 0, 0, 0, {0, 0, 0, 0}, NULL};
REFIT_MENU_SCREEN AboutMenu   = {2, L"About",     NULL, 0, NULL, 0, NULL,              NULL, FALSE, FALSE, 0, 0, 0, 0, {0, 0, 0, 0}, NULL};
REFIT_MENU_SCREEN HelpMenu    = {3, L"Help",      NULL, 0, NULL, 0, NULL,              NULL, FALSE, FALSE, 0, 0, 0, 0, {0, 0, 0, 0}, NULL};

CONST CHAR16* ArgOptional[NUM_OPT] = {
  L"arch=i386",       //0
  L"arch=x86_64",     //1
  L"-v ",             //2
  L"-f ",             //3
  L"-s ",             //4
  L"-x ",             //5
  L"nv_disable=1",    //6
  L"slide=0",         //7
  L"darkwake=0",      //8
  L"-xcpm",           //9
  L"-gux_no_idle",    //10
  L"-gux_nosleep",    //11
  L"-gux_nomsi",      //12
  L"-gux_defer_usb2", //13
  L"keepsyms=1",      //14
  L"debug=0x100",     //15
  L"kextlog=0xffff",  //16
  L"-alcoff",         //17
  L"-shikioff",       //18
  L"nvda_drv=1"       //19
};

//UINTN RunGenericMenu(IN REFIT_MENU_SCREEN *Screen, IN MENU_STYLE_FUNC StyleFunc, IN OUT INTN *DefaultEntryIndex, OUT REFIT_ABSTRACT_MENU_ENTRY **ChosenEntry);


VOID FillInputs(BOOLEAN New)
{
  UINTN i,j; //for loops
  CHAR8 tmp[41];
//  BOOLEAN bit;

  tmp[40] = 0;  //make it null-terminated

  InputItemsCount = 0;
  if (New) {
    InputItems = (__typeof__(InputItems))AllocateZeroPool(130 * sizeof(INPUT_ITEM)); //XXX
  }

  InputItems[InputItemsCount].ItemType = ASString;  //0
  //even though Ascii we will keep value as Unicode to convert later
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(SVALUE_MAX_SIZE);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, SVALUE_MAX_SIZE, L"%a ", gSettings.BootArgs);
  InputItems[InputItemsCount].ItemType = UNIString; //1
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(32);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 32, L"%s", gSettings.DsdtName); // 1-> 2
  InputItems[InputItemsCount].ItemType = UNIString; //2
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(63);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 63, L"%s", gSettings.BlockKexts);

  InputItems[InputItemsCount].ItemType = RadioSwitch;  //3 - Themes chooser
  InputItems[InputItemsCount++].IValue = 3;

  InputItems[InputItemsCount].ItemType = BoolValue; //4
  InputItems[InputItemsCount++].BValue = gSettings.DropSSDT;
  InputItems[InputItemsCount].ItemType = BoolValue;  //5
  InputItems[InputItemsCount++].BValue = gSettings.GeneratePStates;
  InputItems[InputItemsCount].ItemType = BoolValue;  //6
  InputItems[InputItemsCount++].BValue = gSettings.SlpSmiEnable;
  InputItems[InputItemsCount].ItemType = Decimal;  //7
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(8);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 8, L"%02d", gSettings.PLimitDict);
  InputItems[InputItemsCount].ItemType = Decimal;  //8
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(8);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 8, L"%02d", gSettings.UnderVoltStep);
  InputItems[InputItemsCount].ItemType = BoolValue; //9
  InputItems[InputItemsCount++].BValue = gSettings.GenerateCStates;
  InputItems[InputItemsCount].ItemType = BoolValue; //10
  InputItems[InputItemsCount++].BValue = gSettings.EnableC2;
  InputItems[InputItemsCount].ItemType = BoolValue; //11
  InputItems[InputItemsCount++].BValue = gSettings.EnableC4;
  InputItems[InputItemsCount].ItemType = BoolValue; //12
  InputItems[InputItemsCount++].BValue = gSettings.EnableC6;
  InputItems[InputItemsCount].ItemType = BoolValue; //13
  InputItems[InputItemsCount++].BValue = gSettings.EnableISS;
  InputItems[InputItemsCount].ItemType = Decimal;  //14
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(16);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 16, L"%06d", gSettings.QPI);
  InputItems[InputItemsCount].ItemType = BoolValue; //15
  InputItems[InputItemsCount++].BValue = gSettings.PatchNMI;
  InputItems[InputItemsCount].ItemType = BoolValue; //16
  InputItems[InputItemsCount++].BValue = gSettings.PatchVBios;
  InputItems[InputItemsCount].ItemType = Decimal;  //17
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(20);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 16, L"0x%x", gPlatformFeature);
  InputItems[InputItemsCount].ItemType = Hex;  //18
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(36);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 36, L"0x%X", gSettings.BacklightLevel);
  InputItems[InputItemsCount].ItemType = Decimal;  //19
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(16);
  }
  if (gSettings.BusSpeed > 20000) {
    UnicodeSPrint(InputItems[InputItemsCount++].SValue, 16, L"%06d", gSettings.BusSpeed);
  } else {
    UnicodeSPrint(InputItems[InputItemsCount++].SValue, 16, L"%06d", gCPUStructure.ExternalClock);
  }
  InputItemsCount = 20;
  for (i=0; i<NGFX; i++) {
    InputItems[InputItemsCount].ItemType = ASString;  //20+i*6
    if (New) {
      InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(64);
    }
    UnicodeSPrint(InputItems[InputItemsCount++].SValue, 64, L"%a", gGraphics[i].Model);

    if (gGraphics[i].Vendor == Ati) {
      InputItems[InputItemsCount].ItemType = BoolValue; //21+i*6
      InputItems[InputItemsCount++].BValue = gSettings.InjectATI;
      InputItems[InputItemsCount].ItemType = ASString; //22+6i
      if (New) {
        InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(20);
      }
      if (StrLen(gSettings.FBName) > 2) { //fool proof: cfg_name is 3 character or more.
        UnicodeSPrint(InputItems[InputItemsCount++].SValue, 20, L"%s", gSettings.FBName);
      } else {
        UnicodeSPrint(InputItems[InputItemsCount++].SValue, 20, L"%a", gGraphics[i].Config);
      }
    } else if (gGraphics[i].Vendor == Nvidia) {
      InputItems[InputItemsCount].ItemType = BoolValue; //21+i*6
      InputItems[InputItemsCount++].BValue = gSettings.InjectNVidia;
      InputItems[InputItemsCount].ItemType = ASString; //22+6i
      for (j=0; j<8; j++) {
        AsciiSPrint((CHAR8*)&tmp[2*j], 3, "%02x", gSettings.Dcfg[j]);
      }
      if (New) {
        InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(40);
      }
      UnicodeSPrint(InputItems[InputItemsCount++].SValue, 40, L"%a", tmp);

      //InputItems[InputItemsCount++].SValue = PoolPrint(L"%08x",*(UINT64*)&gSettings.Dcfg[0]);
    } else /*if (gGraphics[i].Vendor == Intel) */ {
      InputItems[InputItemsCount].ItemType = BoolValue; //21+i*6
      InputItems[InputItemsCount++].BValue = gSettings.InjectIntel;
      InputItems[InputItemsCount].ItemType = Hex; //22+6i
      if (New) {
        InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(20);
      }
      UnicodeSPrint(InputItems[InputItemsCount++].SValue, 26, L"0x%08X", gSettings.IgPlatform);
 //     InputItemsCount += 3;
 //     continue;
    }

    InputItems[InputItemsCount].ItemType = Decimal;  //23+6i
    if (New) {
      InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(8);
    }
    if (gSettings.VideoPorts > 0) {
      UnicodeSPrint(InputItems[InputItemsCount++].SValue, 8, L"%02d", gSettings.VideoPorts);
    } else {
      UnicodeSPrint(InputItems[InputItemsCount++].SValue, 8, L"%02d", gGraphics[i].Ports);
    }

    if (gGraphics[i].Vendor == Nvidia) {
      InputItems[InputItemsCount].ItemType = ASString; //24+6i
      for (j=0; j<20; j++) {
        AsciiSPrint((CHAR8*)&tmp[2*j], 3, "%02x", gSettings.NVCAP[j]);
      }
      if (New) {
        InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(84);
      }
      UnicodeSPrint(InputItems[InputItemsCount++].SValue, 84, L"%a", tmp);
    } else { //ATI and others there will be connectors
      InputItems[InputItemsCount].ItemType = Hex; //24+6i
      if (New) {
        InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(20);
      }
      UnicodeSPrint(InputItems[InputItemsCount++].SValue, 20, L"%08lx", gGraphics[i].Connectors);
    }

    InputItems[InputItemsCount].ItemType = BoolValue; //25+6i
    InputItems[InputItemsCount++].BValue = gGraphics[i].LoadVBios;
  }
  //and so on

  InputItemsCount = 44;
  InputItems[InputItemsCount].ItemType = BoolValue; //44
  InputItems[InputItemsCount++].BValue = gSettings.KextPatchesAllowed;
  InputItems[InputItemsCount].ItemType = BoolValue; //45
  InputItems[InputItemsCount++].BValue = gSettings.KernelAndKextPatches.KPKernelCpu;
  InputItems[InputItemsCount].ItemType = BoolValue; //46
  InputItems[InputItemsCount++].BValue = gSettings.KernelAndKextPatches.KPAppleIntelCPUPM;
  InputItems[InputItemsCount].ItemType = BoolValue; //47
  InputItems[InputItemsCount++].BValue = gSettings.KernelAndKextPatches.KPAppleRTC;
  InputItems[InputItemsCount].ItemType = BoolValue; //48
  InputItems[InputItemsCount++].BValue = gSettings.KernelAndKextPatches.KPKernelPm;
  InputItems[InputItemsCount].ItemType = BoolValue; //49
  InputItems[InputItemsCount++].BValue = gSettings.FixMCFG;

  InputItems[InputItemsCount].ItemType = Decimal;  //50
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(16);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 16, L"%06d", gSettings.RefCLK);

  InputItems[InputItemsCount].ItemType = ASString;  //51 OS version if non-detected
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(SVALUE_MAX_SIZE);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, SVALUE_MAX_SIZE, L"%a ", NonDetected);

  InputItems[InputItemsCount].ItemType = BoolValue; //52
  InputItems[InputItemsCount++].BValue = gSettings.InjectEDID;

  //VendorEDID & ProductEDID 53, 54
  InputItems[InputItemsCount].ItemType = Decimal;  //53
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(16);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 16, L"0x%04x", gSettings.VendorEDID);
  InputItems[InputItemsCount].ItemType = Decimal;  //54
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(16);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 16, L"0x%04x", gSettings.ProductEDID);

  // ErmaC: NvidiaGeneric menu selector y/n
  InputItems[InputItemsCount].ItemType = BoolValue; //55
  InputItems[InputItemsCount++].BValue = gSettings.NvidiaGeneric;
  InputItems[InputItemsCount].ItemType = BoolValue; //56
  InputItems[InputItemsCount++].BValue = gSettings.NvidiaWeb;
  InputItems[InputItemsCount].ItemType = BoolValue; //57
  InputItems[InputItemsCount++].BValue = gSettings.ResetHDA;
  InputItems[InputItemsCount].ItemType = BoolValue; //58
  InputItems[InputItemsCount++].BValue = gSettings.AFGLowPowerState;
  InputItems[InputItemsCount].ItemType = BoolValue; //59
  InputItems[InputItemsCount++].BValue = gSettings.HDAInjection;
  InputItems[InputItemsCount].ItemType = Decimal;  // 60
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(64);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 64, L"%d", gSettings.HDALayoutId);

  // syscl change here
  InputItems[InputItemsCount].ItemType = BoolValue; //61
  InputItems[InputItemsCount++].BValue = gSettings.KernelAndKextPatches.KPDELLSMBIOS;
  // end of change

  InputItems[InputItemsCount].ItemType = Hex;  //62
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(24);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 24, L"0x%08x", gFwFeatures);

  InputItems[InputItemsCount].ItemType = Hex;  //63
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(24);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 24, L"0x%08x", gFwFeaturesMask);

  // Debug for KernelAndKextPatches
  InputItems[InputItemsCount].ItemType = BoolValue; //64
  InputItems[InputItemsCount++].BValue = gSettings.KernelAndKextPatches.KPDebug;


  // CSR - aka System Integrity Protection configuration
  InputItems[InputItemsCount].ItemType = CheckBit; //65
  InputItems[InputItemsCount++].IValue = gSettings.BooterConfig;
  InputItems[InputItemsCount].ItemType = CheckBit; //66
  InputItems[InputItemsCount++].IValue = gSettings.CsrActiveConfig;


  InputItems[InputItemsCount].ItemType = CheckBit; //67
  InputItems[InputItemsCount++].IValue = gSettings.FixDsdt;
  InputItems[InputItemsCount].ItemType = CheckBit; //68
  InputItems[InputItemsCount++].IValue = gSettings.OptionsBits;
  InputItems[InputItemsCount].ItemType = CheckBit; //69
  InputItems[InputItemsCount++].IValue = gSettings.FlagsBits;

  InputItems[InputItemsCount].ItemType = Decimal;  //70
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(8);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 8, L"%02d", gSettings.PointerSpeed);
  InputItems[InputItemsCount].ItemType = Decimal;  //71
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(16);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 16, L"%04d", gSettings.DoubleClickTime);
  InputItems[InputItemsCount].ItemType = BoolValue; //72
  InputItems[InputItemsCount++].BValue = gSettings.PointerMirror;

  //reserve for mouse and continue

  InputItemsCount = 74;
  InputItems[InputItemsCount].ItemType = BoolValue; //74
  InputItems[InputItemsCount++].BValue = gSettings.USBFixOwnership;

  InputItems[InputItemsCount].ItemType = Hex;  //75
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(16);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 16, L"0x%04x", gSettings.C3Latency);
  InputItems[InputItemsCount].ItemType = Decimal;  //76
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(16);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 16, L"%02d", gSettings.EnabledCores);
  InputItems[InputItemsCount].ItemType = Decimal;  //77
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(16);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 16, L"%02d", gSettings.SavingMode);

  InputItems[InputItemsCount].ItemType = ASString;  //78
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(64);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 64, L"%a", gSettings.ProductName);
  InputItems[InputItemsCount].ItemType = ASString;  //79
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(64);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 64, L"%a", gSettings.VersionNr);
  InputItems[InputItemsCount].ItemType = ASString;  //80
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(64);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 64, L"%a", gSettings.SerialNr);
  InputItems[InputItemsCount].ItemType = ASString;  //81
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(64);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 64, L"%a", gSettings.BoardNumber);
  InputItems[InputItemsCount].ItemType = ASString;  //82
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(64);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 64, L"%a", gSettings.BoardSerialNumber);
  InputItems[InputItemsCount].ItemType = Decimal;  //83
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(64);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 64, L"%d", gSettings.BoardType);
  InputItems[InputItemsCount].ItemType = ASString;  //84
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(64);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 64, L"%a", gSettings.BoardVersion);
  InputItems[InputItemsCount].ItemType = Decimal;  //85
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(64);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 64, L"%d", gSettings.ChassisType);
  InputItems[InputItemsCount].ItemType = ASString;  //86
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(64);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 64, L"%a", gSettings.RomVersion);
  InputItems[InputItemsCount].ItemType = ASString;  //87
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(64);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 64, L"%a", gSettings.ReleaseDate);

  InputItems[InputItemsCount].ItemType = BoolValue; //88
  InputItems[InputItemsCount++].BValue = gSettings.DoubleFirstState;
  InputItems[InputItemsCount].ItemType = BoolValue; //89
  InputItems[InputItemsCount++].BValue = gSettings.EnableC7;
  InputItems[InputItemsCount].ItemType = RadioSwitch; //90
  InputItems[InputItemsCount++].IValue = 90;

  InputItems[InputItemsCount].ItemType = BoolValue; //91
  InputItems[InputItemsCount++].BValue = gSettings.KernelAndKextPatches.KPKernelLapic;

  InputItems[InputItemsCount].ItemType = BoolValue; //92
  InputItems[InputItemsCount++].BValue = gSettings.USBInjection;
  InputItems[InputItemsCount].ItemType = BoolValue; //93
  InputItems[InputItemsCount++].BValue = gSettings.InjectClockID;

  InputItems[InputItemsCount].ItemType = Hex;  //94
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(26);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 26, L"0x%08X", gSettings.FakeATI);
  InputItems[InputItemsCount].ItemType = Hex;  //95
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(26);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 26, L"0x%08X", gSettings.FakeNVidia);
  InputItems[InputItemsCount].ItemType = Hex;  //96
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(26);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 26, L"0x%08X", gSettings.FakeIntel);

  InputItems[InputItemsCount].ItemType = Hex;  //97
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(26);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 26, L"0x%08X", gSettings.FakeLAN);
  InputItems[InputItemsCount].ItemType = Hex;  //98
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(26);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 26, L"0x%08X", gSettings.FakeWIFI);
  InputItems[InputItemsCount].ItemType = Hex;  //99
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(26);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 26, L"0x%08X", gSettings.FakeSATA);
  InputItems[InputItemsCount].ItemType = Hex;  //100
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(26);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 26, L"0x%08X", gSettings.FakeXHCI);
  InputItems[InputItemsCount].ItemType = CheckBit;  //101
  InputItems[InputItemsCount++].IValue = dropDSM;

  InputItems[InputItemsCount].ItemType = BoolValue; //102
  InputItems[InputItemsCount++].BValue = gSettings.DebugDSDT;
  InputItems[InputItemsCount].ItemType = Hex;  //103
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(26);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 26, L"0x%08X", gSettings.FakeIMEI);
  InputItems[InputItemsCount].ItemType = Hex;  //104
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(26);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 26, L"0x%08X", gSettings.KernelAndKextPatches.FakeCPUID);


  InputItems[InputItemsCount].ItemType = BoolValue; //105
  InputItems[InputItemsCount++].BValue = gSettings.KernelAndKextPatches.KPKernelXCPM;

  InputItems[InputItemsCount].ItemType = BoolValue; //106
  InputItems[InputItemsCount++].BValue = gSettings.StringInjector;
  InputItems[InputItemsCount].ItemType = BoolValue; //107
  InputItems[InputItemsCount++].BValue = gSettings.NoDefaultProperties;
  InputItems[InputItemsCount].ItemType = BoolValue; //108
  InputItems[InputItemsCount++].BValue = gSettings.KernelPatchesAllowed;

  InputItems[InputItemsCount].ItemType = Hex; //109
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(16);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 16, L"%01x", gSettings.DualLink);

  InputItems[InputItemsCount].ItemType = BoolValue; //110
  InputItems[InputItemsCount++].BValue = gSettings.NvidiaNoEFI;
  InputItems[InputItemsCount].ItemType = BoolValue; //111
  InputItems[InputItemsCount++].BValue = gSettings.NvidiaSingle;

  InputItems[InputItemsCount].ItemType = Hex;  //112
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(16);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 16, L"0x%04x", gSettings.IntelMaxValue);

  InputItems[InputItemsCount].ItemType = BoolValue; //113
  InputItems[InputItemsCount++].BValue = gSettings.AutoMerge;
  InputItems[InputItemsCount].ItemType = BoolValue; //114
  InputItems[InputItemsCount++].BValue = gSettings.DeInit;
  InputItems[InputItemsCount].ItemType = BoolValue; //115
  InputItems[InputItemsCount++].BValue = gSettings.NoCaches;
  InputItems[InputItemsCount].ItemType = RadioSwitch;  //116 - DSDT chooser
  InputItems[InputItemsCount++].IValue = 116;

  InputItems[InputItemsCount].ItemType = ASString;  //117
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(64);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 64, L"%a", gSettings.EfiVersion);
  InputItems[InputItemsCount].ItemType = ASString;  //118
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(64);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 64, L"%a", gSettings.BooterCfgStr);

  InputItems[InputItemsCount].ItemType = RadioSwitch;  //119 - Audio chooser
  InputItems[InputItemsCount++].IValue = 119;
  InputItems[InputItemsCount].ItemType = Decimal;  //120
  if (New) {
    InputItems[InputItemsCount].SValue = (__typeof__(InputItems[InputItemsCount].SValue))AllocateZeroPool(16);
  }
  UnicodeSPrint(InputItems[InputItemsCount++].SValue, 16, L"%04d", DefaultAudioVolume);
  
  InputItems[InputItemsCount].ItemType = BoolValue; //121
  InputItems[InputItemsCount++].BValue = gSettings.KernelAndKextPatches.KPPanicNoKextDump;




  //menu for drop table
  if (gSettings.ACPIDropTables) {
    ACPI_DROP_TABLE *DropTable = gSettings.ACPIDropTables;
    while (DropTable) {
      DropTable->MenuItem.ItemType = BoolValue;
      DropTable = DropTable->Next;
    }
  }

  if (ACPIPatchedAML) {
    ACPI_PATCHED_AML *ACPIPatchedAMLTmp = ACPIPatchedAML;
    while (ACPIPatchedAMLTmp) {
      ACPIPatchedAMLTmp->MenuItem.ItemType = BoolValue;
      ACPIPatchedAMLTmp = ACPIPatchedAMLTmp->Next;
    }
  }
}


VOID ApplyInputs(VOID)
{
  EFI_STATUS Status = EFI_NOT_FOUND;
  MACHINE_TYPES Model;
  BOOLEAN NeedSave = TRUE;
  INTN i = 0;
  UINTN j;
//  UINT32 k;
  CHAR16 *ch;
  CHAR8  AString[256];
  TagPtr dict;
//  DBG("ApplyInputs\n");
  if (InputItems[i].Valid) {
    ZeroMem(&gSettings.BootArgs, 256);
    gBootChanged = TRUE;
    ch = InputItems[i].SValue;
    do {
      if (*ch == L'\\') {
        *ch = L'_';
      }
    } while (*(++ch));

    AsciiSPrint(gSettings.BootArgs, 255, "%s ", InputItems[i].SValue);
  }
  i++; //1
  if (InputItems[i].Valid) {
    UnicodeSPrint(gSettings.DsdtName, sizeof(gSettings.DsdtName), L"%s", InputItems[i].SValue);
  }
  i++; //2
  if (InputItems[i].Valid) {
    UnicodeSPrint(gSettings.BlockKexts, sizeof(gSettings.BlockKexts), L"%s", InputItems[i].SValue);
  }
  i++; //3
  if (InputItems[i].Valid) {
    if (GlobalConfig.Theme) {
      FreePool(GlobalConfig.Theme);
    }
    if (OldChosenTheme == 0xFFFF) {
      GlobalConfig.Theme = PoolPrint(L"embedded");
    } else {
      GlobalConfig.Theme = PoolPrint(L"%s", ThemesList[OldChosenTheme]);
      GlobalConfig.DarkEmbedded = FALSE;
      GlobalConfig.Font = FONT_ALFA;
    }

    //will change theme after ESC
    gThemeChanged = TRUE;
  }
  i++; //4
  if (InputItems[i].Valid) {
    gSettings.DropSSDT = InputItems[i].BValue;
  }
  i++; //5
  if (InputItems[i].Valid) {
    gSettings.GeneratePStates = InputItems[i].BValue;
  }
  i++; //6
  if (InputItems[i].Valid) {
    gSettings.SlpSmiEnable = InputItems[i].BValue;
  }
  i++; //7
  if (InputItems[i].Valid) {
//    DBG("InputItems[i]: %s\n", InputItems[i].SValue);
    gSettings.PLimitDict = (UINT8)(StrDecimalToUintn(InputItems[i].SValue) & 0x7F);
//    DBG("Item 7=PLimitDict %d\n", gSettings.PLimitDict);
 }
  i++; //8
  if (InputItems[i].Valid) {
    gSettings.UnderVoltStep = (UINT8)(StrDecimalToUintn(InputItems[i].SValue) & 0x3F);
//    DBG("Item 8=UnderVoltStep %d\n", gSettings.UnderVoltStep);
  }
  i++; //9
  if (InputItems[i].Valid) {
    gSettings.GenerateCStates = InputItems[i].BValue;
  }
  i++; //10
  if (InputItems[i].Valid) {
    gSettings.EnableC2 = InputItems[i].BValue;
  }
  i++; //11
  if (InputItems[i].Valid) {
    gSettings.EnableC4 = InputItems[i].BValue;
  }
  i++; //12
  if (InputItems[i].Valid) {
    gSettings.EnableC6 = InputItems[i].BValue;
  }
  i++; //13
  if (InputItems[i].Valid) {
    gSettings.EnableISS = InputItems[i].BValue;
  }
  i++; //14
  if (InputItems[i].Valid) {
    gSettings.QPI = (UINT16)StrDecimalToUintn(InputItems[i].SValue);
    DBG("applied QPI=%d\n", gSettings.QPI);
  }
  i++; //15
  if (InputItems[i].Valid) {
    gSettings.PatchNMI = InputItems[i].BValue;
  }
  i++; //16
  if (InputItems[i].Valid) {
    gSettings.PatchVBios = InputItems[i].BValue;
  }
  i++; //17
  if (InputItems[i].Valid) {
    gPlatformFeature = (UINT64)StrHexToUint64(InputItems[i].SValue);
    DBG("applied PlatformFeature=0x%x\n", gPlatformFeature);
  }
  i++; //18 | Download-Fritz: There is no GUI element for BacklightLevel; please revise
  if (InputItems[i].Valid) {
    gSettings.BacklightLevel = (UINT16)StrHexToUint64(InputItems[i].SValue);
    gSettings.BacklightLevelConfig = TRUE;
  }
  i++; //19
  if (InputItems[i].Valid) {
    gSettings.BusSpeed = (UINT32)StrDecimalToUintn(InputItems[i].SValue);
    DBG("applied BusSpeed=%d\n", gSettings.BusSpeed);
  }

  i = 19;
  for (j = 0; j < NGFX; j++) {
    i++; //20
    if (InputItems[i].Valid) {
      AsciiSPrint(gGraphics[j].Model, 64, "%s",  InputItems[i].SValue);
    }
    i++; //21
    if (InputItems[i].Valid) {
      if (gGraphics[j].Vendor == Ati) {
        gSettings.InjectATI = InputItems[i].BValue;
      } else if (gGraphics[j].Vendor == Nvidia) {
        gSettings.InjectNVidia = InputItems[i].BValue;
      } else if (gGraphics[j].Vendor == Intel) {
        gSettings.InjectIntel = InputItems[i].BValue;
      }
    }
    i++; //22
    if (InputItems[i].Valid) {
      if (gGraphics[j].Vendor == Ati) {
        UnicodeSPrint(gSettings.FBName, 32, L"%s", InputItems[i].SValue);
      } else if (gGraphics[j].Vendor == Nvidia) {
        ZeroMem(AString, 256);
        AsciiSPrint(AString, 255, "%s", InputItems[i].SValue);
        hex2bin(AString, (UINT8*)&gSettings.Dcfg[0], 8);
      } else if (gGraphics[j].Vendor == Intel) {
        //ig-platform-id for Ivy+ and snb-platform-id for Sandy
        gSettings.IgPlatform = (UINT32)StrHexToUint64(InputItems[i].SValue);
        DBG("applied *-platform-id=0x%x\n", gSettings.IgPlatform);
      }
    }

    if (gGraphics[i].Vendor == Intel) {
      i += 3;
      continue;
    }

    i++; //23
    if (InputItems[i].Valid) {
      gGraphics[j].Ports = (UINT8)(StrDecimalToUintn(InputItems[i].SValue) & 0x0F);
    }
    i++; //24
    if (InputItems[i].Valid) {
      if (gGraphics[j].Vendor == Nvidia) {
        ZeroMem(AString, 256);
        if (StrLen(InputItems[i].SValue) > 0) {
          AsciiSPrint(AString, 255, "%s", InputItems[i].SValue);
          hex2bin(AString, (UINT8*)&gSettings.NVCAP[0], 20);
        }
      } else {
        gGraphics[j].Connectors = (UINT32)StrHexToUint64(InputItems[i].SValue);
        gGraphics[j].ConnChanged = TRUE;
      }
    }
    i++; //25
    if (InputItems[i].Valid) {
      gGraphics[j].LoadVBios = InputItems[i].BValue;
    }
  }  //end of Graphics Cards
  // next number == 42

  i = 44;
  if (InputItems[i].Valid) {
    gSettings.KextPatchesAllowed = InputItems[i].BValue;
    gBootChanged = TRUE;
  }
  i++; //45
  if (InputItems[i].Valid) {
    gSettings.KernelAndKextPatches.KPKernelCpu = InputItems[i].BValue;
    gBootChanged = TRUE;
  }
  i++; //46
  if (InputItems[i].Valid) {
    gSettings.KernelAndKextPatches.KPAppleIntelCPUPM = InputItems[i].BValue;
    gBootChanged = TRUE;
  }
  i++; //47
  if (InputItems[i].Valid) {
    gSettings.KernelAndKextPatches.KPAppleRTC = InputItems[i].BValue;
    gBootChanged = TRUE;
  }
  i++; //48
  if (InputItems[i].Valid) {
     gSettings.KernelAndKextPatches.KPKernelPm = InputItems[i].BValue;
     gBootChanged = TRUE;
  }
  i++; //49
  if (InputItems[i].Valid) {
    gSettings.FixMCFG = InputItems[i].BValue;
  }

  i++; //50
  if (InputItems[i].Valid) {
    gSettings.RefCLK = (UINT32)StrDecimalToUintn(InputItems[i].SValue);
  }

  i++; //51
  if (InputItems[i].Valid) {
    AsciiSPrint(NonDetected, 64, "%s", InputItems[i].SValue);
  }

  i++; //52
  if (InputItems[i].Valid) {
    gSettings.InjectEDID = InputItems[i].BValue;
  }
  i++; //53
  if (InputItems[i].Valid) {
    gSettings.VendorEDID = (UINT16)StrHexToUint64(InputItems[i].SValue);
  }
  i++; //54
  if (InputItems[i].Valid) {
    gSettings.ProductEDID = (UINT16)StrHexToUint64(InputItems[i].SValue);
  }
  i++; //55
  // ErmaC: NvidiaGeneric bool(Y/N)
  if (InputItems[i].Valid) {
    gSettings.NvidiaGeneric = InputItems[i].BValue;
  }
  i++; //56
  if (InputItems[i].Valid) {
    gSettings.NvidiaWeb = InputItems[i].BValue;
  }
  i++; //57
  if (InputItems[i].Valid) {
    gSettings.ResetHDA = InputItems[i].BValue;
  }
  i++; //58
  if (InputItems[i].Valid) {
    gSettings.AFGLowPowerState = InputItems[i].BValue;
  }
  i++; //59
  if (InputItems[i].Valid) {
    gSettings.HDAInjection = InputItems[i].BValue;
  }
  i++; //60
  if (InputItems[i].Valid) {
    gSettings.HDALayoutId = (UINT32)(StrDecimalToUintn(InputItems[i].SValue));
  }
  i++; //61
  if (InputItems[i].Valid) {
    gSettings.KernelAndKextPatches.KPDELLSMBIOS = InputItems[i].BValue;
    // yes, we do need to change gRemapSmBiosIsRequire here as well
    gRemapSmBiosIsRequire = InputItems[i].BValue;
    gBootChanged = TRUE;
  }
  i++; //62
  if (InputItems[i].Valid) {
    gFwFeatures = (UINT32)StrHexToUint64(InputItems[i].SValue);
    DBG("applied FirmwareFeatures=0x%x\n", gFwFeatures);
  }
  i++; //63
  if (InputItems[i].Valid) {
    gFwFeaturesMask = (UINT32)StrHexToUint64(InputItems[i].SValue);
    DBG("applied FirmwareFeaturesMask=0x%x\n", gFwFeaturesMask);
  }
  i++; //64
  if (InputItems[i].Valid) {
    gSettings.KernelAndKextPatches.KPDebug = InputItems[i].BValue;
 //   gBootChanged = TRUE;
  }

  // CSR
  i = 65;
  if (InputItems[i].Valid) {
    gSettings.BooterConfig = InputItems[i].IValue & 0x7F;
  }
  i++; //66
  if (InputItems[i].Valid) {
    gSettings.CsrActiveConfig = InputItems[i].IValue;
  }

  i++; //67
  if (InputItems[i].Valid) {
    gSettings.FixDsdt = InputItems[i].IValue;
  }
  i++; //68
  if (InputItems[i].Valid) {
    gSettings.OptionsBits = InputItems[i].IValue;
  }
  i++; //69
  if (InputItems[i].Valid) {
    gSettings.FlagsBits = InputItems[i].IValue;
  }


  i++; //70
  if (InputItems[i].Valid) {
    INTN Minus = 0;
    if (InputItems[i].SValue[0] == '-') {
      Minus = 1;
    }
    gSettings.PointerSpeed = StrDecimalToUintn(&InputItems[i].SValue[Minus]);
    if (Minus) {
      gSettings.PointerSpeed = -gSettings.PointerSpeed;
    }
//    DBG("Pointer Speed=%d\n", gSettings.PointerSpeed);
  }
  i++; //71
  if (InputItems[i].Valid) {
    gSettings.DoubleClickTime = StrDecimalToUintn(InputItems[i].SValue);
//    DBG("DoubleClickTime=%d ms\n", gSettings.DoubleClickTime);
  }
  i++; //72
  if (InputItems[i].Valid) {
    gSettings.PointerMirror = InputItems[i].BValue;
  }


  i = 74;
  if (InputItems[i].Valid) {
    gSettings.USBFixOwnership = InputItems[i].BValue;
  }
  i++; //75
  if (InputItems[i].Valid) {
    gSettings.C3Latency = (UINT16)StrHexToUint64(InputItems[i].SValue);
  }

  i++; //76
  if (InputItems[i].Valid) {
    gSettings.EnabledCores = (UINT8)StrDecimalToUintn(InputItems[i].SValue);
  }
  i++; //77
  if (InputItems[i].Valid) {
    gSettings.SavingMode = (UINT8)StrDecimalToUintn(InputItems[i].SValue);
  }

  i++; //78
  if (InputItems[i].Valid) {
    AsciiSPrint(gSettings.ProductName, 64, "%s", InputItems[i].SValue);
    // let's fill all other fields based on this ProductName
    // to serve as default
    Model = GetModelFromString(gSettings.ProductName);
    if (Model != MaxMachineType) {
      SetDMISettingsForModel(Model, FALSE);
    }
  }

  i++; //79
  if (InputItems[i].Valid) {
    AsciiSPrint(gSettings.VersionNr, 64, "%s", InputItems[i].SValue);
  }
  i++; //80
  if (InputItems[i].Valid) {
    AsciiSPrint(gSettings.SerialNr, 64, "%s", InputItems[i].SValue);
  }
  i++; //81
  if (InputItems[i].Valid) {
    AsciiSPrint(gSettings.BoardNumber, 64, "%s", InputItems[i].SValue);
  }
  i++; //82
  if (InputItems[i].Valid) {
    AsciiSPrint(gSettings.BoardSerialNumber, 64, "%s", InputItems[i].SValue);
  }
  i++; //83
  if (InputItems[i].Valid) {
    gSettings.BoardType = (UINT8)(StrDecimalToUintn(InputItems[i].SValue) & 0x0F);
  }
  i++; //84
  if (InputItems[i].Valid) {
    AsciiSPrint(gSettings.BoardVersion, 64, "%s", InputItems[i].SValue);
  }
  i++; //85
  if (InputItems[i].Valid) {
    gSettings.ChassisType = (UINT8)(StrDecimalToUintn(InputItems[i].SValue) & 0x0F);
  }
  i++; //86
  if (InputItems[i].Valid) {
    AsciiSPrint(gSettings.RomVersion, 64, "%s", InputItems[i].SValue);
  }
  i++; //87
  if (InputItems[i].Valid) {
    AsciiSPrint(gSettings.ReleaseDate, 64, "%s", InputItems[i].SValue);
  }

  i++; //88
  if (InputItems[i].Valid) {
    gSettings.DoubleFirstState = InputItems[i].BValue;
  }
  i++; //89
  if (InputItems[i].Valid) {
    gSettings.EnableC7 = InputItems[i].BValue;
  }

  i++; //90
  if (InputItems[i].Valid) {
    Status = LoadUserSettings(SelfRootDir, ConfigsList[OldChosenConfig], &dict);
    if (!EFI_ERROR(Status)) {
      Status = GetUserSettings(SelfRootDir, dict);
      if (gConfigDict[2]) FreeTag(gConfigDict[2]);
      gConfigDict[2] = dict;
      UnicodeSPrint(gSettings.ConfigName, 64, L"%s", ConfigsList[OldChosenConfig]);
      gBootChanged = TRUE;
      gThemeChanged = TRUE;
    }
    FillInputs(FALSE);
    NeedSave = FALSE;
  }
  i++; //91
  if (InputItems[i].Valid) {
    gSettings.KernelAndKextPatches.KPKernelLapic = InputItems[i].BValue;
    gBootChanged = TRUE;
  }
  i++; //92
  if (InputItems[i].Valid) {
    gSettings.USBInjection = InputItems[i].BValue;
  }
  i++; //93
  if (InputItems[i].Valid) {
    gSettings.InjectClockID = InputItems[i].BValue;
  }
  i++; //94
  if (InputItems[i].Valid) {
    gSettings.FakeATI = (UINT32)StrHexToUint64(InputItems[i].SValue);
  }
  i++; //95
  if (InputItems[i].Valid) {
    gSettings.FakeNVidia = (UINT32)StrHexToUint64(InputItems[i].SValue);
  }
  i++; //96
  if (InputItems[i].Valid) {
    gSettings.FakeIntel = (UINT32)StrHexToUint64(InputItems[i].SValue);
    DBG("applied FakeIntel=0x%x\n", gSettings.FakeIntel);
  }
  i++; //97
  if (InputItems[i].Valid) {
    gSettings.FakeLAN = (UINT32)StrHexToUint64(InputItems[i].SValue);
  }
  i++; //98
  if (InputItems[i].Valid) {
    gSettings.FakeWIFI = (UINT32)StrHexToUint64(InputItems[i].SValue);
  }
  i++; //99
  if (InputItems[i].Valid) {
    gSettings.FakeSATA = (UINT32)StrHexToUint64(InputItems[i].SValue);
  }
  i++; //100
  if (InputItems[i].Valid) {
    gSettings.FakeXHCI = (UINT32)StrHexToUint64(InputItems[i].SValue);
  }

  i++; //101
  if (InputItems[i].Valid) {
//    gSettings.DropOEM_DSM = (UINT16)StrHexToUint64(InputItems[i].SValue);
    gSettings.DropOEM_DSM = (UINT16)InputItems[i].IValue;
    dropDSM = gSettings.DropOEM_DSM; //?
//    defDSM = TRUE;
  }
  i++; //102
  if (InputItems[i].Valid) {
    gSettings.DebugDSDT = InputItems[i].BValue;
  }
  i++; //103
  if (InputItems[i].Valid) {
    gSettings.FakeIMEI = (UINT32)StrHexToUint64(InputItems[i].SValue);
  }

  i++; //104
  if (InputItems[i].Valid) {
    gSettings.KernelAndKextPatches.FakeCPUID = (UINT32)StrHexToUint64(InputItems[i].SValue);
    DBG("applied FakeCPUID=%06x\n", gSettings.KernelAndKextPatches.FakeCPUID);
    gBootChanged = TRUE;
  }

  i++; //105
  if (InputItems[i].Valid) {
    gSettings.KernelAndKextPatches.KPKernelXCPM = InputItems[i].BValue;
    DBG("applied KernelXCPM\n");
    gBootChanged = TRUE;
  }

  i++; //106
  if (InputItems[i].Valid) {
    gSettings.StringInjector = InputItems[i].BValue;
  }

  i++; //107
  if (InputItems[i].Valid) {
    gSettings.NoDefaultProperties = InputItems[i].BValue;
  }

  i++; //108
  if (InputItems[i].Valid) {
    gSettings.KernelPatchesAllowed = InputItems[i].BValue;
    gBootChanged = TRUE;
  }

  i++; //109
  if (InputItems[i].Valid) {
    gSettings.DualLink = (UINT32)StrHexToUint64(InputItems[i].SValue);
    DBG("applied DualLink=%x\n", gSettings.DualLink);
  }

  i++; //110
  if (InputItems[i].Valid) {
    gSettings.NvidiaNoEFI = InputItems[i].BValue;
  }

  i++; //111
  if (InputItems[i].Valid) {
    gSettings.NvidiaSingle = InputItems[i].BValue;
  }
  i++; //112
  if (InputItems[i].Valid) {
    gSettings.IntelMaxValue = (UINT16)StrHexToUint64(InputItems[i].SValue);
  }
  i++; //113
  if (InputItems[i].Valid) {
    gSettings.AutoMerge = InputItems[i].BValue;
  }
  i++; //114
  if (InputItems[i].Valid) {
    gSettings.DeInit = InputItems[i].BValue;
  }
  i++; //115
  if (InputItems[i].Valid) {
    gSettings.NoCaches = InputItems[i].BValue;
  }
  i++; //116
  if (InputItems[i].Valid) {
    if (OldChosenDsdt == 0xFFFF) {
      UnicodeSPrint(gSettings.DsdtName, 64, L"BIOS.aml");
    } else {
      UnicodeSPrint(gSettings.DsdtName, 64, L"%s", DsdtsList[OldChosenDsdt]);
    }
  }
  i++; //117
  if (InputItems[i].Valid) {
    AsciiSPrint(gSettings.EfiVersion, 64, "%s", InputItems[i].SValue);
  }
  i++; //118
  if (InputItems[i].Valid) {
    AsciiSPrint(gSettings.BooterCfgStr, 64, "%s", InputItems[i].SValue);
  }
  i++; //119
  if (InputItems[i].Valid) {
    EFI_DEVICE_PATH_PROTOCOL*  DevicePath = NULL;
    UINT8 TmpIndex = OldChosenAudio & 0xFF;
    DBG("Chosen output %d:%s_%a\n", OldChosenAudio, AudioList[OldChosenAudio].Name, AudioOutputNames[OldChosenAudio]);

    DevicePath = DevicePathFromHandle(AudioList[OldChosenAudio].Handle);
    if (DevicePath != NULL) {
      SetNvramVariable(L"Clover.SoundDevice", &gEfiAppleBootGuid,
                       EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                       GetDevicePathSize(DevicePath), (UINT8 *)DevicePath);
      SetNvramVariable(L"Clover.SoundIndex", &gEfiAppleBootGuid,
                       EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                       1, (UINT8 *)&TmpIndex);

    }
  }
  i++; //120
  if (InputItems[i].Valid) {
    DefaultAudioVolume = (UINT8)StrDecimalToUintn(InputItems[i].SValue);
    if (DefaultAudioVolume > 100) {
        // correct wrong input
        DefaultAudioVolume = 90;
        UnicodeSPrint(InputItems[i].SValue, 16, L"%04d", DefaultAudioVolume);
    }
    SetNvramVariable(L"Clover.SoundVolume", &gEfiAppleBootGuid,
                     EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                     1, &DefaultAudioVolume);
  }
  i++; //121
  if (InputItems[i].Valid) {
    gSettings.KernelAndKextPatches.KPPanicNoKextDump = InputItems[i].BValue;
    gBootChanged = TRUE;
  }


  if (NeedSave) {
    SaveSettings();
  }
}

VOID REFIT_MENU_SCREEN::AddMenuInfo(CONST CHAR16 *Line)
{
  REFIT_INFO_DIALOG *InputBootArgs;

//  InputBootArgs = (__typeof__(InputBootArgs))AllocateZeroPool(sizeof(REFIT_INPUT_DIALOG));
  InputBootArgs = new REFIT_INFO_DIALOG;
  InputBootArgs->Title = PoolPrint(L"%s", Line);
//  InputBootArgs->Tag = TAG_INFO;
//  InputBootArgs->Item = NULL;
  InputBootArgs->AtClick = ActionLight;
  AddMenuEntry(InputBootArgs, true);
}

VOID AboutRefit(VOID)
{
  //  CHAR8* Revision = NULL;
  if (!(GlobalConfig.HideUIFlags & HIDEUI_FLAG_MENU_TITLE_IMAGE)) {
    AboutMenu.TitleImage = BuiltinIcon(BUILTIN_ICON_FUNC_ABOUT);
  } else {
    AboutMenu.TitleImage = NULL;
  }
  if (AboutMenu.Entries.size() == 0) {
    AddMenuInfo(&AboutMenu, PoolPrint(L"Clover Version 2.5k rev %s", gFirmwareRevision)); // by Slice, dmazar, apianti, JrCs, pene and others");
#ifdef FIRMWARE_BUILDDATE
    AddMenuInfo(&AboutMenu, PoolPrint(L" Build: %a", FIRMWARE_BUILDDATE));
#else
    AddMenuInfo(&AboutMenu, L" Build: unknown");
#endif
    AddMenuInfo(&AboutMenu, L"");
    AddMenuInfo(&AboutMenu, L"Based on rEFIt (c) 2006-2010 Christoph Pfisterer");
    AddMenuInfo(&AboutMenu, L"Portions Copyright (c) Intel Corporation");
    AddMenuInfo(&AboutMenu, L"Developers:");
    AddMenuInfo(&AboutMenu, L"  Slice, dmazar, apianti, JrCs, pene, usrsse2");
    AddMenuInfo(&AboutMenu, L"Credits also:");
    AddMenuInfo(&AboutMenu, L"  Kabyl, pcj, jadran, Blackosx, STLVNUB, ycr.ru");
    AddMenuInfo(&AboutMenu, L"  FrodoKenny, skoczi, crazybirdy, Oscar09, xsmile");
    AddMenuInfo(&AboutMenu, L"  cparm, rehabman, nms42, Sherlocks, Zenith432");
    AddMenuInfo(&AboutMenu, L"  stinga11, TheRacerMaster, solstice, SoThOr, DF");
    AddMenuInfo(&AboutMenu, L"  cecekpawon, Micky1979, Needy, joevt, ErmaC, vit9696");
    AddMenuInfo(&AboutMenu, L"  ath, savvas, syscl, goodwin_c, clovy, jief_machak");
    AddMenuInfo(&AboutMenu, L"  projectosx.com, applelife.ru, insanelymac.com");
    AddMenuInfo(&AboutMenu, L"");
    AddMenuInfo(&AboutMenu, L"Running on:");
    AddMenuInfo(&AboutMenu, PoolPrint(L" EFI Revision %d.%02d",
                                      gST->Hdr.Revision >> 16, gST->Hdr.Revision & ((1 << 16) - 1)));
#if defined(MDE_CPU_IA32)
    AddMenuInfo(&AboutMenu, L" Platform: i386 (32 bit)");
#elif defined(MDE_CPU_X64)
    AddMenuInfo(&AboutMenu, L" Platform: x86_64 (64 bit)");
#else
    AddMenuInfo(&AboutMenu, L" Platform: unknown");
#endif
    AddMenuInfo(&AboutMenu, PoolPrint(L" Firmware: %s rev %d.%d", gST->FirmwareVendor, gST->FirmwareRevision >> 16, gST->FirmwareRevision & ((1 << 16) - 1)));
    AddMenuInfo(&AboutMenu, PoolPrint(L" Screen Output: %s", egScreenDescription()));
    AboutMenu.AnimeRun = GetAnime(&AboutMenu);
    AddMenuEntry(&AboutMenu, &MenuEntryReturn, false);
  } else if (AboutMenu.Entries.size() >= 2) {
    /*
      EntryCount instead of InfoLineCount. Lastline == return/back. Is necessary recheck screen res here?
    */
    FreePool(AboutMenu.Entries[AboutMenu.Entries.size()-2].Title);
    AboutMenu.Entries[AboutMenu.Entries.size()-2].Title = PoolPrint(L" Screen Output: %s", egScreenDescription());
  }

  RunMenu(&AboutMenu, NULL);
}

VOID HelpRefit(VOID)
{
  if (!(GlobalConfig.HideUIFlags & HIDEUI_FLAG_MENU_TITLE_IMAGE)) {
    HelpMenu.TitleImage = BuiltinIcon(BUILTIN_ICON_FUNC_HELP);
  } else {
    HelpMenu.TitleImage = NULL;
  }
  if (HelpMenu.Entries.size() == 0) {
    switch (gLanguage)
    {
      case russian:
        AddMenuInfo(&HelpMenu, L"ESC - Выход из подменю, обновление главного меню");
        AddMenuInfo(&HelpMenu, L"F1  - Помощь по горячим клавишам");
        AddMenuInfo(&HelpMenu, L"F2  - Сохранить отчет в preboot.log (только если FAT32)");
        AddMenuInfo(&HelpMenu, L"F3  - Показать скрытые значки в меню");
        AddMenuInfo(&HelpMenu, L"F4  - Родной DSDT сохранить в EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F5  - Патченный DSDT сохранить в EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F6  - Сохранить ВидеоБиос в EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F7  - Проверить звук на выбранном выходе");
        AddMenuInfo(&HelpMenu, L"F8  - Сделать дамп звуковых устройств в EFI/CLOVER/misc/");
        AddMenuInfo(&HelpMenu, L"F9  - Изменить разрешение экрана на одно из возможных");
        AddMenuInfo(&HelpMenu, L"F10 - Снимок экрана в папку EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F11 - Reset NVRAM");
        AddMenuInfo(&HelpMenu, L"F12 - Извлечь указанный DVD");
        AddMenuInfo(&HelpMenu, L"Пробел - Дополнительное меню запуска выбранного тома");
        AddMenuInfo(&HelpMenu, L"Цифры 1-9 - Быстрый запуск тома по порядку в меню");
        AddMenuInfo(&HelpMenu, L"A (About) - О загрузчике");
        AddMenuInfo(&HelpMenu, L"O (Options) - Дополнительные настройки");
        AddMenuInfo(&HelpMenu, L"R (Reset) - Теплый перезапуск");
        AddMenuInfo(&HelpMenu, L"U (go oUt) - Завершить работу в Кловере");
        AddMenuInfo(&HelpMenu, L"S (Shell) - Переход в режим командной строки");
        break;
      case ukrainian:
        AddMenuInfo(&HelpMenu, L"ESC - Вийти з меню, оновити головне меню");
        AddMenuInfo(&HelpMenu, L"F1  - Ця довідка");
        AddMenuInfo(&HelpMenu, L"F2  - Зберегти preboot.log (тiльки FAT32)");
        AddMenuInfo(&HelpMenu, L"F3  - Відображати приховані розділи");
        AddMenuInfo(&HelpMenu, L"F4  - Зберегти OEM DSDT в EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F5  - Зберегти патчений DSDT в EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F7  - Check sound on selected output");
        AddMenuInfo(&HelpMenu, L"F8  - Make audio outputs dump into EFI/CLOVER/misc/");
        AddMenuInfo(&HelpMenu, L"F9  - Switch screen resoluton to next possible mode");
        AddMenuInfo(&HelpMenu, L"F6  - Зберегти VideoBios в EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F10 - Зберегти знімок екрану в EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F11 - Reset NVRAM");
        AddMenuInfo(&HelpMenu, L"F12 - Відкрити обраний диск (DVD)");
        AddMenuInfo(&HelpMenu, L"Пробіл - докладніше про обраний пункт меню");
        AddMenuInfo(&HelpMenu, L"Клавіші 1-9 -  клавіші пунктів меню");
        AddMenuInfo(&HelpMenu, L"A - Про систему");
        AddMenuInfo(&HelpMenu, L"O - Опції меню");
        AddMenuInfo(&HelpMenu, L"R - Перезавантаження");
        AddMenuInfo(&HelpMenu, L"U - Відключити ПК");
        AddMenuInfo(&HelpMenu, L"S - Shell");
        break;
      case spanish:
        AddMenuInfo(&HelpMenu, L"ESC - Salir de submenu o actualizar el menu principal");
        AddMenuInfo(&HelpMenu, L"F1  - Esta Ayuda");
        AddMenuInfo(&HelpMenu, L"F2  - Guardar preboot.log (Solo FAT32)");
        AddMenuInfo(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfo(&HelpMenu, L"F4  - Guardar DSDT oem en EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F5  - Guardar DSDT parcheado en EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F6  - Guardar VideoBios en EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F7  - Check sound on selected output");
        AddMenuInfo(&HelpMenu, L"F8  - Make audio outputs dump into EFI/CLOVER/misc/");
        AddMenuInfo(&HelpMenu, L"F9  - Switch screen resoluton to next possible mode");
        AddMenuInfo(&HelpMenu, L"F10 - Guardar Captura de pantalla en EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F11 - Reset NVRAM");
        AddMenuInfo(&HelpMenu, L"F12 - Expulsar volumen seleccionado (DVD)");
        AddMenuInfo(&HelpMenu, L"Espacio - Detalles acerca selected menu entry");
        AddMenuInfo(&HelpMenu, L"Digitos 1-9 - Atajo a la entrada del menu");
        AddMenuInfo(&HelpMenu, L"A - Menu Acerca de");
        AddMenuInfo(&HelpMenu, L"O - Menu Optiones");
        AddMenuInfo(&HelpMenu, L"R - Reiniciar Equipo");
        AddMenuInfo(&HelpMenu, L"U - Apagar");
        AddMenuInfo(&HelpMenu, L"S - Shell");
        break;
      case portuguese:
      case brasil:
        AddMenuInfo(&HelpMenu, L"ESC - Sai do submenu, atualiza o menu principal");
        AddMenuInfo(&HelpMenu, L"F1  - Esta ajuda");
        AddMenuInfo(&HelpMenu, L"F2  - Salva preboot.log (somente FAT32)");
        AddMenuInfo(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfo(&HelpMenu, L"F4  - Salva oem DSDT em EFI/CLOVER/ACPI/origin/ (somente FAT32)");
        AddMenuInfo(&HelpMenu, L"F5  - Salva DSDT corrigido em EFI/CLOVER/ACPI/origin/ (somente FAT32)");
        AddMenuInfo(&HelpMenu, L"F6  - Salva VideoBios em EFI/CLOVER/misc/ (somente FAT32)");
        AddMenuInfo(&HelpMenu, L"F7  - Check sound on selected output");
        AddMenuInfo(&HelpMenu, L"F8  - Make audio outputs dump into EFI/CLOVER/misc/");
        AddMenuInfo(&HelpMenu, L"F9  - Switch screen resoluton to next possible mode");
        AddMenuInfo(&HelpMenu, L"F10 - Salva screenshot em EFI/CLOVER/misc/ (somente FAT32)");
        AddMenuInfo(&HelpMenu, L"F11 - Reset NVRAM");
        AddMenuInfo(&HelpMenu, L"F12 - Ejeta o volume selecionado (DVD)");
        AddMenuInfo(&HelpMenu, L"Espaco - Detalhes sobre a opcao do menu selecionada");
        AddMenuInfo(&HelpMenu, L"Tecle 1-9 - Atalho para as entradas do menu");
        AddMenuInfo(&HelpMenu, L"A - Sobre o Menu");
        AddMenuInfo(&HelpMenu, L"O - Opcoes do Menu");
        AddMenuInfo(&HelpMenu, L"R - Reiniciar");
        AddMenuInfo(&HelpMenu, L"U - Desligar");
        AddMenuInfo(&HelpMenu, L"S - Shell");
        break;
      case italian:
        AddMenuInfo(&HelpMenu, L"ESC - Esci dal submenu, Aggiorna menu principale");
        AddMenuInfo(&HelpMenu, L"F1  - Aiuto");
        AddMenuInfo(&HelpMenu, L"F2  - Salva il preboot.log (solo su FAT32)");
        AddMenuInfo(&HelpMenu, L"F3  - Mostra volumi nascosti");
        AddMenuInfo(&HelpMenu, L"F4  - Salva il DSDT oem in EFI/CLOVER/ACPI/origin/ (solo suFAT32)");
        AddMenuInfo(&HelpMenu, L"F5  - Salva il patched DSDT in EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F6  - Salva il VideoBios in EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F7  - Controlla il suono sull'uscita selezionata");
        AddMenuInfo(&HelpMenu, L"F8  - Scarica le uscite audio in EFI/CLOVER/misc/");
        AddMenuInfo(&HelpMenu, L"F9  - Cambia la risoluzione dello schermo alla prossima disponibile");
        AddMenuInfo(&HelpMenu, L"F10 - Salva screenshot in EFI/CLOVER/misc/ (solo su FAT32)");
        AddMenuInfo(&HelpMenu, L"F11 - Resetta NVRAM");
        AddMenuInfo(&HelpMenu, L"F12 - Espelli il volume selezionato (DVD)");
        AddMenuInfo(&HelpMenu, L"Spazio - Dettagli sul menu selezionato");
        AddMenuInfo(&HelpMenu, L"Digita 1-9 - Abbreviazioni per il menu");
        AddMenuInfo(&HelpMenu, L"A - Informazioni");
        AddMenuInfo(&HelpMenu, L"O - Menu Opzioni");
        AddMenuInfo(&HelpMenu, L"R - Riavvio");
        AddMenuInfo(&HelpMenu, L"U - Spegnimento");
        AddMenuInfo(&HelpMenu, L"S - Shell");
        break;
      case german:
        AddMenuInfo(&HelpMenu, L"ESC - Zurueck aus Untermenue, Hauptmenue erneuern");
        AddMenuInfo(&HelpMenu, L"F1  - Diese Hilfe");
        AddMenuInfo(&HelpMenu, L"F2  - Sichere preboot.log (nur mit FAT32)");
        AddMenuInfo(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfo(&HelpMenu, L"F4  - Sichere OEM DSDT in EFI/CLOVER/ACPI/origin/ (nur mit FAT32)");
        AddMenuInfo(&HelpMenu, L"F5  - Sichere gepatchtes DSDT in EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F6  - Sichere VideoBios in EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F7  - Check sound on selected output");
        AddMenuInfo(&HelpMenu, L"F8  - Make audio outputs dump into EFI/CLOVER/misc/");
        AddMenuInfo(&HelpMenu, L"F9  - Switch screen resoluton to next possible mode");
        AddMenuInfo(&HelpMenu, L"F10 - Sichere Bildschirmfoto in EFI/CLOVER/misc/ (nur mit FAT32)");
        AddMenuInfo(&HelpMenu, L"F11 - Reset NVRAM");
        AddMenuInfo(&HelpMenu, L"F12 - Volume auswerfen (DVD)");
        AddMenuInfo(&HelpMenu, L"Leertaste - Details über den gewählten Menue Eintrag");
        AddMenuInfo(&HelpMenu, L"Zahlen 1-9 - Kurzwahl zum Menue Eintrag");
        AddMenuInfo(&HelpMenu, L"A - Menue Informationen");
        AddMenuInfo(&HelpMenu, L"O - Menue Optionen");
        AddMenuInfo(&HelpMenu, L"R - Neustart");
        AddMenuInfo(&HelpMenu, L"U - Ausschalten");
        AddMenuInfo(&HelpMenu, L"S - Shell");
        break;
      case dutch:
        AddMenuInfo(&HelpMenu, L"ESC - Verlaat submenu, Vernieuw hoofdmenu");
        AddMenuInfo(&HelpMenu, L"F1  - Onderdeel hulp");
        AddMenuInfo(&HelpMenu, L"F2  - preboot.log opslaan (Alleen FAT32)");
        AddMenuInfo(&HelpMenu, L"F3  - Verborgen opties weergeven");
        AddMenuInfo(&HelpMenu, L"F4  - Opslaan oem DSDT in EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F5  - Opslaan gepatchte DSDT in EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F6  - Opslaan VideoBios in EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F7  - Controleer geluid op geselecteerde uitgang");
        AddMenuInfo(&HelpMenu, L"F8  - Opslaan audio uitgangen in EFI/CLOVER/misc/");
        AddMenuInfo(&HelpMenu, L"F9  - Wijzig schermresolutie naar eerstvolgende mogelijke modus");
        AddMenuInfo(&HelpMenu, L"F10 - Opslaan schermafdruk in EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F11 - Reset NVRAM");
        AddMenuInfo(&HelpMenu, L"F12 - Uitwerpen geselecteerd volume (DVD)");
        AddMenuInfo(&HelpMenu, L"Spatie - Details over geselecteerd menuoptie");
        AddMenuInfo(&HelpMenu, L"Cijfers 1-9 - Snelkoppeling naar menuoptie");
        AddMenuInfo(&HelpMenu, L"A - Menu Over");
        AddMenuInfo(&HelpMenu, L"O - Menu Opties");
        AddMenuInfo(&HelpMenu, L"R - Soft Reset");
        AddMenuInfo(&HelpMenu, L"U - Verlaten");
        AddMenuInfo(&HelpMenu, L"S - Shell");
        break;
      case french:
        AddMenuInfo(&HelpMenu, L"ESC - Quitter sous-menu, Retour menu principal");
        AddMenuInfo(&HelpMenu, L"F1  - Aide");
        AddMenuInfo(&HelpMenu, L"F2  - Enregistrer preboot.log (FAT32 only)");
        AddMenuInfo(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfo(&HelpMenu, L"F4  - Enregistrer oem DSDT dans EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F5  - Enregistrer DSDT modifié dans EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F6  - Enregistrer VideoBios dans EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F7  - Check sound on selected output");
        AddMenuInfo(&HelpMenu, L"F8  - Make audio outputs dump into EFI/CLOVER/misc/");
        AddMenuInfo(&HelpMenu, L"F9  - Switch screen resoluton to next possible mode");
        AddMenuInfo(&HelpMenu, L"F10 - Enregistrer la capture d'écran dans EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F11 - Reset NVRAM");
        AddMenuInfo(&HelpMenu, L"F12 - Ejecter le volume (DVD)");
        AddMenuInfo(&HelpMenu, L"Space - Détails a propos du menu selectionné");
        AddMenuInfo(&HelpMenu, L"Digits 1-9 - Raccourci vers entrée menu");
        AddMenuInfo(&HelpMenu, L"A - A propos");
        AddMenuInfo(&HelpMenu, L"O - Options Menu");
        AddMenuInfo(&HelpMenu, L"R - Redémarrer");
        AddMenuInfo(&HelpMenu, L"U - Eteindre");
        AddMenuInfo(&HelpMenu, L"S - Shell");
        break;
      case indonesian:
        AddMenuInfo(&HelpMenu, L"ESC - Keluar submenu, Refresh main menu");
        AddMenuInfo(&HelpMenu, L"F1  - Help");
        AddMenuInfo(&HelpMenu, L"F2  - Simpan preboot.log ke EFI/CLOVER/ACPI/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfo(&HelpMenu, L"F4  - Simpan oem DSDT ke EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F5  - Simpan patched DSDT ke EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F6  - Simpan VideoBios ke EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F7  - Check sound on selected output");
        AddMenuInfo(&HelpMenu, L"F8  - Make audio outputs dump into EFI/CLOVER/misc/");
        AddMenuInfo(&HelpMenu, L"F9  - Switch screen resoluton to next possible mode");
        AddMenuInfo(&HelpMenu, L"F10 - Simpan screenshot ke EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F11 - Reset NVRAM");
        AddMenuInfo(&HelpMenu, L"F12 - Eject volume (DVD)");
        AddMenuInfo(&HelpMenu, L"Spasi - Detail dari menu yang dipilih");
        AddMenuInfo(&HelpMenu, L"Tombol 1-9 - Shortcut pilihan menu");
        AddMenuInfo(&HelpMenu, L"A - About");
        AddMenuInfo(&HelpMenu, L"O - Opsi");
        AddMenuInfo(&HelpMenu, L"R - Soft Reset");
        AddMenuInfo(&HelpMenu, L"U - Shutdown");
        AddMenuInfo(&HelpMenu, L"S - Shell");
        break;
      case polish:
        AddMenuInfo(&HelpMenu, L"ESC - Wyjscie z podmenu, Odswiezenie glownego menu");
        AddMenuInfo(&HelpMenu, L"F1  - Pomoc");
        AddMenuInfo(&HelpMenu, L"F2  - Zapis preboot.log (tylko FAT32)");
        AddMenuInfo(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfo(&HelpMenu, L"F4  - Zapis DSDT do EFI/CLOVER/ACPI/origin/ (tylko FAT32)");
        AddMenuInfo(&HelpMenu, L"F5  - Zapis poprawionego DSDT do EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F6  - Zapis BIOSu k. graficznej do EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F7  - Check sound on selected output");
        AddMenuInfo(&HelpMenu, L"F8  - Make audio outputs dump into EFI/CLOVER/misc/");
        AddMenuInfo(&HelpMenu, L"F9  - Switch screen resoluton to next possible mode");
        AddMenuInfo(&HelpMenu, L"F10 - Zapis zrzutu ekranu do EFI/CLOVER/misc/ (tylko FAT32)");
        AddMenuInfo(&HelpMenu, L"F11 - Reset NVRAM");
        AddMenuInfo(&HelpMenu, L"F12 - Wysuniecie zaznaczonego dysku (tylko dla DVD)");
        AddMenuInfo(&HelpMenu, L"Spacja - Informacje nt. dostepnych opcji dla zaznaczonego dysku");
        AddMenuInfo(&HelpMenu, L"Znaki 1-9 - Skroty opcji dla wybranego dysku");
        AddMenuInfo(&HelpMenu, L"A - Menu Informacyjne");
        AddMenuInfo(&HelpMenu, L"O - Menu Opcje");
        AddMenuInfo(&HelpMenu, L"R - Restart komputera");
        AddMenuInfo(&HelpMenu, L"U - Wylaczenie komputera");
        AddMenuInfo(&HelpMenu, L"S - Shell");
        break;
      case croatian:
        AddMenuInfo(&HelpMenu, L"ESC - izlaz iz podizbornika, Osvježi glavni izbornik");
        AddMenuInfo(&HelpMenu, L"F1  - Ovaj izbornik");
        AddMenuInfo(&HelpMenu, L"F2  - Spremi preboot.log (samo na FAT32)");
        AddMenuInfo(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfo(&HelpMenu, L"F4  - Spremi oem DSDT u EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F5  - Spremi patched DSDT into EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F6  - Spremi VideoBios into EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F7  - Check sound on selected output");
        AddMenuInfo(&HelpMenu, L"F8  - Make audio outputs dump into EFI/CLOVER/misc/");
        AddMenuInfo(&HelpMenu, L"F9  - Switch screen resoluton to next possible mode");
        AddMenuInfo(&HelpMenu, L"F10 - Spremi screenshot into EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F11 - Reset NVRAM");
        AddMenuInfo(&HelpMenu, L"F12 - Izbaci izabrai (DVD)");
        AddMenuInfo(&HelpMenu, L"Space - Detalji o odabranom sistemu");
        AddMenuInfo(&HelpMenu, L"Brojevi 1 do 9 su prečac do izbora");
        AddMenuInfo(&HelpMenu, L"A - Izbornik o meni");
        AddMenuInfo(&HelpMenu, L"O - Izbornik opcije");
        AddMenuInfo(&HelpMenu, L"R - Restart računala");
        AddMenuInfo(&HelpMenu, L"U - Isključivanje računala");
        AddMenuInfo(&HelpMenu, L"S - Shell");
        break;
      case czech:
        AddMenuInfo(&HelpMenu, L"ESC - Vrátit se do hlavní nabídky");
        AddMenuInfo(&HelpMenu, L"F1  - Tato Nápověda");
        AddMenuInfo(&HelpMenu, L"F2  - Uložit preboot.log (FAT32 only)");
        AddMenuInfo(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfo(&HelpMenu, L"F4  - Uložit oem DSDT do EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F5  - Uložit patchnuté DSDT do EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F6  - Uložit VideoBios do EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F7  - Check sound on selected output");
        AddMenuInfo(&HelpMenu, L"F8  - Make audio outputs dump into EFI/CLOVER/misc/");
        AddMenuInfo(&HelpMenu, L"F9  - Switch screen resoluton to next possible mode");
        AddMenuInfo(&HelpMenu, L"F10 - Uložit snímek obrazovky do EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F11 - Reset NVRAM");
        AddMenuInfo(&HelpMenu, L"F12 - Vysunout vybranou mechaniku (DVD)");
        AddMenuInfo(&HelpMenu, L"Mezerník - Podrobnosti o vybraném disku");
        AddMenuInfo(&HelpMenu, L"čísla 1-9 - Klávesové zkratky pro disky");
        AddMenuInfo(&HelpMenu, L"A - Menu O Programu");
        AddMenuInfo(&HelpMenu, L"O - Menu Možnosti");
        AddMenuInfo(&HelpMenu, L"R - Částečný restart");
        AddMenuInfo(&HelpMenu, L"U - Odejít");
        AddMenuInfo(&HelpMenu, L"S - Shell");
        break;
      case korean:
        AddMenuInfo(&HelpMenu, L"ESC - 하위메뉴에서 나감, 메인메뉴 새로 고침");
        AddMenuInfo(&HelpMenu, L"F1  - 이 도움말");
        AddMenuInfo(&HelpMenu, L"F2  - preboot.log를 저장합니다. (FAT32방식에만 해당됨)");
        AddMenuInfo(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfo(&HelpMenu, L"F4  - oem DSDT를 EFI/CLOVER/ACPI/origin/에 저장합니다. (FAT32)");
        AddMenuInfo(&HelpMenu, L"F5  - 패치된 DSDT를 EFI/CLOVER/ACPI/origin/에 저장합니다. (FAT32)");
        AddMenuInfo(&HelpMenu, L"F6  - VideoBios를 EFI/CLOVER/misc/에 저장합니다. (FAT32)");
        AddMenuInfo(&HelpMenu, L"F7  - 선택한 출력에서 사운드 확인");
        AddMenuInfo(&HelpMenu, L"F8  - 오디오 코덱덤프를 EFI/CLOVER/misc/에 저장합니다.");
        AddMenuInfo(&HelpMenu, L"F9  - Switch screen resoluton to next possible mode");
        AddMenuInfo(&HelpMenu, L"F10 - 스크린샷을 EFI/CLOVER/misc/에 저장합니다. (FAT32)");
        AddMenuInfo(&HelpMenu, L"F11 - NVRAM 초기화");
        AddMenuInfo(&HelpMenu, L"F12 - 선택한 볼륨을 제거합니다. (DVD)");
        AddMenuInfo(&HelpMenu, L"Space - 선택한 메뉴의 상세 설명");
        AddMenuInfo(&HelpMenu, L"Digits 1-9 - 메뉴 단축 번호");
        AddMenuInfo(&HelpMenu, L"A - 단축키 - 이 부트로더에 관하여");
        AddMenuInfo(&HelpMenu, L"O - 단축키 - 부트 옵션");
        AddMenuInfo(&HelpMenu, L"R - 단축키 - 리셋");
        AddMenuInfo(&HelpMenu, L"U - 단축키 - 시스템 종료");
        AddMenuInfo(&HelpMenu, L"S - Shell");
        break;
      case romanian:
        AddMenuInfo(&HelpMenu, L"ESC - Iesire din sub-meniu, Refresh meniul principal");
        AddMenuInfo(&HelpMenu, L"F1  - Ajutor");
        AddMenuInfo(&HelpMenu, L"F2  - Salvare preboot.log (doar pentru FAT32)");
        AddMenuInfo(&HelpMenu, L"F4  - Salvare oem DSDT in EFI/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F5  - Salvare DSDT modificat in EFI/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F6  - Salvare VideoBios in EFI/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F7  - Check sound on selected output");
        AddMenuInfo(&HelpMenu, L"F8  - Make audio outputs dump into EFI/CLOVER/misc/");
        AddMenuInfo(&HelpMenu, L"F9  - Switch screen resoluton to next possible mode");
        AddMenuInfo(&HelpMenu, L"F10 - Salvare screenshot in EFI/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F11 - Reset NVRAM");
        AddMenuInfo(&HelpMenu, L"F12 - Scoatere volum selectat (DVD)");
        AddMenuInfo(&HelpMenu, L"Space - Detalii despre item-ul selectat");
        AddMenuInfo(&HelpMenu, L"Cifre 1-9 - Scurtaturi pentru itemele meniului");
        AddMenuInfo(&HelpMenu, L"A - Despre");
        AddMenuInfo(&HelpMenu, L"O - Optiuni");
        AddMenuInfo(&HelpMenu, L"R - Soft Reset");
        AddMenuInfo(&HelpMenu, L"U - Inchidere");
        AddMenuInfo(&HelpMenu, L"S - Shell");
        break;
      case chinese:
        AddMenuInfo(&HelpMenu, L"ESC - 离开子菜单， 刷新主菜单");
        AddMenuInfo(&HelpMenu, L"F1  - 帮助");
        AddMenuInfo(&HelpMenu, L"F2  - 保存 preboot.log 到 EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F3  - 显示隐藏的启动项");
        AddMenuInfo(&HelpMenu, L"F4  - 保存原始的 DSDT 到 EFI/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F5  - 保存修正后的 DSDT 到 EFI/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F6  - 保存 VideoBios 到 EFI/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F7  - 检查选中输出设备的声音");
        AddMenuInfo(&HelpMenu, L"F8  - 生成声卡输出转储到 EFI/CLOVER/misc/");
        AddMenuInfo(&HelpMenu, L"F9  - 调整屏幕分辨率为下一个可用的模式");
        AddMenuInfo(&HelpMenu, L"F10 - 保存截图到 EFI/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F11 - 重置 NVRAM");
        AddMenuInfo(&HelpMenu, L"F12 - 推出选中的卷 (DVD)");
        AddMenuInfo(&HelpMenu, L"空格 - 关于选中项的详情");
        AddMenuInfo(&HelpMenu, L"数字 1-9 - 菜单快捷键");
        AddMenuInfo(&HelpMenu, L"A - 关于");
        AddMenuInfo(&HelpMenu, L"O - 选项");
        AddMenuInfo(&HelpMenu, L"R - 软复位");
        AddMenuInfo(&HelpMenu, L"U - 退出");
        AddMenuInfo(&HelpMenu, L"S - Shell");
        break;
      case english:
      default:
        AddMenuInfo(&HelpMenu, L"ESC - Escape from submenu, Refresh main menu");
        AddMenuInfo(&HelpMenu, L"F1  - This help");
        AddMenuInfo(&HelpMenu, L"F2  - Save preboot.log into EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F3  - Show hidden entries");
        AddMenuInfo(&HelpMenu, L"F4  - Save oem DSDT into EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F5  - Save patched DSDT into EFI/CLOVER/ACPI/origin/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F6  - Save VideoBios into EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F7  - Check sound on selected output");
        AddMenuInfo(&HelpMenu, L"F8  - Make audio outputs dump into EFI/CLOVER/misc/");
        AddMenuInfo(&HelpMenu, L"F9  - Switch screen resoluton to next possible mode");
        AddMenuInfo(&HelpMenu, L"F10 - Save screenshot into EFI/CLOVER/misc/ (FAT32)");
        AddMenuInfo(&HelpMenu, L"F11 - Reset NVRAM");
        AddMenuInfo(&HelpMenu, L"F12 - Eject selected volume (DVD)");
        AddMenuInfo(&HelpMenu, L"Space - Details about selected menu entry");
        AddMenuInfo(&HelpMenu, L"Digits 1-9 - Shortcut to menu entry");
        AddMenuInfo(&HelpMenu, L"A - Menu About");
        AddMenuInfo(&HelpMenu, L"O - Menu Options");
        AddMenuInfo(&HelpMenu, L"R - Soft Reset");
        AddMenuInfo(&HelpMenu, L"U - Exit from Clover");
        AddMenuInfo(&HelpMenu, L"S - Shell");
        break;
    }
    HelpMenu.AnimeRun = GetAnime(&HelpMenu);
    AddMenuEntry(&HelpMenu, &MenuEntryReturn, false);
  }

  RunMenu(&HelpMenu, NULL);
}

//
// Graphics helper functions
//

/*
  SelectionImages:
    [0] SelectionBig
    [2] SelectionSmall
    [4] SelectionIndicator
  Buttons:
    [0] radio_button
    [1] radio_button_selected
    [2] checkbox
    [3] checkbox_checked
*/

VOID InitSelection(VOID)
{

  if (!AllowGraphicsMode)
    return;
  SelectionBackgroundPixel.r = (GlobalConfig.SelectionColor >> 24) & 0xFF;
  SelectionBackgroundPixel.g = (GlobalConfig.SelectionColor >> 16) & 0xFF;
  SelectionBackgroundPixel.b = (GlobalConfig.SelectionColor >> 8) & 0xFF;
  SelectionBackgroundPixel.a = (GlobalConfig.SelectionColor >> 0) & 0xFF;

  if (SelectionImages[0] != NULL) {
    return;
  }
  // load small selection image
  if (GlobalConfig.SelectionSmallFileName != NULL){
    SelectionImages[2] = egLoadImage(ThemeDir, GlobalConfig.SelectionSmallFileName, FALSE);
  }
  if (SelectionImages[2] == NULL){
    SelectionImages[2] = BuiltinIcon(BUILTIN_SELECTION_SMALL);
    SelectionImages[2]->HasAlpha = FALSE; // support transparensy for selection icons
    CopyMem(&BlueBackgroundPixel, &StdBackgroundPixel, sizeof(EG_PIXEL));
  }
  SelectionImages[2] = egEnsureImageSize(SelectionImages[2],
                                         row1TileSize, row1TileSize, &MenuBackgroundPixel);
  if (SelectionImages[2] == NULL) {
    return;
  }
  // load big selection image
  if (!GlobalConfig.TypeSVG && GlobalConfig.SelectionBigFileName != NULL) {
    SelectionImages[0] = egLoadImage(ThemeDir, GlobalConfig.SelectionBigFileName, FALSE);
    SelectionImages[0] = egEnsureImageSize(SelectionImages[0],
                                           row0TileSize, row0TileSize,
                                           &MenuBackgroundPixel);
  }
  if (SelectionImages[0] == NULL) {
    // calculate big selection image from small one
    SelectionImages[0] = BuiltinIcon(BUILTIN_SELECTION_BIG);
    SelectionImages[0]->HasAlpha = FALSE; // support transparensy for selection icons
    CopyMem(&BlueBackgroundPixel, &StdBackgroundPixel, sizeof(EG_PIXEL));
    if (SelectionImages[0] == NULL) {
      egFreeImage(SelectionImages[2]);
      SelectionImages[2] = NULL;
      return;
    }
    if (GlobalConfig.SelectionOnTop) {
      SelectionImages[0]->HasAlpha = TRUE;
      SelectionImages[2]->HasAlpha = TRUE;
    }
  }

  // BootCampStyle indicator image
  if (GlobalConfig.BootCampStyle) {
    // load indicator selection image
    if (GlobalConfig.SelectionIndicatorName != NULL) {
      SelectionImages[4] = egLoadImage(ThemeDir, GlobalConfig.SelectionIndicatorName, TRUE);
    }
    if (!SelectionImages[4]) {
      SelectionImages[4] = egDecodePNG(ACCESS_EMB_DATA(emb_selection_indicator), ACCESS_EMB_SIZE(emb_selection_indicator), TRUE);

    }
    INTN ScaledIndicatorSize = (INTN)(INDICATOR_SIZE * GlobalConfig.Scale);
    SelectionImages[4] = egEnsureImageSize(SelectionImages[4], ScaledIndicatorSize, ScaledIndicatorSize, &MenuBackgroundPixel);
    if (!SelectionImages[4]) {
      SelectionImages[4] = egCreateFilledImage(ScaledIndicatorSize, ScaledIndicatorSize,
                                               TRUE, &StdBackgroundPixel);

    }
    SelectionImages[5] = egCreateFilledImage(ScaledIndicatorSize, ScaledIndicatorSize,
                                             TRUE, &MenuBackgroundPixel);
  }

  /*
  Button & radio, or any other next icons with builtin icon as fallback should synced to:
   - BUILTIN_ICON_* in lib.h
   - BuiltinIconTable in icns.c
   - Data in egemb_icons.h / scroll_images.h
  */

  // Radio buttons
  Buttons[0] = egLoadImage(ThemeDir, GetIconsExt(L"radio_button", L"png"), TRUE); //memory leak
  Buttons[1] = egLoadImage(ThemeDir, GetIconsExt(L"radio_button_selected", L"png"), TRUE);
  if (!Buttons[0]) {
    Buttons[0] = egLoadIcon(ThemeDir, L"radio_button.png", 48);
  }
  if (!Buttons[0]) {
    Buttons[0] = egDecodePNG(ACCESS_EMB_DATA(emb_radio_button), ACCESS_EMB_SIZE(emb_radio_button), TRUE);
  }
  if (!Buttons[1]) {
    Buttons[1] = egLoadIcon(ThemeDir, L"radio_button_selected.png", 48);
  }

  if (!Buttons[1]) {
    Buttons[1] = egDecodePNG(ACCESS_EMB_DATA(emb_radio_button_selected), ACCESS_EMB_SIZE(emb_radio_button_selected), TRUE);
  }

  // Checkbox
  Buttons[2] = egLoadImage(ThemeDir, GetIconsExt(L"checkbox", L"png"), TRUE);
  Buttons[3] = egLoadImage(ThemeDir, GetIconsExt(L"checkbox_checked", L"png"), TRUE);
  if (!Buttons[2]) {
//    DBG("egLoadIcon checkbox\n");
    Buttons[2] = egLoadIcon(ThemeDir, L"checkbox.png", 48);
  }
  if (!Buttons[3]) {
//    DBG("egLoadIcon checkbox_checked\n");
    Buttons[3] = egLoadIcon(ThemeDir, L"checkbox_checked.png", 48);
  }

  if (!Buttons[2]) {
//    DBG("embedded checkbox\n");
    Buttons[2] = egDecodePNG(ACCESS_EMB_DATA(emb_checkbox), ACCESS_EMB_SIZE(emb_checkbox), TRUE);
  }

  if (!Buttons[3]) {
//    DBG("embedded checkbox_checked\n");
    Buttons[3] = egDecodePNG(ACCESS_EMB_DATA(emb_checkbox_checked), ACCESS_EMB_SIZE(emb_checkbox_checked), TRUE);
  }

  // non-selected background images
  if (GlobalConfig.SelectionBigFileName != NULL) {
    SelectionImages[1] = egCreateFilledImage(row0TileSize, row0TileSize,
                                             TRUE, &MenuBackgroundPixel);
    SelectionImages[3] = egCreateFilledImage(row1TileSize, row1TileSize,
                                             TRUE, &MenuBackgroundPixel);
  } else { // using embedded theme (this is an assumption but a better check is required)
    EG_PIXEL BackgroundPixel;
    if (GlobalConfig.DarkEmbedded || GlobalConfig.TypeSVG) {
      BackgroundPixel = DarkEmbeddedBackgroundPixel;
      BackgroundPixel.a = 0x00;
    } else {
      BackgroundPixel = StdBackgroundPixel;
      BackgroundPixel.a = 0xff;
    }
    if (GlobalConfig.DarkEmbedded) {
      SelectionImages[1] = egCreateFilledImage(row0TileSize, row0TileSize,
                                               TRUE, &BackgroundPixel);
      SelectionImages[3] = egCreateFilledImage(row1TileSize, row1TileSize,
                                               TRUE, &BackgroundPixel);

    } else {
      SelectionImages[1] = egCreateFilledImage(row0TileSize, row0TileSize,
                                               TRUE, &BackgroundPixel); //&StdBackgroundPixel);
      SelectionImages[3] = egCreateFilledImage(row1TileSize, row1TileSize,
                                               TRUE, &BackgroundPixel);
    }
  }
//  DBG("selections inited\n");
}

//
// Scrolling functions
//
#define CONSTRAIN_MIN(Variable, MinValue) if (Variable < MinValue) Variable = MinValue
#define CONSTRAIN_MAX(Variable, MaxValue) if (Variable > MaxValue) Variable = MaxValue

VOID REFIT_MENU_SCREEN::InitScroll(IN INTN ItemCount, IN UINTN MaxCount,
                       IN UINTN VisibleSpace, IN INTN Selected)
{
  //ItemCount - a number to scroll (Row0)
  //MaxCount - total number (Row0 + Row1)
  //VisibleSpace - a number to fit

  ScrollState.LastSelection = ScrollState.CurrentSelection = Selected;
  //MaxIndex, MaxScroll, MaxVisible are indexes, 0..N-1
  ScrollState.MaxIndex = (INTN)MaxCount - 1;
  ScrollState.MaxScroll = ItemCount - 1;

  if (VisibleSpace == 0) {
    ScrollState.MaxVisible = ScrollState.MaxScroll;
  } else {
    ScrollState.MaxVisible = (INTN)VisibleSpace - 1;
  }

  if (ScrollState.MaxVisible >= ItemCount) {
      ScrollState.MaxVisible = ItemCount - 1;
  }

  ScrollState.MaxFirstVisible = ScrollState.MaxScroll - ScrollState.MaxVisible;
  CONSTRAIN_MIN(ScrollState.MaxFirstVisible, 0);
  ScrollState.FirstVisible = MIN(Selected, ScrollState.MaxFirstVisible);


  ScrollState.IsScrolling = (ScrollState.MaxFirstVisible > 0);
  ScrollState.PaintAll = TRUE;
  ScrollState.PaintSelection = FALSE;

  ScrollState.LastVisible = ScrollState.FirstVisible + ScrollState.MaxVisible;
}

VOID REFIT_MENU_SCREEN::UpdateScroll(IN UINTN Movement)
{
  INTN Lines;
  UINTN ScrollMovement = SCROLL_SCROLL_DOWN;
  INTN i;
  ScrollState.LastSelection = ScrollState.CurrentSelection;

  switch (Movement) {
    case SCROLL_SCROLLBAR_MOVE:
      ScrollbarYMovement += ScrollbarNewPointerPlace.YPos - ScrollbarOldPointerPlace.YPos;
      ScrollbarOldPointerPlace.XPos = ScrollbarNewPointerPlace.XPos;
      ScrollbarOldPointerPlace.YPos = ScrollbarNewPointerPlace.YPos;
      Lines = ScrollbarYMovement * ScrollState.MaxIndex / ScrollbarBackground.Height;
      ScrollbarYMovement = ScrollbarYMovement - Lines * (ScrollState.MaxVisible * TextHeight - 16 - 1) / ScrollState.MaxIndex;
      if (Lines < 0) {
        Lines = -Lines;
        ScrollMovement = SCROLL_SCROLL_UP;
      }
      for (i = 0; i < Lines; i++)
        UpdateScroll(ScrollMovement);
      break;

    case SCROLL_LINE_UP: //of left = decrement
      if (ScrollState.CurrentSelection > 0) {
        ScrollState.CurrentSelection --;
        if (ScrollState.CurrentSelection < ScrollState.FirstVisible) {
          ScrollState.PaintAll = TRUE;
          ScrollState.FirstVisible = ScrollState.CurrentSelection;
        }
        if (ScrollState.CurrentSelection == ScrollState.MaxScroll) {
          ScrollState.PaintAll = TRUE;
        }
        if ((ScrollState.CurrentSelection < ScrollState.MaxScroll) &&
             (ScrollState.CurrentSelection > ScrollState.LastVisible)) {
          ScrollState.PaintAll = TRUE;
          ScrollState.LastVisible = ScrollState.CurrentSelection;
          ScrollState.FirstVisible = ScrollState.LastVisible - ScrollState.MaxVisible;
        }
      }
      break;

    case SCROLL_LINE_DOWN: //or right -- increment
      if (ScrollState.CurrentSelection < ScrollState.MaxIndex) {
        ScrollState.CurrentSelection++;
        if ((ScrollState.CurrentSelection > ScrollState.LastVisible) &&
            (ScrollState.CurrentSelection <= ScrollState.MaxScroll)){
          ScrollState.PaintAll = TRUE;
          ScrollState.FirstVisible++;
          CONSTRAIN_MAX(ScrollState.FirstVisible, ScrollState.MaxFirstVisible);
        }
        if (ScrollState.CurrentSelection == ScrollState.MaxScroll + 1) {
          ScrollState.PaintAll = TRUE;
        }
      }
      break;

    case SCROLL_SCROLL_DOWN:
      if (ScrollState.FirstVisible < ScrollState.MaxFirstVisible) {
        if (ScrollState.CurrentSelection == ScrollState.FirstVisible)
          ScrollState.CurrentSelection++;
        ScrollState.FirstVisible++;
        ScrollState.LastVisible++;
        ScrollState.PaintAll = TRUE;
      }
      break;

    case SCROLL_SCROLL_UP:
      if (ScrollState.FirstVisible > 0) {
        if (ScrollState.CurrentSelection == ScrollState.LastVisible)
          ScrollState.CurrentSelection--;
        ScrollState.FirstVisible--;
        ScrollState.LastVisible--;
        ScrollState.PaintAll = TRUE;
      }
      break;

    case SCROLL_PAGE_UP:
      if (ScrollState.CurrentSelection > 0) {
        if (ScrollState.CurrentSelection == ScrollState.MaxIndex) {   // currently at last entry, special treatment
          if (ScrollState.IsScrolling)
            ScrollState.CurrentSelection -= ScrollState.MaxVisible - 1;  // move to second line without scrolling
          else
            ScrollState.CurrentSelection = 0;                // move to first entry
        } else {
          if (ScrollState.FirstVisible > 0)
            ScrollState.PaintAll = TRUE;
          ScrollState.CurrentSelection -= ScrollState.MaxVisible;          // move one page and scroll synchronously
          ScrollState.FirstVisible -= ScrollState.MaxVisible;
        }
        CONSTRAIN_MIN(ScrollState.CurrentSelection, 0);
        CONSTRAIN_MIN(ScrollState.FirstVisible, 0);
        if (ScrollState.CurrentSelection < ScrollState.FirstVisible) {
          ScrollState.PaintAll = TRUE;
          ScrollState.FirstVisible = ScrollState.CurrentSelection;
        }
      }
      break;

    case SCROLL_PAGE_DOWN:
      if (ScrollState.CurrentSelection < ScrollState.MaxIndex) {
        if (ScrollState.CurrentSelection == 0) {   // currently at first entry, special treatment
          if (ScrollState.IsScrolling)
            ScrollState.CurrentSelection += ScrollState.MaxVisible - 1;  // move to second-to-last line without scrolling
          else
            ScrollState.CurrentSelection = ScrollState.MaxIndex;         // move to last entry
        } else {
          if (ScrollState.FirstVisible < ScrollState.MaxFirstVisible)
            ScrollState.PaintAll = TRUE;
          ScrollState.CurrentSelection += ScrollState.MaxVisible;          // move one page and scroll synchronously
          ScrollState.FirstVisible += ScrollState.MaxVisible;
        }
        CONSTRAIN_MAX(ScrollState.CurrentSelection, ScrollState.MaxIndex);
        CONSTRAIN_MAX(ScrollState.FirstVisible, ScrollState.MaxFirstVisible);
        if ((ScrollState.CurrentSelection > ScrollState.LastVisible) &&
            (ScrollState.CurrentSelection <= ScrollState.MaxScroll)){
          ScrollState.PaintAll = TRUE;
          ScrollState.FirstVisible+= ScrollState.MaxVisible;
          CONSTRAIN_MAX(ScrollState.FirstVisible, ScrollState.MaxFirstVisible);
        }
      }
      break;

    case SCROLL_FIRST:
      if (ScrollState.CurrentSelection > 0) {
        ScrollState.CurrentSelection = 0;
        if (ScrollState.FirstVisible > 0) {
          ScrollState.PaintAll = TRUE;
          ScrollState.FirstVisible = 0;
        }
      }
      break;

    case SCROLL_LAST:
      if (ScrollState.CurrentSelection < ScrollState.MaxIndex) {
        ScrollState.CurrentSelection = ScrollState.MaxIndex;
        if (ScrollState.FirstVisible < ScrollState.MaxFirstVisible) {
          ScrollState.PaintAll = TRUE;
          ScrollState.FirstVisible = ScrollState.MaxFirstVisible;
        }
      }
      break;

    case SCROLL_NONE:
      // The caller has already updated CurrentSelection, but we may
      // have to scroll to make it visible.
      if (ScrollState.CurrentSelection < ScrollState.FirstVisible) {
        ScrollState.PaintAll = TRUE;
        ScrollState.FirstVisible = ScrollState.CurrentSelection; // - (ScrollState.MaxVisible >> 1);
        CONSTRAIN_MIN(ScrollState.FirstVisible, 0);
      } else if ((ScrollState.CurrentSelection > ScrollState.LastVisible) &&
                 (ScrollState.CurrentSelection <= ScrollState.MaxScroll)) {
        ScrollState.PaintAll = TRUE;
        ScrollState.FirstVisible = ScrollState.CurrentSelection - ScrollState.MaxVisible;
        CONSTRAIN_MAX(ScrollState.FirstVisible, ScrollState.MaxFirstVisible);
      }
      break;

  }

  if (!ScrollState.PaintAll && ScrollState.CurrentSelection != ScrollState.LastSelection)
    ScrollState.PaintSelection = TRUE;
  ScrollState.LastVisible = ScrollState.FirstVisible + ScrollState.MaxVisible;

  //ycr.ru
  if ((ScrollState.PaintAll) && (Movement != SCROLL_NONE))
    HidePointer();
}

VOID REFIT_MENU_SCREEN::HidePointer()
{
  mPointer->Hide();
}

EFI_STATUS REFIT_MENU_SCREEN::MouseBirth()
{
  return mPointer->MouseBirth();
}

VOID REFIT_MENU_SCREEN::KillMouse()
{
  mPointer->KillMouse();
}
//
// menu helper functions
//

VOID REFIT_MENU_SCREEN::AddMenuInfoLine(IN CONST CHAR16 *InfoLine)
{
  InfoLines.Add(InfoLine);
//  AddListElement((VOID ***) &(Screen->InfoLines), (UINTN*)&(Screen->InfoLines.size()), (CHAR16*)InfoLine); // TODO jief : cast to fix
}

VOID REFIT_MENU_SCREEN::AddMenuEntry(IN REFIT_MENU_ENTRY *Entry, bool freeIt)
{
	if ( !Entry ) return;
	Entries.AddReference(Entry, freeIt);
//  AddListElement((VOID ***) &(Screen->Entries), (UINTN*)&(Screen->Entries.size()), Entry);
}

VOID REFIT_MENU_SCREEN::FreeMenu()
{
//  INTN i;
  REFIT_ABSTRACT_MENU_ENTRY *Tentry = NULL;
//TODO - here we must FreePool for a list of Entries, Screens, InputBootArgs
  if (Entries.size() > 0) {
    for (UINTN i = 0; i < Entries.size(); i++) {
      Tentry = &Entries[i];
      if (Tentry->SubScreen) {
        if (Tentry->SubScreen->Title) {
          FreePool(Tentry->SubScreen->Title);
          Tentry->SubScreen->Title = NULL;
        }
        // don't free image because of reusing them
     //   FreeMenu(Tentry->SubScreen);
        Tentry->SubScreen->FreeMenu();
        Tentry->SubScreen = NULL;
      }
      if (Tentry->getREFIT_MENU_ITEM_RETURN()) { //can't free constants
        if (Tentry->Title) {
          FreePool(Tentry->Title);
          Tentry->Title = NULL;
        }
      }
      FreePool(Tentry);
    }
    Entries.Empty();
//    FreePool(Screen->Entries);
//    Screen->Entries = NULL;
  }
  if (InfoLines.size() > 0) {
    for (UINTN i = 0; i < InfoLines.size(); i++) {
      // TODO: call a user-provided routine for each element here
      FreePool(InfoLines[i]);
    }
    InfoLines.Empty();
//    Screen->InfoLines.size() = 0;
//    FreePool(Screen->InfoLines);
//    Screen->InfoLines = NULL;
  }

}

INTN REFIT_MENU_SCREEN::FindMenuShortcutEntry(IN CHAR16 Shortcut)
{
  if (Shortcut >= 'a' && Shortcut <= 'z')
    Shortcut -= ('a' - 'A');
  if (Shortcut) {
    for (UINTN i = 0; i < Entries.size(); i++) {
      if (Entries[i].ShortcutDigit == Shortcut ||
          Entries[i].ShortcutLetter == Shortcut) {
        return i;
      }
    }
  }
  return -1;
}

//
// generic input menu function
// usr-sse2
//
UINTN REFIT_MENU_SCREEN::InputDialog(IN MENU_STYLE_FUNC  StyleFunc)
{
	if ( !Entries[ScrollState.CurrentSelection].getREFIT_MENU_ITEM_IEM_ABSTRACT() ) {
		DebugLog(2, "BUG : InputDialog called with !Entries[ScrollState.CurrentSelection].REFIT_MENU_ITEM_IEM_ABSTRACT()\n");
		return 0; // is it the best thing to do ? CpuDeadLog ?
	}

  EFI_STATUS    Status;
  EFI_INPUT_KEY key;
  UINTN         ind = 0;
  UINTN         i = 0;
  UINTN         MenuExit = 0;
  //UINTN         LogSize;
  UINTN         Pos = (Entries[ScrollState.CurrentSelection]).Row;
  REFIT_MENU_ITEM_IEM_ABSTRACT& selectedEntry = *Entries[ScrollState.CurrentSelection].getREFIT_MENU_ITEM_IEM_ABSTRACT();
  INPUT_ITEM    *Item = selectedEntry.Item;
  CHAR16        *Backup = EfiStrDuplicate(Item->SValue);
  UINTN         BackupPos, BackupShift;
  CHAR16        *Buffer;
  //SCROLL_STATE  StateLine;

  /*
    I would like to see a LineSize that depends on the Title width and the menu width so
    the edit dialog does not extend beyond the menu width.
    There are 3 cases:
    1) Text menu where MenuWidth is min of ConWidth - 6 and max of 50 and all StrLen(Title)
    2) Graphics menu where MenuWidth is measured in pixels and font is fixed width.
       The following works well in my case but depends on font width and minimum screen size.
         LineSize = 76 - StrLen(Screen->Entries[State->CurrentSelection].Title);
    3) Graphics menu where font is proportional. In this case LineSize would depend on the
       current width of the displayed string which would need to be recalculated after
       every change.
    Anyway, the above will not be implemented for now, and LineSize will remain at 38
    because it works.
  */
  UINTN         LineSize = 38;
#define DBG_INPUTDIALOG 0
#if DBG_INPUTDIALOG
  UINTN         Iteration = 0;
#endif


  if ((Item->ItemType != BoolValue) &&
      (Item->ItemType != RadioSwitch) &&
      (Item->ItemType != CheckBit)) {
    // Grow Item->SValue to SVALUE_MAX_SIZE if we want to edit a text field
    Item->SValue = (__typeof__(Item->SValue))ReallocatePool(StrSize(Item->SValue), SVALUE_MAX_SIZE, Item->SValue);
  }

  Buffer = Item->SValue;
  BackupShift = Item->LineShift;
  BackupPos = Pos;

  do {

    if (Item->ItemType == BoolValue) {
      Item->BValue = !Item->BValue;
      MenuExit = MENU_EXIT_ENTER;
    } else if (Item->ItemType == RadioSwitch) {
      if (Item->IValue == 3) {
        OldChosenTheme = Pos? Pos - 1: 0xFFFF;
      } else if (Item->IValue == 90) {
        OldChosenConfig = Pos;
      } else if (Item->IValue == 116) {
        OldChosenDsdt = Pos? Pos - 1: 0xFFFF;
      } else if (Item->IValue == 119) {
        OldChosenAudio = Pos;
      }
      MenuExit = MENU_EXIT_ENTER;
    } else if (Item->ItemType == CheckBit) {
      Item->IValue ^= Pos;
      MenuExit = MENU_EXIT_ENTER;
    } else {

      Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &key);

      if (Status == EFI_NOT_READY) {
        gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &ind);
        continue;
      }

      switch (key.ScanCode) {
        case SCAN_RIGHT:
          if (Pos + Item->LineShift < StrLen(Buffer)) {
            if (Pos < LineSize)
              Pos++;
            else
              Item->LineShift++;
          }
          break;
        case SCAN_LEFT:
          if (Pos > 0)
            Pos--;
          else if (Item->LineShift > 0)
            Item->LineShift--;
          break;
        case SCAN_HOME:
          Pos = 0;
          Item->LineShift=0;
          break;
        case SCAN_END:
          if (StrLen(Buffer) < LineSize)
            Pos = StrLen(Buffer);
          else {
            Pos = LineSize;
            Item->LineShift = StrLen(Buffer) - LineSize;
          }
          break;
        case SCAN_ESC:
          MenuExit = MENU_EXIT_ESCAPE;
          continue;
          break;
        case SCAN_F2:
          SavePreBootLog = TRUE;
          break;
          //not used here
/*        case SCAN_F6:
          Status = egSaveFile(SelfRootDir, VBIOS_BIN, (UINT8*)(UINTN)0xc0000, 0x20000);
          if (EFI_ERROR(Status)) {
            Status = egSaveFile(NULL, VBIOS_BIN, (UINT8*)(UINTN)0xc0000, 0x20000);
          }
          break; */
        case SCAN_F10:
          egScreenShot();
          break;

        case SCAN_DELETE:
          // forward delete
          if (Pos + Item->LineShift < StrLen(Buffer)) {
            for (i = Pos + Item->LineShift; i < StrLen(Buffer); i++) {
               Buffer[i] = Buffer[i+1];
            }
            /*
            // Commented this out because it looks weird - Forward Delete should not
            // affect anything left of the cursor even if it's just to shift more of the
            // string into view.
            if (Item->LineShift > 0 && Item->LineShift + LineSize > StrLen(Buffer)) {
              Item->LineShift--;
              Pos++;
            }
            */
          }
          break;
      }

      switch (key.UnicodeChar) {
        case CHAR_BACKSPACE:
          if (Buffer[0] != CHAR_NULL && Pos != 0) {
            for (i = Pos + Item->LineShift; i <= StrLen(Buffer); i++) {
               Buffer[i-1] = Buffer[i];
            }
            Item->LineShift > 0 ? Item->LineShift-- : Pos--;
          }

          break;

        case CHAR_LINEFEED:
        case CHAR_CARRIAGE_RETURN:
          MenuExit = MENU_EXIT_ENTER;
          Pos = 0;
          Item->LineShift = 0;
          break;
        default:
          if ((key.UnicodeChar >= 0x20) &&
              (key.UnicodeChar < 0x80)){
            if (StrSize(Buffer) < SVALUE_MAX_SIZE) {
              for (i = StrLen(Buffer)+1; i > Pos + Item->LineShift; i--) {
                 Buffer[i] = Buffer[i-1];
              }
              Buffer[i] = key.UnicodeChar;
              Pos < LineSize ? Pos++ : Item->LineShift++;
            }
          }
          break;
      }
    }
    // Redraw the field
    (Entries[ScrollState.CurrentSelection]).Row = Pos;
    StyleFunc(MENU_FUNCTION_PAINT_SELECTION, NULL);
  } while (!MenuExit);

  switch (MenuExit) {
    case MENU_EXIT_ENTER:
      Item->Valid = TRUE;
      ApplyInputs();
      break;

    case MENU_EXIT_ESCAPE:
      if (StrCmp(Item->SValue, Backup) != 0) {
        UnicodeSPrint(Item->SValue, SVALUE_MAX_SIZE, L"%s", Backup);
        if (Item->ItemType != BoolValue) {
          Item->LineShift = BackupShift;
          (Entries[ScrollState.CurrentSelection]).Row = BackupPos;
        }
        StyleFunc( MENU_FUNCTION_PAINT_SELECTION, NULL);
      }
      break;
  }
  Item->Valid = FALSE;
  FreePool(Backup);
  if (Item->SValue) {
    MsgLog("EDITED: %s\n", Item->SValue);
  }
  return 0;
}


UINTN REFIT_MENU_SCREEN::RunGenericMenu(IN MENU_STYLE_FUNC StyleFunc, IN OUT INTN *DefaultEntryIndex, OUT REFIT_ABSTRACT_MENU_ENTRY **ChosenEntry)
{
  EFI_STATUS    Status;
  EFI_INPUT_KEY key;
  //    UINTN         Index;
  INTN          ShortcutEntry;
  BOOLEAN       HaveTimeout = FALSE;
  INTN          TimeoutCountdown = 0;
  CHAR16        *TimeoutMessage;
  UINTN         MenuExit;

  if (ChosenEntry == NULL) {
    TextStyle = 0;
  } else {
    TextStyle = 2;
  }

  if (GlobalConfig.TypeSVG) {
    if (!textFace[TextStyle].valid) {
      if (textFace[0].valid) {
        TextStyle = 0;
      } else if (textFace[2].valid) {
        TextStyle = 2;
      } else if (textFace[1].valid) {
        TextStyle = 1;
      } else {
        DBG("no valid text style\n");
        textFace[TextStyle].size = TextHeight - 4;
      }
    }
    if (textFace[TextStyle].valid) {
      // TextHeight = (int)((textFace[TextStyle].size + 4) * GlobalConfig.Scale);
      //clovy - row height / text size factor
      TextHeight = (int)((textFace[TextStyle].size * RowHeightFromTextHeight) * GlobalConfig.Scale);
    }
  }

  //no default - no timeout!
  if ((*DefaultEntryIndex != -1) && (TimeoutSeconds > 0)) {
    //      DBG("have timeout\n");
    HaveTimeout = TRUE;
    TimeoutCountdown =  TimeoutSeconds;
  }
  MenuExit = 0;

  StyleFunc(MENU_FUNCTION_INIT, NULL);
  //  DBG("scroll inited\n");
  // override the starting selection with the default index, if any
  if (*DefaultEntryIndex >= 0 && *DefaultEntryIndex <=  ScrollState.MaxIndex) {
     ScrollState.CurrentSelection = *DefaultEntryIndex;
     UpdateScroll(SCROLL_NONE);
  }
  //  DBG("RunGenericMenu CurrentSelection=%d MenuExit=%d\n",
  //      State.CurrentSelection, MenuExit);

  // exhaust key buffer and be sure no key is pressed to prevent option selection
  // when coming with a key press from timeout=0, for example
  while (ReadAllKeyStrokes()) gBS->Stall(500 * 1000);
  while (!MenuExit) {
    // update the screen
    if (ScrollState.PaintAll) {
      StyleFunc(MENU_FUNCTION_PAINT_ALL, NULL);
      ScrollState.PaintAll = FALSE;
    } else if (ScrollState.PaintSelection) {
      StyleFunc(MENU_FUNCTION_PAINT_SELECTION, NULL);
      ScrollState.PaintSelection = FALSE;
    }

    if (HaveTimeout) {
      TimeoutMessage = PoolPrint(L"%s in %d seconds", TimeoutText, TimeoutCountdown);
      StyleFunc(MENU_FUNCTION_PAINT_TIMEOUT, TimeoutMessage);
      FreePool(TimeoutMessage);
    }

    if (gEvent) { //for now used at CD eject.
      MenuExit = MENU_EXIT_ESCAPE;
      ScrollState.PaintAll = TRUE;
      gEvent = 0; //to prevent looping
      break;
    }
    key.UnicodeChar = 0;
    key.ScanCode = 0;
    if (!mGuiReady) {
      mGuiReady = TRUE;
      DBG("GUI ready\n");
    }
    Status = WaitForInputEventPoll(this, 1); //wait for 1 seconds.
    if (Status == EFI_TIMEOUT) {
      if (HaveTimeout) {
        if (TimeoutCountdown <= 0) {
          // timeout expired
          MenuExit = MENU_EXIT_TIMEOUT;
          break;
        } else {
          //     gBS->Stall(100000);
          TimeoutCountdown--;
        }
      }
      continue;
    }

    switch (mAction) {
      case ActionSelect:
        ScrollState.LastSelection = ScrollState.CurrentSelection;
        ScrollState.CurrentSelection = mItemID;
        ScrollState.PaintAll = TRUE;
        HidePointer();
        break;
      case ActionEnter:
        ScrollState.LastSelection = ScrollState.CurrentSelection;
        ScrollState.CurrentSelection = mItemID;
        if ( Entries[mItemID].getREFIT_INPUT_DIALOG() ||  Entries[mItemID].getREFIT_MENU_CHECKBIT() ) {
          MenuExit = InputDialog(StyleFunc);
        } else if (Entries[mItemID].getREFIT_MENU_SWITCH()) {
          MenuExit = InputDialog(StyleFunc);
          ScrollState.PaintAll = TRUE;
          HidePointer();
        } else if (!Entries[mItemID].getREFIT_INFO_DIALOG()) {
          MenuExit = MENU_EXIT_ENTER;
        }
        break;
      case ActionHelp:
        MenuExit = MENU_EXIT_HELP;
        break;
      case ActionOptions:
        ScrollState.LastSelection = ScrollState.CurrentSelection;
        ScrollState.CurrentSelection = mItemID;
        MenuExit = MENU_EXIT_OPTIONS;
        break;
      case ActionDetails:
        ScrollState.LastSelection = ScrollState.CurrentSelection;
        // Index = State.CurrentSelection;
        ScrollState.CurrentSelection = mItemID;
        if ((Entries[mItemID].getREFIT_INPUT_DIALOG()) ||
            (Entries[mItemID].getREFIT_MENU_CHECKBIT())) {
          MenuExit = InputDialog(StyleFunc);
        } else if (Entries[mItemID].getREFIT_MENU_SWITCH()) {
          MenuExit = InputDialog(StyleFunc);
          ScrollState.PaintAll = TRUE;
          HidePointer();
        } else if (!Entries[mItemID].getREFIT_INFO_DIALOG()) {
          MenuExit = MENU_EXIT_DETAILS;
        }
        break;
      case ActionDeselect:
        ScrollState.LastSelection = ScrollState.CurrentSelection;
        ScrollState.PaintAll = TRUE;
        HidePointer();
        break;
      case ActionFinish:
        MenuExit = MENU_EXIT_ESCAPE;
        break;
      case ActionScrollDown:
        UpdateScroll(SCROLL_SCROLL_DOWN);
        break;
      case ActionScrollUp:
        UpdateScroll(SCROLL_SCROLL_UP);
        break;
      case ActionMoveScrollbar:
        UpdateScroll(SCROLL_SCROLLBAR_MOVE);
        break;
      default:
        break;
    }


    // read key press (and wait for it if applicable)
    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &key);
    if ((Status == EFI_NOT_READY) && (mAction == ActionNone)) {
      continue;
    }
    if (mAction == ActionNone) {
      ReadAllKeyStrokes(); //clean to avoid doubles
    }
    if (HaveTimeout) {
      // the user pressed a key, cancel the timeout
      StyleFunc(MENU_FUNCTION_PAINT_TIMEOUT, L"");
      HidePointer(); //ycr.ru
      HaveTimeout = FALSE;
    }

    mAction = ActionNone; //do action once
    // react to key press
    switch (key.ScanCode) {
      case SCAN_UP:
      case SCAN_LEFT:
        UpdateScroll(SCROLL_LINE_UP);
        break;
      case SCAN_DOWN:
      case SCAN_RIGHT:
        UpdateScroll(SCROLL_LINE_DOWN);
        break;
      case SCAN_HOME:
        UpdateScroll(SCROLL_FIRST);
        break;
      case SCAN_END:
        UpdateScroll(SCROLL_LAST);
        break;
      case SCAN_PAGE_UP:
        UpdateScroll(SCROLL_PAGE_UP);
    //    SetNextScreenMode(1);
        StyleFunc(MENU_FUNCTION_INIT, NULL);
        break;
      case SCAN_PAGE_DOWN:
        UpdateScroll(SCROLL_PAGE_DOWN);
     //   SetNextScreenMode(-1);
        StyleFunc(MENU_FUNCTION_INIT, NULL);
        break;
      case SCAN_ESC:
        MenuExit = MENU_EXIT_ESCAPE;
        break;
      case SCAN_INSERT:
        MenuExit = MENU_EXIT_OPTIONS;
        break;

      case SCAN_F1:
        MenuExit = MENU_EXIT_HELP;
        break;
      case SCAN_F2:
        SavePreBootLog = TRUE;
        //let it be twice
        Status = SaveBooterLog(SelfRootDir, PREBOOT_LOG);
        if (EFI_ERROR(Status)) {
          Status = SaveBooterLog(NULL, PREBOOT_LOG);
        }
        break;
      case SCAN_F3:
         MenuExit = MENU_EXIT_HIDE_TOGGLE;
         break;
      case SCAN_F4:
        SaveOemTables();
        break;
      case SCAN_F5:
        SaveOemDsdt(TRUE); //full patch
        break;
      case SCAN_F6:
        Status = egSaveFile(SelfRootDir, VBIOS_BIN, (UINT8*)(UINTN)0xc0000, 0x20000);
        if (EFI_ERROR(Status)) {
          Status = egSaveFile(NULL, VBIOS_BIN, (UINT8*)(UINTN)0xc0000, 0x20000);
        }
        break;
/* just a sample code
      case SCAN_F7:
        Status = egMkDir(SelfRootDir,  L"EFI\\CLOVER\\new_folder");
        DBG("create folder %r\n", Status);
        if (!EFI_ERROR(Status)) {
          Status = egSaveFile(SelfRootDir,  L"EFI\\CLOVER\\new_folder\\new_file.txt", (UINT8*)SomeText, sizeof(*SomeText)+1);
          DBG("create file %r\n", Status);
        }
        break;
*/
      case SCAN_F7:
        if (OldChosenAudio > AudioNum) {
              OldChosenAudio = 0; //security correction
        }
        Status = gBS->HandleProtocol(AudioList[OldChosenAudio].Handle, &gEfiAudioIoProtocolGuid, (VOID**)&AudioIo);
        DBG("open %d audio handle status=%r\n", OldChosenAudio, Status);
        if (!EFI_ERROR(Status)) {
          StartupSoundPlay(SelfRootDir, NULL); //play embedded sound
        }
        break;
      case SCAN_F8:
        testSVG();
        SaveHdaDumpBin();
        SaveHdaDumpTxt();
        break;

      case SCAN_F9:
        SetNextScreenMode(1);
        InitTheme(FALSE, NULL);
        break;
      case SCAN_F10:
        egScreenShot();
        break;
      case SCAN_F11:
        ResetNvram ();
        break;
      case SCAN_F12:
        MenuExit = MENU_EXIT_EJECT;
        ScrollState.PaintAll = TRUE;
        break;

    }
    switch (key.UnicodeChar) {
      case CHAR_LINEFEED:
      case CHAR_CARRIAGE_RETURN:
        if ((Entries[ScrollState.CurrentSelection].getREFIT_INPUT_DIALOG()) ||
            (Entries[ScrollState.CurrentSelection].getREFIT_MENU_CHECKBIT())) {
          MenuExit = InputDialog(StyleFunc);
        } else if (Entries[ScrollState.CurrentSelection].getREFIT_MENU_SWITCH()){
          MenuExit = InputDialog(StyleFunc);
          ScrollState.PaintAll = TRUE;
        } else if (Entries[ScrollState.CurrentSelection].getREFIT_MENU_ENTRY_CLOVER()){
          MenuExit = MENU_EXIT_DETAILS;
        } else if (!Entries[ScrollState.CurrentSelection].getREFIT_INFO_DIALOG()) {
          MenuExit = MENU_EXIT_ENTER;
        }
        break;
      case ' ': //CHAR_SPACE
        if ((Entries[ScrollState.CurrentSelection].getREFIT_INPUT_DIALOG()) ||
            (Entries[ScrollState.CurrentSelection].getREFIT_MENU_CHECKBIT())) {
          MenuExit = InputDialog(StyleFunc);
        } else if (Entries[ScrollState.CurrentSelection].getREFIT_MENU_SWITCH()){
          MenuExit = InputDialog(StyleFunc);
          ScrollState.PaintAll = TRUE;
          HidePointer();
        } else if (!Entries[ScrollState.CurrentSelection].getREFIT_INFO_DIALOG()) {
          MenuExit = MENU_EXIT_DETAILS;
        }
        break;

      default:
        ShortcutEntry = FindMenuShortcutEntry(key.UnicodeChar);
        if (ShortcutEntry >= 0) {
          ScrollState.CurrentSelection = ShortcutEntry;
          MenuExit = MENU_EXIT_ENTER;
        }
        break;
    }
  }

  StyleFunc(MENU_FUNCTION_CLEANUP, NULL);

	if (ChosenEntry) {
    *ChosenEntry = &Entries[ScrollState.CurrentSelection];
	}

  *DefaultEntryIndex = ScrollState.CurrentSelection;

  return MenuExit;
}

/**
 * Text Mode menu.
 */
VOID REFIT_MENU_SCREEN::TextMenuStyle(IN UINTN Function, IN CONST CHAR16 *ParamText)
{
  INTN i = 0, j = 0;
  static UINTN TextMenuWidth = 0,ItemWidth = 0, MenuHeight = 0;
  static UINTN MenuPosY = 0;
  //static CHAR16 **DisplayStrings;
  CHAR16 *TimeoutMessage;
  CHAR16 ResultString[TITLE_MAX_LEN]; // assume a title max length of around 128
	UINTN OldChosenItem = ~(UINTN)0;

  switch (Function) {

    case MENU_FUNCTION_INIT:
      // vertical layout
      MenuPosY = 4;

	  if (InfoLines.size() > 0) {
        MenuPosY += InfoLines.size() + 1;
	  }

      MenuHeight = ConHeight - MenuPosY;

	  if (TimeoutSeconds > 0) {
        MenuHeight -= 2;
	  }

      InitScroll(Entries.size(), Entries.size(), MenuHeight, 0);

      // determine width of the menu
      TextMenuWidth = 50;  // minimum
      for (i = 0; i <= ScrollState.MaxIndex; i++) {
        ItemWidth = StrLen(Entries[i].Title);

		if (TextMenuWidth < ItemWidth) {
          TextMenuWidth = ItemWidth;
        }
      }

	  if (TextMenuWidth > ConWidth - 6) {
        TextMenuWidth = ConWidth - 6;
	  }

    if (Entries[0].getREFIT_MENU_SWITCH() && Entries[0].getREFIT_MENU_SWITCH()->Item->IValue == 90) {
      j = OldChosenConfig;
    } else if (Entries[0].getREFIT_MENU_SWITCH() && Entries[0].getREFIT_MENU_SWITCH()->Item->IValue == 116) {
      j = OldChosenDsdt;
    } else if (Entries[0].getREFIT_MENU_SWITCH() && Entries[0].getREFIT_MENU_SWITCH()->Item->IValue == 119) {
      j = OldChosenAudio;
    }

    break;

    case MENU_FUNCTION_CLEANUP:
      // release temporary memory

			// reset default output colours
	  gST->ConOut->SetAttribute(gST->ConOut, ATTR_BANNER);

      break;

    case MENU_FUNCTION_PAINT_ALL:
        // paint the whole screen (initially and after scrolling)
      gST->ConOut->SetAttribute (gST->ConOut, ATTR_CHOICE_BASIC);
      for (i = 0; i < (INTN)ConHeight - 4; i++) {
        gST->ConOut->SetCursorPosition(gST->ConOut, 0, 4 + i);
        gST->ConOut->OutputString(gST->ConOut, BlankLine);
      }

        BeginTextScreen(Title);

        if (InfoLines.size() > 0) {
          gST->ConOut->SetAttribute (gST->ConOut, ATTR_BASIC);

          for (i = 0; i < (INTN)InfoLines.size(); i++) {
            gST->ConOut->SetCursorPosition (gST->ConOut, 3, 4 + i);
            gST->ConOut->OutputString (gST->ConOut, InfoLines[i].data());
          }
        }

        for (i = ScrollState.FirstVisible; i <= ScrollState.LastVisible && i <= ScrollState.MaxIndex; i++) {
      gST->ConOut->SetCursorPosition (gST->ConOut, 2, MenuPosY + (i - ScrollState.FirstVisible));

      if (i == ScrollState.CurrentSelection) {
        gST->ConOut->SetAttribute (gST->ConOut, ATTR_CHOICE_CURRENT);
      } else {
        gST->ConOut->SetAttribute (gST->ConOut, ATTR_CHOICE_BASIC);
      }

      StrCpyS(ResultString, TITLE_MAX_LEN, Entries[i].Title);

      if (Entries[i].getREFIT_INPUT_DIALOG()) {
        REFIT_INPUT_DIALOG& entry = (REFIT_INPUT_DIALOG&) Entries[i];
        if (entry.getREFIT_INPUT_DIALOG()) {
          if ((entry).Item->ItemType == BoolValue) {
            StrCatS(ResultString, TITLE_MAX_LEN, (entry).Item->BValue ? L": [+]" : L": [ ]");
          } else {
            StrCatS(ResultString, TITLE_MAX_LEN, (entry).Item->SValue);
          }
        } else if (entry.getREFIT_MENU_CHECKBIT()) {
          // check boxes
          StrCatS(ResultString, TITLE_MAX_LEN, ((entry).Item->IValue & (entry.Row)) ? L": [+]" : L": [ ]");
        } else if (entry.getREFIT_MENU_SWITCH()) {
          // radio buttons

          // update chosen config
          if (entry.getREFIT_MENU_SWITCH()->Item->IValue == 90) {
            OldChosenItem = OldChosenConfig;
          } else if (entry.getREFIT_MENU_SWITCH()->Item->IValue == 116) {
            OldChosenItem = OldChosenDsdt;
          } else if (entry.getREFIT_MENU_SWITCH()->Item->IValue == 119) {
            OldChosenItem = OldChosenAudio;
          }

          StrCatS(ResultString, TITLE_MAX_LEN, (entry.Row == OldChosenItem) ? L": (*)" : L": ( )");
        }
      }

      for (j = StrLen(ResultString); j < (INTN)TextMenuWidth; j++) {
        ResultString[j] = L' ';
      }

      ResultString[j] = 0;
        gST->ConOut->OutputString(gST->ConOut, ResultString);
      }

      // scrolling indicators
      gST->ConOut->SetAttribute (gST->ConOut, ATTR_SCROLLARROW);
      gST->ConOut->SetCursorPosition (gST->ConOut, 0, MenuPosY);

      if (ScrollState.FirstVisible > 0) {
          gST->ConOut->OutputString (gST->ConOut, ArrowUp);
      } else {
          gST->ConOut->OutputString (gST->ConOut, L" ");
      }

      gST->ConOut->SetCursorPosition (gST->ConOut, 0, MenuPosY + ScrollState.MaxVisible);

      if (ScrollState.LastVisible < ScrollState.MaxIndex) {
          gST->ConOut->OutputString (gST->ConOut, ArrowDown);
      } else {
          gST->ConOut->OutputString (gST->ConOut, L" ");
      }

      break;

    case MENU_FUNCTION_PAINT_SELECTION:
			// last selection
      // redraw selection cursor
      gST->ConOut->SetCursorPosition (gST->ConOut, 2, MenuPosY + (ScrollState.LastSelection - ScrollState.FirstVisible));
      gST->ConOut->SetAttribute (gST->ConOut, ATTR_CHOICE_BASIC);
      //gST->ConOut->OutputString (gST->ConOut, DisplayStrings[ScrollState.LastSelection]);
			StrCpyS(ResultString, TITLE_MAX_LEN, Entries[ScrollState.LastSelection].Title);
      if (Entries[ScrollState.LastSelection].getREFIT_INPUT_DIALOG()) {
        REFIT_INPUT_DIALOG& entry = (REFIT_INPUT_DIALOG&) Entries[ScrollState.LastSelection];
        if (entry.getREFIT_INPUT_DIALOG()) {
          if (entry.Item->ItemType == BoolValue) {
            StrCatS(ResultString, TITLE_MAX_LEN, entry.Item->BValue ? L": [+]" : L": [ ]");
          } else {
            StrCatS(ResultString, TITLE_MAX_LEN, entry.Item->SValue);
          }
        } else if (entry.getREFIT_MENU_CHECKBIT()) {
          // check boxes
          StrCatS(ResultString, TITLE_MAX_LEN, (entry.Item->IValue & (entry.Row)) ? L": [+]" : L": [ ]");
        } else if (entry.getREFIT_MENU_SWITCH()) {
          // radio buttons

          if (entry.getREFIT_MENU_SWITCH()->Item->IValue == 90) {
            OldChosenItem = OldChosenConfig;
          } else if (entry.getREFIT_MENU_SWITCH()->Item->IValue == 116) {
            OldChosenItem = OldChosenDsdt;
          } else if (entry.getREFIT_MENU_SWITCH()->Item->IValue == 119) {
            OldChosenItem = OldChosenAudio;
          }

          StrCatS(ResultString, TITLE_MAX_LEN, (entry.Row == OldChosenItem) ? L": (*)" : L": ( )");
        }
      }

      for (j = StrLen(ResultString); j < (INTN) TextMenuWidth; j++) {
        ResultString[j] = L' ';
      }

      ResultString[j] = 0;
      gST->ConOut->OutputString (gST->ConOut, ResultString);

        // current selection
      gST->ConOut->SetCursorPosition (gST->ConOut, 2, MenuPosY + (ScrollState.CurrentSelection - ScrollState.FirstVisible));
      gST->ConOut->SetAttribute (gST->ConOut, ATTR_CHOICE_CURRENT);
      StrCpyS(ResultString, TITLE_MAX_LEN, Entries[ScrollState.CurrentSelection].Title);
      if (Entries[ScrollState.CurrentSelection].getREFIT_INPUT_DIALOG()) {
        REFIT_INPUT_DIALOG& entry = (REFIT_INPUT_DIALOG&) Entries[ScrollState.CurrentSelection];
        if (entry.getREFIT_INPUT_DIALOG()) {
          if (entry.Item->ItemType == BoolValue) {
            StrCatS(ResultString, TITLE_MAX_LEN, entry.Item->BValue ? L": [+]" : L": [ ]");
          } else {
            StrCatS(ResultString, TITLE_MAX_LEN, entry.Item->SValue);
          }
        } else if (entry.getREFIT_MENU_CHECKBIT()) {
          // check boxes
          StrCatS(ResultString, TITLE_MAX_LEN, (entry.Item->IValue & (entry.Row)) ? L": [+]" : L": [ ]");
        } else if (entry.getREFIT_MENU_SWITCH()) {
          // radio buttons

          if (entry.getREFIT_MENU_SWITCH()->Item->IValue == 90) {
            OldChosenItem = OldChosenConfig;
          } else if (entry.getREFIT_MENU_SWITCH()->Item->IValue == 116) {
            OldChosenItem = OldChosenDsdt;
          } else if (entry.getREFIT_MENU_SWITCH()->Item->IValue == 119) {
            OldChosenItem = OldChosenAudio;
          }

          StrCatS(ResultString, TITLE_MAX_LEN, (entry.Row == OldChosenItem) ? L": (*)" : L": ( )");
        }
      }

      for (j = StrLen(ResultString); j < (INTN) TextMenuWidth; j++) {
        ResultString[j] = L' ';
      }

      ResultString[j] = 0;
      gST->ConOut->OutputString (gST->ConOut, ResultString);
        //gST->ConOut->OutputString (gST->ConOut, DisplayStrings[ScrollState.CurrentSelection]);

      break;

    case MENU_FUNCTION_PAINT_TIMEOUT:
      if (ParamText[0] == 0) {
        // clear message
        gST->ConOut->SetAttribute (gST->ConOut, ATTR_BASIC);
        gST->ConOut->SetCursorPosition (gST->ConOut, 0, ConHeight - 1);
        gST->ConOut->OutputString (gST->ConOut, BlankLine + 1);
      } else {
        // paint or update message
        gST->ConOut->SetAttribute (gST->ConOut, ATTR_ERROR);
        gST->ConOut->SetCursorPosition (gST->ConOut, 3, ConHeight - 1);
        TimeoutMessage = PoolPrint(L"%s  ", ParamText);
        gST->ConOut->OutputString (gST->ConOut, TimeoutMessage);
        FreePool(TimeoutMessage);
      }

      break;
  }
}

/**
 * Draw text with specific coordinates.
 */
INTN DrawTextXY(IN CONST CHAR16 *Text, IN INTN XPos, IN INTN YPos, IN UINT8 XAlign)
{
  INTN      TextWidth = 0;
  INTN      XText = 0;
  INTN      Height;
  INTN      TextXYStyle = 1;
  EG_IMAGE  *TextBufferXY = NULL;

  if (!Text) {
    return 0;
  }
  if (!textFace[1].valid) {
    if (textFace[2].valid) {
      TextXYStyle = 2;
    } else {
      TextXYStyle = 0;
    }
  }

  egMeasureText(Text, &TextWidth, NULL);

  if (XAlign == X_IS_LEFT) {
    TextWidth = UGAWidth - XPos - 1;
    XText = XPos;
  }

  if (GlobalConfig.TypeSVG) {
    TextWidth += TextHeight * 2; //give more place for buffer
    if (!textFace[TextXYStyle].valid) {
      DBG("no vaid text face for message!\n");
      Height = TextHeight;
    } else {
      Height = (int)(textFace[TextXYStyle].size * RowHeightFromTextHeight * GlobalConfig.Scale);
    }
  } else {
    Height = TextHeight;
  }

  TextBufferXY = egCreateFilledImage(TextWidth, Height, TRUE, &MenuBackgroundPixel);

  // render the text
  TextWidth = egRenderText(Text, TextBufferXY, 0, 0, 0xFFFF, TextXYStyle);

  if (XAlign != X_IS_LEFT) {
    // shift 64 is prohibited
    XText = XPos - (TextWidth >> XAlign);  //X_IS_CENTER = 1
  }
//  DBG("draw text %s\n", Text);
//  DBG("pos=%d width=%d xtext=%d Height=%d Y=%d\n", XPos, TextWidth, XText, Height, YPos);
  BltImageAlpha(TextBufferXY, XText, YPos,  &MenuBackgroundPixel, 16);
  egFreeImage(TextBufferXY);

  return TextWidth;
}

/**
 * Helper function to draw text for Boot Camp Style.
 * @author: Needy
 */
VOID DrawBCSText(IN CONST CHAR16 *Text, IN INTN XPos, IN INTN YPos, IN UINT8 XAlign)
{
  // check if text was provided
  if (!Text) {
    return;
  }

  INTN TextLen = StrLen(Text);

  // number of chars to be drawn on the screen
  INTN MaxTextLen = 13;
  INTN EllipsisLen = 2;
  
  CHAR16 *BCSText = NULL;

  // more space, more characters
  if (GlobalConfig.TileXSpace >= 25 && GlobalConfig.TileXSpace < 30) {
    MaxTextLen = 14;
  } else if (GlobalConfig.TileXSpace >= 30 && GlobalConfig.TileXSpace < 35) {
    MaxTextLen = 15;
  } else if (GlobalConfig.TileXSpace >= 35 && GlobalConfig.TileXSpace < 40) {
    MaxTextLen = 16;
  } else if (GlobalConfig.TileXSpace >= 40 && GlobalConfig.TileXSpace < 45) {
    MaxTextLen = 17;
  } else if (GlobalConfig.TileXSpace >= 45 && GlobalConfig.TileXSpace < 50) {
    MaxTextLen = 18;
  } else if (GlobalConfig.TileXSpace >= 50) {
    MaxTextLen = 19;
  }

  MaxTextLen += EllipsisLen;

  // if the text exceeds the given limit
  if (TextLen > MaxTextLen) {
    BCSText = (__typeof__(BCSText))AllocatePool((sizeof(CHAR16) * MaxTextLen) + 1);

    // error check, not enough memory
    if (!BCSText) {
      return;
    }

    // copy the permited amound of chars minus the ellipsis
    StrnCpyS(BCSText, (MaxTextLen - EllipsisLen) + 1, Text, MaxTextLen - EllipsisLen);
    BCSText[MaxTextLen - EllipsisLen] = '\0';

    // add ellipsis
    StrnCatS(BCSText, MaxTextLen + 1, L"..", EllipsisLen);
    // redundant, used for safety measures
    BCSText[MaxTextLen] = '\0';

    DrawTextXY(BCSText, XPos, YPos, XAlign);
    FreePool(BCSText);
  } else {
		// draw full text
    DrawTextXY(Text, XPos, YPos, XAlign);
  }
}



VOID DrawMenuText(IN CONST CHAR16 *Text, IN INTN SelectedWidth, IN INTN XPos, IN INTN YPos, IN INTN Cursor)
{
  //use Text=null to reinit the buffer
  if (!Text) {
    if (TextBuffer) {
      egFreeImage(TextBuffer);
      TextBuffer = NULL;
    }
    return;
  }

  if (TextBuffer && (TextBuffer->Height != TextHeight)) {
    egFreeImage(TextBuffer);
    TextBuffer = NULL;
  }

  if (TextBuffer == NULL) {
    TextBuffer = egCreateImage(UGAWidth-XPos, TextHeight, TRUE);
  }

  if (Cursor != 0xFFFF) {
    egFillImage(TextBuffer, &MenuBackgroundPixel);
  } else {
    egFillImage(TextBuffer, &InputBackgroundPixel);
  }


  if (SelectedWidth > 0) {
    // draw selection bar background
    egFillImageArea(TextBuffer, 0, 0, (INTN)SelectedWidth, TextHeight,
                    &SelectionBackgroundPixel);
  }

  // render the text
  if (GlobalConfig.TypeSVG) {
    //clovy - text veltically centred on Height
    egRenderText(Text, TextBuffer, 0,
                    (INTN)((TextHeight - (textFace[TextStyle].size * GlobalConfig.Scale)) / 2),
                    Cursor, TextStyle);
  } else {
    egRenderText(Text, TextBuffer, TEXT_XMARGIN, TEXT_YMARGIN, Cursor, TextStyle);
  }
  BltImageAlpha(TextBuffer, (INTN)XPos, (INTN)YPos, &MenuBackgroundPixel, 16);
}


VOID FreeScrollBar(VOID)
{
  if (ScrollbarBackgroundImage) {
    egFreeImage(ScrollbarBackgroundImage);
    ScrollbarBackgroundImage = NULL;
  }
  if (BarStartImage) {
    egFreeImage(BarStartImage);
    BarStartImage = NULL;
  }
  if (BarEndImage) {
    egFreeImage(BarEndImage);
    BarEndImage = NULL;
  }
  if (ScrollbarImage) {
    egFreeImage(ScrollbarImage);
    ScrollbarImage = NULL;
  }
  if (ScrollStartImage) {
    egFreeImage(ScrollStartImage);
    ScrollStartImage = NULL;
  }
  if (ScrollEndImage) {
    egFreeImage(ScrollEndImage);
    ScrollEndImage = NULL;
  }
  if (UpButtonImage) {
    egFreeImage(UpButtonImage);
    UpButtonImage = NULL;
  }
  if (DownButtonImage) {
    egFreeImage(DownButtonImage);
    DownButtonImage = NULL;
  }
}


VOID InitBar(VOID)
{
  if (ThemeDir) {
    if (!ScrollbarBackgroundImage) {
        ScrollbarBackgroundImage = egLoadImage(ThemeDir, GetIconsExt(L"scrollbar\\bar_fill", L"png"), FALSE);
    }
    if (!BarStartImage) {
      BarStartImage = egLoadImage(ThemeDir, GetIconsExt(L"scrollbar\\bar_start", L"png"), TRUE);
    }
    if (!BarEndImage) {
      BarEndImage = egLoadImage(ThemeDir, GetIconsExt(L"scrollbar\\bar_end", L"png"), TRUE);
    }
    if (!ScrollbarImage) {
      ScrollbarImage = egLoadImage(ThemeDir, GetIconsExt(L"scrollbar\\scroll_fill", L"png"), FALSE);
    }
    if (!ScrollStartImage) {
      ScrollStartImage = egLoadImage(ThemeDir, GetIconsExt(L"scrollbar\\scroll_start", L"png"), TRUE);
    }
    if (!ScrollEndImage) {
      ScrollEndImage = egLoadImage(ThemeDir, GetIconsExt(L"scrollbar\\scroll_end", L"png"), TRUE);
    }
    if (!UpButtonImage) {
      UpButtonImage = egLoadImage(ThemeDir, GetIconsExt(L"scrollbar\\up_button", L"png"), TRUE);
    }
    if (!DownButtonImage) {
      DownButtonImage = egLoadImage(ThemeDir, GetIconsExt(L"scrollbar\\down_button", L"png"), TRUE);
    }
  }

  if (!BarStartImage  && !GlobalConfig.TypeSVG) {
    BarStartImage = egDecodePNG(ACCESS_EMB_DATA(emb_scroll_bar_start), ACCESS_EMB_SIZE(emb_scroll_bar_start), TRUE);
  }
  if (!BarEndImage && !GlobalConfig.TypeSVG) {
    BarEndImage = egDecodePNG(ACCESS_EMB_DATA(emb_scroll_bar_end), ACCESS_EMB_SIZE(emb_scroll_bar_end), TRUE);
  }
  if (!ScrollbarBackgroundImage) {
    if (GlobalConfig.TypeSVG) {
       ScrollbarBackgroundImage = egLoadIcon(ThemeDir, L"scrollbar-background.png", 48);
    }
    if (!ScrollbarBackgroundImage) {
      ScrollbarBackgroundImage = egDecodePNG(ACCESS_EMB_DATA(emb_scroll_bar_fill), ACCESS_EMB_SIZE(emb_scroll_bar_fill), TRUE);
    }
  }
  if (!ScrollbarImage) {
    if (GlobalConfig.TypeSVG) {
      ScrollbarImage = egLoadIcon(ThemeDir, L"scrollbar-holder.png", 48);
    }
    if (!ScrollbarImage) {
      ScrollbarImage = egDecodePNG(ACCESS_EMB_DATA(emb_scroll_scroll_fill), ACCESS_EMB_SIZE(emb_scroll_scroll_fill), TRUE);
    }
  }
  if (!ScrollStartImage && !GlobalConfig.TypeSVG) {
    ScrollStartImage = egDecodePNG(ACCESS_EMB_DATA(emb_scroll_scroll_start), ACCESS_EMB_SIZE(emb_scroll_scroll_start), TRUE);
  }
  if (!ScrollEndImage && !GlobalConfig.TypeSVG) {
    ScrollEndImage = egDecodePNG(ACCESS_EMB_DATA(emb_scroll_scroll_end), ACCESS_EMB_SIZE(emb_scroll_scroll_end), TRUE);
  }
  if (!UpButtonImage && !GlobalConfig.TypeSVG) {
    UpButtonImage = egDecodePNG(ACCESS_EMB_DATA(emb_scroll_up_button), ACCESS_EMB_SIZE(emb_scroll_up_button), TRUE);
  }
  if (!DownButtonImage && !GlobalConfig.TypeSVG) {
    DownButtonImage = egDecodePNG(ACCESS_EMB_DATA(emb_scroll_down_button), ACCESS_EMB_SIZE(emb_scroll_down_button), TRUE);
  }
  if (!GlobalConfig.TypeSVG) {
    UpButton.Width      = ScrollWidth; // 16
    UpButton.Height     = ScrollButtonsHeight; // 20
    DownButton.Width    = UpButton.Width;
    DownButton.Height   = ScrollButtonsHeight;
    BarStart.Height     = ScrollBarDecorationsHeight; // 5
    BarEnd.Height       = ScrollBarDecorationsHeight;
    ScrollStart.Height  = ScrollScrollDecorationsHeight; // 7
    ScrollEnd.Height    = ScrollScrollDecorationsHeight;

  } else {
    UpButton.Width      = ScrollWidth; // 16
    UpButton.Height     = 0; // 20
    DownButton.Width    = UpButton.Width;
    DownButton.Height   = 0;
    BarStart.Height     = ScrollBarDecorationsHeight; // 5
    BarEnd.Height       = ScrollBarDecorationsHeight;
    ScrollStart.Height  = 0; // 7
    ScrollEnd.Height    = 0;

  }
}

VOID SetBar(INTN PosX, INTN UpPosY, INTN DownPosY, IN SCROLL_STATE *State)
{
//  DBG("SetBar <= %d %d %d %d %d\n", UpPosY, DownPosY, State->MaxVisible, State->MaxIndex, State->FirstVisible);
//SetBar <= 302 722 19 31 0
  UpButton.XPos = PosX;
  UpButton.YPos = UpPosY;

  DownButton.XPos = UpButton.XPos;
  DownButton.YPos = DownPosY;

  ScrollbarBackground.XPos = UpButton.XPos;
  ScrollbarBackground.YPos = UpButton.YPos + UpButton.Height;
  ScrollbarBackground.Width = UpButton.Width;
  ScrollbarBackground.Height = DownButton.YPos - (UpButton.YPos + UpButton.Height);

  BarStart.XPos = ScrollbarBackground.XPos;
  BarStart.YPos = ScrollbarBackground.YPos;
  BarStart.Width = ScrollbarBackground.Width;

  BarEnd.Width = ScrollbarBackground.Width;
  BarEnd.XPos = ScrollbarBackground.XPos;
  BarEnd.YPos = DownButton.YPos - BarEnd.Height;

  ScrollStart.XPos = ScrollbarBackground.XPos;
  ScrollStart.YPos = ScrollbarBackground.YPos + ScrollbarBackground.Height * State->FirstVisible / (State->MaxIndex + 1);
  ScrollStart.Width = ScrollbarBackground.Width;

  Scrollbar.XPos = ScrollbarBackground.XPos;
  Scrollbar.YPos = ScrollStart.YPos + ScrollStart.Height;
  Scrollbar.Width = ScrollbarBackground.Width;
  Scrollbar.Height = ScrollbarBackground.Height * (State->MaxVisible + 1) / (State->MaxIndex + 1) - ScrollStart.Height;

  ScrollEnd.Width = ScrollbarBackground.Width;
  ScrollEnd.XPos = ScrollbarBackground.XPos;
  ScrollEnd.YPos = Scrollbar.YPos + Scrollbar.Height - ScrollEnd.Height;

  Scrollbar.Height -= ScrollEnd.Height;

  ScrollTotal.XPos = UpButton.XPos;
  ScrollTotal.YPos = UpButton.YPos;
  ScrollTotal.Width = UpButton.Width;
  ScrollTotal.Height = DownButton.YPos + DownButton.Height - UpButton.YPos;
//  DBG("ScrollTotal.Height = %d\n", ScrollTotal.Height);  //ScrollTotal.Height = 420
}

VOID ScrollingBar(IN SCROLL_STATE *State)
{
  EG_IMAGE* Total;
  INTN  i;

  ScrollEnabled = (State->MaxFirstVisible != 0);
  if (ScrollEnabled) {
    Total = egCreateFilledImage(ScrollTotal.Width, ScrollTotal.Height, TRUE, &MenuBackgroundPixel);

    if (ScrollbarBackgroundImage && ScrollbarBackgroundImage->Height) {
      for (i = 0; i < ScrollbarBackground.Height; i+=ScrollbarBackgroundImage->Height) {
        egComposeImage(Total, ScrollbarBackgroundImage, ScrollbarBackground.XPos - ScrollTotal.XPos, ScrollbarBackground.YPos + i - ScrollTotal.YPos);
      }
    }

    egComposeImage(Total, BarStartImage, BarStart.XPos - ScrollTotal.XPos, BarStart.YPos - ScrollTotal.YPos);
    egComposeImage(Total, BarEndImage, BarEnd.XPos - ScrollTotal.XPos, BarEnd.YPos - ScrollTotal.YPos);

    if (ScrollbarImage && ScrollbarImage->Height) {
      for (i = 0; i < Scrollbar.Height; i+=ScrollbarImage->Height) {
        egComposeImage(Total, ScrollbarImage, Scrollbar.XPos - ScrollTotal.XPos, Scrollbar.YPos + i - ScrollTotal.YPos);
      }
    }

    egComposeImage(Total, UpButtonImage, UpButton.XPos - ScrollTotal.XPos, UpButton.YPos - ScrollTotal.YPos);
    egComposeImage(Total, DownButtonImage, DownButton.XPos - ScrollTotal.XPos, DownButton.YPos - ScrollTotal.YPos);
    egComposeImage(Total, ScrollStartImage, ScrollStart.XPos - ScrollTotal.XPos, ScrollStart.YPos - ScrollTotal.YPos);
    egComposeImage(Total, ScrollEndImage, ScrollEnd.XPos - ScrollTotal.XPos, ScrollEnd.YPos - ScrollTotal.YPos);

    BltImageAlpha(Total, ScrollTotal.XPos, ScrollTotal.YPos, &MenuBackgroundPixel, ScrollWidth);
    egFreeImage(Total);
  }
}

/**
 * Graphical menu.
 */
VOID REFIT_MENU_SCREEN::GraphicsMenuStyle(IN UINTN Function, IN CONST CHAR16 *ParamText)
{
  INTN i;
  INTN j = 0;
  INTN ItemWidth = 0;
  INTN X, t1, t2;
  INTN VisibleHeight = 0; //assume vertical layout
  CHAR16 ResultString[TITLE_MAX_LEN]; // assume a title max length of around 128
  INTN PlaceCentre = 0; //(TextHeight / 2) - 7;
  INTN PlaceCentre1 = 0;
  UINTN OldChosenItem = ~(UINTN)0;
	INTN TitleLen = 0;
  INTN ScaledWidth = (INTN)(GlobalConfig.CharWidth * GlobalConfig.Scale);
// clovy
	INTN ctrlX, ctrlY, ctrlTextX;

  HidePointer();

  switch (Function) {

    case MENU_FUNCTION_INIT:
      egGetScreenSize(&UGAWidth, &UGAHeight);
      InitAnime();
			SwitchToGraphicsAndClear();

      EntriesPosY = ((UGAHeight - (int)(LAYOUT_TOTAL_HEIGHT * GlobalConfig.Scale)) >> 1) + (int)(LayoutBannerOffset * GlobalConfig.Scale) + (TextHeight << 1);

      VisibleHeight = ((UGAHeight - EntriesPosY) / TextHeight) - InfoLines.size() - 2;/* - GlobalConfig.PruneScrollRows; */
      //DBG("MENU_FUNCTION_INIT 1 EntriesPosY=%d VisibleHeight=%d\n", EntriesPosY, VisibleHeight);
      if ( Entries[0].getREFIT_INPUT_DIALOG() ) {
        REFIT_INPUT_DIALOG& entry = (REFIT_INPUT_DIALOG&)Entries[0];
        if (entry.getREFIT_MENU_SWITCH()) {
          if (entry.getREFIT_MENU_SWITCH()->Item->IValue == 3) {
            j = (OldChosenTheme == 0xFFFF) ? 0: (OldChosenTheme + 1);
          } else if (entry.getREFIT_MENU_SWITCH()->Item->IValue == 90) {
            j = OldChosenConfig;
          } else if (entry.getREFIT_MENU_SWITCH()->Item->IValue == 116) {
            j = (OldChosenDsdt == 0xFFFF) ? 0: (OldChosenDsdt + 1);
          } else if (entry.getREFIT_MENU_SWITCH()->Item->IValue == 119) {
            j = OldChosenAudio;
          }
        }
      }
      InitScroll(Entries.size(), Entries.size(), VisibleHeight, j);
      // determine width of the menu - not working
      //MenuWidth = 80;  // minimum
      MenuWidth = (int)(LAYOUT_TEXT_WIDTH * GlobalConfig.Scale); //500
      DrawMenuText(NULL, 0, 0, 0, 0);

      if (TitleImage) {
        if (MenuWidth > (INTN)(UGAWidth - (int)(TITLEICON_SPACING * GlobalConfig.Scale) - TitleImage->Width)) {
          MenuWidth = UGAWidth - (int)(TITLEICON_SPACING * GlobalConfig.Scale) - TitleImage->Width - 2;
        }
        EntriesPosX = (UGAWidth - (TitleImage->Width + (int)(TITLEICON_SPACING * GlobalConfig.Scale) + MenuWidth)) >> 1;
        //DBG("UGAWIdth=%d TitleImage=%d MenuWidth=%d\n", UGAWidth,
        //TitleImage->Width, MenuWidth);
        MenuWidth += TitleImage->Width;
      } else {
        EntriesPosX = (UGAWidth - MenuWidth) >> 1;
      }
      TimeoutPosY = EntriesPosY + (Entries.size() + 1) * TextHeight;

      // initial painting
      egMeasureText(Title, &ItemWidth, NULL);
      if (!(GlobalConfig.HideUIFlags & HIDEUI_FLAG_MENU_TITLE)) {
        DrawTextXY(Title, (UGAWidth >> 1), EntriesPosY - TextHeight * 2, X_IS_CENTER);
      }

      if (TitleImage) {
        INTN FilmXPos = (INTN)(EntriesPosX - (TitleImage->Width + (int)(TITLEICON_SPACING * GlobalConfig.Scale)));
        INTN FilmYPos = (INTN)EntriesPosY;
        BltImageAlpha(TitleImage, FilmXPos, FilmYPos, &MenuBackgroundPixel, 16);

        // update FilmPlace only if not set by InitAnime
        if (FilmPlace.Width == 0 || FilmPlace.Height == 0) {
          FilmPlace.XPos = FilmXPos;
          FilmPlace.YPos = FilmYPos;
          FilmPlace.Width = TitleImage->Width;
          FilmPlace.Height = TitleImage->Height;
        }
      }

      if (InfoLines.size() > 0) {
        DrawMenuText(NULL, 0, 0, 0, 0);
        for (i = 0; i < (INTN)InfoLines.size(); i++) {
          DrawMenuText(InfoLines[i], 0, EntriesPosX, EntriesPosY, 0xFFFF);
          EntriesPosY += TextHeight;
        }
        EntriesPosY += TextHeight;  // also add a blank line
      }
      InitBar();

      break;

    case MENU_FUNCTION_CLEANUP:
      HidePointer();
      break;

    case MENU_FUNCTION_PAINT_ALL:
      DrawMenuText(NULL, 0, 0, 0, 0); //should clean every line to avoid artefacts
	//		DBG("PAINT_ALL: EntriesPosY=%d MaxVisible=%d\n", EntriesPosY, ScrollState.MaxVisible);
	//		DBG("DownButton.Height=%d TextHeight=%d\n", DownButton.Height, TextHeight);
      t2 = EntriesPosY + (ScrollState.MaxVisible + 1) * TextHeight - DownButton.Height;
      t1 = EntriesPosX + TextHeight + MenuWidth  + (INTN)((TEXT_XMARGIN + 16) * GlobalConfig.Scale);
	//		DBG("PAINT_ALL: %d %d\n", t1, t2);
      SetBar(t1, EntriesPosY, t2, &ScrollState); //823 302 554

      // blackosx swapped this around so drawing of selection comes before drawing scrollbar.

      for (i = ScrollState.FirstVisible, j = 0; i <= ScrollState.LastVisible; i++, j++) {
        REFIT_ABSTRACT_MENU_ENTRY *Entry = &Entries[i];
        TitleLen = StrLen(Entry->Title);

        Entry->Place.XPos = EntriesPosX;
        Entry->Place.YPos = EntriesPosY + j * TextHeight;
        Entry->Place.Width = TitleLen * ScaledWidth;
        Entry->Place.Height = (UINTN)TextHeight;
        StrCpyS(ResultString, TITLE_MAX_LEN, Entry->Title);
        //clovy//PlaceCentre1 = (TextHeight - (INTN)(Buttons[2]->Height * GlobalConfig.Scale)) / 2;
        //clovy//PlaceCentre = (PlaceCentre>0)?PlaceCentre:0;
        //clovy//PlaceCentre1 = (TextHeight - (INTN)(Buttons[0]->Height * GlobalConfig.Scale)) / 2;
        PlaceCentre = (INTN)((TextHeight - (INTN)(Buttons[2]->Height)) * GlobalConfig.Scale / 2);
        PlaceCentre1 = (INTN)((TextHeight - (INTN)(Buttons[0]->Height)) * GlobalConfig.Scale / 2);
// clovy
        ctrlX = EntriesPosX + (INTN)(TEXT_XMARGIN * GlobalConfig.Scale);
        if (GlobalConfig.TypeSVG)
          ctrlX = EntriesPosX;
        ctrlTextX = ctrlX + Buttons[0]->Width + (INTN)(TEXT_XMARGIN * GlobalConfig.Scale / 2);
        ctrlY = Entry->Place.YPos + PlaceCentre;

        if ( Entry->getREFIT_INPUT_DIALOG() ) {
          REFIT_INPUT_DIALOG* inputDialogEntry = Entry->getREFIT_INPUT_DIALOG();
          if (inputDialogEntry->Item->ItemType == BoolValue) {
            Entry->Place.Width = StrLen(ResultString) * ScaledWidth;
            DrawMenuText(L" ", 0, EntriesPosX, Entry->Place.YPos, 0xFFFF);
            DrawMenuText(ResultString, (i == ScrollState.CurrentSelection) ? (MenuWidth) : 0,
// clovy                    EntriesPosX + (TextHeight + (INTN)(TEXT_XMARGIN * GlobalConfig.Scale)),
                         ctrlTextX,
                         Entry->Place.YPos, 0xFFFF);
            BltImageAlpha( (((REFIT_INPUT_DIALOG*)(Entry))->Item->BValue) ? Buttons[3] :Buttons[2],
                          ctrlX, ctrlY,
                          &MenuBackgroundPixel, 16);
//            DBG("X=%d, Y=%d, ImageY=%d\n", EntriesPosX + (INTN)(TEXT_XMARGIN * GlobalConfig.Scale),
//                Entry->Place.YPos, Entry->Place.YPos + PlaceCentre);

          } else {
            // text input
            StrCatS(ResultString, TITLE_MAX_LEN, ((REFIT_INPUT_DIALOG*)(Entry))->Item->SValue);
            StrCatS(ResultString, TITLE_MAX_LEN, L" ");
            Entry->Place.Width = StrLen(ResultString) * ScaledWidth;
            // Slice - suppose to use Row as Cursor in text
            DrawMenuText(ResultString, (i == ScrollState.CurrentSelection) ? MenuWidth : 0,
                         EntriesPosX,
                         Entry->Place.YPos, TitleLen + Entry->Row);
          }
        } else if (Entry->getREFIT_MENU_CHECKBIT()) {
          DrawMenuText(L" ", 0, EntriesPosX, Entry->Place.YPos, 0xFFFF);
          DrawMenuText(ResultString, (i == ScrollState.CurrentSelection) ? (MenuWidth) : 0,
// clovy                  EntriesPosX + (TextHeight + (INTN)(TEXT_XMARGIN * GlobalConfig.Scale)),
                       ctrlTextX,
                       Entry->Place.YPos, 0xFFFF);
          BltImageAlpha((((REFIT_INPUT_DIALOG*)(Entry))->Item->IValue & Entry->Row) ? Buttons[3] :Buttons[2],
                        ctrlX,
                        ctrlY,
                        &MenuBackgroundPixel, 16);

        } else if (Entry->getREFIT_MENU_SWITCH()) {
          if (Entry->getREFIT_MENU_SWITCH()->Item->IValue == 3) {
            //OldChosenItem = OldChosenTheme;
            OldChosenItem = (OldChosenTheme == 0xFFFF) ? 0: (OldChosenTheme + 1);
          } else if (Entry->getREFIT_MENU_SWITCH()->Item->IValue == 90) {
            OldChosenItem = OldChosenConfig;
          } else if (Entry->getREFIT_MENU_SWITCH()->Item->IValue == 116) {
            OldChosenItem =  (OldChosenDsdt == 0xFFFF) ? 0: (OldChosenDsdt + 1);
          } else if (Entry->getREFIT_MENU_SWITCH()->Item->IValue == 119) {
            OldChosenItem = OldChosenAudio;
          }

          DrawMenuText(ResultString,
                       (i == ScrollState.CurrentSelection) ? MenuWidth : 0,
// clovy                  EntriesPosX + (TextHeight + (INTN)(TEXT_XMARGIN * GlobalConfig.Scale)),
                       ctrlTextX,
                       Entry->Place.YPos, 0xFFFF);
          BltImageAlpha((Entry->Row == OldChosenItem) ? Buttons[1] : Buttons[0],
                        ctrlX,
                        ctrlY,
                        &MenuBackgroundPixel, 16);
        } else {
					//DBG("paint entry %d title=%s\n", i, Entries[i]->Title);
          DrawMenuText(ResultString,
                       (i == ScrollState.CurrentSelection) ? MenuWidth : 0,
                       EntriesPosX, Entry->Place.YPos, 0xFFFF);
        }
      }

      ScrollingBar(&ScrollState);
      //MouseBirth();
      break;

    case MENU_FUNCTION_PAINT_SELECTION:
    {
			// last selection
      REFIT_ABSTRACT_MENU_ENTRY *EntryL = &Entries[ScrollState.LastSelection];
      REFIT_ABSTRACT_MENU_ENTRY *EntryC = &Entries[ScrollState.CurrentSelection];
      TitleLen = StrLen(EntryL->Title);
      StrCpyS(ResultString, TITLE_MAX_LEN, EntryL->Title);
      //clovy//PlaceCentre = (TextHeight - (INTN)(Buttons[2]->Height * GlobalConfig.Scale)) / 2;
      //clovy//PlaceCentre = (PlaceCentre>0)?PlaceCentre:0;
      //clovy//PlaceCentre1 = (TextHeight - (INTN)(Buttons[0]->Height * GlobalConfig.Scale)) / 2;
      PlaceCentre = (INTN)((TextHeight - (INTN)(Buttons[2]->Height)) * GlobalConfig.Scale / 2);
      PlaceCentre1 = (INTN)((TextHeight - (INTN)(Buttons[0]->Height)) * GlobalConfig.Scale / 2);

// clovy
			ctrlX = EntriesPosX + (INTN)(TEXT_XMARGIN * GlobalConfig.Scale);
      if (GlobalConfig.TypeSVG)
        ctrlX = EntriesPosX;
			ctrlTextX = ctrlX + Buttons[0]->Width + (INTN)(TEXT_XMARGIN * GlobalConfig.Scale / 2);

      // redraw selection cursor
      // 1. blackosx swapped this around so drawing of selection comes before drawing scrollbar.
      // 2. usr-sse2
      if ( EntryL->getREFIT_INPUT_DIALOG() ) {
        REFIT_INPUT_DIALOG* inputDialogEntry = (REFIT_INPUT_DIALOG*)EntryL;
        if (inputDialogEntry->Item->ItemType == BoolValue) {
          //clovy//DrawMenuText(ResultString, 0, EntriesPosX + (TextHeight + TEXT_XMARGIN),
          //clovy//             EntryL->Place.YPos, 0xFFFF);
          DrawMenuText(ResultString, 0,
                        ctrlTextX,
                       EntryL->Place.YPos, 0xFFFF);
          BltImageAlpha((inputDialogEntry->Item->BValue)? Buttons[3] : Buttons[2],
                        ctrlX,
                        EntryL->Place.YPos + PlaceCentre,
                        &MenuBackgroundPixel, 16);
//          DBG("se:X=%d, Y=%d, ImageY=%d\n", EntriesPosX + (INTN)(TEXT_XMARGIN * GlobalConfig.Scale),
//              EntryL->Place.YPos, EntryL->Place.YPos + PlaceCentre);
        } else {
          StrCatS(ResultString, TITLE_MAX_LEN, ((REFIT_INPUT_DIALOG*)(EntryL))->Item->SValue +
                 ((REFIT_INPUT_DIALOG*)(EntryL))->Item->LineShift);
          StrCatS(ResultString, TITLE_MAX_LEN, L" ");
          DrawMenuText(ResultString, 0, EntriesPosX,
                       EntriesPosY + (ScrollState.LastSelection - ScrollState.FirstVisible) * TextHeight,
                       TitleLen + EntryL->Row);
        }
      } else if (EntryL->getREFIT_MENU_SWITCH()) {

        if (EntryL->getREFIT_MENU_SWITCH()->Item->IValue == 3) {
          OldChosenItem = (OldChosenTheme == 0xFFFF) ? 0: OldChosenTheme + 1;
        } else if (EntryL->getREFIT_MENU_SWITCH()->Item->IValue == 90) {
          OldChosenItem = OldChosenConfig;
        } else if (EntryL->getREFIT_MENU_SWITCH()->Item->IValue == 116) {
          OldChosenItem = (OldChosenDsdt == 0xFFFF) ? 0: OldChosenDsdt + 1;
        } else if (EntryL->getREFIT_MENU_SWITCH()->Item->IValue == 119) {
          OldChosenItem = OldChosenAudio;
        }

// clovy
//         DrawMenuText(ResultString, 0, EntriesPosX + (TextHeight + (INTN)(TEXT_XMARGIN * GlobalConfig.Scale)),
//                      EntriesPosY + (ScrollState.LastSelection - ScrollState.FirstVisible) * TextHeight, 0xFFFF);
        DrawMenuText(ResultString, 0, ctrlTextX,
                     EntriesPosY + (ScrollState.LastSelection - ScrollState.FirstVisible) * TextHeight, 0xFFFF);
        BltImageAlpha((EntryL->Row == OldChosenItem) ? Buttons[1] : Buttons[0],
                     ctrlX,
                      EntryL->Place.YPos + PlaceCentre1,
                      &MenuBackgroundPixel, 16);
      } else if (EntryL->getREFIT_MENU_CHECKBIT()) {
// clovy
//         DrawMenuText(ResultString, 0, EntriesPosX + (TextHeight + (INTN)(TEXT_XMARGIN * GlobalConfig.Scale)),
//                      EntryL->Place.YPos, 0xFFFF);
        DrawMenuText(ResultString, 0, ctrlTextX,
                     EntryL->Place.YPos, 0xFFFF);
        BltImageAlpha((EntryL->getREFIT_MENU_CHECKBIT()->Item->IValue & EntryL->Row) ? Buttons[3] : Buttons[2],
                     ctrlX,
                      EntryL->Place.YPos + PlaceCentre,
                      &MenuBackgroundPixel, 16);
//        DBG("ce:X=%d, Y=%d, ImageY=%d\n", EntriesPosX + (INTN)(TEXT_XMARGIN * GlobalConfig.Scale),
//            EntryL->Place.YPos, EntryL->Place.YPos + PlaceCentre);
      } else {
        DrawMenuText(EntryL->Title, 0, EntriesPosX,
                     EntriesPosY + (ScrollState.LastSelection - ScrollState.FirstVisible) * TextHeight, 0xFFFF);
      }

      // current selection
      StrCpyS(ResultString, TITLE_MAX_LEN, EntryC->Title);
      TitleLen = StrLen(EntryC->Title);
      if (EntryC->getREFIT_MENU_SWITCH()->Item->IValue == 3) {
        OldChosenItem = (OldChosenTheme == 0xFFFF) ? 0: OldChosenTheme + 1;;
      } else if (EntryC->getREFIT_MENU_SWITCH()->Item->IValue == 90) {
        OldChosenItem = OldChosenConfig;
      } else if (EntryC->getREFIT_MENU_SWITCH()->Item->IValue == 116) {
        OldChosenItem = (OldChosenDsdt == 0xFFFF) ? 0: OldChosenDsdt + 1;
      } else if (EntryC->getREFIT_MENU_SWITCH()->Item->IValue == 119) {
        OldChosenItem = OldChosenAudio;
      }

      if ( EntryC->getREFIT_INPUT_DIALOG() ) {
        REFIT_INPUT_DIALOG* inputDialogEntry = (REFIT_INPUT_DIALOG*)EntryC;
        if (inputDialogEntry->Item->ItemType == BoolValue) {
          DrawMenuText(ResultString, MenuWidth,
                       ctrlTextX,
                       inputDialogEntry->Place.YPos, 0xFFFF);
          BltImageAlpha((inputDialogEntry->Item->BValue)? Buttons[3] : Buttons[2],
                        ctrlX,
                        inputDialogEntry->Place.YPos + PlaceCentre,
                        &MenuBackgroundPixel, 16);
        } else {
          StrCatS(ResultString, TITLE_MAX_LEN, inputDialogEntry->Item->SValue +
                               inputDialogEntry->Item->LineShift);
          StrCatS(ResultString, TITLE_MAX_LEN, L" ");
          DrawMenuText(ResultString, MenuWidth, EntriesPosX,
                       EntriesPosY + (ScrollState.CurrentSelection - ScrollState.FirstVisible) * TextHeight,
                       TitleLen + inputDialogEntry->Row);
        }
      } else if (EntryC->getREFIT_MENU_SWITCH()) {
        StrCpyS(ResultString, TITLE_MAX_LEN, EntryC->Title);
        DrawMenuText(ResultString, MenuWidth,
                     ctrlTextX,
                     EntriesPosY + (ScrollState.CurrentSelection - ScrollState.FirstVisible) * TextHeight,
                     0xFFFF);
        BltImageAlpha((EntryC->Row == OldChosenItem) ? Buttons[1]:Buttons[0],
                      ctrlX,
                      EntryC->Place.YPos + PlaceCentre1,
                      &MenuBackgroundPixel, 16);
      } else if (EntryC->getREFIT_MENU_CHECKBIT()) {
        DrawMenuText(ResultString, MenuWidth,
                     ctrlTextX,
                     EntryC->Place.YPos, 0xFFFF);
        BltImageAlpha((EntryC->getREFIT_MENU_CHECKBIT()->Item->IValue & EntryC->Row) ? Buttons[3] :Buttons[2],
                      ctrlX,
                      EntryC->Place.YPos + PlaceCentre,
                      &MenuBackgroundPixel, 16);
      }else{
        DrawMenuText(EntryC->Title, MenuWidth, EntriesPosX,
                     EntriesPosY + (ScrollState.CurrentSelection - ScrollState.FirstVisible) * TextHeight,
                     0xFFFF);
      }

      ScrollStart.YPos = ScrollbarBackground.YPos + ScrollbarBackground.Height * ScrollState.FirstVisible / (ScrollState.MaxIndex + 1);
      Scrollbar.YPos = ScrollStart.YPos + ScrollStart.Height;
      ScrollEnd.YPos = Scrollbar.YPos + Scrollbar.Height; // ScrollEnd.Height is already subtracted
      ScrollingBar(ScrollState);

      break;
    }

    case MENU_FUNCTION_PAINT_TIMEOUT: //ever be here?
      X = (UGAWidth - StrLen(ParamText) * ScaledWidth) >> 1;
      DrawMenuText(ParamText, 0, X, TimeoutPosY, 0xFFFF);
      break;
  }

  MouseBirth();
}

/**
 * Draw entries for GUI.
 */
VOID DrawMainMenuEntry(REFIT_ABSTRACT_MENU_ENTRY *Entry, BOOLEAN selected, INTN XPos, INTN YPos)
{
  INTN Scale = GlobalConfig.MainEntriesSize >> 3; //usually it is 128>>3 == 16. if 256>>3 == 32

  if ( Entry->getDriveImage()  &&  !(GlobalConfig.HideBadges & HDBADGES_SWAP) /*&& Entry->Row == 0*/) {
    MainImage = Entry->getDriveImage();
  } else {
    MainImage = Entry->Image;
  }

  if (!MainImage) {
    if (!IsEmbeddedTheme()) {
      MainImage = egLoadIcon(ThemeDir, GetIconsExt(L"icons\\os_mac", L"icns"), Scale << 3);
    }
    if (!MainImage) {
      MainImage = DummyImage(Scale << 3);
    }
  }
  //  DBG("Entry title=%s; Width=%d\n", Entry->Title, MainImage->Width);
  if (GlobalConfig.TypeSVG) {
    Scale = 16 * (selected ? 1 : -1);
  } else {
    Scale = ((Entry->Row == 0) ? (Scale * (selected ? 1 : -1)): 16) ;
  }
  if (GlobalConfig.SelectionOnTop) {
    SelectionImages[0]->HasAlpha = TRUE;
    SelectionImages[2]->HasAlpha = TRUE;
    //MainImage->HasAlpha = TRUE;
      BltImageCompositeBadge(MainImage,
                             SelectionImages[((Entry->Row == 0) ? 0 : 2) + (selected ? 0 : 1)],
                             Entry->getBadgeImage(),
                             XPos, YPos, Scale);
  } else {
      BltImageCompositeBadge(SelectionImages[((Entry->Row == 0) ? 0 : 2) + (selected ? 0 : 1)],
                             MainImage,
                             Entry->getBadgeImage(),
                             XPos, YPos, Scale);
  }

  // draw BCS indicator
  // Needy: if Labels (Titles) are hidden there is no point to draw the indicator
  if (GlobalConfig.BootCampStyle && !(GlobalConfig.HideUIFlags & HIDEUI_FLAG_LABEL)) {
    SelectionImages[4]->HasAlpha = TRUE;

    // inidcator is for row 0, main entries, only
    if (Entry->Row == 0) {
      BltImageAlpha(SelectionImages[4 + (selected ? 0 : 1)],
                    XPos + (row0TileSize / 2) - (INTN)(INDICATOR_SIZE * 0.5f * GlobalConfig.Scale),
                    row0PosY + row0TileSize + TextHeight + (INTN)((BCSMargin * 2) * GlobalConfig.Scale),
                    &MenuBackgroundPixel, Scale);
    }
  }

  Entry->Place.XPos = XPos;
  Entry->Place.YPos = YPos;
  Entry->Place.Width = MainImage->Width;
  Entry->Place.Height = MainImage->Height;
}

VOID FillRectAreaOfScreen(IN INTN XPos, IN INTN YPos, IN INTN Width, IN INTN Height, IN EG_PIXEL *Color, IN UINT8 XAlign)
{
  EG_IMAGE *TmpBuffer = NULL;
  INTN X = XPos - (Width >> XAlign);

  if (!Width || !Height) return;

  TmpBuffer = egCreateImage(Width, Height, FALSE);
  if (!BackgroundImage) {
    egFillImage(TmpBuffer, Color);
  } else {
    egRawCopy(TmpBuffer->PixelData,
              BackgroundImage->PixelData + YPos * BackgroundImage->Width + X,
              Width, Height,
              TmpBuffer->Width,
              BackgroundImage->Width);
  }
  BltImage(TmpBuffer, X, YPos);
  egFreeImage(TmpBuffer);
}

VOID REFIT_MENU_SCREEN::DrawMainMenuLabel(IN CONST CHAR16 *Text, IN INTN XPos, IN INTN YPos)
{
  INTN TextWidth;
  INTN BadgeDim = (INTN)(BADGE_DIMENSION * GlobalConfig.Scale);

  egMeasureText(Text, &TextWidth, NULL);

  //Clear old text
  if (OldTextWidth > TextWidth) {
    FillRectAreaOfScreen(OldX, OldY, OldTextWidth, TextHeight, &MenuBackgroundPixel, X_IS_CENTER);
  }

  if (!(GlobalConfig.BootCampStyle)
      && (GlobalConfig.HideBadges & HDBADGES_INLINE) && (!OldRow)
      && (OldTextWidth) && (OldTextWidth != TextWidth)) {
    //Clear badge
    BltImageAlpha(NULL, (OldX - (OldTextWidth >> 1) - (BadgeDim + 16)),
                  (OldY - ((BadgeDim - TextHeight) >> 1)), &MenuBackgroundPixel, BadgeDim >> 3);
  }
  DrawTextXY(Text, XPos, YPos, X_IS_CENTER);

  //show inline badge
  if (!(GlobalConfig.BootCampStyle) &&
       (GlobalConfig.HideBadges & HDBADGES_INLINE) &&
       (Entries[ScrollState.CurrentSelection].Row == 0)) {
    // Display Inline Badge: small icon before the text
    BltImageAlpha(Entries[ScrollState.CurrentSelection].Image,
                  (XPos - (TextWidth >> 1) - (BadgeDim + 16)),
                  (YPos - ((BadgeDim - TextHeight) >> 1)), &MenuBackgroundPixel, BadgeDim >> 3);
  }

  OldX = XPos;
  OldY = YPos;
  OldTextWidth = TextWidth;
  OldRow = Entries[ScrollState.CurrentSelection].Row;
}

VOID REFIT_MENU_SCREEN::CountItems()
{
  INTN i;
  row0PosX = 0;
  row1PosX = Entries.size();
  // layout
  row0Count = 0; //Nr items in row0
  row1Count = 0;
  for (i = 0; i < (INTN)Entries.size(); i++) {
    if (Entries[i].Row == 0) {
      row0Count++;
      CONSTRAIN_MIN(row0PosX, i);
    } else {
      row1Count++;
      CONSTRAIN_MAX(row1PosX, i);
    }
  }
}

VOID DrawTextCorner(UINTN TextC, UINT8 Align)
{
  INTN    Xpos;
  CHAR16  *Text = NULL;

  if (
      // HIDEUI_ALL - included
      ((TextC == TEXT_CORNER_REVISION) && ((GlobalConfig.HideUIFlags & HIDEUI_FLAG_REVISION) != 0)) ||
      ((TextC == TEXT_CORNER_HELP) && ((GlobalConfig.HideUIFlags & HIDEUI_FLAG_HELP) != 0)) ||
      ((TextC == TEXT_CORNER_OPTIMUS) && (GlobalConfig.ShowOptimus == FALSE))
      ) {
    return;
  }

  switch (TextC) {
    case TEXT_CORNER_REVISION:
      // Display Clover boot volume
      if (SelfVolume->VolLabel && SelfVolume->VolLabel[0] != L'#') {
        Text = PoolPrint(L"%s, booted from %s", gFirmwareRevision, SelfVolume->VolLabel);
      }
      if ( !Text ) {
      	Text = PoolPrint(L"%s", gFirmwareRevision, SelfVolume->VolName);
      }
      break;
    case TEXT_CORNER_HELP:
      Text = PoolPrint(L"F1:Help");
      break;
    case TEXT_CORNER_OPTIMUS:
      if (gGraphics[0].Vendor != Intel) {
        Text = PoolPrint(L"Discrete");
      } else {
        Text = PoolPrint(L"Intel");
      }
//      Text = (NGFX == 2)?L"Intel":L"Discrete";
      break;
    default:
      return;
  }

  switch (Align) {
    case X_IS_LEFT:
      Xpos = (INTN)(TextHeight * 0.75f);
      break;
    case X_IS_RIGHT:
      Xpos = UGAWidth - (INTN)(TextHeight * 0.7f);//2
      break;
    case X_IS_CENTER:
      Xpos = UGAWidth >> 1;
      break;
    default:
    	if ( Text ) FreePool(Text);
      return;
  }
  //  DBG("draw text %s at (%d, %d)\n", Text, Xpos, UGAHeight - 5 - TextHeight),
// clovy  DrawTextXY(Text, Xpos, UGAHeight - 5 - TextHeight, Align);
  DrawTextXY(Text, Xpos, UGAHeight - (INTN)(TextHeight * 1.5f), Align);
  if ( Text ) FreePool(Text);
}

VOID REFIT_MENU_SCREEN::MainMenuVerticalStyle(IN UINTN Function, IN CONST CHAR16 *ParamText)
{
  INTN i;
  INTN row0PosYRunning;
  INTN VisibleHeight = 0; //assume vertical layout
  INTN MessageHeight = 20;
  if (GlobalConfig.TypeSVG && textFace[1].valid) {
    MessageHeight = (INTN)(textFace[1].size * RowHeightFromTextHeight * GlobalConfig.Scale);
  }
  else {
    MessageHeight = (INTN)(TextHeight * RowHeightFromTextHeight * GlobalConfig.Scale);
  }

  switch (Function) {

    case MENU_FUNCTION_INIT:
      egGetScreenSize(&UGAWidth, &UGAHeight);
      InitAnime();
			SwitchToGraphicsAndClear();
			//BltClearScreen(FALSE);
      //adjustable by theme.plist?
      EntriesPosY = (int)(LAYOUT_Y_EDGE * GlobalConfig.Scale);
      EntriesGap = (int)(GlobalConfig.TileYSpace * GlobalConfig.Scale);
      EntriesWidth = GlobalConfig.MainEntriesSize + (int)(16 * GlobalConfig.Scale);
      EntriesHeight = GlobalConfig.MainEntriesSize + (int)(16 * GlobalConfig.Scale);
      //
      VisibleHeight = (UGAHeight - EntriesPosY - (int)(LAYOUT_Y_EDGE * GlobalConfig.Scale) + EntriesGap) / (EntriesHeight + EntriesGap);
      EntriesPosX = UGAWidth - EntriesWidth - (int)((BAR_WIDTH + LAYOUT_X_EDGE) * GlobalConfig.Scale);
      TimeoutPosY = UGAHeight - (int)(LAYOUT_Y_EDGE * GlobalConfig.Scale) - MessageHeight * 2; //optimus + timeout texts

      CountItems();
      InitScroll(row0Count, Entries.size(), VisibleHeight, 0);
      row0PosX = EntriesPosX;
      row0PosY = EntriesPosY;
      row1PosX = (UGAWidth + EntriesGap - (row1TileSize + (int)(TILE1_XSPACING * GlobalConfig.Scale)) * row1Count) >> 1;
      textPosY = TimeoutPosY - (int)(GlobalConfig.TileYSpace * GlobalConfig.Scale) - MessageHeight; //message text
      row1PosY = textPosY - row1TileSize - (int)(GlobalConfig.TileYSpace * GlobalConfig.Scale) - LayoutTextOffset;
      if (!itemPosX) {
        itemPosX = (__typeof__(itemPosX))AllocatePool(sizeof(UINT64) * Entries.size());
        itemPosY = (__typeof__(itemPosY))AllocatePool(sizeof(UINT64) * Entries.size());
      }
      row0PosYRunning = row0PosY;
      row1PosXRunning = row1PosX;

      //     DBG("EntryCount =%d\n", Entries.size());
      for (i = 0; i < (INTN)Entries.size(); i++) {
        if (Entries[i].Row == 0) {
          itemPosX[i] = row0PosX;
          itemPosY[i] = row0PosYRunning;
          row0PosYRunning += EntriesHeight + EntriesGap;
        } else {
          itemPosX[i] = row1PosXRunning;
          itemPosY[i] = row1PosY;
          row1PosXRunning += row1TileSize + (int)(TILE1_XSPACING* GlobalConfig.Scale);
          //         DBG("next item in row1 at x=%d\n", row1PosXRunning);
        }
      }
      // initial painting
      InitSelection();

      // Update FilmPlace only if not set by InitAnime
      if (FilmPlace.Width == 0 || FilmPlace.Height == 0) {
        CopyMem(&FilmPlace, &BannerPlace, sizeof(BannerPlace));
      }

      InitBar();
      break;

    case MENU_FUNCTION_CLEANUP:
      FreePool(itemPosX);
      itemPosX = NULL;
      FreePool(itemPosY);
      itemPosY = NULL;
      HidePointer();
      break;

    case MENU_FUNCTION_PAINT_ALL:
      SetBar(EntriesPosX + EntriesWidth + (int)(10 * GlobalConfig.Scale),
             EntriesPosY, UGAHeight - (int)(LAYOUT_Y_EDGE * GlobalConfig.Scale), ScrollState);
      for (i = 0; i <= ScrollState.MaxIndex; i++) {
        if (Entries[i].Row == 0) {
          if ((i >= ScrollState.FirstVisible) && (i <= ScrollState.LastVisible)) {
            DrawMainMenuEntry(&Entries[i], (i == ScrollState.CurrentSelection)?1:0,
                              itemPosX[i - ScrollState.FirstVisible], itemPosY[i - ScrollState.FirstVisible]);
          }
        } else { //row1
          DrawMainMenuEntry(&Entries[i], (i == ScrollState.CurrentSelection)?1:0,
                            itemPosX[i], itemPosY[i]);
        }
      }
      if (!(GlobalConfig.HideUIFlags & HIDEUI_FLAG_LABEL)){
        DrawMainMenuLabel(Entries[ScrollState.CurrentSelection].Title,
                          (UGAWidth >> 1), textPosY);
      }

      ScrollingBar(&ScrollState);
      DrawTextCorner(TEXT_CORNER_REVISION, X_IS_LEFT);
      DrawTextCorner(TEXT_CORNER_OPTIMUS, X_IS_CENTER);
      MouseBirth();
      break;

    case MENU_FUNCTION_PAINT_SELECTION:
      HidePointer();
      if (Entries[ScrollState.LastSelection].Row == 0) {
        DrawMainMenuEntry(&Entries[ScrollState.LastSelection], FALSE,
                          itemPosX[ScrollState.LastSelection - ScrollState.FirstVisible],
                          itemPosY[ScrollState.LastSelection - ScrollState.FirstVisible]);
      } else {
        DrawMainMenuEntry(&Entries[ScrollState.LastSelection], FALSE,
                          itemPosX[ScrollState.LastSelection],
                          itemPosY[ScrollState.LastSelection]);
      }

      if (Entries[ScrollState.CurrentSelection].Row == 0) {
        DrawMainMenuEntry(&Entries[ScrollState.CurrentSelection], TRUE,
                          itemPosX[ScrollState.CurrentSelection - ScrollState.FirstVisible],
                          itemPosY[ScrollState.CurrentSelection - ScrollState.FirstVisible]);
      } else {
        DrawMainMenuEntry(&Entries[ScrollState.CurrentSelection], TRUE,
                          itemPosX[ScrollState.CurrentSelection],
                          itemPosY[ScrollState.CurrentSelection]);
      }
      if (!(GlobalConfig.HideUIFlags & HIDEUI_FLAG_LABEL)) {
        DrawMainMenuLabel(Entries[ScrollState.CurrentSelection].Title,
                          (UGAWidth >> 1), textPosY);
      }

      ScrollingBar(&ScrollState);
      DrawTextCorner(TEXT_CORNER_REVISION, X_IS_LEFT);
      DrawTextCorner(TEXT_CORNER_OPTIMUS, X_IS_CENTER);
      MouseBirth();
      break;

    case MENU_FUNCTION_PAINT_TIMEOUT:
      i = (GlobalConfig.HideBadges & HDBADGES_INLINE)?3:1;
      HidePointer();
      if (!(GlobalConfig.HideUIFlags & HIDEUI_FLAG_LABEL)){
        FillRectAreaOfScreen((UGAWidth >> 1), textPosY + MessageHeight * i,
                             OldTimeoutTextWidth, TextHeight, &MenuBackgroundPixel, X_IS_CENTER);
        OldTimeoutTextWidth = DrawTextXY(ParamText, (UGAWidth >> 1), textPosY + MessageHeight * i, X_IS_CENTER);
      }

      DrawTextCorner(TEXT_CORNER_REVISION, X_IS_LEFT);
      break;
  }
}

/**
 * Main screen text.
 */
VOID REFIT_MENU_SCREEN::MainMenuStyle(IN UINTN Function, IN CONST CHAR16 *ParamText)
{
  EFI_STATUS Status = EFI_SUCCESS;
  INTN i = 0;
  INTN MessageHeight = 0;
// clovy
	if (GlobalConfig.TypeSVG && textFace[1].valid) {
		MessageHeight = (INTN)(textFace[1].size * RowHeightFromTextHeight * GlobalConfig.Scale);
	} else {
		MessageHeight = (INTN)(TextHeight * RowHeightFromTextHeight * GlobalConfig.Scale);
	}

  switch (Function) {

    case MENU_FUNCTION_INIT:
      egGetScreenSize(&UGAWidth, &UGAHeight);
      InitAnime();
			SwitchToGraphicsAndClear();
			//BltClearScreen(FALSE);

      EntriesGap = (int)(GlobalConfig.TileXSpace * GlobalConfig.Scale);
      EntriesWidth = row0TileSize;
      EntriesHeight = GlobalConfig.MainEntriesSize + (int)(16.f * GlobalConfig.Scale);

      MaxItemOnScreen = (UGAWidth - (int)((ROW0_SCROLLSIZE * 2)* GlobalConfig.Scale)) / (EntriesWidth + EntriesGap); //8
      CountItems();
      InitScroll(row0Count, Entries.size(), MaxItemOnScreen, 0);

      row0PosX = EntriesWidth + EntriesGap;
      row0PosX = row0PosX * ((MaxItemOnScreen < row0Count)?MaxItemOnScreen:row0Count);
      row0PosX = row0PosX - EntriesGap;
      row0PosX = UGAWidth - row0PosX;
      row0PosX = row0PosX >> 1;

      row0PosY = (int)(((float)UGAHeight - LayoutMainMenuHeight * GlobalConfig.Scale) * 0.5f +
                  LayoutBannerOffset * GlobalConfig.Scale);

      row1PosX = (UGAWidth + 8 - (row1TileSize + (INTN)(8.0f * GlobalConfig.Scale)) * row1Count) >> 1;

      if (GlobalConfig.BootCampStyle && !(GlobalConfig.HideUIFlags & HIDEUI_FLAG_LABEL)) {
        row1PosY = row0PosY + row0TileSize + (INTN)((BCSMargin * 2) * GlobalConfig.Scale) + TextHeight +
            (INTN)(INDICATOR_SIZE * GlobalConfig.Scale) +
            (INTN)((LayoutButtonOffset + GlobalConfig.TileYSpace) * GlobalConfig.Scale);
      } else {
        row1PosY = row0PosY + EntriesHeight +
            (INTN)((GlobalConfig.TileYSpace + LayoutButtonOffset) * GlobalConfig.Scale);
      }

      if (row1Count > 0) {
          textPosY = row1PosY + MAX(row1TileSize, MessageHeight) + (INTN)((GlobalConfig.TileYSpace + LayoutTextOffset) * GlobalConfig.Scale);
        } else {
          textPosY = row1PosY;
        }

      if (GlobalConfig.BootCampStyle) {
        textPosY = row0PosY + row0TileSize + (INTN)((TEXT_YMARGIN + BCSMargin) * GlobalConfig.Scale);
      }

      FunctextPosY = row1PosY + row1TileSize + (INTN)((GlobalConfig.TileYSpace + LayoutTextOffset) * GlobalConfig.Scale);
      
      if (!itemPosX) {
        itemPosX = (__typeof__(itemPosX))AllocatePool(sizeof(UINT64) * Entries.size());
      }

      row0PosXRunning = row0PosX;
      row1PosXRunning = row1PosX;
      //DBG("EntryCount =%d\n", Entries.size());
      for (i = 0; i < (INTN)Entries.size(); i++) {
        if (Entries[i].Row == 0) {
          itemPosX[i] = row0PosXRunning;
          row0PosXRunning += EntriesWidth + EntriesGap;
        } else {
          itemPosX[i] = row1PosXRunning;
          row1PosXRunning += row1TileSize + (INTN)(TILE1_XSPACING * GlobalConfig.Scale);
          //DBG("next item in row1 at x=%d\n", row1PosXRunning);
        }
      }
      // initial painting
      InitSelection();

      // Update FilmPlace only if not set by InitAnime
      if (FilmPlace.Width == 0 || FilmPlace.Height == 0) {
        CopyMem(&FilmPlace, &BannerPlace, sizeof(BannerPlace));
      }

      //DBG("main menu inited\n");
      break;

    case MENU_FUNCTION_CLEANUP:
      FreePool(itemPosX);
      itemPosX = NULL;
      HidePointer();
      break;

    case MENU_FUNCTION_PAINT_ALL:
    
      for (i = 0; i <= ScrollState.MaxIndex; i++) {
        if (Entries[i].Row == 0) {
          if ((i >= ScrollState.FirstVisible) && (i <= ScrollState.LastVisible)) {
            DrawMainMenuEntry(&Entries[i], (i == ScrollState.CurrentSelection)?1:0,
                              itemPosX[i - ScrollState.FirstVisible], row0PosY);
            // draw static text for the boot options, BootCampStyle
            if (GlobalConfig.BootCampStyle && !(GlobalConfig.HideUIFlags & HIDEUI_FLAG_LABEL)) {
              INTN textPosX = itemPosX[i - ScrollState.FirstVisible] + (row0TileSize / 2);
              // clear the screen
              FillRectAreaOfScreen(textPosX, textPosY, EntriesWidth + GlobalConfig.TileXSpace,
                                   MessageHeight, &MenuBackgroundPixel, X_IS_CENTER);
              // draw the text
              DrawBCSText(Entries[i].Title, textPosX, textPosY, X_IS_CENTER);
            }
          }
        } else {
          DrawMainMenuEntry(&Entries[i], (i == ScrollState.CurrentSelection)?1:0,
                            itemPosX[i], row1PosY);
        }
      }

      // clear the text from the second row, required by the BootCampStyle
      if ((GlobalConfig.BootCampStyle) && (Entries[ScrollState.LastSelection].Row == 1)
          && (Entries[ScrollState.CurrentSelection].Row == 0) && !(GlobalConfig.HideUIFlags & HIDEUI_FLAG_LABEL)) {
          FillRectAreaOfScreen((UGAWidth >> 1), FunctextPosY,
// clovy                               OldTextWidth, TextHeight, &MenuBackgroundPixel, X_IS_CENTER);
                               OldTextWidth, MessageHeight, &MenuBackgroundPixel, X_IS_CENTER);
      }

      // something is wrong with the DrawMainMenuLabel or Entries[ScrollState.CurrentSelection]
      // and it's required to create the first selection text from here
      // used for the second row entries, when BootCampStyle is used
      if ((Entries[ScrollState.LastSelection].Row == 0) && (Entries[ScrollState.CurrentSelection].Row == 1)
          && GlobalConfig.BootCampStyle && !(GlobalConfig.HideUIFlags & HIDEUI_FLAG_LABEL)) {
          DrawMainMenuLabel(Entries[ScrollState.CurrentSelection].Title,
                            (UGAWidth >> 1), FunctextPosY);
      }

      // something is wrong with the DrawMainMenuLabel or Entries[ScrollState.CurrentSelection]
      // and it's required to create the first selection text from here
      // used for all the entries
      if (!(GlobalConfig.BootCampStyle) && !(GlobalConfig.HideUIFlags & HIDEUI_FLAG_LABEL)) {
        DrawMainMenuLabel(Entries[ScrollState.CurrentSelection].Title,
                            (UGAWidth >> 1), textPosY);
      }

      DrawTextCorner(TEXT_CORNER_HELP, X_IS_LEFT);
      DrawTextCorner(TEXT_CORNER_OPTIMUS, X_IS_CENTER);
      DrawTextCorner(TEXT_CORNER_REVISION, X_IS_RIGHT);
      Status = MouseBirth();
      if(EFI_ERROR(Status)) {
        DBG("can't bear mouse at all! Status=%r\n", Status);
      }
      break;

    case MENU_FUNCTION_PAINT_SELECTION:
      HidePointer();
      if (Entries[ScrollState.LastSelection].Row == 0) {
        DrawMainMenuEntry(&Entries[ScrollState.LastSelection], FALSE,
                      itemPosX[ScrollState.LastSelection - ScrollState.FirstVisible], row0PosY);
      } else {
        DrawMainMenuEntry(&Entries[ScrollState.LastSelection], FALSE,
                          itemPosX[ScrollState.LastSelection], row1PosY);
      }

      if (Entries[ScrollState.CurrentSelection].Row == 0) {
        DrawMainMenuEntry(&Entries[ScrollState.CurrentSelection], TRUE,
                      itemPosX[ScrollState.CurrentSelection - ScrollState.FirstVisible], row0PosY);
      } else {
        DrawMainMenuEntry(&Entries[ScrollState.CurrentSelection], TRUE,
                          itemPosX[ScrollState.CurrentSelection], row1PosY);
      }

      // create dynamic text for the second row if BootCampStyle is used
      if ((GlobalConfig.BootCampStyle) && (!(GlobalConfig.HideUIFlags & HIDEUI_FLAG_LABEL))
          && Entries[ScrollState.CurrentSelection].Row == 1) {
          DrawMainMenuLabel(Entries[ScrollState.CurrentSelection].Title,
                            (UGAWidth >> 1), FunctextPosY);
      }

      // create dynamic text for all the entries
      if ((!(GlobalConfig.BootCampStyle)) && (!(GlobalConfig.HideUIFlags & HIDEUI_FLAG_LABEL))) {
          DrawMainMenuLabel(Entries[ScrollState.CurrentSelection].Title,
                            (UGAWidth >> 1), textPosY);
      }

      DrawTextCorner(TEXT_CORNER_HELP, X_IS_LEFT);
      DrawTextCorner(TEXT_CORNER_OPTIMUS, X_IS_CENTER);
      DrawTextCorner(TEXT_CORNER_REVISION, X_IS_RIGHT);
      Status = MouseBirth();
      if(EFI_ERROR(Status)) {
        DBG("can't bear mouse at sel! Status=%r\n", Status);
      }
      break;

    case MENU_FUNCTION_PAINT_TIMEOUT:
      i = (GlobalConfig.HideBadges & HDBADGES_INLINE)?3:1;
      HidePointer();
      if (!(GlobalConfig.HideUIFlags & HIDEUI_FLAG_LABEL)){
        FillRectAreaOfScreen((UGAWidth >> 1), FunctextPosY + MessageHeight * i,
                           OldTimeoutTextWidth, MessageHeight, &MenuBackgroundPixel, X_IS_CENTER);
        OldTimeoutTextWidth = DrawTextXY(ParamText, (UGAWidth >> 1), FunctextPosY + MessageHeight * i, X_IS_CENTER);
      }

      DrawTextCorner(TEXT_CORNER_HELP, X_IS_LEFT);
      DrawTextCorner(TEXT_CORNER_OPTIMUS, X_IS_CENTER);
      DrawTextCorner(TEXT_CORNER_REVISION, X_IS_RIGHT);
      Status = MouseBirth();
      if(EFI_ERROR(Status)) {
        DBG("can't bear mouse at timeout! Status=%r\n", Status);
      }
      break;

  }
}

//
// user-callable dispatcher functions
//

UINTN REFIT_MENU_SCREEN::RunMenu(OUT REFIT_ABSTRACT_MENU_ENTRY **ChosenEntry)
{
  INTN Index = -1;

  if (AllowGraphicsMode)
    return RunGenericMenu(GraphicsMenuStyle, &Index, ChosenEntry);
  else
    return RunGenericMenu(TextMenuStyle, &Index, ChosenEntry);
}

VOID NewEntry_(REFIT_ABSTRACT_MENU_ENTRY *Entry, REFIT_MENU_SCREEN **SubScreen, ACTION AtClick, UINTN ID, CONST CHAR8 *Title)
{
  if (Title) {
    Entry->Title = PoolPrint(L"%a", Title);
  } else {
    Entry->Title = (__typeof__(Entry->Title))AllocateZeroPool(128);
  }

  Entry->Image =  OptionMenu.TitleImage;
  Entry->AtClick = AtClick;
  // create the submenu
  *SubScreen = (__typeof_am__(*SubScreen))AllocateZeroPool(sizeof(**SubScreen));
  (*SubScreen)->Title = Entry->Title;
  (*SubScreen)->TitleImage = Entry->Image;
  (*SubScreen)->ID = ID;
  (*SubScreen)->AnimeRun = GetAnime(*SubScreen);
  Entry->SubScreen = *SubScreen;
}

VOID NewEntry(REFIT_MENU_ENTRY_OTHER **Entry, REFIT_MENU_SCREEN **SubScreen, ACTION AtClick, UINTN ID, CONST CHAR8 *Title)
{
  //create entry
//  *Entry = (__typeof_am__(*Entry))AllocateZeroPool(sizeof(LOADER_ENTRY)); // carefull, **Entry is not a LOADER_ENTRY. Don't use sizeof.
  *Entry = new REFIT_MENU_ITEM_OPTIONS();
  NewEntry_(*Entry, SubScreen, AtClick, ID, Title); // cast ok because super class
//  (*Entry)->Tag = TAG_OPTIONS;
}

VOID NewEntry(LOADER_ENTRY **Entry, REFIT_MENU_SCREEN **SubScreen, ACTION AtClick, UINTN ID, CONST CHAR8 *Title)
{
  //create entry
  *Entry = (__typeof_am__(*Entry))AllocateZeroPool(sizeof(LOADER_ENTRY)); // carefull, **Entry is not a LOADER_ENTRY. Don't use sizeof.
//  *Entry = new LOADER_ENTRY();
  NewEntry_(*Entry, SubScreen, AtClick, ID, Title); // cast ok because super class
}

VOID AddMenuCheck(REFIT_MENU_SCREEN *SubScreen, CONST CHAR8 *Text, UINTN Bit, INTN ItemNum)
{
  REFIT_MENU_CHECKBIT *InputBootArgs;

//  InputBootArgs = (__typeof__(InputBootArgs))AllocateZeroPool(sizeof(REFIT_INPUT_DIALOG));
  InputBootArgs = new REFIT_MENU_CHECKBIT;
  InputBootArgs->Title = PoolPrint(L"%a", Text);
//  InputBootArgs->Tag = TAG_CHECKBIT_OLD;
  InputBootArgs->Row = Bit;
  InputBootArgs->Item = &InputItems[ItemNum];
  InputBootArgs->AtClick = ActionEnter;
  InputBootArgs->AtRightClick = ActionDetails;
  AddMenuEntry(SubScreen, InputBootArgs, true);
}

VOID ModifyTitles(REFIT_ABSTRACT_MENU_ENTRY *ChosenEntry)
{
  if (ChosenEntry->SubScreen->ID == SCREEN_DSDT) {
    UnicodeSPrint((CHAR16*)ChosenEntry->Title, 128, L"DSDT fix mask [0x%08x]->", gSettings.FixDsdt); // TODO jief : cast to fix
    //MsgLog("@ESC: %s\n", (*ChosenEntry)->Title);
  } else if (ChosenEntry->SubScreen->ID == SCREEN_CSR) {
    // CSR
    UnicodeSPrint((CHAR16*)ChosenEntry->Title, 128, L"System Integrity Protection [0x%04x]->", gSettings.CsrActiveConfig); // TODO jief : cast to fix
    // check for the right booter flag to allow the application
    // of the new System Integrity Protection configuration.
    if (gSettings.CsrActiveConfig != 0 && gSettings.BooterConfig == 0) {
      gSettings.BooterConfig = 0x28;
    }

  } else if (ChosenEntry->SubScreen->ID == SCREEN_BLC) {
    UnicodeSPrint((CHAR16*)ChosenEntry->Title, 128, L"boot_args->flags [0x%04x]->", gSettings.BooterConfig); // TODO jief : cast to fix
  } else if (ChosenEntry->SubScreen->ID == SCREEN_DSM) {
    UnicodeSPrint((CHAR16*)ChosenEntry->Title, 128, L"Drop OEM _DSM [0x%04x]->", dropDSM); // TODO jief : cast to fix
  }
}

VOID AddMenuItem_(REFIT_MENU_SCREEN  *SubScreen, REFIT_MENU_ITEM_IEM_ABSTRACT* InputBootArgs, INTN Inx, CONST CHAR8 *Title, BOOLEAN Cursor)
{
  InputBootArgs->Title          = PoolPrint(L"%a", Title);
  if (Inx == 3 || Inx == 116) {
    InputBootArgs->Row          = 0;
  } else {
    InputBootArgs->Row          = Cursor?StrLen(InputItems[Inx].SValue):0xFFFF;
  }
  InputBootArgs->Item                 = &InputItems[Inx];
  InputBootArgs->AtClick        = Cursor?ActionSelect:ActionEnter;
  InputBootArgs->AtRightClick   = Cursor?ActionNone:ActionDetails;
  InputBootArgs->AtDoubleClick  = Cursor?ActionEnter:ActionNone;
  AddMenuEntry(SubScreen, InputBootArgs, true);
}
//
//VOID AddMenuItem(REFIT_MENU_SCREEN  *SubScreen, INTN Inx, CONST CHAR8 *Title, UINTN Tag, BOOLEAN Cursor)
//{
////  REFIT_INPUT_DIALOG *InputBootArgs = (__typeof__(InputBootArgs))AllocateZeroPool(sizeof(REFIT_INPUT_DIALOG));
//  REFIT_INPUT_DIALOG *InputBootArgs = new REFIT_INPUT_DIALOG;
//  AddMenuItem_(SubScreen, InputBootArgs, Inx, Title, Tag, Cursor);
//}

VOID AddMenuItemInput(REFIT_MENU_SCREEN  *SubScreen, INTN Inx, CONST CHAR8 *Title, BOOLEAN Cursor)
{
//  REFIT_INPUT_DIALOG *InputBootArgs = (__typeof__(InputBootArgs))AllocateZeroPool(sizeof(REFIT_INPUT_DIALOG));
  REFIT_INPUT_DIALOG *InputBootArgs = new REFIT_INPUT_DIALOG;
  AddMenuItem_(SubScreen, InputBootArgs, Inx, Title, Cursor);
}

VOID AddMenuItemSwitch(REFIT_MENU_SCREEN  *SubScreen, INTN Inx, CONST CHAR8 *Title, BOOLEAN Cursor)
{
  REFIT_MENU_SWITCH *InputBootArgs = new REFIT_MENU_SWITCH;
  AddMenuItem_(SubScreen, InputBootArgs, Inx, Title, Cursor);
}

REFIT_MENU_ENTRY  *SubMenuGraphics()
{
  UINTN  i, N, Ven = 97;
  REFIT_MENU_ENTRY_OTHER   *Entry; //, *SubEntry;
  REFIT_MENU_SCREEN  *SubScreen;

  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_GRAPHICS, "Graphics Injector->");
  AddMenuInfoLine(SubScreen, PoolPrint(L"Number of VideoCard%a=%d",((NGFX!=1)?"s":""), NGFX));

  AddMenuItemInput(SubScreen, 52, "InjectEDID", FALSE);
  AddMenuItemInput(SubScreen, 53, "Fake Vendor EDID:", TRUE);
  AddMenuItemInput(SubScreen, 54, "Fake Product EDID:", TRUE);
  AddMenuItemInput(SubScreen, 18, "Backlight Level:", TRUE);
  AddMenuItemInput(SubScreen, 112, "Intel Max Backlight:", TRUE); //gSettings.IntelMaxValue


  for (i = 0; i < NGFX; i++) {
    AddMenuInfo(SubScreen, L"----------------------");
    AddMenuInfo(SubScreen, PoolPrint(L"Card DeviceID=%04x", gGraphics[i].DeviceID));
    N = 20 + i * 6;
    AddMenuItemInput(SubScreen, N, "Model:", TRUE);

    if (gGraphics[i].Vendor == Nvidia) {
      AddMenuItemInput(SubScreen, N+1, "InjectNVidia", FALSE);
    } else if (gGraphics[i].Vendor == Ati) {
      AddMenuItemInput(SubScreen, N+1, "InjectATI", FALSE);
    } else if (gGraphics[i].Vendor == Intel) {
      AddMenuItemInput(SubScreen, N+1, "InjectIntel", FALSE);
    } else {
      AddMenuItemInput(SubScreen, N+1, "InjectX3", FALSE);
    }

    if (gGraphics[i].Vendor == Nvidia) {
      Ven = 95;
    } else if (gGraphics[i].Vendor == Ati) {
      Ven = 94;
    } else /*if (gGraphics[i].Vendor == Intel)*/ {
      Ven = 96;
    }

    if ((gGraphics[i].Vendor == Ati) || (gGraphics[i].Vendor == Intel)) {
      AddMenuItemInput(SubScreen, 109, "DualLink:", TRUE);
    }
    if (gGraphics[i].Vendor == Ati) {
      AddMenuItemInput(SubScreen, 114, "DeInit:", TRUE);
    }

    AddMenuItemInput(SubScreen, Ven, "FakeID:", TRUE);

    if (gGraphics[i].Vendor == Nvidia) {
      AddMenuItemInput(SubScreen, N+2, "DisplayCFG:", TRUE);
    } else if (gGraphics[i].Vendor == Ati) {
      AddMenuItemInput(SubScreen, N+2, "FBConfig:", TRUE);
    } else /*if (gGraphics[i].Vendor == Intel)*/{
      AddMenuItemInput(SubScreen, N+2, "*-platform-id:", TRUE);
    }

    // ErmaC: NvidiaGeneric entry
    if (gGraphics[i].Vendor == Nvidia) {
      AddMenuItemInput(SubScreen, 55, "Generic NVIDIA name", FALSE);
      AddMenuItemInput(SubScreen, 110, "NVIDIA No EFI", FALSE);
      AddMenuItemInput(SubScreen, 111, "NVIDIA Single", FALSE);
      AddMenuItemInput(SubScreen, 56, "Use NVIDIA WEB drivers", FALSE);
    }

    if (gGraphics[i].Vendor == Intel) {
      continue;
    }
    AddMenuItemInput(SubScreen, N+3, "Ports:", TRUE);

    if (gGraphics[i].Vendor == Nvidia) {
      AddMenuItemInput(SubScreen, N+4, "NVCAP:", TRUE);
    } else {
      AddMenuItemInput(SubScreen, N+4, "Connectors:", TRUE);
      AddMenuItemInput(SubScreen, 50, "RefCLK:", TRUE);
    }
    AddMenuItemInput(SubScreen, N+5, "Load Video Bios", FALSE);
  }

  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}

// ErmaC: Audio submenu
REFIT_MENU_ENTRY  *SubMenuAudio()
{

  UINTN  i;

  // init
  REFIT_MENU_ENTRY_OTHER   *Entry;
  REFIT_MENU_SCREEN  *SubScreen;

  // create the entry in the main menu
  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_AUDIO, "Audio tuning->");

  // submenu description
  AddMenuInfoLine(SubScreen, PoolPrint(L"Choose options to tune the HDA devices"));
  AddMenuInfoLine(SubScreen, PoolPrint(L"Number of Audio Controller%a=%d", ((NHDA!=1)?"s":""), NHDA));
  for (i = 0; i < NHDA; i++) {
      AddMenuInfoLine(SubScreen, PoolPrint(L"%d) %s [%04x][%04x]",
                                           (i+1),
                                           gAudios[i].controller_name,
                                           gAudios[i].controller_vendor_id,
                                           gAudios[i].controller_device_id)
                      );
  }

  //AddMenuItemInput(SubScreen, 59, "HDAInjection", FALSE);
  if (gSettings.HDAInjection) {
    AddMenuItemInput(SubScreen, 60, "HDALayoutId:", TRUE);
  }

  // avaiable configuration
  AddMenuItemInput(SubScreen, 57, "ResetHDA", FALSE);
  AddMenuItemInput(SubScreen, 58, "AFGLowPowerState", FALSE);

  // return
  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}

#define nya(x) x/10,x%10

REFIT_MENU_ENTRY  *SubMenuSpeedStep()
{
  REFIT_MENU_ENTRY_OTHER   *Entry; //, *SubEntry;
  REFIT_MENU_SCREEN  *SubScreen;

  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_CPU, "CPU tuning->");
  AddMenuInfoLine(SubScreen, PoolPrint(L"%a", gCPUStructure.BrandString));
  AddMenuInfoLine(SubScreen, PoolPrint(L"Model: %2x/%2x/%2x",
      gCPUStructure.Family, gCPUStructure.Model, gCPUStructure.Stepping));
  AddMenuInfoLine(SubScreen, PoolPrint(L"Cores: %d Threads: %d",
                  gCPUStructure.Cores, gCPUStructure.Threads));
  AddMenuInfoLine(SubScreen, PoolPrint(L"FSB speed MHz: %d",
                  DivU64x32(gCPUStructure.FSBFrequency, Mega)));
  AddMenuInfoLine(SubScreen, PoolPrint(L"CPU speed MHz: %d",
                  DivU64x32(gCPUStructure.CPUFrequency, Mega)));
  AddMenuInfoLine(SubScreen, PoolPrint(L"Ratio: Min=%d.%d Max=%d.%d Turbo=%d.%d/%d.%d/%d.%d/%d.%d",
     nya(gCPUStructure.MinRatio), nya(gCPUStructure.MaxRatio),
     nya(gCPUStructure.Turbo4), nya(gCPUStructure.Turbo3), nya(gCPUStructure.Turbo2), nya(gCPUStructure.Turbo1)));

  AddMenuItemInput(SubScreen, 76, "Cores enabled:", TRUE);
  AddMenuItemInput(SubScreen, 6,  "Halt Enabler", FALSE);
  AddMenuItemInput(SubScreen, 7,  "PLimitDict:", TRUE);
  AddMenuItemInput(SubScreen, 8,  "UnderVoltStep:", TRUE);
  AddMenuItemInput(SubScreen, 88, "DoubleFirstState", FALSE);
  AddMenuItemInput(SubScreen, 5,  "GeneratePStates", FALSE);
  AddMenuItemInput(SubScreen, 9,  "GenerateCStates", FALSE);
  AddMenuItemInput(SubScreen, 10, "EnableC2", FALSE);
  AddMenuItemInput(SubScreen, 11, "EnableC4", FALSE);
  AddMenuItemInput(SubScreen, 12, "EnableC6", FALSE);
  AddMenuItemInput(SubScreen, 89, "EnableC7", FALSE);
  AddMenuItemInput(SubScreen, 13, "Use SystemIO", FALSE);
  AddMenuItemInput(SubScreen, 75, "C3Latency:", TRUE);
  AddMenuItemInput(SubScreen, 19, "BusSpeed [kHz]:", TRUE);
  AddMenuItemInput(SubScreen, 14, "QPI [MHz]:", TRUE);
  AddMenuItemInput(SubScreen, 77, "Saving Mode:", TRUE);
  AddMenuItemInput(SubScreen, 15, "PatchAPIC", FALSE);  //-> move to ACPI?

  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}

REFIT_MENU_ENTRY  *SubMenuKextPatches()
{
  REFIT_MENU_ENTRY_OTHER     *Entry;
  REFIT_MENU_SCREEN    *SubScreen;
  REFIT_INPUT_DIALOG   *InputBootArgs;
  INTN                 NrKexts = gSettings.KernelAndKextPatches.NrKexts;
  KEXT_PATCH  *KextPatchesMenu = gSettings.KernelAndKextPatches.KextPatches; //zzzz
  INTN                 Index;

  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_KEXTS, "Custom kexts patches->");

  for (Index = 0; Index < NrKexts; Index++) {
//    InputBootArgs = (__typeof__(InputBootArgs))AllocateZeroPool(sizeof(REFIT_INPUT_DIALOG));
    InputBootArgs = new REFIT_INPUT_DIALOG;
    InputBootArgs->Title = PoolPrint(L"%30a", KextPatchesMenu[Index].Label);
//    InputBootArgs->Tag = TAG_INPUT;
    InputBootArgs->Row = 0xFFFF; //cursor
    InputBootArgs->Item = &(KextPatchesMenu[Index].MenuItem);
    InputBootArgs->AtClick = ActionEnter;
    InputBootArgs->AtRightClick = ActionDetails;
    AddMenuEntry(SubScreen, InputBootArgs, true);
  }

  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}

REFIT_MENU_ENTRY  *SubMenuKextBlockInjection(CONST CHAR16* UniSysVer)
{
  REFIT_MENU_ENTRY_OTHER     *Entry = NULL;
  REFIT_MENU_SCREEN    *SubScreen = NULL;
  REFIT_INPUT_DIALOG   *InputBootArgs;
  UINTN i = 0;
  SIDELOAD_KEXT        *Kext = NULL;
  CHAR8                sysVer[256];

  UnicodeStrToAsciiStrS(UniSysVer, sysVer, sizeof(sysVer));
  for (i = 0; i < sizeof(sysVer)-2; i++) {
    if (sysVer[i] == '\0') {
      sysVer[i+0] = '-';
      sysVer[i+1] = '>';
      sysVer[i+2] = '\0';
      break;
    }
  }

  Kext = InjectKextList;
  while (Kext) {
    if (StrCmp(Kext->KextDirNameUnderOEMPath, UniSysVer) == 0) {
    	if ( SubScreen == NULL ) {
    		NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_KEXT_INJECT, sysVer);
    		AddMenuInfoLine(SubScreen, PoolPrint(L"Choose/check kext to disable:"));
    	}
//      InputBootArgs = (__typeof__(InputBootArgs))AllocateZeroPool(sizeof(REFIT_INPUT_DIALOG));
      InputBootArgs = new REFIT_INPUT_DIALOG;
      InputBootArgs->Title = PoolPrint(L"%s, v.%s", Kext->FileName, Kext->Version);
//      InputBootArgs->Tag = TAG_INPUT;
      InputBootArgs->Row = 0xFFFF; //cursor
      InputBootArgs->Item = &(Kext->MenuItem);
      InputBootArgs->AtClick = ActionEnter;
      InputBootArgs->AtRightClick = ActionDetails;
      AddMenuEntry(SubScreen, InputBootArgs, true);

      SIDELOAD_KEXT *plugInKext = Kext->PlugInList;
      while (plugInKext) {
//        InputBootArgs = (__typeof__(InputBootArgs))AllocateZeroPool(sizeof(REFIT_INPUT_DIALOG));
        InputBootArgs = new REFIT_INPUT_DIALOG;
        InputBootArgs->Title = PoolPrint(L"  |-- %s, v.%s", plugInKext->FileName, plugInKext->Version);
//        InputBootArgs->Tag = TAG_INPUT;
        InputBootArgs->Row = 0xFFFF; //cursor
        InputBootArgs->Item = &(plugInKext->MenuItem);
        InputBootArgs->AtClick = ActionEnter;
        InputBootArgs->AtRightClick = ActionDetails;
        AddMenuEntry(SubScreen, InputBootArgs, true);
        plugInKext = plugInKext->Next;
      }
    }
    Kext = Kext->Next;
  }

  if ( SubScreen != NULL ) AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}

LOADER_ENTRY *SubMenuKextInjectMgmt(LOADER_ENTRY *Entry)
{
	LOADER_ENTRY       *SubEntry;
	REFIT_MENU_SCREEN  *SubScreen;
	CHAR16             *kextDir = NULL;
	UINTN               i;
	CHAR8               ShortOSVersion[8];
//	CHAR16             *UniSysVer = NULL;
	CHAR8              *ChosenOS = Entry->OSVersion;

	NewEntry(&SubEntry, &SubScreen, ActionEnter, SCREEN_SYSTEM, "Block injected kexts->");
	SubEntry->Flags = Entry->Flags;
	if (ChosenOS) {
//    DBG("chosen os %a\n", ChosenOS);
		//shorten os version 10.11.6 -> 10.11
		for (i = 0; i < 8; i++) {
			ShortOSVersion[i] = ChosenOS[i];
			if (ShortOSVersion[i] == '\0') {
				break;
			}
			if (((i > 2) && (ShortOSVersion[i] == '.')) || (i == 5)) {
				ShortOSVersion[i] = '\0';
				break;
			}
		}

		AddMenuInfoLine(SubScreen,
		        PoolPrint(
		                L"Block injected kexts for target version of macOS: %a",
		                ShortOSVersion));

		// Add kext from 10
		{
			AddMenuEntry(SubScreen, SubMenuKextBlockInjection(L"10"), true);

			CHAR16 DirName[256];
			if (OSTYPE_IS_OSX_INSTALLER(Entry->LoaderType)) {
				UnicodeSPrint(DirName, sizeof(DirName), L"10_install");
			}
			else {
				if (OSTYPE_IS_OSX_RECOVERY(Entry->LoaderType)) {
					UnicodeSPrint(DirName, sizeof(DirName), L"10_recovery");
				}
				else {
					UnicodeSPrint(DirName, sizeof(DirName), L"10_normal");
				}
			}
			AddMenuEntry(SubScreen, SubMenuKextBlockInjection(DirName), true);
		}

		// Add kext from 10.{version}
		{
			CHAR16 DirName[256];
			UnicodeSPrint(DirName, sizeof(DirName), L"%a", ShortOSVersion);
			AddMenuEntry(SubScreen, SubMenuKextBlockInjection(DirName), true);

			if (OSTYPE_IS_OSX_INSTALLER(Entry->LoaderType)) {
				UnicodeSPrint(DirName, sizeof(DirName), L"%a_install", ShortOSVersion);
			}
			else {
				if (OSTYPE_IS_OSX_RECOVERY(Entry->LoaderType)) {
					UnicodeSPrint(DirName, sizeof(DirName), L"%a_recovery", ShortOSVersion);
				}
				else {
					UnicodeSPrint(DirName, sizeof(DirName), L"%a_normal", ShortOSVersion);
				}
			}
			AddMenuEntry(SubScreen, SubMenuKextBlockInjection(DirName), true);
		}

		// Add kext from :
		// 10.{version}.0 if NO minor version
		// 10.{version}.{minor version} if minor version is > 0
		{
			{
				CHAR16 OSVersionKextsDirName[256];
				if ( AsciiStrCmp(ShortOSVersion, Entry->OSVersion) == 0 ) {
					UnicodeSPrint(OSVersionKextsDirName, sizeof(OSVersionKextsDirName), L"%a.0", Entry->OSVersion);
				}else{
					UnicodeSPrint(OSVersionKextsDirName, sizeof(OSVersionKextsDirName), L"%a", Entry->OSVersion);
				}
				AddMenuEntry(SubScreen, SubMenuKextBlockInjection(OSVersionKextsDirName), true);
			}

			CHAR16 DirName[256];
			if (OSTYPE_IS_OSX_INSTALLER(Entry->LoaderType)) {
				UnicodeSPrint(DirName, sizeof(DirName), L"%a_install",
				        Entry->OSVersion);
			}
			else {
				if (OSTYPE_IS_OSX_RECOVERY(Entry->LoaderType)) {
					UnicodeSPrint(DirName, sizeof(DirName), L"%a_recovery",
					        Entry->OSVersion);
				}
				else {
					UnicodeSPrint(DirName, sizeof(DirName), L"%a_normal",
					        Entry->OSVersion);
				}
			}
			AddMenuEntry(SubScreen, SubMenuKextBlockInjection(DirName), true);
		}
	}
	else {
		AddMenuInfoLine(SubScreen,
		        PoolPrint(
		                L"Block injected kexts for target version of macOS: %a",
		                ChosenOS));
	}
	if ((kextDir = GetOtherKextsDir(TRUE)) != NULL) {
		AddMenuEntry(SubScreen, SubMenuKextBlockInjection(L"Other"), true);
		FreePool(kextDir);
	}
	if ((kextDir = GetOtherKextsDir(FALSE)) != NULL) {
		AddMenuEntry(SubScreen, SubMenuKextBlockInjection(L"Off"), true);
		FreePool(kextDir);
	}

	AddMenuEntry(SubScreen, &MenuEntryReturn, false);
	return SubEntry;
}



REFIT_MENU_ENTRY  *SubMenuKernelPatches()
{
  REFIT_MENU_ENTRY_OTHER     *Entry;
  REFIT_MENU_SCREEN    *SubScreen;
  REFIT_INPUT_DIALOG   *InputBootArgs;
  INTN                 NrKernels = gSettings.KernelAndKextPatches.NrKernels;
  KERNEL_PATCH  *KernelPatchesMenu = gSettings.KernelAndKextPatches.KernelPatches; //zzzz
  INTN                 Index;

  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_KERNELS, "Custom kernel patches->");

  for (Index = 0; Index < NrKernels; Index++) {
//    InputBootArgs = (__typeof__(InputBootArgs))AllocateZeroPool(sizeof(REFIT_INPUT_DIALOG));
    InputBootArgs = new REFIT_INPUT_DIALOG;
    InputBootArgs->Title = PoolPrint(L"%30a", KernelPatchesMenu[Index].Label);
//    InputBootArgs->Tag = TAG_INPUT;
    InputBootArgs->Row = 0xFFFF; //cursor
    InputBootArgs->Item = &(KernelPatchesMenu[Index].MenuItem);
    InputBootArgs->AtClick = ActionEnter;
    InputBootArgs->AtRightClick = ActionDetails;
    AddMenuEntry(SubScreen, InputBootArgs, true);
  }

  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}

REFIT_MENU_ENTRY  *SubMenuBootPatches()
{
  REFIT_MENU_ENTRY_OTHER     *Entry;
  REFIT_MENU_SCREEN    *SubScreen;
  REFIT_INPUT_DIALOG   *InputBootArgs;
  INTN                 NrBoots = gSettings.KernelAndKextPatches.NrBoots;
  KERNEL_PATCH  *BootPatchesMenu = gSettings.KernelAndKextPatches.BootPatches; //zzzz
  INTN                 Index;

  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_BOOTER, "Custom booter patches->");

  for (Index = 0; Index < NrBoots; Index++) {
//    InputBootArgs = (__typeof__(InputBootArgs))AllocateZeroPool(sizeof(REFIT_INPUT_DIALOG));
    InputBootArgs = new REFIT_INPUT_DIALOG;
    InputBootArgs->Title = PoolPrint(L"%30a", BootPatchesMenu[Index].Label);
//    InputBootArgs->Tag = TAG_INPUT;
    InputBootArgs->Row = 0xFFFF; //cursor
    InputBootArgs->Item = &(BootPatchesMenu[Index].MenuItem);
    InputBootArgs->AtClick = ActionEnter;
    InputBootArgs->AtRightClick = ActionDetails;
    AddMenuEntry(SubScreen, InputBootArgs, true);
  }

  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}

REFIT_MENU_ENTRY  *SubMenuBinaries()
{
  REFIT_MENU_ENTRY_OTHER   *Entry;
  REFIT_MENU_SCREEN  *SubScreen;

  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_BINARIES, "Binaries patching->");

  AddMenuInfoLine(SubScreen, PoolPrint(L"%a", gCPUStructure.BrandString));
  AddMenuInfoLine(SubScreen, PoolPrint(L"Real CPUID: 0x%06x", gCPUStructure.Signature));

  AddMenuItemInput(SubScreen, 64,  "Debug", FALSE);
  AddMenuInfo(SubScreen, L"----------------------");
  AddMenuItemInput(SubScreen, 104, "Fake CPUID:", TRUE);
//  AddMenuItemInput(SubScreen, 108, "Kernel patching allowed", FALSE);
  AddMenuItemInput(SubScreen, 45,  "Kernel Support CPU", FALSE);
  AddMenuItemInput(SubScreen, 91,  "Kernel Lapic", FALSE);
  AddMenuItemInput(SubScreen, 105, "Kernel XCPM", FALSE);
  AddMenuItemInput(SubScreen, 48,  "Kernel PM", FALSE);
  AddMenuItemInput(SubScreen, 121,  "Panic No Kext Dump", FALSE);
  AddMenuEntry(SubScreen, SubMenuKernelPatches(), true);
  AddMenuInfo(SubScreen, L"----------------------");
  AddMenuItemInput(SubScreen, 46,  "AppleIntelCPUPM Patch", FALSE);
  AddMenuItemInput(SubScreen, 47,  "AppleRTC Patch", FALSE);
  AddMenuItemInput(SubScreen, 61,  "Dell SMBIOS Patch", FALSE);
//  AddMenuItemInput(SubScreen, 115, "No Caches", FALSE);
//  AddMenuItemInput(SubScreen, 44,  "Kext patching allowed", FALSE);
  AddMenuEntry(SubScreen, SubMenuKextPatches(), true);
  AddMenuInfo(SubScreen, L"----------------------");
  AddMenuEntry(SubScreen, SubMenuBootPatches(), true);


  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}

REFIT_MENU_ENTRY  *SubMenuDropTables()
{
  CHAR8               sign[5];
  CHAR8               OTID[9];
  REFIT_MENU_ENTRY_OTHER    *Entry;
  REFIT_MENU_SCREEN   *SubScreen;
  REFIT_INPUT_DIALOG  *InputBootArgs;

  sign[4] = 0;
  OTID[8] = 0;

  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_TABLES, "Tables dropping->");

  if (gSettings.ACPIDropTables) {
    ACPI_DROP_TABLE *DropTable = gSettings.ACPIDropTables;
    while (DropTable) {
      CopyMem((CHAR8*)&sign, (CHAR8*)&(DropTable->Signature), 4);
      CopyMem((CHAR8*)&OTID, (CHAR8*)&(DropTable->TableId), 8);
      //MsgLog("adding to menu %a (%x) %a (%lx) L=%d(0x%x)\n",
      //       sign, DropTable->Signature,
      //       OTID, DropTable->TableId,
      //       DropTable->Length, DropTable->Length);
//    InputBootArgs = (__typeof__(InputBootArgs))AllocateZeroPool(sizeof(REFIT_INPUT_DIALOG));
    InputBootArgs = new REFIT_INPUT_DIALOG;
      InputBootArgs->Title = PoolPrint(L"Drop \"%4.4a\" \"%8.8a\" %d", sign, OTID, DropTable->Length);
//      InputBootArgs->Tag = TAG_INPUT;
      InputBootArgs->Row = 0xFFFF; //cursor
      InputBootArgs->Item = &(DropTable->MenuItem);
      InputBootArgs->AtClick = ActionEnter;
      InputBootArgs->AtRightClick = ActionDetails;
      AddMenuEntry(SubScreen, InputBootArgs, true);

      DropTable = DropTable->Next;
    }
  }

  AddMenuItemInput(SubScreen, 4, "Drop all OEM SSDT", FALSE);
  AddMenuItemInput(SubScreen, 113, "Automatic smart merge", FALSE);

  //AddMenuInfoLine(SubScreen, L"PATCHED AML:");
  if (ACPIPatchedAML) {
    ACPI_PATCHED_AML *ACPIPatchedAMLTmp = ACPIPatchedAML;
    while (ACPIPatchedAMLTmp) {
//    InputBootArgs = (__typeof__(InputBootArgs))AllocateZeroPool(sizeof(REFIT_INPUT_DIALOG));
    InputBootArgs = new REFIT_INPUT_DIALOG;
      InputBootArgs->Title = PoolPrint(L"Drop \"%s\"", ACPIPatchedAMLTmp->FileName);
//      InputBootArgs->Tag = TAG_INPUT;
      InputBootArgs->Row = 0xFFFF; //cursor
      InputBootArgs->Item = &(ACPIPatchedAMLTmp->MenuItem);
      InputBootArgs->AtClick = ActionEnter;
      InputBootArgs->AtRightClick = ActionDetails;
      AddMenuEntry(SubScreen, InputBootArgs, true);
      ACPIPatchedAMLTmp = ACPIPatchedAMLTmp->Next;
    }
  }

  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}

REFIT_MENU_ENTRY  *SubMenuSmbios()
{
  REFIT_MENU_ENTRY_OTHER   *Entry;
  REFIT_MENU_SCREEN  *SubScreen;

  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_SMBIOS, "SMBIOS->");

  AddMenuInfoLine(SubScreen, PoolPrint(L"%a", gCPUStructure.BrandString));
  AddMenuInfoLine(SubScreen, PoolPrint(L"%a", gSettings.OEMProduct));
  AddMenuInfoLine(SubScreen, PoolPrint(L"with board %a", gSettings.OEMBoard));

  AddMenuItemInput(SubScreen, 78,  "Product Name:", TRUE);
  AddMenuItemInput(SubScreen, 79,  "Product Version:", TRUE);
  AddMenuItemInput(SubScreen, 80,  "Product SN:", TRUE);
  AddMenuItemInput(SubScreen, 81,  "Board ID:", TRUE);
  AddMenuItemInput(SubScreen, 82,  "Board SN:", TRUE);
  AddMenuItemInput(SubScreen, 83,  "Board Type:", TRUE);
  AddMenuItemInput(SubScreen, 84,  "Board Version:", TRUE);
  AddMenuItemInput(SubScreen, 85,  "Chassis Type:", TRUE);
  AddMenuItemInput(SubScreen, 86,  "ROM Version:", TRUE);
  AddMenuItemInput(SubScreen, 87,  "ROM Release Date:", TRUE);
  AddMenuItemInput(SubScreen, 62,  "FirmwareFeatures:", TRUE);
  AddMenuItemInput(SubScreen, 63,  "FirmwareFeaturesMask:", TRUE);
  AddMenuItemInput(SubScreen, 17,  "PlatformFeature:", TRUE);
  AddMenuItemInput(SubScreen, 117, "EFI Version:", TRUE);

  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}

REFIT_MENU_ENTRY *SubMenuDropDSM()
{
  // init
  REFIT_MENU_ENTRY_OTHER   *Entry;
  REFIT_MENU_SCREEN  *SubScreen;

  // create the entry in the main menu
  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_DSM, NULL);
  //  Entry->Title = PoolPrint(L"Drop OEM _DSM [0x%04x]->", gSettings.DropOEM_DSM);

  // submenu description
  AddMenuInfoLine(SubScreen, PoolPrint(L"Choose devices to drop OEM _DSM methods from DSDT"));

  AddMenuCheck(SubScreen, "ATI/AMD Graphics",     DEV_ATI, 101);
  AddMenuCheck(SubScreen, "Nvidia Graphics",      DEV_NVIDIA, 101);
  AddMenuCheck(SubScreen, "Intel Graphics",       DEV_INTEL, 101);
  AddMenuCheck(SubScreen, "PCI HDA audio",        DEV_HDA, 101);
  AddMenuCheck(SubScreen, "HDMI audio",           DEV_HDMI, 101);
  AddMenuCheck(SubScreen, "PCI LAN Adapter",      DEV_LAN, 101);
  AddMenuCheck(SubScreen, "PCI WiFi Adapter",     DEV_WIFI, 101);
  AddMenuCheck(SubScreen, "IDE HDD",              DEV_IDE, 101);
  AddMenuCheck(SubScreen, "SATA HDD",             DEV_SATA, 101);
  AddMenuCheck(SubScreen, "USB Controllers",      DEV_USB, 101);
  AddMenuCheck(SubScreen, "LPC Controller",       DEV_LPC, 101);
  AddMenuCheck(SubScreen, "SMBUS Controller",     DEV_SMBUS, 101);
  AddMenuCheck(SubScreen, "IMEI Device",          DEV_IMEI, 101);
  AddMenuCheck(SubScreen, "Firewire",             DEV_FIREWIRE, 101);

  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  ModifyTitles(Entry);

  return Entry;
}

REFIT_MENU_ENTRY  *SubMenuDsdtFix()
{
  REFIT_MENU_ENTRY_OTHER   *Entry; //, *SubEntry;
  REFIT_MENU_SCREEN  *SubScreen;
//  REFIT_INPUT_DIALOG *InputBootArgs;

  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_DSDT, NULL);
  //  Entry->Title = PoolPrint(L"DSDT fix mask [0x%08x]->", gSettings.FixDsdt);

  AddMenuCheck(SubScreen, "Add DTGP",     FIX_DTGP, 67);
  AddMenuCheck(SubScreen, "Fix Darwin as WinXP",   FIX_WARNING, 67);
  AddMenuCheck(SubScreen, "Fix Darwin as Win7",   FIX_DARWIN, 67);
  AddMenuCheck(SubScreen, "Fix shutdown", FIX_SHUTDOWN, 67);
  AddMenuCheck(SubScreen, "Add MCHC",     FIX_MCHC, 67);
  AddMenuCheck(SubScreen, "Fix HPET",     FIX_HPET, 67);
  AddMenuCheck(SubScreen, "Fake LPC",     FIX_LPC, 67);
  AddMenuCheck(SubScreen, "Fix IPIC",     FIX_IPIC, 67);
  AddMenuCheck(SubScreen, "Add SMBUS",    FIX_SBUS, 67);
  AddMenuCheck(SubScreen, "Fix display",  FIX_DISPLAY, 67);
  AddMenuCheck(SubScreen, "Fix IDE",      FIX_IDE, 67);
  AddMenuCheck(SubScreen, "Fix SATA",     FIX_SATA, 67);
  AddMenuCheck(SubScreen, "Fix Firewire", FIX_FIREWIRE, 67);
  AddMenuCheck(SubScreen, "Fix USB",      FIX_USB, 67);
  AddMenuCheck(SubScreen, "Fix LAN",      FIX_LAN, 67);
  AddMenuCheck(SubScreen, "Fix Airport",  FIX_WIFI, 67);
  AddMenuCheck(SubScreen, "Fix sound",    FIX_HDA, 67);
//  AddMenuCheck(SubScreen, "Fix new way",  FIX_NEW_WAY, 67);
  AddMenuCheck(SubScreen, "Fix RTC",      FIX_RTC, 67);
  AddMenuCheck(SubScreen, "Fix TMR",      FIX_TMR, 67);
  AddMenuCheck(SubScreen, "Add IMEI",     FIX_IMEI, 67);
  AddMenuCheck(SubScreen, "Fix IntelGFX", FIX_INTELGFX, 67);
  AddMenuCheck(SubScreen, "Fix _WAK",     FIX_WAK, 67);
  AddMenuCheck(SubScreen, "Del unused",   FIX_UNUSED, 67);
  AddMenuCheck(SubScreen, "Fix ADP1",     FIX_ADP1, 67);
  AddMenuCheck(SubScreen, "Add PNLF",     FIX_PNLF, 67);
  AddMenuCheck(SubScreen, "Fix S3D",      FIX_S3D, 67);
  AddMenuCheck(SubScreen, "Rename ACST",  FIX_ACST, 67);
  AddMenuCheck(SubScreen, "Add HDMI",     FIX_HDMI, 67);
  AddMenuCheck(SubScreen, "Fix Regions",  FIX_REGIONS, 67);
  AddMenuCheck(SubScreen, "Fix Headers",  FIX_HEADERS, 67);
  AddMenuCheck(SubScreen, "Fix Mutex",    FIX_MUTEX, 67);


  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  ModifyTitles(Entry);

  return Entry;
}

REFIT_MENU_ENTRY  *SubMenuDSDTPatches()  //yyyy
{
  REFIT_MENU_ENTRY_OTHER     *Entry;
  REFIT_MENU_SCREEN    *SubScreen;
  REFIT_INPUT_DIALOG   *InputBootArgs;

  INTN             PatchDsdtNum = gSettings.PatchDsdtNum;
  INPUT_ITEM   *DSDTPatchesMenu = gSettings.PatchDsdtMenuItem;
  INTN                 Index;

  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_DSDT_PATCHES, "Custom DSDT patches->");

  for (Index = 0; Index < PatchDsdtNum; Index++) {
//    InputBootArgs = (__typeof__(InputBootArgs))AllocateZeroPool(sizeof(REFIT_INPUT_DIALOG));
    InputBootArgs = new REFIT_INPUT_DIALOG;
    InputBootArgs->Title = PoolPrint(L"%a", gSettings.PatchDsdtLabel[Index]);
//    InputBootArgs->Tag = TAG_INPUT;
    InputBootArgs->Row = 0xFFFF; //cursor
    InputBootArgs->Item = &DSDTPatchesMenu[Index];
    InputBootArgs->AtClick = ActionEnter;
    InputBootArgs->AtRightClick = ActionDetails;
    AddMenuEntry(SubScreen, InputBootArgs, true);
  }

  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}

REFIT_MENU_ENTRY  *SubMenuDsdts()
{
  REFIT_MENU_ENTRY_OTHER   *Entry;
  REFIT_MENU_SCREEN  *SubScreen;
  REFIT_MENU_SWITCH *InputBootArgs;
  UINTN               i;

  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_ACPI, "Dsdt name->");

  AddMenuInfoLine(SubScreen, L"Select a DSDT file:");
  AddMenuItemSwitch(SubScreen, 116,  "BIOS.aml", FALSE);

  for (i = 0; i < DsdtsNum; i++) {
//    InputBootArgs = (__typeof__(InputBootArgs))AllocateZeroPool(sizeof(REFIT_INPUT_DIALOG));
    InputBootArgs = new REFIT_MENU_SWITCH;
    InputBootArgs->Title = PoolPrint(L"%s", DsdtsList[i]);
//    InputBootArgs->Tag = TAG_SWITCH_OLD;
    InputBootArgs->Row = i + 1;
    InputBootArgs->Item = &InputItems[116];
    InputBootArgs->AtClick = ActionEnter;
    InputBootArgs->AtRightClick = ActionDetails;
    AddMenuEntry(SubScreen, InputBootArgs, true);
  }
  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}


REFIT_MENU_ENTRY *SubMenuACPI()
{
  // init
  REFIT_MENU_ENTRY_OTHER   *Entry;
  REFIT_MENU_SCREEN  *SubScreen;

  // create the entry in the options menu
  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_ACPI, "ACPI patching->");

  // submenu description
  AddMenuInfoLine(SubScreen, PoolPrint(L"Choose options to patch ACPI"));

  AddMenuItemInput(SubScreen, 102, "Debug DSDT", FALSE);

  AddMenuEntry(SubScreen, SubMenuDsdts(), true);
  AddMenuEntry(SubScreen, SubMenuDropTables(), true);
  AddMenuEntry(SubScreen, SubMenuDropDSM(), true);
  AddMenuEntry(SubScreen, SubMenuDsdtFix(), true);
  AddMenuEntry(SubScreen, SubMenuDSDTPatches(), true);
  AddMenuItemInput(SubScreen, 49, "Fix MCFG", FALSE);

  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}

REFIT_MENU_ENTRY  *SubMenuAudioPort()
{
  REFIT_MENU_ENTRY_OTHER   *Entry;
  REFIT_MENU_SCREEN  *SubScreen;
  REFIT_MENU_SWITCH *InputBootArgs;
  UINTN               i;

  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_AUDIOPORTS, "Startup sound output->");

  AddMenuInfoLine(SubScreen, L"Select an audio output, press F7 to test");
  AddMenuItemInput(SubScreen, 120, "Volume:", TRUE);

  for (i = 0; i < AudioNum; i++) {
//    InputBootArgs = (__typeof__(InputBootArgs))AllocateZeroPool(sizeof(REFIT_INPUT_DIALOG));
    InputBootArgs = new REFIT_MENU_SWITCH;
    InputBootArgs->Title = PoolPrint(L"%s_%a", AudioList[i].Name, AudioOutputNames[AudioList[i].Device]);
//    InputBootArgs->Tag = TAG_SWITCH_OLD;
    InputBootArgs->Row = i;
    InputBootArgs->Item = &InputItems[119];
    InputBootArgs->AtClick = ActionEnter;
    InputBootArgs->AtRightClick = ActionDetails;
    AddMenuEntry(SubScreen, InputBootArgs, true);
  }

  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}

VOID CreateMenuProps(REFIT_MENU_SCREEN   *SubScreen, DEV_PROPERTY *Prop)
{
	REFIT_INPUT_DIALOG  *InputBootArgs;

//    InputBootArgs = (__typeof__(InputBootArgs))AllocateZeroPool(sizeof(REFIT_INPUT_DIALOG));
    InputBootArgs = new REFIT_INPUT_DIALOG;
	InputBootArgs->Title = PoolPrint(L"  key: %a", Prop->Key);
//	InputBootArgs->Tag = TAG_INPUT;
	InputBootArgs->Row = 0xFFFF; //cursor
									   //     InputBootArgs->Item = ADDRESS_OF(DEV_PROPERTY, Prop, INPUT_ITEM, MenuItem);
	InputBootArgs->Item = &Prop->MenuItem;
	InputBootArgs->AtClick = ActionEnter;
	InputBootArgs->AtRightClick = ActionDetails;
	AddMenuEntry(SubScreen, InputBootArgs, true);
	switch (Prop->ValueType) {
	case kTagTypeInteger:
		AddMenuInfo(SubScreen, PoolPrint(L"     value: 0x%08x", *(UINT64*)Prop->Value));
		break;
	case kTagTypeString:
		AddMenuInfo(SubScreen, PoolPrint(L"     value: %30a", Prop->Value));
		break;
	case   kTagTypeFalse:
		AddMenuInfo(SubScreen, PoolPrint(L"     value: false"));
		break;
	case   kTagTypeTrue:
		AddMenuInfo(SubScreen, PoolPrint(L"     value: true"));
		break;

	default: //type data, print first 24 bytes
			 //CHAR8* Bytes2HexStr(UINT8 *data, UINTN len)
		AddMenuInfo(SubScreen, PoolPrint(L"     value[%d]: %24a", Prop->ValueLen, Bytes2HexStr((UINT8*)Prop->Value, MIN(24, Prop->ValueLen))));
		break;
	}

}

REFIT_MENU_ENTRY  *SubMenuCustomDevices()
{
  REFIT_MENU_ENTRY_OTHER    *Entry;
  REFIT_MENU_SCREEN   *SubScreen;

  UINT32              DevAddr, OldDevAddr = 0;

  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_DEVICES, "Custom properties->");

  if (gSettings.ArbProperties) {
    DEV_PROPERTY *Prop = gSettings.ArbProperties;
	if (Prop && (Prop->Device == 0))
	{
		DEV_PROPERTY *Props = NULL;
		while (Prop) {
			AddMenuInfo(SubScreen, L"------------");
			AddMenuInfo(SubScreen, PoolPrint(L"%a", Prop->Label));
			Props = Prop->Child;
			while (Props) {
				CreateMenuProps(SubScreen, Props);
				Props = Props->Next;
			}
			Prop = Prop->Next;
		}
	}
    while (Prop) {
      DevAddr = Prop->Device;
      if (DevAddr != 0 && DevAddr != OldDevAddr) {
        OldDevAddr = DevAddr;
        AddMenuInfo(SubScreen, L"------------");
        AddMenuInfo(SubScreen, PoolPrint(L"%a", Prop->Label));
        CreateMenuProps(SubScreen, Prop);
      }
      Prop = Prop->Next;
    }
  }
  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  Entry->SubScreen = SubScreen;
  return Entry;
}


REFIT_MENU_ENTRY  *SubMenuPCI()
{
  REFIT_MENU_ENTRY_OTHER   *Entry;
  REFIT_MENU_SCREEN  *SubScreen;

  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_USB, "PCI devices->");

  AddMenuItemInput(SubScreen, 74,  "USB Ownership", FALSE);
  AddMenuItemInput(SubScreen, 92,  "USB Injection", FALSE);
  AddMenuItemInput(SubScreen, 93,  "Inject ClockID", FALSE);
  AddMenuItemInput(SubScreen, 106, "Inject EFI Strings", FALSE);
  AddMenuItemInput(SubScreen, 107, "No Default Properties", FALSE);
  AddMenuItemInput(SubScreen, 97,  "FakeID LAN:", TRUE);
  AddMenuItemInput(SubScreen, 98,  "FakeID WIFI:", TRUE);
  AddMenuItemInput(SubScreen, 99,  "FakeID SATA:", TRUE);
  AddMenuItemInput(SubScreen, 100, "FakeID XHCI:", TRUE);
  AddMenuItemInput(SubScreen, 103, "FakeID IMEI:", TRUE);
  AddMenuEntry(SubScreen, SubMenuCustomDevices(), true);

  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  Entry->SubScreen = SubScreen;
  return Entry;
}


REFIT_MENU_ENTRY  *SubMenuThemes()
{
  REFIT_MENU_ENTRY_OTHER   *Entry;
  REFIT_MENU_SCREEN  *SubScreen;
  REFIT_MENU_SWITCH *InputBootArgs;
  UINTN               i;

  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_THEME, "Themes->");

  AddMenuInfoLine(SubScreen, L"Installed themes:");
  //add embedded
  AddMenuItemSwitch(SubScreen, 3,  "embedded", FALSE);

  for (i = 0; i < ThemesNum; i++) {
//    InputBootArgs = (__typeof__(InputBootArgs))AllocateZeroPool(sizeof(REFIT_INPUT_DIALOG));
    InputBootArgs = new REFIT_MENU_SWITCH;
    InputBootArgs->Title = PoolPrint(L"%s", ThemesList[i]);
//    InputBootArgs->Tag = TAG_SWITCH_OLD;
    InputBootArgs->Row = i + 1;
    InputBootArgs->Item = &InputItems[3];
    InputBootArgs->AtClick = ActionEnter;
    InputBootArgs->AtRightClick = ActionDetails;
    AddMenuEntry(SubScreen, InputBootArgs, true);
  }
  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}

REFIT_MENU_ENTRY *SubMenuGUI()
{
  // init
  REFIT_MENU_ENTRY_OTHER   *Entry;
  REFIT_MENU_SCREEN  *SubScreen;

  // create the entry in the options menu
  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_GUI, "GUI tuning->");

  // submenu description
  AddMenuInfoLine(SubScreen, PoolPrint(L"Choose options to tune the Interface"));

  AddMenuItemInput(SubScreen, 70, "Pointer Speed:", TRUE);
  AddMenuItemInput(SubScreen, 72, "Mirror Move", FALSE);

  AddMenuEntry(SubScreen, SubMenuThemes(), true);

  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}


/*
 * This is a simple and user friendly submenu which makes it possible to modify
 * the System Integrity Protection configuration from the Clover's GUI.
 * Author: Needy.
 * The below function is based on the SubMenuDsdtFix function.
 */
REFIT_MENU_ENTRY *SubMenuCSR()
{
  // init
  REFIT_MENU_ENTRY_OTHER   *Entry;
  REFIT_MENU_SCREEN  *SubScreen;

  // create the entry in the main menu
  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_CSR, NULL);

  // submenu description
  AddMenuInfoLine(SubScreen, PoolPrint(L"Modify the System Integrity Protection configuration."));
  AddMenuInfoLine(SubScreen, PoolPrint(L"All configuration changes apply to the entire machine."));

  // available configurations
  AddMenuCheck(SubScreen, "Allow Untrusted Kexts", CSR_ALLOW_UNTRUSTED_KEXTS, 66);
  AddMenuCheck(SubScreen, "Allow Unrestricted FS", CSR_ALLOW_UNRESTRICTED_FS, 66);
  AddMenuCheck(SubScreen, "Allow Task For PID", CSR_ALLOW_TASK_FOR_PID, 66);
  AddMenuCheck(SubScreen, "Allow Kernel Debuger", CSR_ALLOW_KERNEL_DEBUGGER, 66);
  AddMenuCheck(SubScreen, "Allow Apple Internal", CSR_ALLOW_APPLE_INTERNAL, 66);
  AddMenuCheck(SubScreen, "Allow Unrestricted DTrace", CSR_ALLOW_UNRESTRICTED_DTRACE, 66);
  AddMenuCheck(SubScreen, "Allow Unrestricted NVRAM", CSR_ALLOW_UNRESTRICTED_NVRAM, 66);
  AddMenuCheck(SubScreen, "Allow Device Configuration", CSR_ALLOW_DEVICE_CONFIGURATION, 66);
  AddMenuCheck(SubScreen, "Allow Any Recovery OS", CSR_ALLOW_ANY_RECOVERY_OS, 66);
  AddMenuCheck(SubScreen, "Allow Unapproved Kexts", CSR_ALLOW_UNAPPROVED_KEXTS, 66);

  // return
  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  ModifyTitles(Entry);
  return Entry;
}

REFIT_MENU_ENTRY *SubMenuBLC()
{
  // init
  REFIT_MENU_ENTRY_OTHER   *Entry;
  REFIT_MENU_SCREEN  *SubScreen;

  // create the entry in the main menu
  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_BLC, NULL);
//  Entry->Title = PoolPrint(L"boot_args->flags [0x%02x]->", gSettings.BooterConfig);

  // submenu description
  AddMenuInfoLine(SubScreen, PoolPrint(L"Modify flags for boot.efi"));

  AddMenuCheck(SubScreen, "Reboot On Panic",    kBootArgsFlagRebootOnPanic, 65);
  AddMenuCheck(SubScreen, "Hi DPI",             kBootArgsFlagHiDPI, 65);
  AddMenuCheck(SubScreen, "Black Screen",       kBootArgsFlagBlack, 65);
  AddMenuCheck(SubScreen, "CSR Active Config",  kBootArgsFlagCSRActiveConfig, 65);
  AddMenuCheck(SubScreen, "CSR Pending Config", kBootArgsFlagCSRPendingConfig, 65);
  AddMenuCheck(SubScreen, "CSR Boot",           kBootArgsFlagCSRBoot, 65);
  AddMenuCheck(SubScreen, "Black Background",   kBootArgsFlagBlackBg, 65);
  AddMenuCheck(SubScreen, "Login UI",           kBootArgsFlagLoginUI, 65);
  AddMenuCheck(SubScreen, "Install UI",         kBootArgsFlagInstallUI, 65);

  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  ModifyTitles(Entry);
  return Entry;
}

REFIT_MENU_ENTRY *SubMenuSystem()
{
  // init
  REFIT_MENU_ENTRY_OTHER   *Entry;
  REFIT_MENU_SCREEN  *SubScreen;

  // create the entry in the options menu
  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_SYSTEM, "System Parameters->");

  // submenu description
  AddMenuInfoLine(SubScreen, PoolPrint(L"Choose options for booted OS"));

  AddMenuItemInput(SubScreen, 2,  "Block kext:", TRUE);
  AddMenuItemInput(SubScreen, 51, "Set OS version if not:", TRUE);
  AddMenuItemInput(SubScreen, 118, "Booter Cfg Command:", TRUE);

  AddMenuEntry(SubScreen, SubMenuCSR(), true);
  AddMenuEntry(SubScreen, SubMenuBLC(), true);

  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}

REFIT_MENU_ENTRY  *SubMenuConfigs()
{
  REFIT_MENU_ENTRY_OTHER   *Entry;
  REFIT_MENU_SCREEN  *SubScreen;
  REFIT_MENU_SWITCH *InputBootArgs;
  UINTN               i;

  NewEntry(&Entry, &SubScreen, ActionEnter, SCREEN_THEME, "Configs->");

  AddMenuInfoLine(SubScreen, L"Select a config file:");

  for (i = 0; i < ConfigsNum; i++) {
//    InputBootArgs = (__typeof__(InputBootArgs))AllocateZeroPool(sizeof(REFIT_INPUT_DIALOG));
    InputBootArgs = new REFIT_MENU_SWITCH;
    InputBootArgs->Title = PoolPrint(L"%s", ConfigsList[i]);
//    InputBootArgs->Tag = TAG_SWITCH_OLD;
    InputBootArgs->Row = i;
    InputBootArgs->Item = &InputItems[90];
    InputBootArgs->AtClick = ActionEnter;
    InputBootArgs->AtRightClick = ActionDetails;
    AddMenuEntry(SubScreen, InputBootArgs, true);
  }
  AddMenuEntry(SubScreen, &MenuEntryReturn, false);
  return Entry;
}

VOID  OptionsMenu(OUT REFIT_ABSTRACT_MENU_ENTRY **ChosenEntry, IN CHAR8 *LastChosenOS)
{
  REFIT_ABSTRACT_MENU_ENTRY    *TmpChosenEntry = NULL;
  REFIT_ABSTRACT_MENU_ENTRY    *NextChosenEntry = NULL;
  UINTN               MenuExit = 0;
  UINTN               SubMenuExit;
  UINTN               NextMenuExit;
  //CHAR16*           Flags;
  MENU_STYLE_FUNC     Style = TextMenuStyle;
  INTN                EntryIndex = 0;
  INTN                SubEntryIndex = -1; //value -1 means old position to remember
  INTN                NextEntryIndex = -1;

  //  REFIT_INPUT_DIALOG* InputBootArgs;
  BOOLEAN             OldFontStyle = GlobalConfig.Proportional;

  GlobalConfig.Proportional = FALSE; //temporary disable proportional

  if (AllowGraphicsMode) {
    Style = GraphicsMenuStyle;
  }

  // remember, if you extended this menu then change procedures
  // FillInputs and ApplyInputs

  if (!(GlobalConfig.HideUIFlags & HIDEUI_FLAG_MENU_TITLE_IMAGE)) {
    OptionMenu.TitleImage = BuiltinIcon(BUILTIN_ICON_FUNC_OPTIONS);
  } else {
    OptionMenu.TitleImage = NULL;
  }

  gThemeOptionsChanged = FALSE;

  if (OptionMenu.Entries.size() == 0) {
    gThemeOptionsChanged = TRUE;
    OptionMenu.ID = SCREEN_OPTIONS;
    OptionMenu.AnimeRun = GetAnime(&OptionMenu); //FALSE;

    AddMenuItemInput(&OptionMenu, 0, "Boot Args:", TRUE);

//    AddMenuItemInput(&OptionMenu, 90, "Config:", TRUE);
//   InputBootArgs->ShortcutDigit = 0xF1;
    AddMenuEntry(&OptionMenu, SubMenuConfigs(), true);

    if (AllowGraphicsMode) {
      AddMenuEntry(&OptionMenu, SubMenuGUI(), true);
    }
    AddMenuEntry(&OptionMenu, SubMenuACPI(), true);
    AddMenuEntry(&OptionMenu, SubMenuSmbios(), true);
    AddMenuEntry(&OptionMenu, SubMenuPCI(), true);
    AddMenuEntry(&OptionMenu, SubMenuSpeedStep(), true);
    AddMenuEntry(&OptionMenu, SubMenuGraphics(), true);
    AddMenuEntry(&OptionMenu, SubMenuAudio(), true);
    AddMenuEntry(&OptionMenu, SubMenuAudioPort(), true);
    AddMenuEntry(&OptionMenu, SubMenuBinaries(), true);
    AddMenuEntry(&OptionMenu, SubMenuSystem(), true);
    AddMenuEntry(&OptionMenu, &MenuEntryReturn, false);
    //DBG("option menu created entries=%d\n", OptionMenu.Entries.size());
  }

  while (!MenuExit) {
    MenuExit = RunGenericMenu(&OptionMenu, Style, &EntryIndex, ChosenEntry);
    //    MenuExit = RunMenu(&OptionMenu, ChosenEntry);
    if (  MenuExit == MENU_EXIT_ESCAPE || (*ChosenEntry)->getREFIT_MENU_ITEM_RETURN()  )
      break;
    if (MenuExit == MENU_EXIT_ENTER || MenuExit == MENU_EXIT_DETAILS) {
      //enter input dialog or subscreen
      if ((*ChosenEntry)->SubScreen != NULL) {
        SubMenuExit = 0;
        while (!SubMenuExit) {
          SubMenuExit = RunGenericMenu((*ChosenEntry)->SubScreen, Style, &SubEntryIndex, &TmpChosenEntry);
          if (SubMenuExit == MENU_EXIT_ESCAPE || TmpChosenEntry->getREFIT_MENU_ITEM_RETURN()  ){
            ApplyInputs();
            ModifyTitles(*ChosenEntry);
            break;
          }
          if (SubMenuExit == MENU_EXIT_ENTER || MenuExit == MENU_EXIT_DETAILS) {
            if (TmpChosenEntry->SubScreen != NULL) {
              NextMenuExit = 0;
              while (!NextMenuExit) {
                NextMenuExit = RunGenericMenu(TmpChosenEntry->SubScreen, Style, &NextEntryIndex, &NextChosenEntry);
                if (NextMenuExit == MENU_EXIT_ESCAPE || NextChosenEntry->getREFIT_MENU_ITEM_RETURN()  ){
                  ApplyInputs();
                  ModifyTitles(TmpChosenEntry);
                  break;
                }
                if (NextMenuExit == MENU_EXIT_ENTER || MenuExit == MENU_EXIT_DETAILS) {
                  // enter input dialog
                  NextMenuExit = 0;
                  ApplyInputs();
                  ModifyTitles(TmpChosenEntry);
                }
              } //while(!NextMenuExit)
            }
            // enter input dialog
            SubMenuExit = 0;
            ApplyInputs();
            ModifyTitles(TmpChosenEntry);
          }
        } //while(!SubMenuExit)
      }
      MenuExit = 0;
    } // if MENU_EXIT_ENTER
  }
//exit:
  GlobalConfig.Proportional = OldFontStyle;
  ApplyInputs();
}

UINT32 EncodeOptions(CONST CHAR16 *Options)
{
  UINT32 OptionsBits = 0;
  INTN Index;
  if (!Options) {
    return 0;
  }
  for (Index = 0; Index < NUM_OPT; Index++) {
    if (StrStr(Options, ArgOptional[Index])) {
      OptionsBits |= (1 << Index);
      if (Index == 1) {
        OptionsBits &= ~1;
      }
    }
  }
  return OptionsBits;
}

VOID DecodeOptions(REFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER *Entry)
{
  //set checked option
  INTN Index;
  if (!Entry) {
    return;
  }
  for (Index = 0; Index < INX_NVWEBON; Index++) { //not including INX_NVWEBON
    if (gSettings.OptionsBits & (1 << Index)) {
      Entry->LoadOptions = AddLoadOption(Entry->LoadOptions, ArgOptional[Index]);
    }
  }
  //remove unchecked options
  for (Index = 0; Index < INX_NVWEBON; Index++) { //not including INX_NVWEBON
    if ((gSettings.OptionsBits & (1 << Index)) == 0) {
      Entry->LoadOptions = RemoveLoadOption(Entry->LoadOptions, ArgOptional[Index]);
    }
  }

  if (Entry->getLOADER_ENTRY()) {
    LOADER_ENTRY* loaderEntry = Entry->getLOADER_ENTRY();
    // Only for non-legacy entries, as LEGACY_ENTRY doesn't have OSVersion
    if (gSettings.OptionsBits & OPT_NVWEBON) {
      if (AsciiOSVersionToUint64(loaderEntry->OSVersion) >= AsciiOSVersionToUint64("10.12")) {
        gSettings.NvidiaWeb = TRUE;
      } else {
        Entry->LoadOptions = AddLoadOption(loaderEntry->LoadOptions, ArgOptional[INX_NVWEBON]);
      }
    }
    if ((gSettings.OptionsBits & OPT_NVWEBON) == 0) {
      if (AsciiOSVersionToUint64(loaderEntry->OSVersion) >= AsciiOSVersionToUint64("10.12")) {
        gSettings.NvidiaWeb = FALSE;
      } else {
        Entry->LoadOptions = RemoveLoadOption(loaderEntry->LoadOptions, ArgOptional[INX_NVWEBON]);
      }
    }
  }
}


UINTN REFIT_MENU_SCREEN::RunMainMenu(IN INTN DefaultSelection, OUT REFIT_ABSTRACT_MENU_ENTRY **ChosenEntry)
{
  MENU_STYLE_FUNC     Style             = TextMenuStyle;
  MENU_STYLE_FUNC     MainStyle         = TextMenuStyle;
  REFIT_ABSTRACT_MENU_ENTRY    *TempChosenEntry  = 0;
  REFIT_ABSTRACT_MENU_ENTRY    *MainChosenEntry  = 0;
  REFIT_ABSTRACT_MENU_ENTRY    *NextChosenEntry  = NULL;
  UINTN               MenuExit = 0, SubMenuExit = 0;
  INTN                DefaultEntryIndex = DefaultSelection;
  INTN                SubMenuIndex;

  if (AllowGraphicsMode) {
    Style = GraphicsMenuStyle;
    if (GlobalConfig.VerticalLayout) {
      MainStyle = MainMenuVerticalStyle;
    } else {
      MainStyle = MainMenuStyle;
    }
  }

  while (!MenuExit) {
    AnimeRun = MainAnime;
    MenuExit = RunGenericMenu(Screen, MainStyle, &DefaultEntryIndex, &MainChosenEntry);
    TimeoutSeconds = 0;

    if (MenuExit == MENU_EXIT_DETAILS && MainChosenEntry->SubScreen != NULL) {
      CHAR16 *TmpArgs = NULL;
      if (AsciiStrLen(gSettings.BootArgs) > 0) {
        TmpArgs = PoolPrint(L"%a", gSettings.BootArgs);
      }
      SubMenuIndex = -1;

      gSettings.OptionsBits = EncodeOptions(TmpArgs);
//      DBG("main OptionsBits = 0x%x\n", gSettings.OptionsBits);
      if ( MainChosenEntry->getREFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER() ) gSettings.OptionsBits |= EncodeOptions(MainChosenEntry->getREFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER()->LoadOptions);
//      DBG("add OptionsBits = 0x%x\n", gSettings.OptionsBits);
      if ( MainChosenEntry->getREFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER() ) DecodeOptions(MainChosenEntry->getREFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER());
      //      DBG(" enter menu with LoadOptions: %s\n", ((LOADER_ENTRY*)MainChosenEntry)->LoadOptions);
      if (MainChosenEntry->getLOADER_ENTRY()) {
        // Only for non-legacy entries, as LEGACY_ENTRY doesn't have Flags
        gSettings.FlagsBits = MainChosenEntry->getLOADER_ENTRY()->Flags;
      }
 //          DBG(" MainChosenEntry with FlagsBits = 0x%x\n", gSettings.FlagsBits);

      if (TmpArgs) {
        FreePool(TmpArgs);
        TmpArgs = NULL;
      }
      SubMenuExit = 0;
      while (!SubMenuExit) {
        //running details menu
        SubMenuExit = RunGenericMenu(MainChosenEntry->SubScreen, Style, &SubMenuIndex, &TempChosenEntry);
        if ( MainChosenEntry->getREFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER() ) DecodeOptions(MainChosenEntry->getREFIT_MENU_ITEM_ABSTRACT_ENTRY_LOADER());
//        DBG("get OptionsBits = 0x%x\n", gSettings.OptionsBits);
//        DBG(" TempChosenEntry FlagsBits = 0x%x\n", ((LOADER_ENTRY*)TempChosenEntry)->Flags);
        if (SubMenuExit == MENU_EXIT_ESCAPE || TempChosenEntry->getREFIT_MENU_ITEM_RETURN() ) {
          SubMenuExit = MENU_EXIT_ENTER;
          MenuExit = 0;
          break;
        }
        if (MainChosenEntry->getREFIT_MENU_ENTRY_CLOVER()) {
          MainChosenEntry->getREFIT_MENU_ENTRY_CLOVER()->LoadOptions = EfiStrDuplicate(((REFIT_MENU_ENTRY_CLOVER*)TempChosenEntry)->LoadOptions);
        }
        //       DBG(" exit menu with LoadOptions: %s\n", ((LOADER_ENTRY*)MainChosenEntry)->LoadOptions);
        if (SubMenuExit == MENU_EXIT_ENTER && MainChosenEntry->getLOADER_ENTRY() && TempChosenEntry->getLOADER_ENTRY()) {
          // Only for non-legacy entries, as LEGACY_ENTRY doesn't have Flags
          MainChosenEntry->getLOADER_ENTRY()->Flags = TempChosenEntry->getLOADER_ENTRY()->Flags;
//           DBG(" get MainChosenEntry FlagsBits = 0x%x\n", ((LOADER_ENTRY*)MainChosenEntry)->Flags);
        }
        if (/*MenuExit == MENU_EXIT_ENTER &&*/ MainChosenEntry->getLOADER_ENTRY()) {
          if (MainChosenEntry->getLOADER_ENTRY()->LoadOptions) {
            AsciiSPrint(gSettings.BootArgs, 255, "%s", ((LOADER_ENTRY*)MainChosenEntry)->LoadOptions); // cast ok
          } else {
            ZeroMem(&gSettings.BootArgs, 255);
          }
          DBG(" boot with args: %a\n", gSettings.BootArgs);
        }
        //---- Details submenu (kexts disabling etc)
        if (SubMenuExit == MENU_EXIT_ENTER || MenuExit == MENU_EXIT_DETAILS) {
          if (TempChosenEntry->SubScreen != NULL) {
            UINTN NextMenuExit = 0;
            INTN NextEntryIndex = -1;
            while (!NextMenuExit) {
              NextMenuExit = RunGenericMenu(TempChosenEntry->SubScreen, Style, &NextEntryIndex, &NextChosenEntry);
              if (NextMenuExit == MENU_EXIT_ESCAPE || NextChosenEntry->getREFIT_MENU_ITEM_RETURN() ) {
                SubMenuExit = 0;
                NextMenuExit = MENU_EXIT_ENTER;
                break;
              }
 //             DBG(" get NextChosenEntry FlagsBits = 0x%x\n", ((LOADER_ENTRY*)NextChosenEntry)->Flags);
              //---- Details submenu (kexts disabling etc) second level
              if (NextMenuExit == MENU_EXIT_ENTER || MenuExit == MENU_EXIT_DETAILS) {
                if (NextChosenEntry->SubScreen != NULL) {
                  UINTN DeepMenuExit = 0;
                  INTN DeepEntryIndex = -1;
                  REFIT_ABSTRACT_MENU_ENTRY    *DeepChosenEntry  = NULL;
                  while (!DeepMenuExit) {
                    DeepMenuExit = RunGenericMenu(NextChosenEntry->SubScreen, Style, &DeepEntryIndex, &DeepChosenEntry);
                    if (DeepMenuExit == MENU_EXIT_ESCAPE || DeepChosenEntry->getREFIT_MENU_ITEM_RETURN() ) {
                      DeepMenuExit = MENU_EXIT_ENTER;
                      NextMenuExit = 0;
                      break;
                    }
 //                   DBG(" get DeepChosenEntry FlagsBits = 0x%x\n", ((LOADER_ENTRY*)DeepChosenEntry)->Flags);
                  } //while(!DeepMenuExit)
                }
              }

            } //while(!NextMenuExit)
          }
        }
        //---------
      }
    }
  }

  if (ChosenEntry) {
    *ChosenEntry = MainChosenEntry;
  }
  return MenuExit;
}

