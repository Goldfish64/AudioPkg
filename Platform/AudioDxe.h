/*
 * File: AudioDxe.h
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

#ifndef __EFI_AUDIODXE_H__
#define __EFI_AUDIODXE_H__

//
// Common UEFI includes and library classes.
//
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/SynchronizationLib.h>
#include <Protocol/DevicePathUtilities.h>

//
// Proctols that are consumed.
//
#include <Protocol/DevicePath.h>
#include <Protocol/DriverBinding.h>

// Driver version
#define AUDIODXE_VERSION    0xA

// Driver Bindings.
EFI_DRIVER_BINDING_PROTOCOL gHdaControllerDriverBinding;
EFI_DRIVER_BINDING_PROTOCOL gHdaCodecDriverBinding;

// Vendor IDs.
#define VEN_AMD_ID      0x1002
#define VEN_IDT_ID      0x111D
#define VEN_INTEL_ID    0x8086
#define VEN_NVIDIA_ID   0x10DE
#define VEN_QEMU_ID     0x1AF4
#define VEN_REALTEK_ID  0x10EC
#define VEN_INVALID_ID  0xFFFF

#define GET_PCI_VENDOR_ID(a)    (a & 0xFFFF)
#define GET_PCI_DEVICE_ID(a)    ((a >> 16) & 0xFFFF)
#define GET_CODEC_VENDOR_ID(a)  ((a >> 16) & 0xFFFF)
#define GET_CODEC_DEVICE_ID(a)  (a & 0xFFFF)

// Vendor strings.
#define VEN_INTEL_CONTROLLER_STR    L"Intel HDA Controller"
#define VEN_NVIDIA_CONTROLLER_STR   L"NVIDIA HDA Controller"
#define VEN_AMD_CONTROLLER_STR      L"AMD HDA Controller"
#define VEN_INVALID_CONTROLLER_STR  L"HDA Controller"

#define VEN_AMD_STR         L"AMD"
#define VEN_IDT_STR         L"IDT"
#define VEN_INTEL_STR       L"Intel"
#define VEN_NVIDIA_STR      L"NVIDIA"
#define VEN_QEMU_STR        L"QEMU"
#define VEN_REALTEK_STR     L"Realtek"
#define HDA_CODEC_STR       L"HDA Codec"

CHAR16*
EFIAPI
AudioDxeGetCodecName(
    IN UINT16 VendorId,
    IN UINT16 DeviceId);

#endif
