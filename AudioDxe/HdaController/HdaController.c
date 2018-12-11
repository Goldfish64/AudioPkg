/*
 * File: HdaController.c
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
#include "HdaControllerMem.h"
#include "HdaRegisters.h"
#include "HdaControllerComponentName.h"

#include "HdaCodec/HdaCodec.h"
#include "HdaCodec/HdaCodecComponentName.h"

#include <Guid/FileInfo.h>

// HDA I/O Device Path GUID.
EFI_GUID gEfiHdaIoDevicePathGuid = EFI_HDA_IO_DEVICE_PATH_GUID;

VOID
HdaControllerStreamPollTimerHandler(
    IN EFI_EVENT Event,
    IN VOID *Context) {
    
    // Create variables.
    EFI_STATUS Status;
    HDA_STREAM *HdaStream = (HDA_STREAM*)Context;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaStream->HdaDev->PciIo;
    UINT8 HdaStreamSts = 0;

    // Get stream status.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthFifoUint8, PCI_HDA_BAR, HDA_REG_SDNSTS(HdaStream->Index), 1, &HdaStreamSts);
    ASSERT_EFI_ERROR(Status);

    // Has the completion bit been set?
    if (HdaStreamSts & HDA_REG_SDNSTS_BCIS) {
        // Is this an output stream (copy data to)?
        if (HdaStream->Output) {
            // Copy data to DMA buffer.
            if (HdaStream->DoUpperHalf)
                CopyMem(HdaStream->BufferData + HDA_STREAM_BUF_SIZE_HALF, HdaStream->BufferSource + HdaStream->BufferSourcePosition, HDA_STREAM_BUF_SIZE_HALF);
            else
                CopyMem(HdaStream->BufferData, HdaStream->BufferSource + HdaStream->BufferSourcePosition, HDA_STREAM_BUF_SIZE_HALF);
            
        } else { // Input stream (copy data from).
            // Copy data from DMA buffer.
            if (HdaStream->DoUpperHalf)
                CopyMem(HdaStream->BufferSource + HdaStream->BufferSourcePosition, HdaStream->BufferData + HDA_STREAM_BUF_SIZE_HALF, HDA_STREAM_BUF_SIZE_HALF);
            else
                CopyMem(HdaStream->BufferSource + HdaStream->BufferSourcePosition, HdaStream->BufferData, HDA_STREAM_BUF_SIZE_HALF);
        }

        // Increase position and flip upper half variable.
        HdaStream->BufferSourcePosition += HDA_STREAM_BUF_SIZE_HALF;
        HdaStream->DoUpperHalf = !HdaStream->DoUpperHalf;

        // Reset completion bit.
        HdaStreamSts = HDA_REG_SDNSTS_BCIS;
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNSTS(HdaStream->Index), 1, &HdaStreamSts);
        ASSERT_EFI_ERROR(Status);
        DEBUG((DEBUG_INFO, "Bit set %u!\n", HdaStream->DoUpperHalf));
    }
}

/**                                                                 
  Retrieves this codec's address.

  @param  This                  A pointer to the HDA_CODEC_PROTOCOL instance.
  @param  CodecAddress          The codec's address.

  @retval EFI_SUCCESS           The codec's address was returned.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                      
**/
EFI_STATUS
EFIAPI
HdaControllerCodecProtocolGetAddress(
    IN EFI_HDA_IO_PROTOCOL *This,
    OUT UINT8 *CodecAddress) {
    HDA_CONTROLLER_PRIVATE_DATA *HdaPrivateData;

    // If parameters are NULL, return error.
    if (This == NULL || CodecAddress == NULL)
        return EFI_INVALID_PARAMETER;

    // Get private data and codec address.
    HdaPrivateData = HDA_CONTROLLER_PRIVATE_DATA_FROM_THIS(This);
    *CodecAddress = HdaPrivateData->HdaCodecAddress;
    return EFI_SUCCESS;
}

/**                                                                 
  Sends a single command to the codec.

  @param  This                  A pointer to the HDA_CODEC_PROTOCOL instance.
  @param  Node                  The destination node.
  @param  Verb                  The verb to send.
  @param  Response              The response received.

  @retval EFI_SUCCESS           The verb was sent successfully and a response received.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                      
**/
EFI_STATUS
EFIAPI
HdaControllerCodecProtocolSendCommand(
    IN EFI_HDA_IO_PROTOCOL *This,
    IN UINT8 Node,
    IN UINT32 Verb,
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

  @param  This                  A pointer to the HDA_CODEC_PROTOCOL instance.
  @param  Node                  The destination node.
  @param  Verbs                 The verbs to send. Responses will be delievered in the same list.

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
    HDA_CONTROLLER_PRIVATE_DATA *HdaPrivateData;

    // If parameters are NULL, return error.
    if (This == NULL || Verbs == NULL)
        return EFI_INVALID_PARAMETER;

    // Get private data and send commands.
    HdaPrivateData = HDA_CONTROLLER_PRIVATE_DATA_FROM_THIS(This);
    return HdaControllerSendCommands(HdaPrivateData->HdaDev, HdaPrivateData->HdaCodecAddress, Node, Verbs);
}

VOID
HdaControllerResponsePollTimerHandler(
    IN EFI_EVENT Event,
    IN VOID *Context) {
    HDA_CONTROLLER_DEV *HdaDev = (HDA_CONTROLLER_DEV*)Context;
    
    HDA_STREAM *HdaOutStream = HdaDev->OutputStreams + 0;

    // get status.
    UINT8 status;
    HdaDev->PciIo->Mem.Read(HdaDev->PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNSTS(HdaOutStream->Index), 1, &status);

    if (status & HDA_REG_SDNSTS_BCIS) {
        if (HdaDev->uphalf)
            CopyMem(HdaOutStream->BufferData + (HDA_STREAM_BUF_SIZE / 2), HdaDev->filebuffer + HdaDev->position, HDA_STREAM_BUF_SIZE / 2);
        else
            CopyMem(HdaOutStream->BufferData, HdaDev->filebuffer + HdaDev->position, HDA_STREAM_BUF_SIZE / 2);
        HdaDev->position += (HDA_STREAM_BUF_SIZE / 2);

        HdaDev->uphalf = !HdaDev->uphalf;

        status = HDA_REG_SDNSTS_BCIS;
        HdaDev->PciIo->Mem.Write(HdaDev->PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNSTS(HdaOutStream->Index), 1, &status);
        DEBUG((DEBUG_INFO, "Bit set %u!\n", HdaDev->uphalf));
      //  UINTN bufferLengthActual = HDA_STREAM_BUF_SIZE;
        // HdaDev->token->Read(HdaDev->token, &bufferLengthActual, HdaOutStream->BufferData);
       //// HdaDev->position += bufferLengthActual;
       // HdaDev->token->SetPosition(HdaDev->token, HdaDev->position);
       // DEBUG((DEBUG_INFO, "%u bytes\n", bufferLengthActual));
       // ASSERT_EFI_ERROR(Status);
    }
}

EFI_STATUS
EFIAPI
HdaControllerReset(
    IN EFI_PCI_IO_PROTOCOL *PciIo) {
    DEBUG((DEBUG_INFO, "HdaControllerReset()\n"));

    // Create variables.
    EFI_STATUS Status;
    UINT32 HdaGCtl;
    UINT64 Tmp;

    // Get value of CRST bit.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, 1, &HdaGCtl);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;

    // Check if the controller is already in reset. If not, clear bit.
    if (!(HdaGCtl & HDA_REG_GCTL_CRST)) {
        HdaGCtl &= ~HDA_REG_GCTL_CRST;
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, 1, &HdaGCtl);
        ASSERT_EFI_ERROR(Status);
        if (EFI_ERROR(Status))
            return Status;
    }

    // Set CRST bit to begin the process of coming out of reset.
    HdaGCtl |= HDA_REG_GCTL_CRST;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, 1, &HdaGCtl);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;

    // Wait for bit to be set. Once bit is set, the controller is ready.
    Status = PciIo->PollMem(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL,
        HDA_REG_GCTL_CRST, HDA_REG_GCTL_CRST, MS_TO_NANOSECOND(100), &Tmp);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;

    // Wait 50ms to ensure all codecs have also reset.
    gBS->Stall(MS_TO_MICROSECOND(50));

    // Controller is reset.
    DEBUG((DEBUG_INFO, "HDA controller is reset!\n"));
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerScanCodecs(
    IN HDA_CONTROLLER_DEV *HdaDev,
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle) {
    DEBUG((DEBUG_INFO, "HdaControllerScanCodecs(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;
    UINT16 HdaStatests;

    EFI_DEVICE_PATH_PROTOCOL *HdaCodecDevicePath;
    EFI_HANDLE ProtocolHandle;
    VOID *TmpProtocol;

    // Get STATEST register.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_STATESTS, 1, &HdaStatests);
    if (EFI_ERROR(Status))
        return Status;

    // Iterate through register looking for active codecs.
    for (UINT8 i = 0; i < HDA_MAX_CODECS; i++) {
        // Do we have a codec at this address?
        if (HdaStatests & (1 << i)) {
            DEBUG((DEBUG_INFO, "HdaControllerScanCodecs(): found codec @ 0x%X\n", i));

            // Create private data.
            HDA_CONTROLLER_PRIVATE_DATA *HdaPrivateData = AllocateZeroPool(sizeof(HDA_CONTROLLER_PRIVATE_DATA));
            HdaPrivateData->Signature = HDA_CONTROLLER_PRIVATE_DATA_SIGNATURE;
            HdaPrivateData->HdaCodecAddress = i;
            HdaPrivateData->HdaDev = HdaDev;
            HdaPrivateData->HdaCodec.GetAddress = HdaControllerCodecProtocolGetAddress;
            HdaPrivateData->HdaCodec.SendCommand = HdaControllerCodecProtocolSendCommand;

            // Add to array.
            HdaDev->PrivateDatas[i] = HdaPrivateData;
            Status = EFI_SUCCESS;
          //  continue;

//HDA_CODEC_CLEANUP:
    //        DEBUG((DEBUG_INFO, "HdaControllerScanCodecs(): failed to load driver for codec @ 0x%X\n", i));
       //     HdaDev->PrivateDatas[i] = NULL;

            // Free objects.
       //     FreePool(HdaPrivateData);
        }
    }

    // Install protocols on each codec.
    for (UINT8 i = 0; i < HDA_MAX_CODECS; i++) {
        // Do we have a codec at this address?
        if (HdaDev->PrivateDatas[i] != NULL) {
            // Create Device Path for codec.
            EFI_HDA_IO_DEVICE_PATH HdaCodecDevicePathNode = EFI_HDA_IO_DEVICE_PATH_TEMPLATE;
            HdaCodecDevicePathNode.Address = i;
            HdaCodecDevicePath = AppendDevicePathNode(HdaDev->DevicePath, (EFI_DEVICE_PATH_PROTOCOL*)&HdaCodecDevicePathNode);
            if (HdaCodecDevicePath == NULL) {
                Status = EFI_INVALID_PARAMETER;
                goto HDA_CODEC_CLEANUP_POST;
            }

            // Install protocols for the codec. The codec driver will later bind to this.
            ProtocolHandle = NULL;
            Status = gBS->InstallMultipleProtocolInterfaces(&ProtocolHandle, &gEfiDevicePathProtocolGuid, HdaCodecDevicePath,
                &gEfiHdaIoProtocolGuid, &HdaDev->PrivateDatas[i]->HdaCodec, NULL);
            if (EFI_ERROR(Status))
                goto HDA_CODEC_CLEANUP_POST;

            // Connect child to parent.
            Status = gBS->OpenProtocol(HdaDev->ControllerHandle, &gEfiPciIoProtocolGuid, &TmpProtocol,
                HdaDev->DriverBinding->DriverBindingHandle, ProtocolHandle, EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER);
            if (EFI_ERROR(Status))
                goto HDA_CODEC_CLEANUP_POST;

            // Codec registered successfully.
            Status = EFI_SUCCESS;
            continue;

HDA_CODEC_CLEANUP_POST:
            DEBUG((DEBUG_INFO, "HdaControllerScanCodecs(): failed to load driver for codec @ 0x%X\n", i));

            // Remove protocol interfaces.
            gBS->UninstallMultipleProtocolInterfaces(&ProtocolHandle, &gEfiDevicePathProtocolGuid, HdaCodecDevicePath,
                &gEfiHdaIoProtocolGuid, &HdaDev->PrivateDatas[i]->HdaCodec, NULL);

            // Free objects.
            FreePool(HdaCodecDevicePath);
            FreePool(HdaDev->PrivateDatas[i]);
            HdaDev->PrivateDatas[i] = NULL;
        }
    }
    
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerSendCommands(
    IN HDA_CONTROLLER_DEV *HdaDev,
    IN UINT8 CodecAddress,
    IN UINT8 Node,
    IN EFI_HDA_IO_VERB_LIST *Verbs) {
    //DEBUG((DEBUG_INFO, "HdaControllerSendCommands(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;
    UINT32 *HdaCorb;
    UINT64 *HdaRirb;

    // Ensure parameters are valid.
    if (CodecAddress >= HDA_MAX_CODECS || Verbs == NULL || Verbs->Count < 1)
        return EFI_INVALID_PARAMETER;

    // Get pointers to CORB and RIRB.
    HdaCorb = HdaDev->CorbBuffer;
    HdaRirb = HdaDev->RirbBuffer;

    UINT32 RemainingVerbs = Verbs->Count;
    UINT32 RemainingResponses = Verbs->Count;
    UINT16 HdaCorbReadPointer;
    UINT16 HdaRirbWritePointer;
    BOOLEAN ResponseReceived;
    UINT8 ResponseTimeout;
    UINT64 RirbResponse;
    UINT32 VerbCommand;

    // Lock.
    AcquireSpinLock(&HdaDev->SpinLock);

    do {
        // Keep sending verbs until they are all sent.
        if (RemainingVerbs) {
            // Get current CORB read pointer.
            Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, 1, &HdaCorbReadPointer);
            if (EFI_ERROR(Status))
                goto DONE;

            // Add verbs to CORB until all of them are added or the CORB becomes full.
            //DEBUG((DEBUG_INFO, "HdaControllerSendCommands(): current CORB W: %u R: %u\n", HdaDev->CorbWritePointer, HdaCorbReadPointer));
            while (RemainingVerbs && ((HdaDev->CorbWritePointer + 1 % HdaDev->CorbEntryCount) != HdaCorbReadPointer)) {
                //DEBUG((DEBUG_INFO, "HdaControllerSendCommands(): %u verbs remaining\n", RemainingVerbs));

                // Move write pointer and write verb to CORB.
                HdaDev->CorbWritePointer++;
                HdaDev->CorbWritePointer %= HdaDev->CorbEntryCount;
                VerbCommand = HDA_CORB_VERB(CodecAddress, Node, Verbs->Verbs[Verbs->Count - RemainingVerbs]);
                HdaCorb[HdaDev->CorbWritePointer] = VerbCommand;
                //DEBUG((DEBUG_INFO, "HdaControllerSendCommands(): 0x%8X verb written\n", VerbCommand));

                // Move to next verb.
                RemainingVerbs--;
            }

            // Set CORB write pointer.
            Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBWP, 1, &HdaDev->CorbWritePointer);
            if (EFI_ERROR(Status))
                goto DONE;
        }

       // UINT16 corpwrite;
       // Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBWP, 1, &corpwrite);
        //    if (EFI_ERROR(Status))
        //        goto Done;
       // Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, 1, &HdaCorbReadPointer);
       // if (EFI_ERROR(Status))
        //    goto Done;
        //DEBUG((DEBUG_INFO, "HdaControllerSendCommands(): current CORB W: %u R: %u\n", corpwrite, HdaCorbReadPointer));

        // Get responses from RIRB.
        ResponseReceived = FALSE;
        ResponseTimeout = 10;
        while (!ResponseReceived) {
            // Get current RIRB write pointer.
            Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_RIRBWP, 1, &HdaRirbWritePointer);
            if (EFI_ERROR(Status))
                goto DONE;

            // If the read and write pointers differ, there are responses waiting.
            //DEBUG((DEBUG_INFO, "HdaControllerSendCommands(): current RIRB W: %u R: %u\n", HdaRirbWritePointer, HdaDev->RirbReadPointer));
            while (HdaDev->RirbReadPointer != HdaRirbWritePointer) {
               // DEBUG((DEBUG_INFO, "HdaControllerSendCommands(): %u responses remaining\n", RemainingResponses));
                
                // Increment RIRB read pointer.
                HdaDev->RirbReadPointer++;
                HdaDev->RirbReadPointer %= HdaDev->RirbEntryCount;

                // Get response and ensure it belongs to the current codec.
                RirbResponse = HdaRirb[HdaDev->RirbReadPointer];
                //DEBUG((DEBUG_INFO, "HdaControllerSendCommands(): 0x%16llX response read\n", RirbResponse));
                if (HDA_RIRB_CAD(RirbResponse) != CodecAddress || HDA_RIRB_UNSOL(RirbResponse))
                    continue;

                // Add response to list.
                Verbs->Responses[Verbs->Count - RemainingResponses] = HDA_RIRB_RESP(RirbResponse);
                RemainingResponses--;
                ResponseReceived = TRUE;
            }

            // If no response still, wait a bit.
            if (!ResponseReceived) {
                // If timeout reached, fail.
                if (!ResponseTimeout) {
                    Status = EFI_TIMEOUT;
                    goto DONE;
                }

                ResponseTimeout--;
                gBS->Stall(MS_TO_MICROSECOND(5));
                if (ResponseTimeout < 5)
                    DEBUG((DEBUG_INFO, "%u timeouts reached while waiting for response!\n", ResponseTimeout));
            }
        }

    } while (RemainingVerbs || RemainingResponses);
    //DEBUG((DEBUG_INFO, "HdaControllerSendCommands(): done\n"));

    Status = EFI_SUCCESS;
DONE:
    ReleaseSpinLock(&HdaDev->SpinLock);
    return Status;
}

EFI_STATUS
EFIAPI
HdaControllerInstallInfoProtocol(
    IN HDA_CONTROLLER_DEV *HdaControllerDev) {
    DEBUG((DEBUG_INFO, "HdaControllerInstallInfoProtocol(): start\n"));

    // Create variables.
    HDA_CONTROLLER_INFO_PRIVATE_DATA *HdaControllerInfoData;

    // Allocate space for info protocol data.
    HdaControllerInfoData = AllocateZeroPool(sizeof(HDA_CONTROLLER_INFO_PRIVATE_DATA));
    if (HdaControllerInfoData == NULL)
        return EFI_OUT_OF_RESOURCES;

    // Populate data.
    HdaControllerInfoData->Signature = HDA_CONTROLLER_PRIVATE_DATA_SIGNATURE;
    HdaControllerInfoData->HdaControllerDev = HdaControllerDev;
    HdaControllerInfoData->HdaControllerInfo.GetName = HdaControllerInfoGetName;

    // Install protocol.
    HdaControllerDev->HdaControllerInfoData = HdaControllerInfoData;
    return gBS->InstallMultipleProtocolInterfaces(&HdaControllerDev->ControllerHandle,
        &gEfiHdaControllerInfoProtocolGuid, &HdaControllerInfoData->HdaControllerInfo, NULL);
}

EFI_STATUS
EFIAPI
HdaControllerDriverBindingSupported(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL) {

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo;
    
    HDA_PCI_CLASSREG HdaClassReg;
    UINTN HdaSegmentNo, HdaBusNo, HdaDeviceNo, HdaFunctionNo;
    UINT32 HdaVendorDeviceId;

    // Open PCI I/O protocol. If this fails, its probably not a PCI device.
    Status = gBS->OpenProtocol(ControllerHandle, &gEfiPciIoProtocolGuid, (VOID**)&PciIo,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    if (EFI_ERROR(Status))
        return Status;

    // Read class code from PCI.
    Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, PCI_CLASSCODE_OFFSET,
        sizeof(HDA_PCI_CLASSREG) / sizeof(UINT8), &HdaClassReg);
    if (EFI_ERROR(Status))
        goto CLOSE_PCIIO;

    // Check class code. If not an HDA controller, we cannot support it.
    if (HdaClassReg.Class != PCI_CLASS_MEDIA || HdaClassReg.SubClass != PCI_CLASS_MEDIA_HDA) {
        Status = EFI_UNSUPPORTED;
        goto CLOSE_PCIIO;
    }

    // If we get here, we found a controller.
    // Get location of PCI device.
    Status = PciIo->GetLocation(PciIo, &HdaSegmentNo, &HdaBusNo, &HdaDeviceNo, &HdaFunctionNo);
    if (EFI_ERROR(Status))
        goto CLOSE_PCIIO;

    // Get vendor and device IDs of PCI device.
    Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint32, PCI_VENDOR_ID_OFFSET, 1, &HdaVendorDeviceId);
    if (EFI_ERROR (Status))
        goto CLOSE_PCIIO;

    // Print HDA controller info.
    DEBUG((DEBUG_INFO, "Found HDA controller (%4X:%4X) at %2X:%2X.%X!\n", GET_PCI_VENDOR_ID(HdaVendorDeviceId),
        GET_PCI_DEVICE_ID(HdaVendorDeviceId), HdaBusNo, HdaDeviceNo, HdaFunctionNo));
    Status = EFI_SUCCESS;

CLOSE_PCIIO:
    // Close PCI I/O protocol and return status.
    gBS->CloseProtocol(ControllerHandle, &gEfiPciIoProtocolGuid, This->DriverBindingHandle, ControllerHandle);
    return Status;
}

EFI_STATUS
EFIAPI
HdaControllerDriverBindingStart(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL) {
    DEBUG((DEBUG_INFO, "HdaControllerDriverBindingStart(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo;
    EFI_DEVICE_PATH_PROTOCOL *HdaDevicePath;

    UINT64 OriginalPciAttributes;
    UINT64 PciSupports;
    BOOLEAN PciAttributesSaved = FALSE;
    UINT32 HdaVendorDeviceId;

    // Version.
    UINT8 HdaMajorVersion, HdaMinorVersion;

    // Open PCI I/O protocol.
    Status = gBS->OpenProtocol(ControllerHandle, &gEfiPciIoProtocolGuid, (VOID**)&PciIo,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    if (EFI_ERROR (Status))
        return Status;

    // Open Device Path protocol.
    Status = gBS->OpenProtocol(ControllerHandle, &gEfiDevicePathProtocolGuid, (VOID**)&HdaDevicePath,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    if (EFI_ERROR (Status))
        return Status;

    // Get original PCI I/O attributes.
    Status = PciIo->Attributes(PciIo, EfiPciIoAttributeOperationGet, 0, &OriginalPciAttributes);
    if (EFI_ERROR(Status))
        goto CLOSE_PCIIO;
    PciAttributesSaved = TRUE;

    // Get currently supported PCI I/O attributes.
    Status = PciIo->Attributes(PciIo, EfiPciIoAttributeOperationSupported, 0, &PciSupports);
    if (EFI_ERROR(Status))
        goto CLOSE_PCIIO;

    // Enable the PCI device.
    PciSupports &= EFI_PCI_DEVICE_ENABLE;
    Status = PciIo->Attributes(PciIo, EfiPciIoAttributeOperationEnable, PciSupports, NULL);
    if (EFI_ERROR(Status))
        goto CLOSE_PCIIO;

    // Get vendor and device IDs of PCI device.
    Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint32, PCI_VENDOR_ID_OFFSET, 1, &HdaVendorDeviceId);
    if (EFI_ERROR (Status))
        goto CLOSE_PCIIO;

    DEBUG((DEBUG_INFO, "HdaControllerDriverBindingStart(): attached to controller %4X:%4X\n",
        GET_PCI_VENDOR_ID(HdaVendorDeviceId), GET_PCI_DEVICE_ID(HdaVendorDeviceId)));

    // Is this an Intel controller?
    if (GET_PCI_VENDOR_ID(HdaVendorDeviceId) == VEN_INTEL_ID) {
        UINT8 HdaTcSel;
        
        // Set TC0 in TCSEL register.
        Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_TCSEL_OFFSET, 1, &HdaTcSel);
        if (EFI_ERROR (Status))
            goto CLOSE_PCIIO;
        HdaTcSel &= PCI_HDA_TCSEL_TC0_MASK;
        Status = PciIo->Pci.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_TCSEL_OFFSET, 1, &HdaTcSel);
        if (EFI_ERROR (Status))
            goto CLOSE_PCIIO;
    }

    // Disable No Snoop Enable bit.
    UINT16 HdaDevC;
    Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_DEVC_OFFSET, 1, &HdaDevC);
    if (EFI_ERROR (Status))
        goto CLOSE_PCIIO;

    HdaDevC &= ~PCI_HDA_DEVC_NOSNOOPEN;
    Status = PciIo->Pci.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_DEVC_OFFSET, 1, &HdaDevC);
    if (EFI_ERROR (Status))
        goto CLOSE_PCIIO;

    // Get major/minor version.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_VMAJ, 1, &HdaMajorVersion);
    if (EFI_ERROR(Status))
        goto CLOSE_PCIIO;
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_VMIN, 1, &HdaMinorVersion);
    if (EFI_ERROR(Status))
        goto CLOSE_PCIIO;

    // Validate version. If invalid abort.
    DEBUG((DEBUG_INFO, "HDA controller version %u.%u.\n", HdaMajorVersion, HdaMinorVersion));
    if (HdaMajorVersion < HDA_VERSION_MIN_MAJOR) {
        Status = EFI_UNSUPPORTED;
        goto CLOSE_PCIIO;
    }

    // Allocate device.
    HDA_CONTROLLER_DEV *HdaDev = HdaControllerAllocDevice(PciIo, HdaDevicePath, OriginalPciAttributes);
    HdaDev->ControllerHandle = ControllerHandle;
    HdaDev->DriverBinding = This;
    HdaDev->MajorVersion = HdaMajorVersion;
    HdaDev->MinorVersion = HdaMinorVersion;
    HdaDev->VendorId = HdaVendorDeviceId;

    // Try to match controller name.
    HdaDev->Name = NULL;
    UINTN ControllerIndex = 0;
    while (gHdaControllerList[ControllerIndex].Id != 0) {
        // Check ID and revision against array element.
        if (gHdaControllerList[ControllerIndex].Id == HdaDev->VendorId)
            HdaDev->Name = gHdaControllerList[ControllerIndex].Name;
        ControllerIndex++;
    }

    // If match wasn't found, try again with a generic device ID.
    if (HdaDev->Name == NULL) {
        ControllerIndex = 0;
        while (gHdaControllerList[ControllerIndex].Id != 0) {
            // Check ID and revision against array element.
            if (gHdaControllerList[ControllerIndex].Id == GET_PCI_GENERIC_ID(HdaDev->VendorId))
                HdaDev->Name = gHdaControllerList[ControllerIndex].Name;
            ControllerIndex++;
        }
    }

    // If match still wasn't found, codec is unknown.
    if (HdaDev->Name == NULL)
        HdaDev->Name = L"HD Audio Controller";
    DEBUG((DEBUG_INFO, "Controller name: %s\n", HdaDev->Name));

    // Get capabilities.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_GCAP, 1, &HdaDev->Capabilities);
    if (EFI_ERROR(Status))
        goto CLOSE_PCIIO;
    DEBUG((DEBUG_INFO, "HDA controller capabilities:\n  64-bit: %s  Serial Data Out Signals: %u\n",
        HdaDev->Capabilities & HDA_REG_GCAP_64OK ? L"Yes" : L"No", HDA_REG_GCAP_NSDO(HdaDev->Capabilities)));    
    DEBUG((DEBUG_INFO, "  Bidir streams: %u  Input streams: %u  Output streams: %u\n", HDA_REG_GCAP_BSS(HdaDev->Capabilities),
        HDA_REG_GCAP_ISS(HdaDev->Capabilities), HDA_REG_GCAP_OSS(HdaDev->Capabilities)));

    // Reset controller.
    Status = HdaControllerReset(PciIo);
    if (EFI_ERROR(Status))
        goto CLOSE_PCIIO;

    // Install info protocol.
    Status = HdaControllerInstallInfoProtocol(HdaDev);
    ASSERT_EFI_ERROR(Status);

        UINT32 *dmaList;
    EFI_PHYSICAL_ADDRESS dmaListAddr;
    VOID *dmaListMapping;
    UINTN dmaListLengthActual;
    Status = PciIo->AllocateBuffer(PciIo, AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES(4096),
        (VOID**)&dmaList, 0);
    if (EFI_ERROR(Status))
        ASSERT_EFI_ERROR(Status);
    ZeroMem(dmaList, 4096);

        // Map buffer descriptor list.
    dmaListLengthActual = 4096;
    Status = PciIo->Map(PciIo, EfiPciIoOperationBusMasterCommonBuffer, dmaList, &dmaListLengthActual,
        &dmaListAddr, &dmaListMapping);
    if (EFI_ERROR(Status))
        ASSERT_EFI_ERROR(Status);
    if (dmaListLengthActual != 4096) {
        Status = EFI_OUT_OF_RESOURCES;
        ASSERT_EFI_ERROR(Status);
    }
    HdaDev->dmaList = dmaList;

    // Set buffer lower base address.
   /* UINT32 dmaListLowerBaseAddr = (UINT32)dmaListAddr;
    dmaListLowerBaseAddr |= 0x1;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, 0x70, 1, &dmaListLowerBaseAddr);
    if (EFI_ERROR(Status))
       ASSERT_EFI_ERROR(Status);

    // If 64-bit supported, set upper base address.
    if (HdaDev->Capabilities & HDA_REG_GCAP_64OK) {
        UINT32 dmaListUpperBaseAddr = (UINT32)(dmaListAddr >> 32);
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, 0x74, 1, &dmaListUpperBaseAddr);
        if (EFI_ERROR(Status))
           ASSERT_EFI_ERROR(Status);
    }*/

    // Initialize CORB and RIRB.
    Status = HdaControllerInitCorb(HdaDev);
    if (EFI_ERROR(Status))
        goto CLEANUP_CORB_RIRB;
    Status = HdaControllerInitRirb(HdaDev);
    if (EFI_ERROR(Status))
        goto CLEANUP_CORB_RIRB;



    // needed for QEMU.
   // UINT16 dd = 0xFF;
   // PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_RINTCNT, 1, &dd);

    // Start CORB and RIRB
    Status = HdaControllerSetCorb(HdaDev, TRUE);
    if (EFI_ERROR(Status))
        goto CLEANUP_CORB_RIRB;
    Status = HdaControllerSetRirb(HdaDev, TRUE);
    if (EFI_ERROR(Status))
        goto CLEANUP_CORB_RIRB;

    // Init streams.
    Status = HdaControllerInitStreams(HdaDev);
    if (EFI_ERROR(Status))
        goto CLEANUP_CORB_RIRB;

    // Scan for codecs.
    Status = HdaControllerScanCodecs(HdaDev, This, ControllerHandle);
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

        Status = root->Open(root, &token, L"welcome.raw", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
        if (!(EFI_ERROR(Status)))
            break;
    }
    HdaDev->token = token;

   // UINTN blocklength = 2000000;

    // Get stream.
    HDA_STREAM *HdaOutStream = HdaDev->OutputStreams + 0;

    // Get size.
    EFI_FILE_INFO FileInfo;
    UINTN FileInfoSize;
    Status = token->GetInfo(token, &gEfiFileInfoGuid, &FileInfoSize, &FileInfo);
    ASSERT_EFI_ERROR(Status);

    HdaOutStream->BufferSource = AllocatePool(FileInfo.FileSize);
    HdaOutStream->BufferSourceLength = FileInfo.FileSize;

    // Read file data.
    Status = token->Read(token, &HdaOutStream->BufferSourceLength, HdaOutStream->BufferSource);
    ASSERT_EFI_ERROR(Status);
    ASSERT(HdaOutStream->BufferSourceLength == FileInfo.FileSize);

    // Fill buffer initially
    CopyMem(HdaOutStream->BufferData, HdaOutStream->BufferSource, HDA_STREAM_BUF_SIZE);
    HdaOutStream->BufferSourcePosition = HDA_STREAM_BUF_SIZE;
    HdaOutStream->DoUpperHalf = FALSE;



    UINT16 stmFormat = 0x4011;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_SDNFMT(HdaOutStream->Index), 1, &stmFormat);
    ASSERT_EFI_ERROR(Status);

    // poll timer
    Status = gBS->SetTimer(HdaOutStream->PollTimer, TimerPeriodic, EFI_TIMER_PERIOD_MILLISECONDS(100));
    if (EFI_ERROR(Status))
        goto CLEANUP_CORB_RIRB;

    // Update stream.
    Status = HdaControllerSetStream(HdaOutStream, TRUE, 6);
    ASSERT_EFI_ERROR(Status);
    
    DEBUG((DEBUG_INFO, "HdaControllerDriverBindingStart(): done\n"));
    return EFI_SUCCESS;

CLEANUP_CORB_RIRB:
    DEBUG((DEBUG_INFO, "HdaControllerDriverBindingStart(): Abort via CLEANUP_CORB_RIRB.\n"));

    // Cleanup CORB and RIRB.
    HdaControllerCleanupCorb(HdaDev);
    HdaControllerCleanupRirb(HdaDev);

CLOSE_PCIIO:
    DEBUG((DEBUG_INFO, "HdaControllerDriverBindingStart(): Abort via CLOSE_PCIIO.\n"));

    // Restore PCI attributes if needed.
    if (PciAttributesSaved)
        PciIo->Attributes(PciIo, EfiPciIoAttributeOperationSet, OriginalPciAttributes, NULL);

    // Close PCI I/O protocol.
    gBS->CloseProtocol(ControllerHandle, &gEfiPciIoProtocolGuid, This->DriverBindingHandle, ControllerHandle);
    return Status;
}

EFI_STATUS
EFIAPI
HdaControllerDriverBindingStop(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN UINTN NumberOfChildren,
    IN EFI_HANDLE *ChildHandleBuffer OPTIONAL) {
    DEBUG((DEBUG_INFO, "HdaControllerDriverBindingStop()\n"));
    //EFI_STATUS status;

    // Get private data.
  //  BOOTCHIME_PRIVATE_DATA *private;
   // private = BOOTCHIME_PRIVATE_DATA_FROM_THIS(This);

    // Reset PCI I/O attributes and close protocol.
  //  status = private->PciIo->Attributes(private->PciIo, EfiPciIoAttributeOperationSet, private->OriginalPciAttributes, NULL);
    gBS->CloseProtocol(ControllerHandle, &gEfiPciIoProtocolGuid, This->DriverBindingHandle, ControllerHandle);
    //FreePool(private);
    return EFI_SUCCESS;
}
