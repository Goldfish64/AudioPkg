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

#include "HdaCodec.h"
#include "HdaCodecProtocol.h"

EFI_STATUS
EFIAPI
HdaCodecDriverBindingSupported(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL) {

    // Create variables.
    EFI_STATUS Status;
    HDA_CODEC_PROTOCOL *HdaCodec;

    
    VOID *interfa;
    Status = gBS->LocateProtocol(&gHdaCodecProtocolGuid, NULL, &interfa);
    DEBUG((DEBUG_INFO, "new handle: 0x%p\n", ControllerHandle));

    HDA_CODEC_PROTOCOL *newPt = (HDA_CODEC_PROTOCOL*)interfa;
    DEBUG((DEBUG_INFO, "%u\n", newPt->Address));

    Status = gBS->HandleProtocol(This->DriverBindingHandle, &gHdaCodecProtocolGuid, &interfa);
    if (Status == EFI_SUCCESS) {
        DEBUG((DEBUG_INFO, "blol\n"));
       // while(1);
    }

    Status = gBS->OpenProtocol(This->DriverBindingHandle, &gHdaCodecProtocolGuid, (VOID**)&HdaCodec,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    if (EFI_ERROR(Status))
        return Status;

    DEBUG((DEBUG_INFO, "boop 0x%X\n", HdaCodec->Address));
    while(1);
    return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
HdaCodecDriverBindingStart(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL) {
    DEBUG((DEBUG_INFO, "HdaCodecDriverBindingStart()\n"));
    return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
HdaCodecDriverBindingStop(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN UINTN NumberOfChildren,
    IN EFI_HANDLE *ChildHandleBuffer OPTIONAL) {
    DEBUG((DEBUG_INFO, "HdaCodecDriverBindingStop()\n"));
    return EFI_UNSUPPORTED;
}

EFI_DRIVER_BINDING_PROTOCOL gHdaCodecDriverBinding = {
    HdaCodecDriverBindingSupported,
    HdaCodecDriverBindingStart,
    HdaCodecDriverBindingStop,
    AUDIODXE_VERSION,
    NULL,
    NULL
};

EFI_STATUS
EFIAPI
HdaCodecRegisterDriver(
    IN EFI_HANDLE DriverHandle) {
    EFI_STATUS Status;

    // Register driver binding.
    Status = EfiLibInstallDriverBinding(gAudioDxeImageHandle, gAudioDxeSystemTable, &gHdaCodecDriverBinding, DriverHandle);
    ASSERT_EFI_ERROR(Status);
    return Status;
}
