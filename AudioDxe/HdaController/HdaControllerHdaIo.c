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
HdaControllerCodecProtocolGetAddress(
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
HdaControllerCodecProtocolSendCommand(
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
    return HdaControllerCodecProtocolSendCommands(This, Node, &HdaCodecVerbList);
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
HdaControllerCodecProtocolSendCommands(
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
