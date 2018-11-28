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

#include "HdaCodecProtocol.h"
#include "HdaCodec.h"
#include <Protocol/DevicePathUtilities.h>

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
    
    UINT16 HdaCorbRp;
    HdaDev->PciIo->Mem.Read(HdaDev->PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBWP, 1, &HdaCorbRp);
    UINT16 HdaRirbWp;
    HdaDev->PciIo->Mem.Read(HdaDev->PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_RIRBWP, 1, &HdaRirbWp);
    //DEBUG((DEBUG_INFO, "CORB: 0x%X RIRB: 0x%X 0x%X\n", HdaCorbRp, HdaRirbWp, HdaDev->RirbBuffer[1]));
}

EFI_STATUS
EFIAPI
HdaControllerReset(
    IN EFI_PCI_IO_PROTOCOL *PciIo) {
    DEBUG((DEBUG_INFO, "HdaControllerReset()\n"));

    // Create variables.
    EFI_STATUS Status;
    UINT32 hdaGCtl;
    UINT64 hdaGCtlPoll;

    // Get value of CRST bit.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, 1, &hdaGCtl);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;

    // Check if the controller is already in reset. If not, set bit to zero.
    if (!(hdaGCtl & HDA_REG_GCTL_CRST)) {
        hdaGCtl &= ~HDA_REG_GCTL_CRST;
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, 1, &hdaGCtl);
        ASSERT_EFI_ERROR(Status);
        if (EFI_ERROR(Status))
            return Status;
    }

    // Write a one to the CRST bit to begin the process of coming out of reset.
    hdaGCtl |= HDA_REG_GCTL_CRST;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, 1, &hdaGCtl);
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

            // Create Device Path for codec.
            EFI_HDA_CODEC_DEVICE_PATH HdaCodecDevicePathNode = EFI_HDA_CODEC_DEVICE_PATH_TEMPLATE;
            HdaCodecDevicePathNode.Address = i;
            HdaCodecDevicePath = AppendDevicePathNode(HdaDev->DevicePath, (EFI_DEVICE_PATH_PROTOCOL*)&HdaCodecDevicePathNode);

            // Install protocols for the codec. The codec driver will later bind to this.
            ProtocolHandle = NULL;
            Status = gBS->InstallMultipleProtocolInterfaces(&ProtocolHandle, &gEfiDevicePathProtocolGuid, HdaCodecDevicePath,
                &gEfiHdaCodecProtocolGuid, &HdaPrivateData->HdaCodec, NULL);
            ASSERT_EFI_ERROR(Status); // TODO: Free device path and such on error.

            // Connect child to parent.
            Status = gBS->OpenProtocol(HdaDev->ControllerHandle, &gEfiPciIoProtocolGuid, &TmpProtocol,
                HdaDev->DriverBinding->DriverBindingHandle, ProtocolHandle, EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER);
            ASSERT_EFI_ERROR(Status);
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
    DEBUG((DEBUG_INFO, "HdaControllerSendCommands(): start\n"));

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

    do {
        // Keep sending verbs until they are all sent.
        if (RemainingVerbs) {
            // Get current CORB read pointer.
            Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, 1, &HdaCorbReadPointer);
            if (EFI_ERROR(Status))
                return Status;

            // Add verbs to CORB until all of them are added or the CORB becomes full.
            DEBUG((DEBUG_INFO, "HdaControllerSendCommands(): current CORB W: %u R: %u\n", HdaDev->CorbWritePointer, HdaCorbReadPointer));
            while (RemainingVerbs && ((HdaDev->CorbWritePointer + 1 % HdaDev->CorbEntryCount) != HdaCorbReadPointer)) {
                DEBUG((DEBUG_INFO, "HdaControllerSendCommands(): %u verbs remaining\n", RemainingVerbs));

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
                return Status;
        }

        UINT16 corpwrite;
        Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBWP, 1, &corpwrite);
            if (EFI_ERROR(Status))
                return Status;
        Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, 1, &HdaCorbReadPointer);
        if (EFI_ERROR(Status))
            return Status;
        DEBUG((DEBUG_INFO, "HdaControllerSendCommands(): current CORB W: %u R: %u\n", corpwrite, HdaCorbReadPointer));

        // Get responses from RIRB.
        ResponseReceived = FALSE;
        ResponseTimeout = 10;
        while (!ResponseReceived) {
            // Get current RIRB write pointer.
            Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_RIRBWP, 1, &HdaRirbWritePointer);
            if (EFI_ERROR(Status))
                return Status;

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
                    return EFI_TIMEOUT;
                }

                ResponseTimeout--;
                gBS->Stall(50000);
                DEBUG((DEBUG_INFO, "Timeout reached while waiting for response!\n"));
            }
        }

    } while (RemainingVerbs || RemainingResponses);
    //DEBUG((DEBUG_INFO, "HdaControllerSendCommands(): done\n"));

    return EFI_SUCCESS;
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
    DEBUG((DEBUG_INFO, "Found HDA controller (%4X:%4X) at %2X:%2X.%X!\n", HdaVendorDeviceId & 0xFFFF,
        (HdaVendorDeviceId >> 16) & 0xFFFF, HdaBusNo, HdaDeviceNo, HdaFunctionNo));
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

    // Version, GCAP, and whether or not 64-bit addressing is supported.
    UINT8 HdaMajorVersion, HdaMinorVersion;
    UINT16 HdaGCap;
    BOOLEAN Hda64BitSupported;

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
        (HdaVendorDeviceId >> 16) & 0xFFFF, HdaVendorDeviceId & 0xFFFF));

    // Is this an Intel controller?
    if ((HdaVendorDeviceId & 0xFFFF) == INTEL_VEN_ID) {
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

    // Get major/minor version and GCAP.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_VMAJ, 1, &HdaMajorVersion);
    if (EFI_ERROR(Status))
        goto CLOSE_PCIIO;
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_VMIN, 1, &HdaMinorVersion);
    if (EFI_ERROR(Status))
        goto CLOSE_PCIIO;
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_GCAP, 1, &HdaGCap);
    if (EFI_ERROR(Status))
        goto CLOSE_PCIIO;
    
    // Get value of 64BIT bit.
    Hda64BitSupported = (BOOLEAN)(HdaGCap & HDA_REG_GCAP_64BIT);

    // Validate version. If invalid abort.
    DEBUG((DEBUG_INFO, "HDA controller version %u.%u. Max addressing mode supported: %u-bit.\n", HdaMajorVersion,
        HdaMinorVersion, Hda64BitSupported ? 64 : 32));
    if (HdaMajorVersion < HDA_VERSION_MIN_MAJOR) {
        Status = EFI_UNSUPPORTED;
        goto CLOSE_PCIIO;
    }

    // Allocate device.
    HDA_CONTROLLER_DEV *HdaDev = HdaControllerAllocDevice(PciIo, HdaDevicePath, OriginalPciAttributes);
    HdaDev->Buffer64BitSupported = Hda64BitSupported;
    HdaDev->ControllerHandle = ControllerHandle;
    HdaDev->DriverBinding = This;

    // Reset controller.
    Status = HdaControllerReset(PciIo);
    if (EFI_ERROR(Status))
        goto CLOSE_PCIIO;

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
    UINT16 dd = 0xFF;
    PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_RINTCNT, 1, &dd);

    // Start CORB and RIRB
    Status = HdaControllerEnableCorb(HdaDev);
    if (EFI_ERROR(Status))
        goto CLEANUP_CORB_RIRB;
    Status = HdaControllerEnableRirb(HdaDev);
    if (EFI_ERROR(Status))
        goto CLEANUP_CORB_RIRB;

    // Scan for codecs.
    Status = HdaControllerScanCodecs(HdaDev, This, ControllerHandle);
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





//
// Driver Binding Protocol
//
EFI_DRIVER_BINDING_PROTOCOL gHdaControllerDriverBinding = {
    HdaControllerDriverBindingSupported,
    HdaControllerDriverBindingStart,
    HdaControllerDriverBindingStop,
    AUDIODXE_VERSION,
    NULL,
    NULL
};

EFI_STATUS
EFIAPI
HdaControllerRegisterDriver(VOID) {
    EFI_STATUS Status;

    // Register HdaController driver binding.
    Status = EfiLibInstallDriverBindingComponentName2(gAudioDxeImageHandle, gAudioDxeSystemTable, &gHdaControllerDriverBinding,
        gAudioDxeImageHandle, &gHdaControllerComponentName, &gHdaControllerComponentName2);
    ASSERT_EFI_ERROR(Status);
    return Status;
}
