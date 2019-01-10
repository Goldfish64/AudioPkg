/*
 * File: BootChimeCfg.h
 * 
 * Description: BootChimeDxe configuration application.
 *
 * Copyright (c) 2018-2019 John Davis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _EFI_BOOT_CHIME_CFG_H_
#define _EFI_BOOT_CHIME_CFG_H_

// Common UEFI includes and library classes.
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BootChimeLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>

// Consumed protocols.
#include <Protocol/AudioIo.h>
#include <Protocol/DevicePath.h>

#define PROMPT_ANY_KEY  L"Press any key to continue..."
#define BCFG_ARG_LIST   L'L'
#define BCFG_ARG_SELECT L'S'
#define BCFG_ARG_VOLUME L'V'
#define BCFG_ARG_TEST   L'T'
#define BCFG_ARG_CLEAR  L'X'
#define BCFG_ARG_QUIT   L'Q'

#define MAX_CHARS       12

// Boot chime output device.
typedef struct {
    EFI_AUDIO_IO_PROTOCOL *AudioIo;
    EFI_DEVICE_PATH_PROTOCOL *DevicePath;
    EFI_AUDIO_IO_PROTOCOL_PORT OutputPort;
    UINTN OutputPortIndex;
} BOOT_CHIME_DEVICE;

//
// Functions.
//
VOID
EFIAPI
FlushKeystrokes(
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *SimpleTextIn);

EFI_STATUS
EFIAPI
WaitForKey(
    IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *SimpleTextIn,
    OUT CHAR16 *KeyValue);

EFI_STATUS
EFIAPI
GetOutputDevices(
    OUT BOOT_CHIME_DEVICE **Devices,
    OUT UINTN *DevicesCount);

EFI_STATUS
EFIAPI
PrintDevices(
    IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *SimpleTextIn,
    IN BOOT_CHIME_DEVICE *Devices,
    IN UINTN DevicesCount);

EFI_STATUS
EFIAPI
SelectDevice(
    IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *SimpleTextIn,
    IN BOOT_CHIME_DEVICE *Devices,
    IN UINTN DevicesCount);

EFI_STATUS
EFIAPI
SelectVolume(
    IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *SimpleTextIn);

EFI_STATUS
EFIAPI
TestOutput(VOID);

EFI_STATUS
EFIAPI
ClearVars(VOID);

VOID
EFIAPI
DisplayMenu(VOID);

EFI_STATUS
EFIAPI
BootChimeCfgMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable);

#endif
