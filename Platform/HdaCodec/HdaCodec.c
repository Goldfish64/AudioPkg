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
#include "HdaCodecComponentName.h"
#include "HdaVerbs.h"

EFI_STATUS
EFIAPI
HdaCodecDriverBindingSupported(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL) {
    //DEBUG((DEBUG_INFO, "HdaCodecDriverBindingSupported():start \n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_HDA_CODEC_PROTOCOL *HdaCodec;
    UINT8 CodecAddress;

    // Attempt to open the HDA codec protocol. If it can be opened, we can support it.
    Status = gBS->OpenProtocol(ControllerHandle, &gEfiHdaCodecProtocolGuid, (VOID**)&HdaCodec,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    if (EFI_ERROR(Status))
        return Status;
    
    // Get address of codec.
    Status = HdaCodec->GetAddress(HdaCodec, &CodecAddress);
    if (EFI_ERROR(Status))
        goto CLOSE_HDA;

    DEBUG((DEBUG_INFO, "HdaCodecDriverBindingSupported(): attaching to codec 0x%X\n", CodecAddress));
    Status = EFI_SUCCESS;

CLOSE_HDA:
    // Close protocol.
    gBS->CloseProtocol(ControllerHandle, &gEfiHdaCodecProtocolGuid, This->DriverBindingHandle, ControllerHandle);
    return Status;
}

EFI_STATUS
EFIAPI
HdaCodecDriverBindingStart(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL) {
    DEBUG((DEBUG_INFO, "HdaCodecDriverBindingStart(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_HDA_CODEC_PROTOCOL *HdaCodec;
    EFI_DEVICE_PATH_PROTOCOL *HdaDevicePath;
    UINT8 CodecAddress;
    UINT32 VendorId;

    Status = gBS->OpenProtocol(ControllerHandle, &gEfiHdaCodecProtocolGuid, (VOID**)&HdaCodec,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    if (EFI_ERROR(Status))
        return Status;

    // Get address.
    HdaCodec->GetAddress(HdaCodec, &CodecAddress);

    // Get vendor/device IDs.
    Status = HdaCodec->SendCommand(HdaCodec, HDA_NID_ROOT, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_VENDOR_ID), &VendorId);
    if (EFI_ERROR(Status))
        goto CLOSE_HDA;

    DEBUG((DEBUG_INFO, "HdaCodecDriverBindingStart(): attached to codec %4X:%4X @ 0x%X\n",
        (VendorId >> 16) & 0xFFFF, VendorId & 0xFFFF, CodecAddress));

    // Open Device Path protocol.
    Status = gBS->OpenProtocol(ControllerHandle, &gEfiDevicePathProtocolGuid, (VOID**)&HdaDevicePath,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    //ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR (Status))
        return Status;

    // Get function group count.
    HDA_SUBNODE_COUNT FuncGroupCount;
    Status = HdaCodec->SendCommand(HdaCodec, HDA_NID_ROOT, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUBNODE_COUNT), (UINT32*)&FuncGroupCount);
    ASSERT_EFI_ERROR(Status);

    DEBUG((DEBUG_INFO, "%u function groups. Starting node: 0x%X\n", FuncGroupCount.NodeCount, FuncGroupCount.StartNode));

    // Go through function groups.
    for (UINT8 fNid = FuncGroupCount.StartNode; fNid < FuncGroupCount.StartNode + FuncGroupCount.NodeCount; fNid++) {
        DEBUG((DEBUG_INFO, "Probing function group 0x%X\n", fNid));

        // Get type.
        UINT32 fType;
        Status = HdaCodec->SendCommand(HdaCodec, fNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_FUNC_GROUP_TYPE), &fType);
        ASSERT_EFI_ERROR(Status);

        // Get caps.
        UINT32 fCaps;
        Status = HdaCodec->SendCommand(HdaCodec, fNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_FUNC_GROUP_CAPS), &fCaps);
        ASSERT_EFI_ERROR(Status);

        // Get node count.
        HDA_SUBNODE_COUNT fNodes;
        Status = HdaCodec->SendCommand(HdaCodec, fNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUBNODE_COUNT), (UINT32*)&fNodes);
        ASSERT_EFI_ERROR(Status);

        DEBUG((DEBUG_INFO, "Function Type: 0x%X Caps: 0x%X Nodes: %u (0x%X)\n", fType, fCaps, fNodes.NodeCount, fNodes.StartNode));

        // Go through each widget.
        for (UINT8 wNid = fNodes.StartNode; wNid < fNodes.StartNode + fNodes.NodeCount; wNid++) {
            // Get caps.
            UINT32 wCaps;
            Status = HdaCodec->SendCommand(HdaCodec, wNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_WIDGET_CAPS), &wCaps);
            ASSERT_EFI_ERROR(Status);

            DEBUG((DEBUG_INFO, "Widget caps: 0x%8X\n", wCaps));
        }
    }


    return EFI_SUCCESS;
CLOSE_HDA:
    // Close protocol.
    gBS->CloseProtocol(ControllerHandle, &gEfiHdaCodecProtocolGuid, This->DriverBindingHandle, ControllerHandle);
    return Status;
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
