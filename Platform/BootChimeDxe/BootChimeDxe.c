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

STATIC EFI_EXIT_BOOT_SERVICES mOrigExitBootServices;

STATIC BOOLEAN mPlayed = FALSE;

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

    // Create variables.
    EFI_STATUS Status;
    EFI_AUDIO_IO_PROTOCOL *AudioIo;
    UINTN OutputIndex;
    UINT8 OutputVolume;

    // Check to see if we have played the chime already, and if not, if
    // the loaded binary is the Apple boot.efi.
    if ((!mPlayed) && (BootChimeIsAppleBootLoader(ImageHandle))) {
        mPlayed = TRUE;

        // Get stored audio settings.
        Status = BootChimeGetStoredOutput(ImageHandle, &AudioIo, &OutputIndex, &OutputVolume);
        if (EFI_ERROR(Status)) {
            if (Status == EFI_NOT_FOUND) {
                Print(L"BootChimeDxe: A saved playback device couldn't be found, using default device.\n");
                Print(L"BootChimeDxe: Please run BootChimeCfg if the selected device is wrong.\n");
                Status = BootChimeGetDefaultOutput(ImageHandle, &AudioIo, &OutputIndex, &OutputVolume);
                if (EFI_ERROR(Status)) {
                    Print(L"BootChimeDxe: An error occurred getting the default device. Please run BootChimeCfg.\n");
                    goto DONE_ERROR;
                }
            } else {
                Print(L"BootChimeDxe: An error occurred fetching the stored settings. Please run BootChimeCfg.\n");
                goto DONE_ERROR;
            }
        }

        // Setup playback.
        Status = AudioIo->SetupPlayback(AudioIo, OutputIndex, OutputVolume,
            ChimeDataFreq, ChimeDataBits, ChimeDataChannels);
        if (EFI_ERROR(Status)) {
            Print(L"BootChimeDxe: Error setting up playback: %r\n", Status);
            goto DONE_ERROR;
        }

        // Start playback.
        Status = AudioIo->StartPlayback(AudioIo, ChimeData, ChimeDataLength, 0);
        if (EFI_ERROR(Status)) {
            Print(L"BootChimeDxe: Error starting playback: %r\n", Status);
            goto DONE_ERROR;
        }

        // Success.
        goto EXIT_BOOT_SERVICES;

DONE_ERROR:
        Print(L"BootChimeDxe: Pausing for 5 seconds...\n");
        gBS->Stall(ERROR_WAIT_TIME);
        goto EXIT_BOOT_SERVICES;
    }

EXIT_BOOT_SERVICES:
    return mOrigExitBootServices(ImageHandle, MapKey);
}

EFI_STATUS
EFIAPI
BootChimeDxeMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable) {
    DEBUG((DEBUG_INFO, "BootChimeDxeMain(): start\n"));

    // Replace ExitBootServices().
    mOrigExitBootServices = gBS->ExitBootServices;
    gBS->ExitBootServices = BootChimeExitBootServices;

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

    return EFI_SUCCESS;
}
