/*
 * File: AudioDemo.c
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

#include "AudioDemo.h"

EFI_STATUS
EFIAPI
AudioDemoMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable) {
    Print(L"AudioDemo start\n");

    // Create variables.
    EFI_STATUS Status;
    EFI_HANDLE *AudioIoHandles;
    UINTN AudioIoHandleCount;
    EFI_AUDIO_IO_PROTOCOL *AudioIo;

    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiAudioIoProtocolGuid, NULL, &AudioIoHandleCount, &AudioIoHandles);
    ASSERT_EFI_ERROR(Status);

    Status = gBS->OpenProtocol(AudioIoHandles[0], &gEfiAudioIoProtocolGuid, (VOID**)&AudioIo, NULL, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    ASSERT_EFI_ERROR(Status);

        EFI_HANDLE* handles = NULL;   
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
    EFI_FILE_INFO FileInfo;
    UINTN FileInfoSize;
    Status = token->GetInfo(token, &gEfiFileInfoGuid, &FileInfoSize, &FileInfo);
    ASSERT_EFI_ERROR(Status);

    Print(L"File size: %u bytes\n", FileInfo.FileSize);
    
    // Read file into buffer.
    UINTN bytesLength = FileInfo.FileSize;
    UINT8 *bytes = AllocateZeroPool(bytesLength);
    Status = token->Read(token, &bytesLength, bytes);
    ASSERT_EFI_ERROR(Status);


    return EFI_SUCCESS;
}
