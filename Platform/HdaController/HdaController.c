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
#include "ComponentName.h"

#include "HdaCodecProtocol.h"
#include "HdaCodec.h"

VOID
HdaControllerResponsePollTimerHandler(
    IN EFI_EVENT Event,
    IN VOID *Context) {
    HDA_CONTROLLER_DEV *HdaDev = (HDA_CONTROLLER_DEV*)Context;
    
    UINT16 HdaCorbRp;
    HdaDev->PciIo->Mem.Read(HdaDev->PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBWP, 1, &HdaCorbRp);
    UINT16 HdaRirbWp;
    HdaDev->PciIo->Mem.Read(HdaDev->PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_RIRBWP, 1, &HdaRirbWp);
    DEBUG((DEBUG_INFO, "CORB: 0x%X RIRB: 0x%X 0x%X\n", HdaCorbRp, HdaRirbWp, HdaDev->RirbBuffer[1]));
}

EFI_STATUS
EFIAPI
HdaControllerReset(
    IN EFI_PCI_IO_PROTOCOL *PciIo) {
    DEBUG((DEBUG_INFO, "HdaControllerReset()\n"));
    EFI_STATUS Status;

    UINT32 hdaGCtl;
    UINT64 hdaGCtlPoll;


    // Get value of CRST bit.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, 1, &hdaGCtl);
    if (EFI_ERROR(Status))
        return Status;

    // Check if the controller is already in reset. If not, set bit to zero.
    if (!(hdaGCtl & HDA_REG_GCTL_CRST)) {
        hdaGCtl &= ~HDA_REG_GCTL_CRST;
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, 1, &hdaGCtl);
        if (EFI_ERROR(Status))
            return Status;
    }

    // Write a one to the CRST bit to begin the process of coming out of reset.
    hdaGCtl |= HDA_REG_GCTL_CRST;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, 1, &hdaGCtl);
    if (EFI_ERROR(Status))
        return Status;

    // Wait for bit to be set. Once bit is set, the controller is ready.
    Status = PciIo->PollMem(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_GCTL, HDA_REG_GCTL_CRST, HDA_REG_GCTL_CRST, 5, &hdaGCtlPoll);
    if (EFI_ERROR(Status))
        return Status;

    // Wait 10ms to ensure all codecs have also reset.
    gBS->Stall(10);

    // Controller is reset.
    DEBUG((DEBUG_INFO, "HDA controller is reset!\n"));
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerScanCodecs(
    IN HDA_CONTROLLER_DEV *HdaDev) {
    DEBUG((DEBUG_INFO, "HdaControllerScanCodecs(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;
    UINT16 HdaStatests;

    // Get STATEST register.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_STATESTS, 1, &HdaStatests);
    if (EFI_ERROR(Status))
        return Status;

    // Iterate through register looking for active codecs.
    for (UINT8 i = 0; i < HDA_MAX_CODECS; i++) {
        // Do we have a codec at this address?
        if (HdaStatests & (1 << i)) {
            DEBUG((DEBUG_INFO, "HdaControllerScanCodecs(): found codec @ 0x%X\n", i));

            // Allocate protocol.
            HDA_CODEC_PROTOCOL *codecProtocol = AllocateZeroPool(sizeof(HDA_CODEC_PROTOCOL));
            codecProtocol->Address = 34;
            EFI_HANDLE ProtocolHandle = NULL;

            // Install a protocol for the codec. The codec driver will later bind to this.
            gBS->InstallMultipleProtocolInterfaces(&ProtocolHandle, &gHdaCodecProtocolGuid, codecProtocol);
            HdaCodecRegisterDriver(ProtocolHandle);
            
        }
    }

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
    DEBUG((DEBUG_INFO, "HdaControllerDriverBindingStart()\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo;
    EFI_DEVICE_PATH_PROTOCOL *HdaDevicePath;

    UINT64 OriginalPciAttributes;
    UINT64 PciSupports;
    BOOLEAN PciAttributesSaved = FALSE;

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
    HDA_CONTROLLER_DEV *HdaDev = HdaControllerAllocDevice(PciIo, RemainingDevicePath, OriginalPciAttributes);
    HdaDev->Buffer64BitSupported = Hda64BitSupported;

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
    Status = HdaControllerScanCodecs(HdaDev);

    // send test.
    HdaDev->CorbBuffer[1] = 0xF0000;
    UINT16 HdaCorbWp = 1;
    PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBWP, 1, &HdaCorbWp);

    //while(1);
    
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
HdaControllerRegisterDriver(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS Status;

    // Register HdaControllerDxe driver binding.
    Status = EfiLibInstallDriverBindingComponentName2(gAudioDxeImageHandle, gAudioDxeSystemTable, &gHdaControllerDriverBinding,
        ImageHandle, &gHdaControllerComponentName, &gHdaControllerComponentName2);
    ASSERT_EFI_ERROR(Status);
    return Status;
}
