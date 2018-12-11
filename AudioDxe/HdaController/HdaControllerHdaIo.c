/*
 * File: HdaControllerHdaIo.c
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

#include "HdaController.h"
#include <Library/HdaRegisters.h>

// HDA I/O Device Path GUID.
EFI_GUID gEfiHdaIoDevicePathGuid = EFI_HDA_IO_DEVICE_PATH_GUID;

/**                                                                 
  Retrieves this codec's address.

  @param[in]  This              A pointer to the HDA_IO_PROTOCOL instance.
  @param[out] CodecAddress      The codec's address.

  @retval EFI_SUCCESS           The codec's address was returned.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                      
**/
EFI_STATUS
EFIAPI
HdaControllerHdaIoGetAddress(
    IN  EFI_HDA_IO_PROTOCOL *This,
    OUT UINT8 *CodecAddress) {
    HDA_IO_PRIVATE_DATA *HdaPrivateData;

    // If parameters are NULL, return error.
    if (This == NULL || CodecAddress == NULL)
        return EFI_INVALID_PARAMETER;

    // Get private data and codec address.
    HdaPrivateData = HDA_IO_PRIVATE_DATA_FROM_THIS(This);
    *CodecAddress = HdaPrivateData->HdaCodecAddress;
    return EFI_SUCCESS;
}

/**                                                                 
  Sends a single command to the codec.

  @param[in]  This              A pointer to the HDA_IO_PROTOCOL instance.
  @param[in]  Node              The destination node.
  @param[in]  Verb              The verb to send.
  @param[out] Response          The response received.

  @retval EFI_SUCCESS           The verb was sent successfully and a response received.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                      
**/
EFI_STATUS
EFIAPI
HdaControllerHdaIoSendCommand(
    IN  EFI_HDA_IO_PROTOCOL *This,
    IN  UINT8 Node,
    IN  UINT32 Verb,
    OUT UINT32 *Response) {

    // Create verb list with single item.
    EFI_HDA_IO_VERB_LIST HdaCodecVerbList;
    HdaCodecVerbList.Count = 1;
    HdaCodecVerbList.Verbs = &Verb;
    HdaCodecVerbList.Responses = Response;

    // Call SendCommands().
    return HdaControllerHdaIoSendCommands(This, Node, &HdaCodecVerbList);
}

/**                                                                 
  Sends a set of commands to the codec.

  @param[in] This               A pointer to the HDA_IO_PROTOCOL instance.
  @param[in] Node               The destination node.
  @param[in] Verbs              The verbs to send. Responses will be delievered in the same list.

  @retval EFI_SUCCESS           The verbs were sent successfully and all responses received.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                      
**/
EFI_STATUS
EFIAPI
HdaControllerHdaIoSendCommands(
    IN EFI_HDA_IO_PROTOCOL *This,
    IN UINT8 Node,
    IN EFI_HDA_IO_VERB_LIST *Verbs) {
    // Create variables.
    HDA_IO_PRIVATE_DATA *HdaPrivateData;

    // If parameters are NULL, return error.
    if (This == NULL || Verbs == NULL)
        return EFI_INVALID_PARAMETER;

    // Get private data and send commands.
    HdaPrivateData = HDA_IO_PRIVATE_DATA_FROM_THIS(This);
    return HdaControllerSendCommands(HdaPrivateData->HdaControllerDev, HdaPrivateData->HdaCodecAddress, Node, Verbs);
}

EFI_STATUS
EFIAPI
HdaControllerHdaIoSetupStream(
    IN  EFI_HDA_IO_PROTOCOL *This,
    IN  EFI_HDA_IO_PROTOCOL_TYPE Type,
    IN  EFI_HDA_IO_PROTOCOL_FREQ Freq,
    IN  EFI_HDA_IO_PROTOCOL_BITS Bits,
    IN  UINT8 Channels,
    IN  VOID *Buffer,
    IN  UINTN BufferLength,
    OUT UINT8 *StreamId) {
    DEBUG((DEBUG_INFO, "HdaControllerHdaIoSetupStream(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    HDA_IO_PRIVATE_DATA *HdaIoPrivateData;
    HDA_CONTROLLER_DEV *HdaControllerDev;
    EFI_PCI_IO_PROTOCOL *PciIo;

    // Stream.
    HDA_STREAM *HdaStream;
    UINT8 HdaStreamId;
    EFI_TPL OldTpl = 0;
    UINT8 StreamBits, StreamDiv, StreamMult = 0;
    BOOLEAN StreamBase44kHz = FALSE;
    UINT16 HdaStreamFmt;

    // If a parameter is invalid, return error.
    if ((This == NULL) || (Type >= EfiHdaIoTypeMaximum) || (Freq >= EfiHdaIoFreqMaximum) ||
        (Bits >= EfiHdaIoBitsMaximum) || (Channels == 0) || (Channels > EFI_HDA_IO_PROTOCOL_MAX_CHANNELS) ||
        (Buffer == NULL) || (BufferLength == 0) || (StreamId == NULL))
        return EFI_INVALID_PARAMETER;

    // Get private data.
    HdaIoPrivateData = HDA_IO_PRIVATE_DATA_FROM_THIS(This);
    HdaControllerDev = HdaIoPrivateData->HdaControllerDev;
    PciIo = HdaControllerDev->PciIo;

    // Get stream.
    if (Type == EfiHdaIoTypeOutput)
        HdaStream = HdaIoPrivateData->HdaOutputStream;
    else
        HdaStream = HdaIoPrivateData->HdaInputStream;

    // Get current stream ID.
    Status = HdaControllerGetStreamId(HdaStream, &HdaStreamId);
    if (EFI_ERROR(Status))
        goto DONE;

    // Is a stream ID allocated already? If so that means the stream is already
    // set up and we'll need to tear it down first.
    if (HdaStreamId > 0) {
        Status = EFI_ALREADY_STARTED;
        goto DONE;
    }

    // Raise TPL so we can't be messed with.
    OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL);

    // Find and allocate stream ID.
    for (UINT8 i = HDA_STREAM_ID_MIN; i <= HDA_STREAM_ID_MAX; i++) {
        if (!(HdaControllerDev->StreamIdMapping & (1 << i))) {
            HdaControllerDev->StreamIdMapping |= (1 << i);
            HdaStreamId = i;
            break;
        }
    }

    // If stream ID is still zero, fail.
    if (HdaStreamId == 0) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DONE;
    }

    // Set stream ID.
    Status = HdaControllerSetStreamId(HdaIoPrivateData->HdaOutputStream, HdaStreamId);
    if (EFI_ERROR(Status))
        goto DONE;

    // Determine bitness of samples.
    switch (Bits) {
        // 8-bit.
        case EfiHdaIoBits8:
            StreamBits = HDA_REG_SDNFMT_BITS_8;
            break;
        
        // 16-bit.
        case EfiHdaIoBits16:
            StreamBits = HDA_REG_SDNFMT_BITS_16;
            break;

        // 20-bit.
        case EfiHdaIoBits20:
            StreamBits = HDA_REG_SDNFMT_BITS_20;
            break;

        // 24-bit.
        case EfiHdaIoBits24:
            StreamBits = HDA_REG_SDNFMT_BITS_24;
            break;

        // 32-bit.
        case EfiHdaIoBits32:
            StreamBits = HDA_REG_SDNFMT_BITS_32;
            break;

        // Others.
        default:
            Status = EFI_INVALID_PARAMETER;
            goto DONE;
    }

    // Determine base, divisor, and multipler.
    switch (Freq) {
        // 8 kHz.
        case EfiHdaIoFreq8kHz:
            StreamBase44kHz = FALSE;
            StreamDiv = 6;
            StreamMult = 1;
            break;

        // 11.025 kHz.
        case EfiHdaIoFreq11kHz:
            StreamBase44kHz = FALSE;
            StreamDiv = 4;
            StreamMult = 1;
            break;

        // 16 kHz.
        case EfiHdaIoFreq16kHz:
            StreamBase44kHz = FALSE;
            StreamDiv = 3;
            StreamMult = 1;
            break;

        // 22.05 kHz.
        case EfiHdaIoFreq22kHz:
            StreamBase44kHz = FALSE;
            StreamDiv = 2;
            StreamMult = 1;
            break;

        // 32 kHz.
        case EfiHdaIoFreq32kHz:
            StreamBase44kHz = FALSE;
            StreamDiv = 3;
            StreamMult = 2;
            break;

        // 44.1 kHz.
        case EfiHdaIoFreq44kHz:
            StreamBase44kHz = TRUE;
            StreamDiv = 1;
            StreamMult = 1;
            break;

        // 48 kHz.
        case EfiHdaIoFreq48kHz:
            StreamBase44kHz = FALSE;
            StreamDiv = 1;
            StreamMult = 1;
            break;

        // 88 kHz.
        case EfiHdaIoFreq88kHz:
            StreamBase44kHz = TRUE;
            StreamDiv = 1;
            StreamMult = 2;
            break;

        // 96 kHz.
        case EfiHdaIoFreq96kHz:
            StreamBase44kHz = FALSE;
            StreamDiv = 1;
            StreamMult = 2;
            break;

        // 192 kHz.
        case EfiHdaIoFreq192kHz:
            StreamBase44kHz = FALSE;
            StreamDiv = 1;
            StreamMult = 4;
            break;

        // Others.
        default:
            Status = EFI_INVALID_PARAMETER;
            goto DONE;
    }

    // Set stream format.
    HdaStreamFmt = HDA_REG_SDNFMT_SET(Channels - 1, StreamBits,
        StreamDiv - 1, StreamMult - 1, StreamBase44kHz);
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR,
        HDA_REG_SDNFMT(HdaStream->Index), 1, &HdaStreamFmt);
    if (EFI_ERROR(Status))
        goto DONE;

    // Save pointer to buffer.
    HdaStream->BufferSource = Buffer;
    HdaStream->BufferSourceLength = BufferLength;
    HdaStream->BufferSourcePosition = 0;
    
    // Stream is ready.
    Status = EFI_SUCCESS;

DONE:
    // Restore TPL if needed.
    if (OldTpl)
        gBS->RestoreTPL(OldTpl);

    return Status;
}

EFI_STATUS
EFIAPI
HdaControllerHdaIoCloseStream(
    IN EFI_HDA_IO_PROTOCOL *This,
    IN EFI_HDA_IO_PROTOCOL_TYPE Type) {
    DEBUG((DEBUG_INFO, "HdaControllerHdaIoCloseStream(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    HDA_IO_PRIVATE_DATA *HdaIoPrivateData;
    HDA_CONTROLLER_DEV *HdaControllerDev;

    // Stream.
    HDA_STREAM *HdaStream;
    UINT8 HdaStreamId;
    EFI_TPL OldTpl = 0;

    // If a parameter is invalid, return error.
    if ((This == NULL) || (Type >= EfiHdaIoTypeMaximum))
        return EFI_INVALID_PARAMETER;

    // Get private data.
    HdaIoPrivateData = HDA_IO_PRIVATE_DATA_FROM_THIS(This);
    HdaControllerDev = HdaIoPrivateData->HdaControllerDev;

    // Get stream.
    if (Type == EfiHdaIoTypeOutput)
        HdaStream = HdaIoPrivateData->HdaOutputStream;
    else
        HdaStream = HdaIoPrivateData->HdaInputStream;

    // Get current stream ID.
    Status = HdaControllerGetStreamId(HdaStream, &HdaStreamId);
    if (EFI_ERROR(Status))
        goto DONE;

    // Is a stream ID already at zero?
    if (HdaStreamId == 0) {
        Status = EFI_SUCCESS;
        goto DONE;
    }

    // Raise TPL so we can't be messed with.
    OldTpl = gBS->RaiseTPL(TPL_HIGH_LEVEL);

    // Set stream ID to zero.
    Status = HdaControllerSetStreamId(HdaStream, 0);
    if (EFI_ERROR(Status))
        goto DONE;

    // De-allocate stream ID from bitmap.
    HdaControllerDev->StreamIdMapping &= ~(1 << HdaStreamId);

    // Remove source buffer pointer.
    HdaStream->BufferSource = NULL;
    HdaStream->BufferSourceLength = 0;
    HdaStream->BufferSourcePosition = 0;

    // Stream closed successfully.
    Status = EFI_SUCCESS;

DONE:
    // Restore TPL if needed.
    if (OldTpl)
        gBS->RestoreTPL(OldTpl);

    return Status;
}

EFI_STATUS
EFIAPI
HdaControllerHdaIoGetStream(
    IN  EFI_HDA_IO_PROTOCOL *This,
    IN  EFI_HDA_IO_PROTOCOL_TYPE Type,
    OUT BOOLEAN *State) {
    DEBUG((DEBUG_INFO, "HdaControllerHdaIoGetStream(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    HDA_IO_PRIVATE_DATA *HdaIoPrivateData;
    HDA_CONTROLLER_DEV *HdaControllerDev;

    // Stream.
    HDA_STREAM *HdaStream;
    UINT8 HdaStreamId;

    // If a parameter is invalid, return error.
    if ((This == NULL) || (Type >= EfiHdaIoTypeMaximum) || (State == NULL))
        return EFI_INVALID_PARAMETER;

    // Get private data.
    HdaIoPrivateData = HDA_IO_PRIVATE_DATA_FROM_THIS(This);
    HdaControllerDev = HdaIoPrivateData->HdaControllerDev;

    // Get stream.
    if (Type == EfiHdaIoTypeOutput)
        HdaStream = HdaIoPrivateData->HdaOutputStream;
    else
        HdaStream = HdaIoPrivateData->HdaInputStream;

    // Get current stream ID.
    Status = HdaControllerGetStreamId(HdaStream, &HdaStreamId);
    if (EFI_ERROR(Status))
        return Status;

    // Is a stream ID zero? If so that means the stream is not setup yet.
    if (HdaStreamId == 0)
        return EFI_NOT_READY;

    // Get stream state.
    return HdaControllerGetStream(HdaStream, State);
}

EFI_STATUS
EFIAPI
HdaControllerHdaIoSetStream(
    IN EFI_HDA_IO_PROTOCOL *This,
    IN EFI_HDA_IO_PROTOCOL_TYPE Type,
    IN BOOLEAN State) {
    DEBUG((DEBUG_INFO, "HdaControllerHdaIoSetStream(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    HDA_IO_PRIVATE_DATA *HdaIoPrivateData;
    HDA_CONTROLLER_DEV *HdaControllerDev;

    // Stream.
    HDA_STREAM *HdaStream;
    UINT8 HdaStreamId;

    // If a parameter is invalid, return error.
    if ((This == NULL) || (Type >= EfiHdaIoTypeMaximum))
        return EFI_INVALID_PARAMETER;

    // Get private data.
    HdaIoPrivateData = HDA_IO_PRIVATE_DATA_FROM_THIS(This);
    HdaControllerDev = HdaIoPrivateData->HdaControllerDev;

    // Get stream.
    if (Type == EfiHdaIoTypeOutput)
        HdaStream = HdaIoPrivateData->HdaOutputStream;
    else
        HdaStream = HdaIoPrivateData->HdaInputStream;

    // Get current stream ID.
    Status = HdaControllerGetStreamId(HdaStream, &HdaStreamId);
    if (EFI_ERROR(Status))
        return Status;

    // Is a stream ID zero? If so that means the stream is not setup yet.
    if (HdaStreamId == 0)
        return EFI_NOT_READY;

    // Change stream state.
    return HdaControllerSetStream(HdaStream, State);
}
