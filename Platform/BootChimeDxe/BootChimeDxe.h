/*
 * File: BootChimeDxe.h
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

#ifndef _EFI_BOOT_CHIME_DXE_H_
#define _EFI_BOOT_CHIME_DXE_H_

// Common UEFI includes and library classes.
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BootChimeLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/WaveLib.h>

#include <Guid/FileInfo.h>

// Consumed protocols.
#include <Protocol/AudioIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/SimpleFileSystem.h>

#define ERROR_WAIT_TIME 5000000
#define AUDIO_FILE_NAME L"bootchime.wav"

//
// Functions.
//
BOOLEAN
EFIAPI
BootChimeIsAppleBootLoader(
    IN EFI_HANDLE ImageHandle);

EFI_STATUS
EFIAPI
BootChimeStartImage(
    IN  EFI_HANDLE ImageHandle,
    OUT UINTN *ExitDataSize,
    OUT CHAR16 **ExitData OPTIONAL);

EFI_STATUS
EFIAPI
BootChimeGetMemoryMap(
    IN OUT UINTN *MemoryMapSize,
    IN OUT EFI_MEMORY_DESCRIPTOR *MemoryMap,
    OUT    UINTN *MapKey,
    OUT    UINTN *DescriptorSize,
    OUT    UINT32 *DescriptorVersion);

EFI_STATUS
EFIAPI
BootChimeDxePlay(VOID);

EFI_STATUS
EFIAPI
BootChimeDxeMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable);

#endif
