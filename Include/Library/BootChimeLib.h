/*
 * File: BootChimeLib.h
 *
 * Description: Boot Chime library.
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

#ifndef _EFI_CHIME_DATA_LIB_H_
#define _EFI_CHIME_DATA_LIB_H_

#include <Uefi.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Protocol/AudioIo.h>

// BootChime vendor variable GUID.
#define BOOT_CHIME_VENDOR_VARIABLE_GUID { \
    0x89D4F995, 0x67E3, 0x4895, { 0x8F, 0x18, 0x45, 0x4B, 0x65, 0x1D, 0x92, 0x15 } \
}
extern EFI_GUID gBootChimeVendorVariableGuid;
#define BOOT_CHIME_VAR_ATTRIBUTES   (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)
#define BOOT_CHIME_VAR_DEVICE       (L"Device")
#define BOOT_CHIME_VAR_INDEX        (L"Index")
#define BOOT_CHIME_VAR_VOLUME       (L"Volume")

// Chime data.
extern UINT8 ChimeData[];
extern UINTN ChimeDataLength;
extern UINT8 ChimeDataChannels;
extern EFI_AUDIO_IO_PROTOCOL_BITS ChimeDataBits;
extern EFI_AUDIO_IO_PROTOCOL_FREQ ChimeDataFreq;

EFI_STATUS
EFIAPI
BootChimeGetStoredOutput(
    IN  EFI_HANDLE ImageHandle,
    OUT EFI_AUDIO_IO_PROTOCOL **AudioIo,
    OUT UINTN *Index,
    OUT UINT8 *Volume);

#endif
