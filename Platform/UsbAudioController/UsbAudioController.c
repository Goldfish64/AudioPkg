/*
 * File: UsbAudioController.c
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

#include "UsbAudioController.h"

EFI_STATUS
EFIAPI
UsbAudioControllerDriverBindingSupported(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL) {

    // Create variables.
    EFI_STATUS Status;
    EFI_USB_IO_PROTOCOL *UsbIo;
    EFI_USB_INTERFACE_DESCRIPTOR UsbInterfaceDesc;

     // Open USB I/O protocol. If this fails, its probably not a USB device.
    Status = gBS->OpenProtocol(ControllerHandle, &gEfiUsbIoProtocolGuid, (VOID**)&UsbIo,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    if (EFI_ERROR(Status))
        return Status;

    // Get USB interface descriptor.
    Status = UsbIo->UsbGetInterfaceDescriptor(UsbIo, &UsbInterfaceDesc);
    if (EFI_ERROR(Status))
        goto CLOSE_USB;

    // Check if device is of the audio class. If not we cannot support it.
    if (!(UsbInterfaceDesc.InterfaceClass == USB_AUDIO_CLASS)) {
        Status = EFI_UNSUPPORTED;
        goto CLOSE_USB;
    }

CLOSE_USB:
    // Close USB I/O protocol.
    gBS->CloseProtocol(ControllerHandle, &gEfiUsbIoProtocolGuid, This->DriverBindingHandle, ControllerHandle);
    return Status;
}

EFI_STATUS
EFIAPI
UsbAudioControllerDriverBindingStart(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL) {
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UsbAudioControllerDriverBindingStop(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN UINTN NumberOfChildren,
    IN EFI_HANDLE *ChildHandleBuffer OPTIONAL) {
    return EFI_UNSUPPORTED;
}
