/*
 * File: HdaControllerDxe.c
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

#include "HdaControllerDxe.h"

EFI_STATUS
EFIAPI
HdaControllerDxeSupported(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL) {
    
    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo;
    UINTN segmentNo, busNo, deviceNo, functionNo;
    UINT16 vendorId;
    UINT16 deviceId;
    UINT16 classCode;

    // Open PCI I/O protocol.
    Status = gBS->OpenProtocol(ControllerHandle, &gEfiPciIoProtocolGuid, (VOID**)&PciIo,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    if (EFI_ERROR(Status))
        return Status;
        
    // Read 16-bit class code from PCI.
    Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint16, PCI_REG_CLASSCODE_OFFSET, 1, &classCode);
    if (EFI_ERROR(Status))
        goto Done;

    // Check class code. If not an HDA controller, we cannot support it.
    if (classCode != ((PCI_CLASS_MEDIA << 8) | PCI_SUBCLASS_HDA)) {
        Status = EFI_UNSUPPORTED;
        goto Done;
    }

    // If we get here, we found a controller.
    // Get location of PCI device.
    Status = PciIo->GetLocation(PciIo, &segmentNo, &busNo, &deviceNo, &functionNo);
    if (EFI_ERROR(Status))
        goto Done;

    // Get vendor and device IDs of PCI device.
    Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint16, PCI_VENDOR_ID_OFFSET, 1, &vendorId);
    if (EFI_ERROR (Status))
        goto Done;
    Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint16, PCI_DEVICE_ID_OFFSET, 1, &deviceId);
    if (EFI_ERROR(Status))
        goto Done;

    // Print HDA controller info.
    DEBUG((DEBUG_INFO, "Found HDA controller (%4X:%4X) at %2X:%2X.%X!\n", vendorId, deviceId, busNo, deviceNo, functionNo));
    Status = EFI_SUCCESS;

Done:
    // Close PCI I/O protocol and return status.
    gBS->CloseProtocol(ControllerHandle, &gEfiPciIoProtocolGuid, This->DriverBindingHandle, ControllerHandle);
    return Status;
}

EFI_STATUS
EFIAPI
HdaControllerDxeStart(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL) {

    DEBUG((DEBUG_INFO, "HdaControllerDxeStart()\n"));
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerDxeStop(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN UINTN NumberOfChildren,
    IN EFI_HANDLE *ChildHandleBuffer OPTIONAL) {
    return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
HdaControllerDxeGetDriverName(
    IN EFI_COMPONENT_NAME_PROTOCOL *This,
    IN CHAR8 *Language,
    OUT CHAR16 **DriverName) {
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerDxeGetControllerName(
    IN EFI_COMPONENT_NAME_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_HANDLE ChildHandle OPTIONAL,
    IN CHAR8 *Language,
    OUT CHAR16 **ControllerName) {
    return EFI_SUCCESS;
}

//
// Driver Binding Protocol
//
EFI_DRIVER_BINDING_PROTOCOL gHdaControllerDxeDriverBinding = {
    HdaControllerDxeSupported,
    HdaControllerDxeStart,
    HdaControllerDxeStop,
    AUDIODXE_VERSION,
    NULL,
    NULL
};

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_COMPONENT_NAME_PROTOCOL gHdaControllerDxeComponentName = {
    (EFI_COMPONENT_NAME_GET_DRIVER_NAME)HdaControllerDxeGetDriverName,
    (EFI_COMPONENT_NAME_GET_CONTROLLER_NAME)HdaControllerDxeGetControllerName,
    "eng"
};

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_COMPONENT_NAME2_PROTOCOL gHdaControllerDxeComponentName2 = {
    (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)HdaControllerDxeGetDriverName,
    (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)HdaControllerDxeGetControllerName,
    "en"
};

EFI_STATUS
EFIAPI
HdaControllerDxeRegisterDriver(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS Status;

    // Register HdaControllerDxe driver binding.
    Status = EfiLibInstallDriverBindingComponentName2(ImageHandle, SystemTable, &gHdaControllerDxeDriverBinding,
        ImageHandle, &gHdaControllerDxeComponentName,&gHdaControllerDxeComponentName2);
    ASSERT_EFI_ERROR(Status);
    return Status;
}
