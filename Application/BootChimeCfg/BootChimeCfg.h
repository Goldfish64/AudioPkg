/*
 * File: BootChimeCfg.h
 * 
 * Description: BootChimeDxe configuration application.
 *
 * Copyright (c) 2018 John Davis
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
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BootChimeLib.h>

#include <Guid/FileInfo.h>

// Consumed protocols.
#include <Protocol/AudioIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DevicePathUtilities.h>
#include <Library/DevicePathLib.h>

#define BCFG_NAME       (L"BOOTCHIMECFG.EFI")
#define BCFG_ARG_HELP   (L"-?")
#define BCFG_ARG_LIST   (L"-l")
#define BCFG_ARG_SELECT (L"-s")
#define BCFG_ARG_VOLUME (L"-v")
#define BCFG_ARG_TEST   (L"-t")
#define BCFG_ARG_CLEAR  (L"-x")

typedef struct {
    EFI_AUDIO_IO_PROTOCOL *AudioIo;
    EFI_DEVICE_PATH_PROTOCOL *DevicePath;
    EFI_AUDIO_IO_PROTOCOL_PORT OutputPort;
    UINTN OutputPortIndex;
} BOOT_CHIME_DEVICE;

#endif
