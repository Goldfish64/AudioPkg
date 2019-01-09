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

// Original Boot Services functions.
STATIC EFI_IMAGE_START mOrigStartImage;
STATIC EFI_EXIT_BOOT_SERVICES mOrigExitBootServices;

// Audio file data.
STATIC UINT8 *mSoundData;
STATIC UINTN mSoundDataLength;
STATIC EFI_AUDIO_IO_PROTOCOL_FREQ mSoundFreq;
STATIC EFI_AUDIO_IO_PROTOCOL_BITS mSoundBits;
STATIC UINT8 mSoundChannels;

STATIC BOOLEAN mIsAppleBoot;
STATIC BOOLEAN mPlaybackComplete;

VOID
EFIAPI
BootChimeAudioIoCallback(
    IN EFI_AUDIO_IO_PROTOCOL *AudioIo,
    IN VOID *Context) {
    DEBUG((DEBUG_INFO, "BootChimeAudioIoCallback(): start\n"));
    *((BOOLEAN*)Context) = TRUE;
}

BOOLEAN
EFIAPI
BootChimeIsAppleBootLoader(
    IN EFI_HANDLE ImageHandle) {
    DEBUG((DEBUG_INFO, "BootChimeIsAppleBootLoader(%lx): start\n", ImageHandle));

    // Create variables.
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

    // Check to see if path matches up with where boot.efi would be.
    if (LastPathNode) {
        DEBUG((DEBUG_INFO, "Path: %s\n", LastPathNode->PathName));
        PathLen = StrLen(LastPathNode->PathName);
        BootPathName = LastPathNode->PathName + PathLen - BootPathLen;
        if (PathLen >= BootPathLen) {
            if (((PathLen == BootPathLen) || (*(BootPathName - 1) == L'\\')) && (StrCmp(BootPathName, L"boot.efi") == 0)) {
                DEBUG((DEBUG_INFO, "BootChimeIsAppleBootLoader(%lx): image is Apple boot.efi\n", ImageHandle));
                return TRUE;
            }
        }
    }
    return FALSE;
}

EFI_STATUS
EFIAPI
BootChimeStartImage(
    IN  EFI_HANDLE ImageHandle,
    OUT UINTN *ExitDataSize,
    OUT CHAR16 **ExitData OPTIONAL) {
    DEBUG((DEBUG_INFO, "BootChimeStartImage(%lx): start\n", ImageHandle));

    // Create variables.
    EFI_STATUS Status;

    // If the image being loaded is boot.efi, start playback of chime.
    if (!mIsAppleBoot && BootChimeIsAppleBootLoader(ImageHandle)) {
        Status = BootChimeDxePlay();
        if (!EFI_ERROR(Status))
            mIsAppleBoot = TRUE;
    }

    // Call original StartImage.
    return mOrigStartImage(ImageHandle, ExitDataSize, ExitData);
}

EFI_STATUS
EFIAPI
BootChimeExitBootServices(
    IN EFI_HANDLE ImageHandle,
    IN UINTN MapKey) {
    DEBUG((DEBUG_INFO, "BootChimeExitBootServices(): start\n"));

    // If playback is in progress, wait.
    if (mIsAppleBoot) {
        while (!mPlaybackComplete)
            CpuPause();
        mIsAppleBoot = FALSE;
    }

    // Call original ExitBootServices.
    return mOrigExitBootServices(ImageHandle, MapKey);
}

EFI_STATUS
EFIAPI
BootChimeDxePlay(VOID) {
    DEBUG((DEBUG_INFO, "BootChimeDxePlay(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_AUDIO_IO_PROTOCOL *AudioIo;
    UINTN OutputIndex;
    UINT8 OutputVolume;

    // Get stored audio output.
    Status = BootChimeGetStoredOutput(&AudioIo, &OutputIndex);
    if (EFI_ERROR(Status)) {
        if (Status == EFI_NOT_FOUND) {
            Print(L"BootChimeDxe: Using default device. Please run BootChimeCfg if the selected device is wrong.\n");
            Status = BootChimeGetDefaultOutput(&AudioIo, &OutputIndex);
            if (EFI_ERROR(Status)) {
                if (Status == EFI_NOT_FOUND)
                    Print(L"BootChimeDxe: No audio outputs were found. Ensure AudioDxe is loaded.\n");
                else
                    Print(L"BootChimeDxe: An error occurred getting the default device. Please run BootChimeCfg.\n");
                goto DONE_ERROR;
            }
        } else {
            Print(L"BootChimeDxe: An error occurred fetching the stored output. Please run BootChimeCfg.\n");
            goto DONE_ERROR;
        }
    }

    // Get stored audio volume.
    Status = BootChimeGetStoredVolume(&OutputVolume);
    if (EFI_ERROR(Status)) {
        if (Status == EFI_NOT_FOUND) {
            Print(L"BootChimeDxe: Using default volume. Please run BootChimeCfg if the volume is too high.\n");
            OutputVolume = BOOT_CHIME_DEFAULT_VOLUME;
            Status = EFI_SUCCESS;
        } else {
            Print(L"BootChimeDxe: An error occurred fetching the stored volume. Please run BootChimeCfg.\n");
            goto DONE_ERROR;
        }
    }

    // Setup playback.
    Status = AudioIo->SetupPlayback(AudioIo, OutputIndex, OutputVolume,
        mSoundFreq, mSoundBits, mSoundChannels);
    if (EFI_ERROR(Status)) {
        Print(L"BootChimeDxe: Error setting up playback: %r\n", Status);
        goto DONE_ERROR;
    }

    // Start playback.
    mPlaybackComplete = FALSE;
    Status = AudioIo->StartPlaybackAsync(AudioIo, mSoundData, mSoundDataLength, 0, BootChimeAudioIoCallback, &mPlaybackComplete);
    if (EFI_ERROR(Status)) {
        Print(L"BootChimeDxe: Error starting playback: %r\n", Status);
        goto DONE_ERROR;
    }

    // Success.
    return EFI_SUCCESS;

DONE_ERROR:
    Print(L"BootChimeDxe: Pausing for 5 seconds...\n");
    gBS->Stall(ERROR_WAIT_TIME);
    return Status;
}

EFI_STATUS
EFIAPI
BootChimeDxeMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable) {
    DEBUG((DEBUG_INFO, "BootChimeDxeMain(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_TPL OldTpl;

    // Image info.
    EFI_DEVICE_PATH_PROTOCOL *LoadedImageDevicePath = NULL;
    EFI_DEVICE_PATH_PROTOCOL *DevicePath = NULL;
    EFI_DEVICE_PATH_PROTOCOL *ParentDeviceNode = NULL;

    // Filesystem.
    EFI_HANDLE *SfsHandles = NULL;
    UINTN SfsHandleCount = 0;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *SfsIo = NULL;
    EFI_FILE_PROTOCOL *FileRootVolume = NULL;
    EFI_FILE_PROTOCOL *FileAudio = NULL;

    // File info.
    EFI_FILE_INFO *AudioFileInfo = NULL;
    UINTN AudioFileInfoSize = 0;
    UINT8 *AudioFileBuffer;
    UINTN AudioFileBufferSize;
    EFI_AUDIO_IO_PROTOCOL_BITS AudioFileBits;
    EFI_AUDIO_IO_PROTOCOL_FREQ AudioFileFreq;
    UINT8 AudioFileChannels;
    WAVE_FILE_DATA WaveData;

    // Default to using built-in data.
    mSoundData = ChimeData;
    mSoundDataLength = ChimeDataLength;
    mSoundFreq = ChimeDataFreq;
    mSoundBits = ChimeDataBits;
    mSoundChannels = ChimeDataChannels;
    mIsAppleBoot = FALSE;

    // Open Loaded Image Device Path protocol.
    Status = gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageDevicePathProtocolGuid, (VOID**)&LoadedImageDevicePath);
    if (EFI_ERROR(Status) && (Status != EFI_UNSUPPORTED))
        goto DONE;

    // Get parent device path if supported.
    if (LoadedImageDevicePath) {
        DevicePath = LoadedImageDevicePath;
        while(!IsDevicePathEnd(DevicePath)) {
            if (!((DevicePath->Type == MEDIA_DEVICE_PATH) && (DevicePath->SubType == MEDIA_FILEPATH_DP)))
                ParentDeviceNode = DevicePath;
            DevicePath = NextDevicePathNode(DevicePath);
        }
    }

    // Get filesystem volumes in system.
    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &SfsHandleCount, &SfsHandles);
    if (EFI_ERROR(Status))
        goto DONE;

    // Check each filesystem.
    for (UINTN f = 0; f < SfsHandleCount; f++) {
        // Open Simple Filesystem protocol.
        Status = gBS->HandleProtocol(SfsHandles[f], &gEfiSimpleFileSystemProtocolGuid, (VOID**)&SfsIo);
        if (EFI_ERROR(Status))
            continue;

        // Should we check the device paths?
        if (LoadedImageDevicePath) {
            // Get Device Path protocol.
            Status = gBS->HandleProtocol(SfsHandles[f], &gEfiDevicePathProtocolGuid, (VOID**)&DevicePath);
            if (EFI_ERROR(Status))
                continue;

            // Get end of device path.
            while (!IsDevicePathEnd(NextDevicePathNode(DevicePath)))
                DevicePath = NextDevicePathNode(DevicePath);

            // Compare nodes. If they don't match, move to the next handle.
            if (CompareMem(DevicePath, ParentDeviceNode, DevicePathNodeLength(ParentDeviceNode)))
                continue;
        }

        // Open the volume.
        Status = SfsIo->OpenVolume(SfsIo, &FileRootVolume);
        if (EFI_ERROR(Status))
            continue;

        // Try to open the file.
        Status = FileRootVolume->Open(FileRootVolume, &FileAudio, AUDIO_FILE_NAME,
            EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
        if (!(EFI_ERROR(Status)))
            break;
    }

    // If an audio file was found, read that and use it for a chime.
    if (FileAudio) {
        DEBUG((DEBUG_INFO, "BootChimeDxeMain(): found file %s\n", AUDIO_FILE_NAME));

        // Get file info size.
        AudioFileInfoSize = 0;
        Status = FileAudio->GetInfo(FileAudio, &gEfiFileInfoGuid, &AudioFileInfoSize, NULL);
        if (EFI_ERROR(Status) && (Status != EFI_BUFFER_TOO_SMALL))
            goto DONE;

        // Allocate space for file info.
        AudioFileInfo = AllocateZeroPool(AudioFileInfoSize);
        if (AudioFileInfo == NULL) {
            Status = EFI_OUT_OF_RESOURCES;
            goto DONE;
        }

        // Get file info.
        Status = FileAudio->GetInfo(FileAudio, &gEfiFileInfoGuid, &AudioFileInfoSize, (VOID*)AudioFileInfo);
        if (EFI_ERROR(Status))
            goto DONE;
        DEBUG((DEBUG_INFO, "BootChimeDxeMain(): file size is %u bytes\n", AudioFileInfo->FileSize));

        // Allocate space for file buffer.
        AudioFileBufferSize = AudioFileInfo->FileSize;
        AudioFileBuffer = AllocatePool(AudioFileBufferSize);
        if (AudioFileBuffer == NULL) {
            Status = EFI_OUT_OF_RESOURCES;
            goto DONE;
        }

        // Read file data.
        Status = FileAudio->Read(FileAudio, &AudioFileBufferSize, AudioFileBuffer);
        if (EFI_ERROR(Status)) {
            FreePool(AudioFileBuffer);
            goto DONE;
        }

        // Get WAVE info.
        Status = WaveGetFileData(AudioFileBuffer, AudioFileBufferSize, &WaveData);
        if (EFI_ERROR(Status)) {
            FreePool(AudioFileBuffer);
            goto DONE;
        }

        // Determine bits.
        switch (WaveData.Format->BitsPerSample) {
            case 8:
                AudioFileBits = EfiAudioIoBits8;
                break;

            case 16:
                AudioFileBits = EfiAudioIoBits16;
                break;

            case 20:
                AudioFileBits = EfiAudioIoBits20;
                break;

            case 24:
                AudioFileBits = EfiAudioIoBits24;
                break;

            case 32:
                AudioFileBits = EfiAudioIoBits32;
                break;

            default:
                Print(L"BootChimeDxe: Unsupported bit depth for file: %u.\n", WaveData.Format->BitsPerSample);
                FreePool(AudioFileBuffer);
                goto DONE;
        }

        // Determine frequency.
        switch (WaveData.Format->SamplesPerSec) {
            case 8000:
                AudioFileFreq = EfiAudioIoFreq8kHz;
                break;

            case 11025:
                AudioFileFreq = EfiAudioIoFreq11kHz;
                break;

            case 16000:
                AudioFileFreq = EfiAudioIoFreq16kHz;
                break;

            case 22050:
                AudioFileFreq = EfiAudioIoFreq22kHz;
                break;

            case 32000:
                AudioFileFreq = EfiAudioIoFreq32kHz;
                break;

            case 44100:
                AudioFileFreq = EfiAudioIoFreq44kHz;
                break;

            case 48000:
                AudioFileFreq = EfiAudioIoFreq48kHz;
                break;

            case 88200:
                AudioFileFreq = EfiAudioIoFreq88kHz;
                break;

            case 96000:
                AudioFileFreq = EfiAudioIoFreq96kHz;
                break;

            case 192000:
                AudioFileFreq = EfiAudioIoFreq192kHz;
                break;

            default:
                Print(L"BootChimeDxe: Unsupported sample rate for file: %u Hz.\n", WaveData.Format->SamplesPerSec);
                FreePool(AudioFileBuffer);
                goto DONE;
        }

        // Check channels.
        if ((WaveData.Format->Channels == 0) || (WaveData.Format->Channels > EFI_AUDIO_IO_PROTOCOL_MAX_CHANNELS)) {
            Print(L"BootChimeDxe: Unsupported number of channels for file: %u.\n", WaveData.Format->Channels);
            FreePool(AudioFileBuffer);
            goto DONE;
        }
        AudioFileChannels = (UINT8)WaveData.Format->Channels;

        // Show warning if file is not optimal.
        if ((AudioFileBits != EfiAudioIoBits16) || (AudioFileFreq != EfiAudioIoFreq48kHz) || (AudioFileChannels != 2))
            Print(L"BootChimeDxe: The specified file is not 16-bit stereo @ 48 kHz. Support may vary.\n");

        // Use file instead of built-in data.
        mSoundData = WaveData.Samples;
        mSoundDataLength = WaveData.SamplesLength;
        mSoundBits = AudioFileBits;
        mSoundFreq = AudioFileFreq;
        mSoundChannels = AudioFileChannels;
    }

DONE:
    if (SfsHandles)
        FreePool(SfsHandles);
    if (AudioFileInfo)
        FreePool(AudioFileInfo);

    // Raise TPL.
    OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL);

    // Replace Boot Services functions.
    mOrigStartImage = gBS->StartImage;
    gBS->StartImage = BootChimeStartImage;
    mOrigExitBootServices = gBS->ExitBootServices;
    gBS->ExitBootServices = BootChimeExitBootServices;

    // Recalculate CRC and revert TPL.
    gBS->Hdr.CRC32 = 0;
    gBS->CalculateCrc32(gBS, gBS->Hdr.HeaderSize, &gBS->Hdr.CRC32);
    gBS->RestoreTPL(OldTpl);
    return EFI_SUCCESS;
}
