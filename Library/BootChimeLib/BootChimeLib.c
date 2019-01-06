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
    OUT EFI_AUDIO_IO_PROTOCOL **AudioIo,
    OUT UINTN *Index) {

    // Create variables.
    EFI_STATUS Status;

    // Device Path.
    EFI_DEVICE_PATH_PROTOCOL *StoredDevicePath = NULL;
    UINTN StoredDevicePathSize = 0;
    EFI_DEVICE_PATH_PROTOCOL *DevicePath = NULL;

    // Audio I/O.
    EFI_HANDLE *AudioIoHandles = NULL;
    UINTN AudioIoHandleCount = 0;
    EFI_AUDIO_IO_PROTOCOL *AudioIoProto = NULL;

    // Output.
    UINTN OutputPortIndex;
    UINTN OutputPortIndexSize = sizeof(OutputPortIndex);

    // Check if parameters are valid.
    if (!AudioIo || !Index)
        return EFI_INVALID_PARAMETER;

    // Get Audio I/O protocols.
    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiAudioIoProtocolGuid, NULL, &AudioIoHandleCount, &AudioIoHandles);
    if (EFI_ERROR(Status))
        goto DONE;

    // Get stored device path size.
    Status = gRT->GetVariable(BOOT_CHIME_VAR_DEVICE, &gBootChimeVendorVariableGuid, NULL,
        &StoredDevicePathSize, NULL);
    if (EFI_ERROR(Status) && (Status != EFI_BUFFER_TOO_SMALL))
        goto DONE;

    // Allocate space for device path.
    StoredDevicePath = AllocateZeroPool(StoredDevicePathSize);
    if (!StoredDevicePath) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DONE;
    }

    // Get stored device path string.
    Status = gRT->GetVariable(BOOT_CHIME_VAR_DEVICE, &gBootChimeVendorVariableGuid, NULL,
        &StoredDevicePathSize, StoredDevicePath);
    if (EFI_ERROR(Status))
        goto DONE;

    // Try to find the matching device exposing an Audio I/O protocol.
    for (UINTN h = 0; h < AudioIoHandleCount; h++) {
        // Open Device Path protocol.
        Status = gBS->HandleProtocol(AudioIoHandles[h], &gEfiDevicePathProtocolGuid, (VOID**)&DevicePath);
        if (EFI_ERROR(Status))
            continue;

        // Compare Device Paths. If they match, we have our Audio I/O device.
        if (!CompareMem(StoredDevicePath, DevicePath, StoredDevicePathSize)) {
            // Open Audio I/O protocol.
            Status = gBS->HandleProtocol(AudioIoHandles[h], &gEfiAudioIoProtocolGuid, (VOID**)&AudioIoProto);
            if (EFI_ERROR(Status))
                goto DONE;
            break;
        }
    }

    // If the Audio I/O variable is still null, we couldn't find it.
    if (!AudioIoProto) {
        Status = EFI_NOT_FOUND;
        goto DONE;
    }

    // Get stored device index.
    Status = gRT->GetVariable(BOOT_CHIME_VAR_INDEX, &gBootChimeVendorVariableGuid, NULL,
        &OutputPortIndexSize, &OutputPortIndex);
    if (EFI_ERROR(Status))
        goto DONE;

    // Success.
    *AudioIo = AudioIoProto;
    *Index = OutputPortIndex;
    Status = EFI_SUCCESS;

DONE:
    if (AudioIoHandles)
        FreePool(AudioIoHandles);
    return Status;
}

EFI_STATUS
EFIAPI
BootChimeGetStoredVolume(
    OUT UINT8 *Volume) {

    // Create variables.
    EFI_STATUS Status;
    UINT8 OutputVolume;
    UINTN OutputVolumeSize = sizeof(OutputVolume);

    // Check if parameters are valid.
    if (!Volume)
        return EFI_INVALID_PARAMETER;

    // Get stored volume.
    Status = gRT->GetVariable(BOOT_CHIME_VAR_VOLUME, &gBootChimeVendorVariableGuid, NULL,
        &OutputVolumeSize, &OutputVolume);
    if (EFI_ERROR(Status))
        goto DONE;

    // Success.
    *Volume = OutputVolume;
    Status = EFI_SUCCESS;

DONE:
    return Status;
}

EFI_STATUS
EFIAPI
BootChimeGetDefaultOutput(
    OUT EFI_AUDIO_IO_PROTOCOL **AudioIo,
    OUT UINTN *Index) {
    // Create variables.
    EFI_STATUS Status;

    // Audio I/O.
    EFI_HANDLE *AudioIoHandles = NULL;
    UINTN AudioIoHandleCount = 0;
    EFI_AUDIO_IO_PROTOCOL *AudioIoProto = NULL;

    // Outputs.
    EFI_AUDIO_IO_PROTOCOL_PORT *OutputPorts = NULL;
    UINTN OutputPortsCount = 0;
    UINTN OutputPortIndex;

    // Check if parameters are valid.
    if (!AudioIo || !Index)
        return EFI_INVALID_PARAMETER;

    // Get Audio I/O protocols.
    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiAudioIoProtocolGuid, NULL, &AudioIoHandleCount, &AudioIoHandles);
    if (EFI_ERROR(Status))
        goto DONE;

    // Look through each Audio I/O protocol.
    for (UINTN h = 0; h < AudioIoHandleCount; h++) {
        // Open Audio I/O protocol.
        Status = gBS->HandleProtocol(AudioIoHandles[h], &gEfiAudioIoProtocolGuid, (VOID**)&AudioIoProto);
        if (EFI_ERROR(Status))
            continue;

        // Get output ports.
        Status = AudioIoProto->GetOutputs(AudioIoProto, &OutputPorts, &OutputPortsCount);
        if (EFI_ERROR(Status))
            continue;

        // Look for built-in speakers.
        for (UINTN o = 0; o < OutputPortsCount; o++) {
            if (OutputPorts[o].Device == EfiAudioIoDeviceSpeaker) {
                OutputPortIndex = o;
                goto FOUND_OUTPUT;
            }
        }

        // Look for line out.
        for (UINTN o = 0; o < OutputPortsCount; o++) {
            if (OutputPorts[o].Device == EfiAudioIoDeviceLine) {
                OutputPortIndex = o;
                goto FOUND_OUTPUT;
            }
        }

        // Free output ports array.
        FreePool(OutputPorts);
    }

    // If we get here, we couldn't find an output.
    Status = EFI_NOT_FOUND;
    goto DONE;

FOUND_OUTPUT:
    // Output was found.
    *AudioIo = AudioIoProto;
    *Index = OutputPortIndex;
    Status = EFI_SUCCESS;

DONE:
    if (AudioIoHandles)
        FreePool(AudioIoHandles);
    if (OutputPorts)
        FreePool(OutputPorts);
    return Status;
}
