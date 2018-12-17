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
#include <Library/WaveLib.h>
#include <Library/ShellLib.h>

VOID callback(
    IN EFI_AUDIO_IO_PROTOCOL *AudioIo,
    IN VOID *Context) {
    Print(L"audio complete\n");
    EFI_HANDLE ImageHandle = (EFI_HANDLE)Context;

    gBS->Exit(ImageHandle, EFI_SUCCESS, 0, NULL);
}

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

    LIST_ENTRY *Package;
    CHAR16 *ProblemParam;
    Status = ShellCommandLineParse(EmptyParamList, &Package, &ProblemParam, TRUE);
    ASSERT_EFI_ERROR(Status);

    Print(L"%u command line args\n", ShellCommandLineGetCount(Package));

    // Get filename.
    CHAR16 *filename = (CHAR16*)ShellCommandLineGetRawValue(Package, 1);

    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiAudioIoProtocolGuid, NULL, &AudioIoHandleCount, &AudioIoHandles);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "audio handles %u\n", AudioIoHandleCount));

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

        Status = root->Open(root, &token, filename, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
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
    UINTN bytesLength = FileInfo->FileSize;
    UINT8 *bytes = AllocateZeroPool(bytesLength);
    Status = token->Read(token, &bytesLength, bytes);
    ASSERT_EFI_ERROR(Status);

    // Read WAVE.
    WAVE_FILE_DATA WaveData;
    Status = WaveGetFileData(bytes, (UINT32)bytesLength, &WaveData);
    ASSERT_EFI_ERROR(Status);
    Print(L"data length: %u bytes\n", WaveData.DataLength);
    Print(L"Format length: %u bytes\n", WaveData.FormatLength);
    Print(L"  Channels: %u  Sample rate: %u Hz  Bits: %u\n", WaveData.Format->Channels, WaveData.Format->SamplesPerSec, WaveData.Format->BitsPerSample);
    Print(L"Samples length: %u bytes\n", WaveData.SamplesLength);

    EFI_AUDIO_IO_PROTOCOL_BITS bits;
    switch (WaveData.Format->BitsPerSample) {
        case 16:
            bits = EfiAudioIoBits16;
            break;

        case 20:
            bits = EfiAudioIoBits20;
            break;

        case 24:
            bits = EfiAudioIoBits24;
            break;

        default:
            return EFI_UNSUPPORTED;
    }

    EFI_AUDIO_IO_PROTOCOL_FREQ freq;
    switch (WaveData.Format->SamplesPerSec) {
        case 44100:
            freq = EfiAudioIoFreq44kHz;
            break;

        case 48000:
            freq = EfiAudioIoFreq48kHz;
            break;

        default:
            return EFI_UNSUPPORTED;
    }

    //gBS->Stall(10000000);

    //return EFI_SUCCESS;

    for (UINT8 a = 0; a < AudioIoHandleCount; a++) {

    Status = gBS->OpenProtocol(AudioIoHandles[a], &gEfiAudioIoProtocolGuid, (VOID**)&AudioIo, NULL, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    ASSERT_EFI_ERROR(Status);



    // Get outputs.
    EFI_AUDIO_IO_PROTOCOL_PORT *Outputs;
    UINTN OutputsCount;
    Status = AudioIo->GetOutputs(AudioIo, &Outputs, &OutputsCount);
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
    }

    if (!hasHP)
        continue;

    for (UINT8 i = 0; i < OutputsCount; i++) {
        Print(L"Output %u: %s @ %s %s\n", i, Devices[Outputs[i].Device],
            Locations[Outputs[i].Location], Surfaces[Outputs[i].Surface]);

        // Play audio.
        Print(L"Now playing audio at 90%% volume...\n");
        Status = AudioIo->SetupPlayback(AudioIo, i, 90, freq, bits, (UINT8)WaveData.Format->Channels);
        ASSERT_EFI_ERROR(Status);

        Status = AudioIo->StartPlayback(AudioIo, WaveData.Samples, WaveData.SamplesLength, 0);// (SIZE_1MB * 4) + 0x40000, 0);
        ASSERT_EFI_ERROR(Status);

        //gBS->Stall(10000000);

        // Play audio at 80%.
        Print(L"Now playing audio at 60%% volume...\n");
        Status = AudioIo->SetupPlayback(AudioIo, i, 60, freq, bits, (UINT8)WaveData.Format->Channels);
        ASSERT_EFI_ERROR(Status);

        Status = AudioIo->StartPlayback(AudioIo, WaveData.Samples, WaveData.SamplesLength, 0);// (SIZE_1MB * 4) + 0x40000, 0);
        ASSERT_EFI_ERROR(Status);

        //gBS->Stall(10000000);
    }
    }

    // Play audio.
      //  Status = AudioIo->SetupPlayback(AudioIo, 0, 75, EfiAudioIoFreq44kHz, EfiAudioIoBits16, 2);
    //    ASSERT_EFI_ERROR(Status);

    // play async.
  //  Status = AudioIo->StartPlaybackAsync(AudioIo, bytes, bytesLength, 0, callback, ImageHandle);
  //  ASSERT_EFI_ERROR(Status);

   // while(TRUE);
    return EFI_SUCCESS;
}
