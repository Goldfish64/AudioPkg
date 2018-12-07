/*
 * File: UsbAudioController.h
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

#ifndef _EFI_USB_AUDIO_CONTROLLER_H_
#define _EFI_USB_AUDIO_CONTROLLER_H_

#include "AudioDxe.h"
#include "UsbAudioDesc.h"

typedef struct _USB_AUDIO_DEV USB_AUDIO_DEV;

struct _USB_AUDIO_DEV {
    // Consumed protocols.
    EFI_USB_IO_PROTOCOL *UsbIoControl;
    EFI_DEVICE_PATH_PROTOCOL *DevicePathControl;

    // USB Audio stuff.
    UINT8 ProtocolVersion;
    UINT8 *ControlDescriptors;
    UINTN ControlDescriptorsLength;

    // Associated interfaces.
    UINT8 *AssocInterfaces;
    EFI_HANDLE *AssocHandles;
    EFI_USB_IO_PROTOCOL **AssocUsbIo;
    UINT8 AssocInterfacesLength;
};

EFI_STATUS
EFIAPI
UsbAudioControllerDriverBindingSupported(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL);

EFI_STATUS
EFIAPI
UsbAudioControllerDriverBindingStart(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL);

EFI_STATUS
EFIAPI
UsbAudioControllerDriverBindingStop(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN UINTN NumberOfChildren,
    IN EFI_HANDLE *ChildHandleBuffer OPTIONAL);

#endif
