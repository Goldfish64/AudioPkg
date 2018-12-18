/*
 * File: BootChimeLib.c
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

// Libraries.
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BootChimeLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

// Consumed protocols.
#include <Protocol/AudioIo.h>
#include <Protocol/DevicePath.h>

//
// Boot Chime vendor variable GUID.
//
EFI_GUID gBootChimeVendorVariableGuid = BOOT_CHIME_VENDOR_VARIABLE_GUID;

EFI_STATUS
EFIAPI
BootChimeGetStoredOutput(
    IN  EFI_HANDLE ImageHandle,
    OUT EFI_AUDIO_IO_PROTOCOL **AudioIo,
    OUT UINTN *Index,
    OUT UINT8 *Volume) {
    // Create variables.
    EFI_STATUS Status;

    // Device Path.
    CHAR16 *StoredDevicePathStr = NULL;
    UINTN StoredDevicePathStrSize = 0;
    EFI_DEVICE_PATH_PROTOCOL *DevicePath = NULL;
    CHAR16 *DevicePathStr = NULL;

    // Audio I/O.
    EFI_HANDLE *AudioIoHandles = NULL;
    UINTN AudioIoHandleCount = 0;
    EFI_AUDIO_IO_PROTOCOL *AudioIoProto = NULL;

    // Output.
    UINTN OutputPortIndex;
    UINTN OutputPortIndexSize = sizeof(OutputPortIndex);
    UINT8 OutputVolume;
    UINTN OutputVolumeSize = sizeof(OutputVolume);

    // Get Audio I/O protocols.
    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiAudioIoProtocolGuid, NULL, &AudioIoHandleCount, &AudioIoHandles);
    if (EFI_ERROR(Status))
        goto DONE;

    // Get stored device path string size.
    Status = gRT->GetVariable(BOOT_CHIME_VAR_DEVICE, &gBootChimeVendorVariableGuid, NULL,
        &StoredDevicePathStrSize, NULL);
    if (EFI_ERROR(Status) && (Status != EFI_BUFFER_TOO_SMALL))
        goto DONE;

    // Allocate space for device path string.
    StoredDevicePathStr = AllocateZeroPool(StoredDevicePathStrSize);
    if (StoredDevicePathStr == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DONE;
    }

    // Get stored device path string.
    Status = gRT->GetVariable(BOOT_CHIME_VAR_DEVICE, &gBootChimeVendorVariableGuid, NULL,
        &StoredDevicePathStrSize, StoredDevicePathStr);
    if (EFI_ERROR(Status))
        goto DONE;

    // Try to find the matching device exposing an Audio I/O protocol.
    for (UINTN h = 0; h < AudioIoHandleCount; h++) {
        // Open Device Path protocol.
        Status = gBS->OpenProtocol(AudioIoHandles[h], &gEfiDevicePathProtocolGuid, (VOID**)&DevicePath,
            NULL, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if (EFI_ERROR(Status))
            continue;

        // Convert Device Path to string for comparison.
        DevicePathStr = ConvertDevicePathToText(DevicePath, FALSE, FALSE);
        if (DevicePathStr == NULL)
            continue;

        // Compare Device Paths. If they match, we have our Audio I/O device.
        if (StrCmp(StoredDevicePathStr, DevicePathStr) == 0) {
            // Open Audio I/O protocol.
            Status = gBS->OpenProtocol(AudioIoHandles[h], &gEfiAudioIoProtocolGuid, (VOID**)&AudioIoProto,
                NULL, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
            if (EFI_ERROR(Status))
                goto DONE;
            break;
        }
        FreePool(DevicePathStr);
        DevicePathStr = NULL;
    }

    // If the Audio I/O variable is still null, we couldn't find it.
    if (AudioIoProto == NULL) {
        Status = EFI_NOT_FOUND;
        goto DONE;
    }

    // Get stored device index.
    Status = gRT->GetVariable(BOOT_CHIME_VAR_INDEX, &gBootChimeVendorVariableGuid, NULL,
        &OutputPortIndexSize, &OutputPortIndex);
    if (EFI_ERROR(Status))
        goto DONE;

    // Get stored volume. If this fails, just use the max.
    Status = gRT->GetVariable(BOOT_CHIME_VAR_VOLUME, &gBootChimeVendorVariableGuid, NULL,
        &OutputVolumeSize, &OutputVolume);
    if (EFI_ERROR(Status)) {
        OutputVolume = EFI_AUDIO_IO_PROTOCOL_MAX_VOLUME;
        Status = EFI_SUCCESS;
    }

    // Success.
    *AudioIo = AudioIoProto;
    *Index = OutputPortIndex;
    *Volume = OutputVolume;
    Status = EFI_SUCCESS;

DONE:
    if (StoredDevicePathStr != NULL)
        FreePool(StoredDevicePathStr);
    if (AudioIoHandles != NULL)
        FreePool(AudioIoHandles);
    if (DevicePathStr != NULL)
        FreePool(DevicePathStr);
    return Status;
}
