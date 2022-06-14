#ifndef _ILI9486_COMMANDS_H_
#define _ILI9486_COMMANDS_H_
/**
 * This command is an empty command; it does not have any effect on ILI9486L. However it can be used to terminate Frame
 * Memory Write or Read as described in RAMWR (Memory Write) and RAMRD (Memory Read) Commands.
 * X = Don’t care
 */
static constexpr uint8_t CMD_NOP = 0x00;
/**
 * When the Software Reset command is written, it causes software reset. It resets the commands and parameters to their
 * S/W Reset default values. (See default tables in each command description.)
 * The display is blank immediately
 * Note: The Frame Memory contents is kept or not by this command.
 * It will be necessary to wait 5msec before sending new command following software reset. The display module loads all
 * display supplier factory default values to the registers during this 5msec. If Software Reset is applied during Sleep Out
 * mode, it will be necessary to wait 120msec before sending Sleep out command. Software Reset Command cannot be sent
 * during Sleep Out sequence.
 */
static constexpr uint8_t CMD_SoftReset = 0x01;
/**
 * This read byte returns 24 bits display identification information.
 * The 1st parameter is dummy data.
 * The 2nd parameter (ID1 [7:0]): LCD module’s manufacturer ID.
 * The 3rd parameter (ID2 [7:0]): LCD module/driver version ID.
 * The 4th parameter (ID3 [7:0]): LCD module/driver ID.
 */
static constexpr uint8_t CMD_ReadID = 0x04;
static constexpr uint8_t CMD_ReadDSIError = 0x05;
static constexpr uint8_t CMD_ReadDisplayStatus = 0x09;
static constexpr uint8_t CMD_ReadDisplayPowerMode = 0x0A;
static constexpr uint8_t CMD_ReadDisplayMADCTL = 0x0B;
static constexpr uint8_t CMD_ReadPixelFormat = 0x0C;
static constexpr uint8_t CMD_ReadDisplayImageMode = 0x0D;
static constexpr uint8_t CMD_ReadDisplaySignalMode = 0x0E;
static constexpr uint8_t CMD_ReadDisplaySelfDiagnostic = 0x0F;
static constexpr uint8_t CMD_SleepIn = 0x10;
static constexpr uint8_t CMD_SleepOut = 0x11;
static constexpr uint8_t CMD_PartialModeOn = 0x12;
static constexpr uint8_t CMD_NormalModeOn = 0x13;
static constexpr uint8_t CMD_DisplayInversionOff = 0x20;
static constexpr uint8_t CMD_DisplayInversionOn = 0x21;
static constexpr uint8_t CMD_DisplayOff = 0x28;
static constexpr uint8_t CMD_DisplayOn = 0x29;
static constexpr uint8_t CMD_ColumnAddressSet = 0x2A;
static constexpr uint8_t CMD_PageAddressSet = 0x2B;
static constexpr uint8_t CMD_MemoryWrite = 0x2C;
static constexpr uint8_t CMD_MemoryRead = 0x2E;
static constexpr uint8_t CMD_PartialArea = 0x30;
static constexpr uint8_t CMD_VerticalScrollingDefinition = 0x33;
static constexpr uint8_t CMD_TearingEffectLineOff = 0x34;
static constexpr uint8_t CMD_TearingEffectLineOn = 0x35;
static constexpr uint8_t CMD_MemoryAccessControl = 0x36;
static constexpr uint8_t CMD_VerticalScrollingStartAddress = 0x37;
static constexpr uint8_t CMD_IdleModeOff = 0x38;
static constexpr uint8_t CMD_IdleModeOn = 0x39;
static constexpr uint8_t CMD_InterfacePixelFormat = 0x3A;
static constexpr uint8_t CMD_MemoryWriteContinue = 0x3C;
static constexpr uint8_t CMD_MemoryReadContinue = 0x3E;
static constexpr uint8_t CMD_WriteTearScanLine = 0x44;
static constexpr uint8_t CMD_ReadTearScanLine = 0x45;
static constexpr uint8_t CMD_WriteDisplayBrightnessValue = 0x51;
static constexpr uint8_t CMD_ReadDisplayBrightnessValue = 0x52;
static constexpr uint8_t CMD_WriteCTRLDisplayValue = 0x53;
static constexpr uint8_t CMD_ReadCTRLDisplayValue = 0x54;
static constexpr uint8_t CMD_WriteContentAdaptiveBrightness = 0x55;
static constexpr uint8_t CMD_ReadContentAdaptiveBrightness = 0x56;
static constexpr uint8_t CMD_CABCMinimumBrightness = 0x5E;
static constexpr uint8_t CMD_CABCMaximumBrightness = 0x5F;
static constexpr uint8_t CMD_ReadFirstChecksum = 0xAA;
static constexpr uint8_t CMD_ReadContinueChecksum = 0xAF;
static constexpr uint8_t CMD_ReadID1 = 0xDA;
static constexpr uint8_t CMD_ReadID2 = 0xDB;
static constexpr uint8_t CMD_ReadID3 = 0xDC;
static constexpr uint8_t CMD_InterfaceModeControl = 0xB0;
static constexpr uint8_t CMD_FrameRateControl_NormalMode = 0xB1;
static constexpr uint8_t CMD_FrameRateControl_IdleMode = 0xB2;
static constexpr uint8_t CMD_FrameRateControl_PartialMode = 0xB3;
static constexpr uint8_t CMD_DisplayInversionControl = 0xB4;
static constexpr uint8_t CMD_BlankingPorchControl = 0xB5;
static constexpr uint8_t CMD_DisplayFunctionControl = 0xB6;
static constexpr uint8_t CMD_EntryModeSet = 0xB7;
static constexpr uint8_t CMD_PowerControl1 = 0xC0;
static constexpr uint8_t CMD_PowerControl2 = 0xC1;
static constexpr uint8_t CMD_PowerControl3 = 0xC2;
static constexpr uint8_t CMD_PowerControl4 = 0xC3;
static constexpr uint8_t CMD_PowerControl5 = 0xC4;
static constexpr uint8_t CMD_VCOMControl1 = 0xC5;
static constexpr uint8_t CMD_CABCControl1 = 0xC6;
static constexpr uint8_t CMD_CABCControl2 = 0xC8;
static constexpr uint8_t CMD_CABCControl3 = 0xC9;
static constexpr uint8_t CMD_CABCControl4 = 0xCA;
static constexpr uint8_t CMD_CABCControl5 = 0xCB;
static constexpr uint8_t CMD_CABCControl6 = 0xCC;
static constexpr uint8_t CMD_CABCControl7 = 0xCD;
static constexpr uint8_t CMD_CABCControl8 = 0xCE;
static constexpr uint8_t CMD_CABCControl9 = 0xCF;
static constexpr uint8_t CMD_NVMemoryWrite = 0xD0;
static constexpr uint8_t CMD_NVMemoryProtectionKey = 0xD1;
static constexpr uint8_t CMD_NVMemoryStatusRead = 0xD2;
static constexpr uint8_t CMD_ReadID4 = 0xD3;
static constexpr uint8_t CMD_PositiveGammaControl = 0xE0;
static constexpr uint8_t CMD_NegativeGammaControl = 0xE1;
static constexpr uint8_t CMD_DisplayGammaControl1 = 0xE2;
static constexpr uint8_t CMD_DisplayGammaControl2 = 0xE3;
static constexpr uint8_t CMD_SPIReadCommandSetting = 0xFB;
static constexpr uint8_t CMD_Delay = 0x80;

static constexpr uint8_t initCommands_lovyan[] =
    {
        CMD_SoftReset, 2, 0x17,                   // VRH1 -- VREG1OUT is 5.00, used for positive gamma
        0x15,                                     // VRH2 -- VREG2OUT is -4.8750, used for negative gamma
        CMD_PowerControl2, 1, 0x41,               // VGH = Vci1x6, VGL = -Vci1 4. Note: To prevent the device damage, please keep VGH – DDVDH < 8V condition.
        CMD_VCOMControl1, 3, 0x00,                // nVM -- 0 : NV memory is not programmed
        0x12,                                     // VCM_REG [7:0] is used to set factor to generate VCOM voltage from the reference voltage VREG2OUT. -- VCOM = -1.71875
        0x80,                                     // VCM_REG_EN -- 1: VCOM value from VCM_REG [7:0].
        CMD_FrameRateControl_NormalMode, 1, 0xA0, // Frame rate = 62Hz
        CMD_DisplayInversionControl, 1, 0x02,     // Display Inversion Control = 2dot inversion
        CMD_DisplayFunctionControl, 3, 0x02,      // Nomal scan
        0x22,                                     // 5 frames
        0x3B,                                     // LCD Drive Line : 480
        CMD_EntryModeSet, 1, 0xC6,                //
                                                  //   CMD_ADJCTL3, 4, 0xA9,  // Adjust Control 3 ????
                                                  //   0x51,
                                                  //   0x2C,
                                                  //   0x82,

        CMD_PositiveGammaControl, 15, 0x0F, 0x1F, 0x1C, 0x0C, 0x0F, 0x08, 0x48, 0x98,
        0x37, 0x0A, 0x13, 0x04, 0x11, 0x0D, 0x00,

        CMD_NegativeGammaControl, 15, 0x0F, 0x32, 0x2E, 0x0B, 0x0D, 0x05, 0x47, 0x75,
        0x37, 0x06, 0x10, 0x03, 0x24, 0x20, 0x00,

        CMD_SleepOut, 0 + CMD_Delay, 120, // Exit sleep mode It will be necessary to wait 5msec before sending next command; this is to allow time for the supply voltages and clock circuits to stabilize
        CMD_IdleModeOff, 0,
        CMD_DisplayOn, 0 + CMD_Delay, 100,
        0xFF, 0xFF, // end
};

static constexpr uint8_t initCommands_bodmer_lovyan[] =
    {
        CMD_PowerControl1, 2, 0x17, // VRH1 -- VREG1OUT is 5.00, used for positive gamma
        0x15,                       // VRH2 -- VREG2OUT is -4.8750, used for negative gamma
        CMD_PowerControl2, 1, 0x41, // VGH = Vci1x6, VGL = -Vci1 4. Note: To prevent the device damage, please keep VGH – DDVDH < 8V condition.
        CMD_PowerControl3, 1, 0x44,
        CMD_VCOMControl1, 3, 0x00,                // nVM -- 0 : NV memory is not programmed
        0x12,                                     // VCM_REG [7:0] is used to set factor to generate VCOM voltage from the reference voltage VREG2OUT. -- VCOM = -1.71875
        0x80,                                     // VCM_REG_EN -- 1: VCOM value from VCM_REG [7:0].
        CMD_FrameRateControl_NormalMode, 1, 0xD0, // Frame rate = 90Hz
        CMD_InterfacePixelFormat, 1, 0x55,        // Interface pixel format 16-bit RGB
        CMD_MemoryAccessControl, 1, 0x48,         // Portrait
        CMD_PositiveGammaControl, 15, 0x0F, 0x1F, 0x1C, 0x0C, 0x0F, 0x08, 0x48, 0x98, 0x37, 0x0A, 0x13, 0x04, 0x11, 0x0D, 0x00,
        CMD_NegativeGammaControl, 15, 0x0F, 0x32, 0x2E, 0x0B, 0x0D, 0x05, 0x47, 0x75, 0x37, 0x06, 0x10, 0x03, 0x24, 0x20, 0x00,
        CMD_SleepOut, 0 + CMD_Delay, 120, // Exit sleep mode It will be necessary to wait 5msec before sending next command; this is to allow time for the supply voltages and clock circuits to stabilize
        CMD_IdleModeOff, 0,
        CMD_DisplayOn, 0 + CMD_Delay, 100,
        0xFF, 0xFF, // end
};

static constexpr uint8_t initCommands_bodmer[] =
    {
        CMD_SoftReset, 0 + CMD_Delay, 120,
        CMD_SleepOut, 0 + CMD_Delay, 120,
        CMD_InterfacePixelFormat, 1, 0x55,
        CMD_PowerControl3, 1, 0x44,
        CMD_VCOMControl1, 4, 0x00, 0x00, 0x00, 0x00,
        CMD_PositiveGammaControl, 15, 0x0F, 0x1F, 0x1C, 0x0C, 0x0F, 0x08, 0x48, 0x98, 0x37, 0x0A, 0x13, 0x04, 0x11, 0x0D, 0x00,
        CMD_NegativeGammaControl, 15, 0x0F, 0x32, 0x2E, 0x0B, 0x0D, 0x05, 0x47, 0x75, 0x37, 0x06, 0x10, 0x03, 0x24, 0x20, 0x00,
        CMD_DisplayInversionOff, 0,
        CMD_MemoryAccessControl, 1, 0x48,
        CMD_DisplayOn, 0 + CMD_Delay, 150,
        0xFF, 0xFF, // end
};

#endif