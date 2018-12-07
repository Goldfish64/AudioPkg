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

    // Check if device is an audio class control. If not we cannot support it.
    if (!((UsbInterfaceDesc.InterfaceClass == USB_AUDIO_CLASS) &&
        (UsbInterfaceDesc.InterfaceSubClass == USB_AUDIO_SUBCLASS_CONTROL))) {
        Status = EFI_UNSUPPORTED;
        goto CLOSE_USB;
    }

    // Ensure device is compliant with V1 or V2 of the spec. If not we cannot support it.
    if (!((UsbInterfaceDesc.InterfaceProtocol == USB_AUDIO_PROTOCOL_VER_01_00) ||
        (UsbInterfaceDesc.InterfaceProtocol == USB_AUDIO_PROTOCOL_VER_02_00))) {
        Status = EFI_UNSUPPORTED;
        goto CLOSE_USB;
    }

    // Device can be supported.
    Status = EFI_SUCCESS;

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
    DEBUG((DEBUG_INFO, "UsbAudioControllerDriverBindingStart(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_USB_IO_PROTOCOL *UsbIo;
    EFI_DEVICE_PATH_PROTOCOL *DevicePath;
    EFI_USB_INTERFACE_DESCRIPTOR UsbInterfaceDesc;
    EFI_USB_DEVICE_DESCRIPTOR UsbDeviceDesc;

    // Open USB I/O protocol. If this fails, its probably not a USB device.
    Status = gBS->OpenProtocol(ControllerHandle, &gEfiUsbIoProtocolGuid, (VOID**)&UsbIo,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    if (EFI_ERROR(Status))
        return Status;

    // Open Device Path protocol.
    Status = gBS->OpenProtocol(ControllerHandle, &gEfiDevicePathProtocolGuid, (VOID**)&DevicePath,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    ASSERT_EFI_ERROR(Status);
    CHAR16 *tttt = ConvertDevicePathToText(DevicePath, FALSE, FALSE);    
        DEBUG((DEBUG_INFO, "us: %s\n", tttt));
        FreePool(tttt);
    

    // TODO need to open other interfaces too.
    EFI_HANDLE* handles = NULL;   
    UINTN handleCount = 0;

    // Get USB devices.
    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiUsbIoProtocolGuid, NULL, &handleCount, &handles);
    ASSERT_EFI_ERROR(Status);

    EFI_DEVICE_PATH_PROTOCOL *CurrentDevicePathNode;
    EFI_DEVICE_PATH_PROTOCOL *CurrentDevicePathHandleNode;
    for (UINTN i = 0; i < handleCount; i++) {
       // EFI_USB_IO_PROTOCOL *UsbIoNew;

        // Get device path.
        EFI_DEVICE_PATH_PROTOCOL *DevPathTmp = DevicePathFromHandle(handles[i]);
        if (DevPathTmp == NULL)
            continue;

        CHAR16 *txt;// = ConvertDevicePathToText(DevPathTmp, FALSE, FALSE);
       // DEBUG((DEBUG_INFO, "%s (0x%X, 0x%X)\n", txt, DevPathTmp->Type, DevPathTmp->SubType));
        //FreePool(txt);

        // Compare paths.
        CurrentDevicePathNode = DevicePath;
        CurrentDevicePathHandleNode = DevPathTmp;
        while (CompareMem(CurrentDevicePathNode, CurrentDevicePathHandleNode, DevicePathNodeLength(CurrentDevicePathNode)) == 0) {

            EFI_DEVICE_PATH_PROTOCOL *NextDevNode = NextDevicePathNode(CurrentDevicePathNode);
            EFI_DEVICE_PATH_PROTOCOL *NextDevHandleNode = NextDevicePathNode(CurrentDevicePathHandleNode);

            // If we are about to hit the end, break out.
            if (IsDevicePathEnd(NextDevNode) || IsDevicePathEnd(NextDevHandleNode))
                break;

            CurrentDevicePathNode = NextDevNode;
            CurrentDevicePathHandleNode = NextDevHandleNode;
        }
        
        // Make sure these are the last nodes in the path.
        if (!(IsDevicePathEnd(NextDevicePathNode(CurrentDevicePathNode)) && IsDevicePathEnd(NextDevicePathNode(CurrentDevicePathHandleNode))))
            continue;

        // Check to see if nodes are the same type.
        if (!((CurrentDevicePathNode->Type == CurrentDevicePathHandleNode->Type) &&
            (CurrentDevicePathNode->SubType == CurrentDevicePathHandleNode->SubType)))
            continue;

        // Ensure the nodes share the same USB port. A difference means the node belongs to another USB device.
        USB_DEVICE_PATH *UsbDevicePathNode = (USB_DEVICE_PATH*)CurrentDevicePathNode;
        USB_DEVICE_PATH *UsbDevicePathHandleNode = (USB_DEVICE_PATH*)CurrentDevicePathHandleNode;
        if (UsbDevicePathNode->ParentPortNumber != UsbDevicePathHandleNode->ParentPortNumber)
            continue;

        CHAR16 *txt2 = ConvertDevicePathToText(CurrentDevicePathNode, FALSE, FALSE);
        txt = ConvertDevicePathToText(CurrentDevicePathHandleNode, FALSE, FALSE);
        DEBUG((DEBUG_INFO, "%s %s\n", txt, txt2));
        FreePool(txt);
        FreePool(txt2);

        // Get end.
        EFI_DEVICE_PATH_PROTOCOL *DevPathNode = DevPathTmp;
        while (!IsDevicePathEnd(NextDevicePathNode(DevPathNode)))
            DevPathNode = NextDevicePathNode(DevPathNode);
        
        if ((DevPathNode->Type != MESSAGING_DEVICE_PATH) || (DevPathNode->SubType != MSG_USB_DP))
            continue;

       // USB_DEVICE_PATH *UsbDevicePath = (USB_DEVICE_PATH*)DevPathNode;
      //  USB_DEVICE_PATH *UsbControlPath = (USB_DEVICE_PATH*)DevicePath;
      //  if ( != UsbControlPath->ParentPortNumber)
         //   continue;

      //  txt = ConvertDevicePathToText(DevPathNode, FALSE, FALSE);
      //  DEBUG((DEBUG_INFO, "%s (port num 0x%X)\n", txt, UsbDevicePath->ParentPortNumber));
      //  FreePool(txt);
    }



    // Create device object.
    USB_AUDIO_DEV *UsbAudioDev = AllocateZeroPool(sizeof(USB_AUDIO_DEV));
    UsbAudioDev->UsbIoControl = UsbIo;
    UsbAudioDev->DevicePath = DevicePath;
    

    // Get USB interface descriptor.
    Status = UsbIo->UsbGetInterfaceDescriptor(UsbIo, &UsbInterfaceDesc);
    ASSERT_EFI_ERROR(Status);

    // Store version.
    UsbAudioDev->ProtocolVersion = UsbInterfaceDesc.InterfaceProtocol;

    // Get USB device descriptor.
    Status = UsbIo->UsbGetDeviceDescriptor(UsbIo, &UsbDeviceDesc);
    ASSERT_EFI_ERROR(Status);

    // Get USB lang ids.
    UINT16 *Langs;
    UINT16 LangLength;
    Status = UsbIo->UsbGetSupportedLanguages(UsbIo, &Langs, &LangLength);

    CHAR16 *ProductStr;
    Status = UsbIo->UsbGetStringDescriptor(UsbIo, Langs[0], UsbDeviceDesc.StrProduct, &ProductStr);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "Device string: %s\n", ProductStr));

    // Get USB configuration descriptor.
    EFI_USB_CONFIG_DESCRIPTOR UsbConfigDesc;
    Status = UsbIo->UsbGetConfigDescriptor(UsbIo, &UsbConfigDesc);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "Got config descriptor total length %u %bytes\n", UsbConfigDesc.TotalLength));

    // Get complete config descriptor.
    UINT8 *UsbConfigDescFull = AllocateZeroPool(UsbConfigDesc.TotalLength);
    //UINT8 *UsbAcDesc;
    UINT32 UsbStatus;
    Status = UsbGetDescriptor(UsbIo, 0x0200, 0, UsbConfigDesc.TotalLength, UsbConfigDescFull, &UsbStatus);
    ASSERT_EFI_ERROR(Status);

    // Search for our interface descriptor.
    UINTN DescOffset = 0;
    while (DescOffset < UsbConfigDesc.TotalLength) {
        if (CompareMem(&UsbInterfaceDesc, UsbConfigDescFull + DescOffset, UsbInterfaceDesc.Length) == 0)
            break;
        DescOffset++;
    }
    DEBUG((DEBUG_INFO, "Standard interface descriptor is at offset 0x%X\n", DescOffset));

    // Search for audio control descriptor header.
    while (DescOffset < UsbConfigDesc.TotalLength) {
        USB_AUDIO_DESCRIPTOR_HEADER *UsbAudioDescHeader = (USB_AUDIO_DESCRIPTOR_HEADER*)(UsbConfigDescFull + DescOffset);
        if ((UsbAudioDescHeader->DescriptorType == USB_AUDIO_CLASS_DESCRIPTOR_INTERFACE) && (UsbAudioDescHeader->DescriptorSubtype == 0x01))
            break;
        DescOffset++;
    }

    DEBUG((DEBUG_INFO, "Class interface descriptor is at offset 0x%X\n", DescOffset));

    // Get audio control descriptors.
    if (UsbAudioDev->ProtocolVersion == USB_AUDIO_PROTOCOL_VER_01_00) {
        // USB device implements ADC v1.
        USB_AUDIO_CLASS_INTERFACE_DESCRIPTOR_V1 *UsbAudioDesc = (USB_AUDIO_CLASS_INTERFACE_DESCRIPTOR_V1*)(UsbConfigDescFull + DescOffset);
        UsbAudioDev->ControlDescriptorsLength = UsbAudioDesc->TotalLength;
        DEBUG((DEBUG_INFO, "Device implements spec version 1.0\nTotal length of audio control descriptors: %u bytes\n",
            UsbAudioDev->ControlDescriptorsLength));

    } else if (UsbAudioDev->ProtocolVersion == USB_AUDIO_PROTOCOL_VER_02_00) {
        // USB device implements ADC v2.
        DEBUG((DEBUG_INFO, "Device implements spec version 2.0\n"));
    } else {
        // UNSUPPORTED..clean up.
        return EFI_UNSUPPORTED;
    }

    // Allocate space for descriptors and copy them.
    UsbAudioDev->ControlDescriptors = AllocateZeroPool(UsbAudioDev->ControlDescriptorsLength);
    CopyMem(UsbAudioDev->ControlDescriptors, UsbConfigDescFull + DescOffset, UsbAudioDev->ControlDescriptorsLength);

    // Free configuration descriptor.
    FreePool(UsbConfigDescFull);

    // Print descriptors.
    DescOffset = 0;
    while (DescOffset < UsbAudioDev->ControlDescriptorsLength) {
        USB_AUDIO_DESCRIPTOR_HEADER *Header = (USB_AUDIO_DESCRIPTOR_HEADER*)(UsbAudioDev->ControlDescriptors + DescOffset);

        DEBUG((DEBUG_INFO, "Control descriptor length: %u bytes, type: 0x%X; subtype: 0x%X\n", Header->Length, Header->DescriptorType, Header->DescriptorSubtype));
        DescOffset += Header->Length;
    }

    // Get feature unit.
    INT16 volume = 0;
    INT16 maxvolume = 0;
    INT16 minvolume = 0;
    USB_DEVICE_REQUEST FeatureRequest;
    FeatureRequest.RequestType = 0xA1;
    FeatureRequest.Request = 0x83;
    FeatureRequest.Value = 0x0201; // 0x0201
    FeatureRequest.Index = 0x0200;
    FeatureRequest.Length = sizeof(INT16);

    Status = UsbIo->UsbControlTransfer(UsbIo, &FeatureRequest, EfiUsbDataIn, 10000, &maxvolume, sizeof(INT16), &UsbStatus);
    ASSERT_EFI_ERROR(Status);

    FeatureRequest.Request = 0x81;
    Status = UsbIo->UsbControlTransfer(UsbIo, &FeatureRequest, EfiUsbDataIn, 10000, &volume, sizeof(INT16), &UsbStatus);
    ASSERT_EFI_ERROR(Status);

    FeatureRequest.Request = 0x82;
    Status = UsbIo->UsbControlTransfer(UsbIo, &FeatureRequest, EfiUsbDataIn, 10000, &minvolume, sizeof(INT16), &UsbStatus);
    ASSERT_EFI_ERROR(Status);

    float volumemultipler = 1 / (float)256;
    INT16 volumeDb = volumemultipler * volume;
    INT16 volumemaxDb = volumemultipler * maxvolume;
    INT16 volumeminDb = volumemultipler * minvolume;

    DEBUG((DEBUG_INFO, "Volume %d dB (0x%X), min %d dB (0x%X), max %d dB (0x%X)\n", volumeDb, volume, volumeminDb, minvolume, volumemaxDb, maxvolume));

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
