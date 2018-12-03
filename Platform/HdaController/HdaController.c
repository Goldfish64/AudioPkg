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

#include "HdaCodec/HdaCodecProtocol.h"
#include "HdaCodec/HdaCodec.h"
#include "HdaCodec/HdaCodecComponentName.h"
#include "HdaCodec/HdaVerbs.h"

// HDA Codec Device Path GUID.
EFI_GUID gEfiHdaCodecDevicePathGuid = EFI_HDA_CODEC_DEVICE_PATH_GUID;

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
    IN EFI_HDA_CODEC_PROTOCOL *This,
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
    IN EFI_HDA_CODEC_PROTOCOL *This,
    IN UINT8 Node,
    IN UINT32 Verb,
    OUT UINT32 *Response) {

    // Create verb list with single item.
    EFI_HDA_CODEC_VERB_LIST HdaCodecVerbList;
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
    IN EFI_HDA_CODEC_PROTOCOL *This,
    IN UINT8 Node,
    IN EFI_HDA_CODEC_VERB_LIST *Verbs) {
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
    
    UINT32 lpib;
    HdaDev->PciIo->Mem.Read(HdaDev->PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_SDLPIB(0), 1, &lpib);
   // DEBUG((DEBUG_INFO, "LPIB: 0x%X\n", lpib));

  // DEBUG((DEBUG_INFO, "pos streams %u %u %u %u %u %u\n", HdaDev->dmaList[0], HdaDev->dmaList[2], HdaDev->dmaList[4], HdaDev->dmaList[6], HdaDev->dmaList[8], HdaDev->dmaList[10]));
}

EFI_STATUS
EFIAPI
HdaControllerReset(
    IN EFI_PCI_IO_PROTOCOL *PciIo) {
    DEBUG((DEBUG_INFO, "HdaControllerReset()\n"));

    // Create variables.
    EFI_STATUS Status;
    HDA_GCTL HdaGCtl;
    UINT64 hdaGCtlPoll;

    // Get value of CRST bit.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, 1, (UINT32*)&HdaGCtl);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;

    // Check if the controller is already in reset. If not, set bit to zero.
    if (!HdaGCtl.Reset) {
        HdaGCtl.Reset = FALSE;
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, 1, (UINT32*)&HdaGCtl);
        ASSERT_EFI_ERROR(Status);
        if (EFI_ERROR(Status))
            return Status;
    }

    // Write a one to the CRST bit to begin the process of coming out of reset.
    HdaGCtl.Reset = TRUE;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, 1, (UINT32*)&HdaGCtl);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;

    // Wait for bit to be set. Once bit is set, the controller is ready.
    Status = PciIo->PollMem(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, HDA_REG_GCTL_CRST, HDA_REG_GCTL_CRST, 1000, &hdaGCtlPoll);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;

    // Wait 50ms to ensure all codecs have also reset.
    gBS->Stall(50000);

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
    UINT32 HdaCodecVendorDeviceId;

    CHAR16 *HdaCodecName;
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

            // Get vendor/device ID for codec.
            Status = HdaPrivateData->HdaCodec.SendCommand(&HdaPrivateData->HdaCodec, HDA_NID_ROOT,
                HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_VENDOR_ID), &HdaCodecVendorDeviceId);
            if (EFI_ERROR(Status))
                goto HDA_CODEC_CLEANUP;
            HdaPrivateData->VendorId = GET_CODEC_VENDOR_ID(HdaCodecVendorDeviceId);
            HdaPrivateData->DeviceId = GET_CODEC_DEVICE_ID(HdaCodecVendorDeviceId);

            // Get codec name.
            HdaCodecName = AudioDxeGetCodecName(HdaPrivateData->VendorId, HdaPrivateData->DeviceId);

            // Add names.
            Status = AddUnicodeString2("eng", gHdaCodecComponentName.SupportedLanguages, &HdaPrivateData->HdaCodecNameTable, HdaCodecName, TRUE);
            if (EFI_ERROR(Status))
                goto HDA_CODEC_CLEANUP;
            Status = AddUnicodeString2("en", gHdaCodecComponentName2.SupportedLanguages, &HdaPrivateData->HdaCodecNameTable, HdaCodecName, TRUE);
            if (EFI_ERROR(Status))
                goto HDA_CODEC_CLEANUP;

            // Add to array.
            HdaDev->PrivateDatas[i] = HdaPrivateData;
            Status = EFI_SUCCESS;
            continue;

HDA_CODEC_CLEANUP:
            DEBUG((DEBUG_INFO, "HdaControllerScanCodecs(): failed to load driver for codec @ 0x%X\n", i));
            HdaDev->PrivateDatas[i] = NULL;

            // Free objects.
            FreeUnicodeStringTable(HdaPrivateData->HdaCodecNameTable);
            FreePool(HdaPrivateData);
        }
    }

    // Install protocols on each codec.
    for (UINT8 i = 0; i < HDA_MAX_CODECS; i++) {
        // Do we have a codec at this address?
        if (HdaDev->PrivateDatas[i] != NULL) {
            // Create Device Path for codec.
            EFI_HDA_CODEC_DEVICE_PATH HdaCodecDevicePathNode = EFI_HDA_CODEC_DEVICE_PATH_TEMPLATE;
            HdaCodecDevicePathNode.Address = i;
            HdaCodecDevicePath = AppendDevicePathNode(HdaDev->DevicePath, (EFI_DEVICE_PATH_PROTOCOL*)&HdaCodecDevicePathNode);
            if (HdaCodecDevicePath == NULL) {
                Status = EFI_INVALID_PARAMETER;
                goto HDA_CODEC_CLEANUP_POST;
            }

            // Install protocols for the codec. The codec driver will later bind to this.
            ProtocolHandle = NULL;
            Status = gBS->InstallMultipleProtocolInterfaces(&ProtocolHandle, &gEfiDevicePathProtocolGuid, HdaCodecDevicePath,
                &gEfiHdaCodecProtocolGuid, &HdaDev->PrivateDatas[i]->HdaCodec, NULL);
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
                &gEfiHdaCodecProtocolGuid, &HdaDev->PrivateDatas[i]->HdaCodec, NULL);

            // Free objects.
            FreeUnicodeStringTable(HdaDev->PrivateDatas[i]->HdaCodecNameTable);
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
    IN EFI_HDA_CODEC_VERB_LIST *Verbs) {
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
        DEBUG((DEBUG_INFO, "HDA controller is Intel.\n"));
        UINT8 HdaTcSel;
        
        // Set TC0 in TCSEL register.
        Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_TCSEL_OFFSET, 1, &HdaTcSel);
        if (EFI_ERROR (Status))
            goto CLOSE_PCIIO;
        HdaTcSel &= PCI_HDA_TCSEL_TC0_MASK;
        Status = PciIo->Pci.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_TCSEL_OFFSET, 1, &HdaTcSel);
        if (EFI_ERROR (Status))
            goto CLOSE_PCIIO;
    } else {
        return EFI_SUCCESS; //temp.
    }

    // Disable No Snoop Enable bit.
    UINT16 HdaDevC;
    Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_DEVC_OFFSET, 1, &HdaDevC);
    if (EFI_ERROR (Status))
        goto CLOSE_PCIIO;
    if (HdaDevC & PCI_HDA_DEVC_NOSNOOPEN)
        DEBUG((DEBUG_INFO, "snoop\n"));

    HdaDevC &= ~PCI_HDA_DEVC_NOSNOOPEN;
    Status = PciIo->Pci.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_DEVC_OFFSET, 1, &HdaDevC);
    if (EFI_ERROR (Status))
        goto CLOSE_PCIIO;
    DEBUG((DEBUG_INFO, "snoop register: 0x%X 0x%X 0x%X\n", HdaDevC, PCI_HDA_DEVC_NOSNOOPEN, ~PCI_HDA_DEVC_NOSNOOPEN));

    Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_DEVC_OFFSET, 1, &HdaDevC);
    if (EFI_ERROR (Status))
        goto CLOSE_PCIIO;
    DEBUG((DEBUG_INFO, "snoop register: 0x%X\n", HdaDevC));

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

    // Get capabilities.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_GCAP, 1, (UINT16*)&HdaDev->Capabilities);
    if (EFI_ERROR(Status))
        goto CLOSE_PCIIO;
    DEBUG((DEBUG_INFO, "HDA controller capabilities:\n  64-bit: %s  Serial Data Out Signals: %u\n",
        HdaDev->Capabilities.Addressing64Bit ? L"Yes" : L"No", HdaDev->Capabilities.NumSerialDataOutSignals));    
    DEBUG((DEBUG_INFO, "  Bidir streams: %u  Input streams: %u  Output streams: %u\n", HdaDev->Capabilities.NumBidirStreams,
        HdaDev->Capabilities.NumInputStreams, HdaDev->Capabilities.NumOutputStreams));

    // Reset controller.
    Status = HdaControllerReset(PciIo);
    if (EFI_ERROR(Status))
        goto CLOSE_PCIIO;

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
    UINT32 dmaListLowerBaseAddr = (UINT32)dmaListAddr;
    dmaListLowerBaseAddr |= 0x1;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, 0x70, 1, &dmaListLowerBaseAddr);
    if (EFI_ERROR(Status))
       ASSERT_EFI_ERROR(Status);

    // If 64-bit supported, set upper base address.
    if (HdaDev->Capabilities.Addressing64Bit) {
        UINT32 dmaListUpperBaseAddr = (UINT32)(dmaListAddr >> 32);
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, 0x74, 1, &dmaListUpperBaseAddr);
        if (EFI_ERROR(Status))
           ASSERT_EFI_ERROR(Status);
    }

    // Initialize CORB and RIRB.
    Status = HdaControllerInitCorb(HdaDev);
    if (EFI_ERROR(Status))
        goto CLEANUP_CORB_RIRB;
    Status = HdaControllerInitRirb(HdaDev);
    if (EFI_ERROR(Status))
        goto CLEANUP_CORB_RIRB;

    // Setup response ring buffer poll timer.
    Status = gBS->SetTimer(HdaDev->ResponsePollTimer, TimerPeriodic, EFI_TIMER_PERIOD_MILLISECONDS(1000));
    if (EFI_ERROR(Status))
        goto CLEANUP_CORB_RIRB;

    // needed for QEMU.
   // UINT16 dd = 0xFF;
   // PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_RINTCNT, 1, &dd);

    // Start CORB and RIRB
    Status = HdaControllerEnableCorb(HdaDev);
    if (EFI_ERROR(Status))
        goto CLEANUP_CORB_RIRB;
    Status = HdaControllerEnableRirb(HdaDev);
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

    Status = gBS->HandleProtocol(handles[0], &gEfiSimpleFileSystemProtocolGuid, (void**)&fs);
    ASSERT_EFI_ERROR(Status);

    EFI_FILE_PROTOCOL* root = NULL;
    Status = fs->OpenVolume(fs, &root);
    ASSERT_EFI_ERROR(Status);

    // opewn file.
    EFI_FILE_PROTOCOL* token = NULL;
    Status = root->Open(root, &token, L"win.raw", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    ASSERT_EFI_ERROR(Status);

    UINTN blocklength = 2000000;
     VOID *buffer1;
        EFI_PHYSICAL_ADDRESS buffer1Addr;
        VOID *buffer1Mapping;
        UINTN bufferLengthActual;
        Status = PciIo->AllocateBuffer(PciIo, AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES(blocklength),
            &buffer1, 0);
        if (EFI_ERROR(Status))
            ASSERT_EFI_ERROR(Status);
        ZeroMem(buffer1, blocklength);

            // Map buffer descriptor list.
        bufferLengthActual = blocklength;
        Status = PciIo->Map(PciIo, EfiPciIoOperationBusMasterCommonBuffer, buffer1, &bufferLengthActual,
            &buffer1Addr, &buffer1Mapping);
        if (EFI_ERROR(Status))
            ASSERT_EFI_ERROR(Status);
        if (bufferLengthActual != blocklength) {
            Status = EFI_OUT_OF_RESOURCES;
            ASSERT_EFI_ERROR(Status);
        }

        // copy le data.
        bufferLengthActual = blocklength;
        Status = token->Read(token, &bufferLengthActual, buffer1);
       // DEBUG((DEBUG_INFO, "%u bytes\n", bufferLengthActual));
        ASSERT_EFI_ERROR(Status);
        //Status = token->SetPosition(token, 4096);

        UINT32 blockcount = 2;
        UINT32 blocksize = blocklength / blockcount;

    for (int i = 1; i <= blockcount; i++) {


        HdaDev->OutputStreams[0].BufferList[i].Address = (UINT32)buffer1Addr;
        HdaDev->OutputStreams[0].BufferList[i].AddressHigh = (UINT32)(buffer1Addr >> 32);
        HdaDev->OutputStreams[0].BufferList[i].Length = blocksize;
        buffer1Addr += blocksize;
    }

    HDA_STREAMCTL HdaStreamCtl;

    // Get value of control register.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDCTL(1+4), 3, (UINT8*)&HdaStreamCtl);
    ASSERT_EFI_ERROR(Status);


    HdaStreamCtl.Number = 6;
    HdaStreamCtl.BidirOutput = 1;
    HdaStreamCtl.StripeControl = 0;
    HdaStreamCtl.PriorityTraffic = 0;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDCTL(1+4), 3, (UINT8*)&HdaStreamCtl);
    ASSERT_EFI_ERROR(Status);

    UINT16 lvi = blockcount - 1;
   // Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_SDLVI(1+4), 1, &lvi);
    //ASSERT_EFI_ERROR(Status);
   // lvi &= 0xFF00;
  // lvi |= 63;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_SDLVI(1+4), 1, &lvi);
    ASSERT_EFI_ERROR(Status);

    UINT32 cbllength = blockcount * blocksize;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_SDCBL(1+4), 1, &cbllength);
    ASSERT_EFI_ERROR(Status);

    UINT16 stmFormat = 0x4011;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_SDFMT(1+4), 1, &stmFormat);
    ASSERT_EFI_ERROR(Status);

    Status = HdaControllerEnableStream(HdaDev, 0);
    ASSERT_EFI_ERROR(Status);

    gBS->Stall(MS_TO_MICROSECOND(1000));

    // Get value of control register.
    UINT32 stes;
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_SDCTL(1+4), 1, &stes);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "stream cntl 0x%X\n", stes));
    
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
