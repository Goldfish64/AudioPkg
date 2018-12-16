/*
 * File: BootChimeDxe.c
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

#include "BootChimeDxe.h"

STATIC EFI_IMAGE_START mOrigStartImage;
STATIC EFI_EXIT_BOOT_SERVICES mOrigExitBootServices;
STATIC EFI_AUDIO_IO_PROTOCOL *AudioIo;
//STATIC UINTN bytesLength;
 //   STATIC UINT8 *bytes;

STATIC EFI_EVENT protNot;
STATIC VOID* reg;

STATIC BOOLEAN played = FALSE;

STATIC UINT8 index = 0;

BOOLEAN
EFIAPI
BootChimeIsAppleBootLoader(
    IN EFI_HANDLE ImageHandle) {
    EFI_STATUS Status;
    EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
    EFI_DEVICE_PATH_PROTOCOL *DevicePath;
    FILEPATH_DEVICE_PATH *LastPathNode = NULL;
    UINTN PathLen;
    UINTN BootPathLen = sizeof("boot.efi") - 1;
    CONST CHAR16 *BootPathName;

    // Open Loaded Image protocol.
    Status = gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&LoadedImage);
    if (EFI_ERROR(Status))
        return FALSE;

    // Get end of path.
    DevicePath = LoadedImage->FilePath;
    while(!IsDevicePathEnd(DevicePath)) {
        if ((DevicePath->Type == MEDIA_DEVICE_PATH) && (DevicePath->SubType == MEDIA_FILEPATH_DP))
            LastPathNode = (FILEPATH_DEVICE_PATH*)DevicePath;
        DevicePath = NextDevicePathNode(DevicePath);
    }

    if (LastPathNode != NULL) {
        DEBUG((DEBUG_INFO, "Path: %s\n", LastPathNode->PathName));
        PathLen = StrLen(LastPathNode->PathName);
        BootPathName = LastPathNode->PathName + PathLen - BootPathLen;
        if (PathLen >= BootPathLen) {
            if (((PathLen == BootPathLen) || (*(BootPathName - 1) == L'\\')) && (StrCmp(BootPathName, L"boot.efi") == 0))
                return TRUE;
        }
    }

    return FALSE;
}

EFI_STATUS
EFIAPI
BootChimeExitBootServices(
    IN EFI_HANDLE ImageHandle,
    IN UINTN MapKey) {
    DEBUG((DEBUG_INFO, "BootChimeExitBootServices(): start\n"));
    EFI_STATUS Status;

    if ((!played) && (BootChimeIsAppleBootLoader(ImageHandle))) {
        played = TRUE;

        // Setup playback.
        Status = AudioIo->SetupPlayback(AudioIo, index, 80, EfiAudioIoFreq48kHz, EfiAudioIoBits16, 2);
        if (EFI_ERROR(Status)) {
            Print(L"BootChimeDxe: Error setting up playback: 0x%X\n", Status);
            goto EXIT_BOOT_SERVICES;
        }

        // Start playback.
        Status = AudioIo->StartPlayback(AudioIo, ChimeData, ChimeDataLength, 0);
        if (EFI_ERROR(Status)) {
            Print(L"BootChimeDxe: Error starting playback: 0x%X\n", Status);
            goto EXIT_BOOT_SERVICES;
        }
    }

EXIT_BOOT_SERVICES:
    return mOrigExitBootServices(ImageHandle, MapKey);
}

VOID
EFIAPI
BootChimeNotify(
IN EFI_EVENT Event,
    IN VOID *Context) {
    EFI_STATUS Status;
    EFI_HANDLE ImageHandle = (EFI_HANDLE)Context;
    EFI_HANDLE AudioIoHandle;
    UINTN AudioIoHandleCount;
    EFI_AUDIO_IO_PROTOCOL *dd;

    Status = gBS->LocateHandle(ByProtocol, &gEfiAudioIoProtocolGuid, reg, &AudioIoHandleCount, &AudioIoHandle);
    if (EFI_ERROR(Status))
        return;

    Status = gBS->OpenProtocol(AudioIoHandle, &gEfiAudioIoProtocolGuid, (VOID**)&dd, NULL, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    ASSERT_EFI_ERROR(Status);



    // Get outputs.
    EFI_AUDIO_IO_PROTOCOL_PORT *Outputs;
    UINTN OutputsCount;
    Status = dd->GetOutputs(dd, &Outputs, &OutputsCount);
    ASSERT_EFI_ERROR(Status);

    CHAR16 *Devices[EfiAudioIoDeviceMaximum] = { L"Line", L"Speaker", L"Headphones", L"SPDIF", L"Mic", L"Other" };
    CHAR16 *Locations[EfiAudioIoLocationMaximum] = { L"N/A", L"rear", L"front", L"left", L"right", L"top", L"bottom", L"other" };
    CHAR16 *Surfaces[EfiAudioIoSurfaceMaximum] = { L"external", L"internal", L"other" };

    BOOLEAN hasHP = FALSE;
    for (UINTN i = 0; i < OutputsCount; i++) {
        Print(L"Output %u: %s @ %s %s\n", i, Devices[Outputs[i].Device],
            Locations[Outputs[i].Location], Surfaces[Outputs[i].Surface]);
        if (Outputs[i].Device == EfiAudioIoDeviceHeadphones)
            hasHP = TRUE;

        if (Outputs[i].Device == EfiAudioIoDeviceSpeaker)
            index = i;
    }

    if (!hasHP)
        return;

    AudioIo = dd;
}

EFI_STATUS
EFIAPI
BootChimeDxeMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable) {
    Print(L"BootChimeDxe start\n");

    // Replace ImageStart.
   // mOrigStartImage = gBS->StartImage;
   // gBS->StartImage = BootChimeStartImage;
    mOrigExitBootServices = gBS->ExitBootServices;
    gBS->ExitBootServices = BootChimeExitBootServices;

    // Create variables.
    EFI_STATUS Status;
    EFI_HANDLE *AudioIoHandles;
    UINTN AudioIoHandleCount;

    Status = gBS->CreateEvent(EVT_NOTIFY_SIGNAL, TPL_NOTIFY, BootChimeNotify, ImageHandle, &protNot);
    ASSERT_EFI_ERROR(Status);
    Status = gBS->RegisterProtocolNotify(&gEfiAudioIoProtocolGuid, protNot, &reg);
    ASSERT_EFI_ERROR(Status);

    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiAudioIoProtocolGuid, NULL, &AudioIoHandleCount, &AudioIoHandles);
   // ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "audio handles %u\n", AudioIoHandleCount));

   /* EFI_HANDLE* handles = NULL;   
    UINTN handleCount = 0;

    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &handleCount, &handles);
    ASSERT_EFI_ERROR(Status);
    
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs = NULL;
    DEBUG((DEBUG_INFO, "Handles %u\n", handleCount));
    EFI_FILE_PROTOCOL* root = NULL;

    // opewn file.
    EFI_FILE_PROTOCOL* token = NULL;
    for (UINTN handle = 0; handle < handleCount; handle++) {
        Status = gBS->HandleProtocol(handles[handle], &gEfiSimpleFileSystemProtocolGuid, (void**)&fs);
        ASSERT_EFI_ERROR(Status);

        Status = fs->OpenVolume(fs, &root);
        ASSERT_EFI_ERROR(Status);

        Status = root->Open(root, &token, L"quadra.raw", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
        if (!(EFI_ERROR(Status)))
            break;
    }

    // Get size.
    EFI_FILE_INFO *FileInfo;

    UINTN FileInfoSize = 512;
    VOID *fileinfobuf = AllocateZeroPool(FileInfoSize);
    Status = token->GetInfo(token, &gEfiFileInfoGuid, &FileInfoSize, fileinfobuf);
    ASSERT_EFI_ERROR(Status);

    FileInfo = (EFI_FILE_INFO*)fileinfobuf;

    Print(L"File size: %u bytes\n", FileInfo->FileSize);
    
    // Read file into buffer.
    bytesLength = FileInfo->FileSize;
    bytes = AllocateZeroPool(bytesLength);
    Status = token->Read(token, &bytesLength, bytes);
    ASSERT_EFI_ERROR(Status);*/

    EFI_AUDIO_IO_PROTOCOL *dd;

    for (UINT8 a = 0; a < AudioIoHandleCount; a++) {

    Status = gBS->OpenProtocol(AudioIoHandles[a], &gEfiAudioIoProtocolGuid, (VOID**)&dd, NULL, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    ASSERT_EFI_ERROR(Status);



    // Get outputs.
    EFI_AUDIO_IO_PROTOCOL_PORT *Outputs;
    UINTN OutputsCount;
    Status = dd->GetOutputs(dd, &Outputs, &OutputsCount);
    ASSERT_EFI_ERROR(Status);

    CHAR16 *Devices[EfiAudioIoDeviceMaximum] = { L"Line", L"Speaker", L"Headphones", L"SPDIF", L"Mic", L"Other" };
    CHAR16 *Locations[EfiAudioIoLocationMaximum] = { L"N/A", L"rear", L"front", L"left", L"right", L"top", L"bottom", L"other" };
    CHAR16 *Surfaces[EfiAudioIoSurfaceMaximum] = { L"external", L"internal", L"other" };

    BOOLEAN hasHP = FALSE;
    for (UINTN i = 0; i < OutputsCount; i++) {
        Print(L"Output %u: %s @ %s %s\n", i, Devices[Outputs[i].Device],
            Locations[Outputs[i].Location], Surfaces[Outputs[i].Surface]);
        if (Outputs[i].Device == EfiAudioIoDeviceHeadphones)
            hasHP = TRUE;

        if (Outputs[i].Device == EfiAudioIoDeviceSpeaker)
            index = i;
    }

    if (!hasHP)
        continue;

    AudioIo = dd;

    // Play audio.
        
    }
    return EFI_SUCCESS;
}
