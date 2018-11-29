/*
 * File: HdaControllerComponentName.c
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

#include "HdaControllerComponentName.h"

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_UNICODE_STRING_TABLE gHdaDriverNameTable[] = {
    {
        "eng;en",
        L"HDA Controller Driver"
    },
    {
        NULL,
        NULL
    }
};

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_COMPONENT_NAME_PROTOCOL gHdaControllerComponentName = {
    HdaControllerComponentNameGetDriverName,
    HdaControllerComponentNameGetControllerName,
    "eng"
};

GLOBAL_REMOVE_IF_UNREFERENCED
EFI_COMPONENT_NAME2_PROTOCOL gHdaControllerComponentName2 = {
    (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)HdaControllerComponentNameGetDriverName,
    (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME)HdaControllerComponentNameGetControllerName,
    "en"
};

EFI_STATUS
EFIAPI
HdaControllerComponentNameGetDriverName(
    IN EFI_COMPONENT_NAME_PROTOCOL *This,
    IN CHAR8 *Language,
    OUT CHAR16 **DriverName) {
    return LookupUnicodeString2(Language, This->SupportedLanguages, gHdaDriverNameTable,
        DriverName, (BOOLEAN)(This == &gHdaControllerComponentName));
}

EFI_STATUS
EFIAPI
HdaControllerComponentNameGetControllerName(
    IN EFI_COMPONENT_NAME_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_HANDLE ChildHandle OPTIONAL,
    IN CHAR8 *Language,
    OUT CHAR16 **ControllerName) {

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo;
    UINT32 PciVendorDeviceId = VEN_INVALID_ID;

    // Ensure there is no child handle.
    if (ChildHandle != NULL)
        return EFI_UNSUPPORTED;

    // Ensure controller is being managed by HdaController.
    Status = EfiTestManagedDevice(ControllerHandle, gHdaControllerDriverBinding.DriverBindingHandle, &gEfiPciIoProtocolGuid);
    if (EFI_ERROR(Status))
        return Status;

    // Get PCI I/O protocol.
    Status = gBS->OpenProtocol(ControllerHandle, &gEfiPciIoProtocolGuid, (VOID **)&PciIo,
        gHdaControllerDriverBinding.DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (EFI_ERROR(Status))
        return Status;

    // Get PCI device/vendor ID.
    Status = PciIo->Pci.Read(PciIo, EfiPciIoWidthUint32, PCI_VENDOR_ID_OFFSET, 1, &PciVendorDeviceId);
    ASSERT_EFI_ERROR(Status);

    // Determine vendor of controller.
    switch (GET_PCI_VENDOR_ID(PciVendorDeviceId)) {
        case VEN_INTEL_ID:
            *ControllerName = VEN_INTEL_CONTROLLER_STR;
            break;
        
        case VEN_NVIDIA_ID:
            *ControllerName = VEN_NVIDIA_CONTROLLER_STR;
            break;

        case VEN_AMD_ID:
            *ControllerName = VEN_AMD_CONTROLLER_STR;
            break;

        // If unknown vendor, use generic string.
        default:
            *ControllerName = VEN_INVALID_CONTROLLER_STR;
    }
    return EFI_SUCCESS;
}
