/*
 * File: HdaCodecAudioIo.c
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

#include "HdaCodec.h"

/**                                                                 
  Sets up the device to play audio data.

  @param[in] This               A pointer to the EFI_AUDIO_IO_PROTOCOL instance.
  @param[in] OutputIndex        The zero-based index of the desired output.
  @param[in] Bits               The width in bits of the source data.
  @param[in] Freq               The frequency of the source data.
  @param[in] Channels           The number of channels the source data contains.

  @retval EFI_SUCCESS           The audio data was played successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
HdaCodecAudioIoSetupPlayback(
    IN EFI_AUDIO_IO_PROTOCOL *This,
    IN UINT32 OutputIndex,
    IN EFI_AUDIO_IO_PROTOCOL_FREQ Freq,
    IN EFI_AUDIO_IO_PROTOCOL_BITS Bits,
    IN UINT8 Channels) {
    DEBUG((DEBUG_INFO, "HdaCodecAudioIoSetupPlayback(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    AUDIO_IO_PRIVATE_DATA *AudioIoPrivateData;
    HDA_CODEC_DEV *HdaCodecDev;
    EFI_HDA_IO_PROTOCOL *HdaIo;

    // Widgets.
    HDA_WIDGET_DEV *PinWidget;
    HDA_WIDGET_DEV *OutputWidget;
    UINT32 SupportedRates;
    UINT8 HdaStreamId;

    // Stream.
    UINT8 StreamBits, StreamDiv, StreamMult = 0;
    BOOLEAN StreamBase44kHz = FALSE;
    UINT16 StreamFmt;

    // If a parameter is invalid, return error.
    if ((This == NULL) || (Freq >= EfiAudioIoFreqMaximum) ||
        (Bits >= EfiAudioIoBitsMaximum))
        return EFI_INVALID_PARAMETER;

    // Get private data.
    AudioIoPrivateData = AUDIO_IO_PRIVATE_DATA_FROM_THIS(This);
    HdaCodecDev = AudioIoPrivateData->HdaCodecDev;
    HdaIo = HdaCodecDev->HdaIo;

    // Check that output index is within bounds and get our desired output.
    if (OutputIndex >= HdaCodecDev->OutputPortsCount)
        return EFI_INVALID_PARAMETER;
    PinWidget = HdaCodecDev->OutputPorts[OutputIndex];

    // Get the output DAC for the path.
    Status = HdaCodecGetOutputDac(PinWidget, &OutputWidget);
    if (EFI_ERROR(Status))
        return Status;

    // Get supported stream formats.
    Status = HdaCodecGetSupportedPcmRates(OutputWidget, &SupportedRates);
    if (EFI_ERROR(Status))
        return Status;

    // Determine bitness of samples, ensuring desired bitness is supported.
    switch (Bits) {
        // 8-bit.
        case EfiAudioIoBits8:
            if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_8BIT))
                return EFI_UNSUPPORTED;
            StreamBits = HDA_CONVERTER_FORMAT_BITS_8;
            break;
        
        // 16-bit.
        case EfiAudioIoBits16:
            if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_16BIT))
                return EFI_UNSUPPORTED;
            StreamBits = HDA_CONVERTER_FORMAT_BITS_16;
            break;

        // 20-bit.
        case EfiAudioIoBits20:
            if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_20BIT))
                return EFI_UNSUPPORTED;
            StreamBits = HDA_CONVERTER_FORMAT_BITS_20;
            break;

        // 24-bit.
        case EfiAudioIoBits24:
            if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_24BIT))
                return EFI_UNSUPPORTED;
            StreamBits = HDA_CONVERTER_FORMAT_BITS_24;
            break;

        // 32-bit.
        case EfiAudioIoBits32:
            if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_32BIT))
                return EFI_UNSUPPORTED;
            StreamBits = HDA_CONVERTER_FORMAT_BITS_32;
            break;

        // Others.
        default:
            return EFI_INVALID_PARAMETER;
    }

    // Determine base, divisor, and multipler.
    switch (Freq) {
        // 8 kHz.
        case EfiAudioIoFreq8kHz:
            if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_8KHZ))
                return EFI_UNSUPPORTED;
            StreamBase44kHz = FALSE;
            StreamDiv = 6;
            StreamMult = 1;
            break;

        // 11.025 kHz.
        case EfiAudioIoFreq11kHz:
            if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_11KHZ))
                return EFI_UNSUPPORTED;
            StreamBase44kHz = FALSE;
            StreamDiv = 4;
            StreamMult = 1;
            break;

        // 16 kHz.
        case EfiAudioIoFreq16kHz:
            if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_16KHZ))
                return EFI_UNSUPPORTED;
            StreamBase44kHz = FALSE;
            StreamDiv = 3;
            StreamMult = 1;
            break;

        // 22.05 kHz.
        case EfiAudioIoFreq22kHz:
            if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_22KHZ))
                return EFI_UNSUPPORTED;
            StreamBase44kHz = FALSE;
            StreamDiv = 2;
            StreamMult = 1;
            break;

        // 32 kHz.
        case EfiAudioIoFreq32kHz:
            if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_32KHZ))
                return EFI_UNSUPPORTED;
            StreamBase44kHz = FALSE;
            StreamDiv = 3;
            StreamMult = 2;
            break;

        // 44.1 kHz.
        case EfiAudioIoFreq44kHz:
            if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_44KHZ))
                return EFI_UNSUPPORTED;
            StreamBase44kHz = TRUE;
            StreamDiv = 1;
            StreamMult = 1;
            break;

        // 48 kHz.
        case EfiAudioIoFreq48kHz:
            if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_48KHZ))
                return EFI_UNSUPPORTED;
            StreamBase44kHz = FALSE;
            StreamDiv = 1;
            StreamMult = 1;
            break;

        // 88 kHz.
        case EfiAudioIoFreq88kHz:
            if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_88KHZ))
                return EFI_UNSUPPORTED;
            StreamBase44kHz = TRUE;
            StreamDiv = 1;
            StreamMult = 2;
            break;

        // 96 kHz.
        case EfiAudioIoFreq96kHz:
            if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_96KHZ))
                return EFI_UNSUPPORTED;
            StreamBase44kHz = FALSE;
            StreamDiv = 1;
            StreamMult = 2;
            break;

        // 192 kHz.
        case EfiAudioIoFreq192kHz:
            if (!(SupportedRates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_192KHZ))
                return EFI_UNSUPPORTED;
            StreamBase44kHz = FALSE;
            StreamDiv = 1;
            StreamMult = 4;
            break;

        // Others.
        default:
            return EFI_INVALID_PARAMETER;
    }

    // Disable all widget paths.
    for (UINT8 w = 0; w < HdaCodecDev->OutputPortsCount; w++) {
        Status = HdaCodecDisableWidgetPath(HdaCodecDev->OutputPorts[w]);
        if (EFI_ERROR(Status))
            return Status;
    }

    // Calculate stream format and setup stream.
    StreamFmt = HDA_CONVERTER_FORMAT_SET(Channels - 1, StreamBits,
        StreamDiv - 1, StreamMult - 1, StreamBase44kHz);
    DEBUG((DEBUG_INFO, "HdaCodecAudioIoPlay(): Stream format 0x%X\n", StreamFmt));
    Status = HdaIo->SetupStream(HdaIo, EfiHdaIoTypeOutput, StreamFmt, &HdaStreamId);
    if (EFI_ERROR(Status))
        return Status;

    // Setup widget path for desired output.
    AudioIoPrivateData->SelectedOutputIndex = OutputIndex;
    Status = HdaCodecEnableWidgetPath(PinWidget, 100, HdaStreamId, StreamFmt);
    if (EFI_ERROR(Status))
        goto CLOSE_STREAM;
    return EFI_SUCCESS;

CLOSE_STREAM:
    // Close stream.
    HdaIo->CloseStream(HdaIo, EfiHdaIoTypeOutput);
    return Status;
}

/**                                                                 
  Begins playback on the device and waits for playback to complete.

  @param[in] This               A pointer to the EFI_AUDIO_IO_PROTOCOL instance.
  @param[in] Data               A pointer to the buffer containing the audio data to play.
  @param[in] DataLength         The size, in bytes, of the data buffer specified by Data.
  @param[in] Position           The position in the buffer to start at.

  @retval EFI_SUCCESS           The audio data was played successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
HdaCodecAudioIoStartPlayback(
    IN EFI_AUDIO_IO_PROTOCOL *This,
    IN VOID *Data,
    IN UINTN DataLength,
    IN UINTN Position OPTIONAL) {
    DEBUG((DEBUG_INFO, "HdaCodecAudioIoStartPlayback(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    AUDIO_IO_PRIVATE_DATA *AudioIoPrivateData;
    EFI_HDA_IO_PROTOCOL *HdaIo;
    BOOLEAN StreamRunning;

    // If a parameter is invalid, return error.
    if ((This == NULL) || (Data == NULL) || (DataLength == 0))
        return EFI_INVALID_PARAMETER;

    // Get private data.
    AudioIoPrivateData = AUDIO_IO_PRIVATE_DATA_FROM_THIS(This);
    HdaIo = AudioIoPrivateData->HdaCodecDev->HdaIo;

    // Start stream.
    Status = HdaIo->StartStream(HdaIo, EfiHdaIoTypeOutput, Data, DataLength, Position);
    if (EFI_ERROR(Status))
        return Status;

    // Wait for stream to stop.
    StreamRunning = TRUE;
    while (StreamRunning) {
        Status = HdaIo->GetStream(HdaIo, EfiHdaIoTypeOutput, &StreamRunning);
        if (EFI_ERROR(Status)) {
            HdaIo->StopStream(HdaIo, EfiHdaIoTypeOutput);
            return Status;
        }

        // Wait 100ms.
        gBS->Stall(MS_TO_MICROSECOND(100));
    }
    return EFI_SUCCESS;
}

/**                                                                 
  Stops playback on the device.

  @param[in] This               A pointer to the EFI_AUDIO_IO_PROTOCOL instance.

  @retval EFI_SUCCESS           The audio data was played successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
EFI_STATUS
EFIAPI
HdaCodecAudioIoStopPlayback(
    IN EFI_AUDIO_IO_PROTOCOL *This) {
    DEBUG((DEBUG_INFO, "HdaCodecAudioIoStopPlayback(): start\n"));
    
    // Create variables.
    AUDIO_IO_PRIVATE_DATA *AudioIoPrivateData;
    EFI_HDA_IO_PROTOCOL *HdaIo;

    // If a parameter is invalid, return error.
    if (This == NULL)
        return EFI_INVALID_PARAMETER;

    // Get private data.
    AudioIoPrivateData = AUDIO_IO_PRIVATE_DATA_FROM_THIS(This);
    HdaIo = AudioIoPrivateData->HdaCodecDev->HdaIo;

    // Stop stream.
    return HdaIo->StopStream(HdaIo, EfiHdaIoTypeOutput);
}
