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
    IN  UINT16 Format,
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

    // If a parameter is invalid, return error.
    if ((This == NULL) || (Type >= EfiHdaIoTypeMaximum) || (StreamId == NULL))
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
    *StreamId = HdaStreamId;

    // Set stream format.
    DEBUG((DEBUG_INFO, "HdaControllerHdaIoSetupStream(): setting format 0x%X\n", Format));
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR,
        HDA_REG_SDNFMT(HdaStream->Index), 1, &Format);
    if (EFI_ERROR(Status))
        goto DONE;

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

    // Stop stream.
    Status = HdaControllerHdaIoStopStream(This, Type);
    if (EFI_ERROR(Status))
        goto DONE;

    // Set stream ID to zero.
    Status = HdaControllerSetStreamId(HdaStream, 0);
    if (EFI_ERROR(Status))
        goto DONE;

    // De-allocate stream ID from bitmap.
    HdaControllerDev->StreamIdMapping &= ~(1 << HdaStreamId);

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
HdaControllerHdaIoStartStream(
    IN EFI_HDA_IO_PROTOCOL *This,
    IN EFI_HDA_IO_PROTOCOL_TYPE Type,
    IN VOID *Buffer,
    IN UINTN BufferLength,
    IN UINTN BufferPosition) {
    DEBUG((DEBUG_INFO, "HdaControllerHdaIoStartStream(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    HDA_IO_PRIVATE_DATA *HdaIoPrivateData;
    HDA_CONTROLLER_DEV *HdaControllerDev;

    // Stream.
    HDA_STREAM *HdaStream;
    UINT8 HdaStreamId;

    // If a parameter is invalid, return error.
    if ((This == NULL) || (Type >= EfiHdaIoTypeMaximum) ||
        (Buffer == NULL) || (BufferLength == 0))
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

    // Save pointer to buffer.
    HdaStream->BufferSource = Buffer;
    HdaStream->BufferSourceLength = BufferLength;
    HdaStream->BufferSourcePosition = BufferPosition;

    // Fill initial buffer.
    CopyMem(HdaStream->BufferData,
        HdaStream->BufferSource + HdaStream->BufferSourcePosition, HDA_STREAM_BUF_SIZE);
    HdaStream->BufferSourcePosition += HDA_STREAM_BUF_SIZE;
    HdaStream->DoUpperHalf = FALSE;

    // Setup polling timer.
    Status = gBS->SetTimer(HdaStream->PollTimer, TimerPeriodic, HDA_STREAM_POLL_TIME);
    if (EFI_ERROR(Status))
        return Status;

    // Change stream state.
    return HdaControllerSetStream(HdaStream, TRUE);
}

EFI_STATUS
EFIAPI
HdaControllerHdaIoStopStream(
    IN EFI_HDA_IO_PROTOCOL *This,
    IN EFI_HDA_IO_PROTOCOL_TYPE Type) {
    DEBUG((DEBUG_INFO, "HdaControllerHdaIoStopStream(): start\n"));

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

    // Is the stream ID zero? If so that means the stream is not setup yet.
    if (HdaStreamId == 0)
        return EFI_NOT_READY;

    // Cancel polling timer.
    Status = gBS->SetTimer(HdaStream->PollTimer, TimerCancel, 0);
    if (EFI_ERROR(Status))
        return Status;

    // Stop stream.
    Status = HdaControllerSetStream(HdaStream, FALSE);
    if (EFI_ERROR(Status))
        return Status;

    // Remove source buffer pointer.
    HdaStream->BufferSource = NULL;
    HdaStream->BufferSourceLength = 0;
    HdaStream->BufferSourcePosition = 0;
    return EFI_SUCCESS;
}
