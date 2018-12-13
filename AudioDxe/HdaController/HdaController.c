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
#include <Library/HdaRegisters.h>
#include "HdaControllerComponentName.h"

#include "HdaCodec/HdaCodec.h"
#include "HdaCodec/HdaCodecComponentName.h"

VOID
HdaControllerStreamPollTimerHandler(
    IN EFI_EVENT Event,
    IN VOID *Context) {
    
    // Create variables.
    EFI_STATUS Status;
    HDA_STREAM *HdaStream = (HDA_STREAM*)Context;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaStream->HdaDev->PciIo;
    UINT8 HdaStreamSts = 0;
    UINT32 HdaStreamDmaPos;
    UINTN HdaSourceLength;

    // Get stream status.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthFifoUint8, PCI_HDA_BAR, HDA_REG_SDNSTS(HdaStream->Index), 1, &HdaStreamSts);
    ASSERT_EFI_ERROR(Status);

    // If there was a FIFO error or DESC error, halt.
    ASSERT ((HdaStreamSts & (HDA_REG_SDNSTS_FIFOE | HDA_REG_SDNSTS_DESE)) == 0);

    // Has the completion bit been set?
    if (HdaStreamSts & HDA_REG_SDNSTS_BCIS) {
        // Have we reached the end of the source buffer? If so we need to stop the stream.
        if (HdaStream->BufferSourcePosition >= HdaStream->BufferSourceLength) {
            // Stop stream.
            Status = HdaControllerSetStream(HdaStream, FALSE);
            ASSERT_EFI_ERROR(Status);

            // Stop timer.
            Status = gBS->SetTimer(HdaStream->PollTimer, TimerCancel, 0);
            ASSERT_EFI_ERROR(Status);

            // Trigger callback.
            if (HdaStream->Callback)
                HdaStream->Callback(HdaStream->Output ? EfiHdaIoTypeOutput : EfiHdaIoTypeInput,
                    HdaStream->CallbackContext1, HdaStream->CallbackContext2, HdaStream->CallbackContext3);
        } else {
            // Get stream DMA position.
            HdaStreamDmaPos = HdaStream->HdaDev->DmaPositions[HdaStream->Index].Position;

            // Determine number of bytes to pull from or push to source data.
            HdaSourceLength = HDA_STREAM_BUF_SIZE_HALF;
            if ((HdaStream->BufferSourcePosition + HdaSourceLength) > HdaStream->BufferSourceLength)
                HdaSourceLength = HdaStream->BufferSourceLength - HdaStream->BufferSourcePosition;

            // Is this an output stream (copy data to)?
            if (HdaStream->Output) {
                // Copy data to DMA buffer, zeroing first if we are at the end of the source buffer.
                if (HdaStreamDmaPos < HDA_STREAM_BUF_SIZE_HALF) {
                    if (HdaSourceLength < HDA_STREAM_BUF_SIZE_HALF)
                        ZeroMem(HdaStream->BufferData + HDA_STREAM_BUF_SIZE_HALF, HDA_STREAM_BUF_SIZE_HALF);
                    CopyMem(HdaStream->BufferData + HDA_STREAM_BUF_SIZE_HALF, HdaStream->BufferSource + HdaStream->BufferSourcePosition, HdaSourceLength);
                }
                else {
                    if (HdaSourceLength < HDA_STREAM_BUF_SIZE_HALF)
                        ZeroMem(HdaStream->BufferData, HDA_STREAM_BUF_SIZE_HALF);
                    CopyMem(HdaStream->BufferData, HdaStream->BufferSource + HdaStream->BufferSourcePosition, HdaSourceLength);
                }
            } else { // Input stream (copy data from).
                // Copy data from DMA buffer.
                if (HdaStreamDmaPos < HDA_STREAM_BUF_SIZE_HALF)
                    CopyMem(HdaStream->BufferSource + HdaStream->BufferSourcePosition, HdaStream->BufferData + HDA_STREAM_BUF_SIZE_HALF, HdaSourceLength);
                else
                    CopyMem(HdaStream->BufferSource + HdaStream->BufferSourcePosition, HdaStream->BufferData, HdaSourceLength);
            }

            // Increase source position.
            HdaStream->BufferSourcePosition += HdaSourceLength;
            DEBUG((DEBUG_INFO, "%s half filled! (current position 0x%X)\n",
                (HdaStreamDmaPos < HDA_STREAM_BUF_SIZE_HALF) ? L"Upper" : L"Lower", HdaStreamDmaPos));
        }

        // Reset completion bit.
        HdaStreamSts = HDA_REG_SDNSTS_BCIS;
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNSTS(HdaStream->Index), 1, &HdaStreamSts);
        ASSERT_EFI_ERROR(Status);
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
    IN HDA_CONTROLLER_DEV *HdaControllerDev) {
    DEBUG((DEBUG_INFO, "HdaControllerScanCodecs(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaControllerDev->PciIo;
    UINT16 HdaStatests;
    EFI_HDA_IO_VERB_LIST HdaCodecVerbList;
    UINT32 VendorVerb;
    UINT32 VendorResponse;

    // Streams.
    UINTN CurrentOutputStreamIndex = 0;
    UINTN CurrentInputStreamIndex = 0;

    // Protocols.
    HDA_IO_PRIVATE_DATA *HdaIoPrivateData;
    EFI_DEVICE_PATH_PROTOCOL *HdaIoDevicePath;
    EFI_HANDLE ProtocolHandle;
    VOID *TmpProtocol;

    // Get STATESTS register.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_STATESTS, 1, &HdaStatests);
    if (EFI_ERROR(Status))
        return Status;

    // Create verb list with single item.
    VendorVerb = HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_VENDOR_ID);
    ZeroMem(&HdaCodecVerbList, sizeof(EFI_HDA_IO_VERB_LIST));
    HdaCodecVerbList.Count = 1;
    HdaCodecVerbList.Verbs = &VendorVerb;
    HdaCodecVerbList.Responses = &VendorResponse;

    // Iterate through register looking for active codecs.
    for (UINT8 i = 0; i < HDA_MAX_CODECS; i++) {
        // Do we have a codec at this address?
        if (HdaStatests & (1 << i)) {
            DEBUG((DEBUG_INFO, "HdaControllerScanCodecs(): found codec @ 0x%X\n", i));

            // Try to get the vendor ID. If this fails, ignore the codec.
            VendorResponse = 0;
            Status = HdaControllerSendCommands(HdaControllerDev, i, HDA_NID_ROOT, &HdaCodecVerbList);
            if ((EFI_ERROR(Status)) || (VendorResponse == 0))
                continue;

            // Create HDA I/O protocol private data structure.
            HdaIoPrivateData = AllocateZeroPool(sizeof(HDA_IO_PRIVATE_DATA));
            if (HdaIoPrivateData == NULL) {
                Status = EFI_OUT_OF_RESOURCES;
                goto FREE_CODECS;
            }

            // Fill HDA I/O protocol private data structure.
            HdaIoPrivateData->Signature = HDA_CONTROLLER_PRIVATE_DATA_SIGNATURE;
            HdaIoPrivateData->HdaCodecAddress = i;
            HdaIoPrivateData->HdaControllerDev = HdaControllerDev;
            HdaIoPrivateData->HdaIo.GetAddress = HdaControllerHdaIoGetAddress;
            HdaIoPrivateData->HdaIo.SendCommand = HdaControllerHdaIoSendCommand;
            HdaIoPrivateData->HdaIo.SetupStream = HdaControllerHdaIoSetupStream;
            HdaIoPrivateData->HdaIo.CloseStream = HdaControllerHdaIoCloseStream;
            HdaIoPrivateData->HdaIo.GetStream = HdaControllerHdaIoGetStream;
            HdaIoPrivateData->HdaIo.StartStream = HdaControllerHdaIoStartStream;
            HdaIoPrivateData->HdaIo.StopStream = HdaControllerHdaIoStopStream;
            
            // Assign output stream.
            if (CurrentOutputStreamIndex < HdaControllerDev->OutputStreamsCount) {
                DEBUG((DEBUG_INFO, "Assigning output stream %u to codec\n", CurrentOutputStreamIndex));
                HdaIoPrivateData->HdaOutputStream = HdaControllerDev->OutputStreams + CurrentOutputStreamIndex;
                CurrentOutputStreamIndex++;
            }

            // Assign input stream.
            if (CurrentInputStreamIndex < HdaControllerDev->InputStreamsCount) {
                DEBUG((DEBUG_INFO, "Assigning input stream %u to codec\n", CurrentInputStreamIndex));
                HdaIoPrivateData->HdaInputStream = HdaControllerDev->InputStreams + CurrentInputStreamIndex;
                CurrentInputStreamIndex++;
            }

            // Add to array.
            HdaControllerDev->PrivateDatas[i] = HdaIoPrivateData;
        }
    }

    // Clear STATESTS register.
    HdaStatests = HDA_REG_STATESTS_CLEAR;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_STATESTS, 1, &HdaStatests);
    if (EFI_ERROR(Status))
        return Status;

    // Install protocols on each codec.
    for (UINT8 i = 0; i < HDA_MAX_CODECS; i++) {
        // Do we have a codec at this address?
        if (HdaControllerDev->PrivateDatas[i] != NULL) {
            // Create Device Path for codec.
            EFI_HDA_IO_DEVICE_PATH HdaIoDevicePathNode = EFI_HDA_IO_DEVICE_PATH_TEMPLATE;
            HdaIoDevicePathNode.Address = i;
            HdaIoDevicePath = AppendDevicePathNode(HdaControllerDev->DevicePath, (EFI_DEVICE_PATH_PROTOCOL*)&HdaIoDevicePathNode);
            if (HdaIoDevicePath == NULL) {
                Status = EFI_INVALID_PARAMETER;
                goto FREE_CODECS;
            }

            // Install protocols for the codec. The codec driver will later bind to this.
            ProtocolHandle = NULL;
            Status = gBS->InstallMultipleProtocolInterfaces(&ProtocolHandle, &gEfiDevicePathProtocolGuid, HdaIoDevicePath,
                &gEfiHdaIoProtocolGuid, &HdaControllerDev->PrivateDatas[i]->HdaIo, NULL);
            if (EFI_ERROR(Status))
                goto FREE_CODECS;

            // Connect child to parent.
            Status = gBS->OpenProtocol(HdaControllerDev->ControllerHandle, &gEfiPciIoProtocolGuid, &TmpProtocol,
                HdaControllerDev->DriverBinding->DriverBindingHandle, ProtocolHandle, EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER);
            if (EFI_ERROR(Status))
                goto FREE_CODECS;
        }
    }
    
    return EFI_SUCCESS;

FREE_CODECS:
    //DEBUG((DEBUG_INFO, "HdaControllerScanCodecs(): failed to load driver for codec @ 0x%X\n", i));

    return Status;
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
            while (RemainingVerbs && ((HdaDev->CorbWritePointer + 1 % HdaDev->CorbEntryCount) != HdaCorbReadPointer)) {
                // Move write pointer and write verb to CORB.
                HdaDev->CorbWritePointer++;
                HdaDev->CorbWritePointer %= HdaDev->CorbEntryCount;
                VerbCommand = HDA_CORB_VERB(CodecAddress, Node, Verbs->Verbs[Verbs->Count - RemainingVerbs]);
                HdaCorb[HdaDev->CorbWritePointer] = VerbCommand;

                // Move to next verb.
                RemainingVerbs--;
            }

            // Set CORB write pointer.
            Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBWP, 1, &HdaDev->CorbWritePointer);
            if (EFI_ERROR(Status))
                goto DONE;
        }

        // Get responses from RIRB.
        ResponseReceived = FALSE;
        ResponseTimeout = 10;
        while (!ResponseReceived) {
            // Get current RIRB write pointer.
            Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_RIRBWP, 1, &HdaRirbWritePointer);
            if (EFI_ERROR(Status))
                goto DONE;

            // If the read and write pointers differ, there are responses waiting.
            while (HdaDev->RirbReadPointer != HdaRirbWritePointer) {
                // Increment RIRB read pointer.
                HdaDev->RirbReadPointer++;
                HdaDev->RirbReadPointer %= HdaDev->RirbEntryCount;

                // Get response and ensure it belongs to the current codec.
                RirbResponse = HdaRirb[HdaDev->RirbReadPointer];
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
    Status = HdaControllerScanCodecs(HdaDev);
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
