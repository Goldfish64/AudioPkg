/*
 * File: AudioDxe.c
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

#include "AudioDxe.h"
#include "HdaController/HdaController.h"
#include "HdaController/HdaControllerComponentName.h"
#include "HdaCodec/HdaCodec.h"
#include "HdaCodec/HdaCodecComponentName.h"

// HdaController Driver Binding.
EFI_DRIVER_BINDING_PROTOCOL gHdaControllerDriverBinding = {
    HdaControllerDriverBindingSupported,
    HdaControllerDriverBindingStart,
    HdaControllerDriverBindingStop,
    AUDIODXE_VERSION,
    NULL,
    NULL
};

// HdaCodec Driver Binding.
EFI_DRIVER_BINDING_PROTOCOL gHdaCodecDriverBinding = {
    HdaCodecDriverBindingSupported,
    HdaCodecDriverBindingStart,
    HdaCodecDriverBindingStop,
    AUDIODXE_VERSION,
    NULL,
    NULL
};

// Outputs a byte to the specified port.
void outb(UINT16 port, UINT8 data)
{
    asm volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

// Gets a byte from the specified port.
UINT8 inb(UINT16 port)
{
    UINT8 data;
    asm volatile("inb %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

// Outputs a short (word) to the specified port.
void outw(UINT16 port, UINT16 data)
{
    asm volatile("outw %0, %1" : : "a"(data), "Nd"(port));
}

// Gets a short (word) from the specified port.
UINT16 inw(UINT16 port)
{
    UINT16 data;
    asm volatile("inw %1, %0" : "=a"(data) : "Nd"(port));
    return data;
}

// -----------------------------------------------------------------------------

// Outputs 4 bytes to the specified port.
void outl(UINT16 port, UINT32 data)
{
    asm volatile("outl %%eax, %%dx" : : "dN" (port), "a" (data));
}

// Gets 4 bytes from the specified port.
UINT32 inl(UINT16 port)
{
    UINT32 rv;
    asm volatile ("inl %%dx, %%eax" : "=a" (rv) : "dN" (port));
    return rv;
}




EFI_STATUS
EFIAPI
AudioAppInit(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable) {
    DEBUG((DEBUG_INFO, "Starting Audioapp...\n"));

    EFI_PCI_IO_PROTOCOL *PciIo;

    EFI_HANDLE* handles = NULL;   
    UINTN handleCount = 0;
    EFI_STATUS Status;

    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiPciIoProtocolGuid, NULL, &handleCount, &handles);
    ASSERT_EFI_ERROR(Status);
    BOOLEAN gotdevice = FALSE;

    Print(L"handles %u\n", handleCount);
    UINTN device;

    // Get audio controller.
    for (int i = 0; i < handleCount; i++) {
        Status = gBS->HandleProtocol(handles[i], &gEfiPciIoProtocolGuid, (void**)&PciIo);
        ASSERT_EFI_ERROR(Status);

        UINTN tmp;
        
        Status = PciIo->GetLocation(PciIo, &tmp, &tmp, &device, &tmp);
        ASSERT_EFI_ERROR(Status);
        Print(L"devic 0x%X\n", device);

        if (device == 0x1B) {
            gotdevice = TRUE;
            break;
        }
    }

    if (!gotdevice)
        return EFI_UNSUPPORTED;

    

    

   /* UINTN bytes = 0x150;
    DEBUG((DEBUG_INFO, "== 8-bit: ==\n"));
    for (int i = 0; i < bytes; i++) {
        UINT8 data;
        PciIo->Pci.Read(PciIo, EfiPciIoWidthUint8, i, 1, &data);
        DEBUG((DEBUG_INFO, "0x%X: %2X\n", i, data));
    }

    DEBUG((DEBUG_INFO, "== 16-bit: ==\n"));
    for (int i = 0; i < bytes; i+=2) {
        UINT16 data;
        PciIo->Pci.Read(PciIo, EfiPciIoWidthUint16, i, 1, &data);
        DEBUG((DEBUG_INFO, "0x%X: %4X\n", i, data));
    }*/

    DEBUG((DEBUG_INFO, "== 32-bit: ==\n"));
    for (int i = 0; i < 0x50; i+=4) {
        UINT32 data;
        //outl(0xCF8, (device << 11) | i | 0x80000000);

        //UINT32 data = inl(0xCFC);

        
       // UINT32 data = PciRead32(PCI_LIB_ADDRESS(0, device, 0, i));
        PciIo->Mem.Read(PciIo, EfiPciIoWidthUint32, EFI_PCI_IO_PASS_THROUGH_BAR, i, 1, &data);
        DEBUG((DEBUG_INFO, "0x%X: %8X\n", i, data));
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
AudioDxeInit(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable) {
    DEBUG((DEBUG_INFO, "Starting AudioDxe...\n"));

    // Create variables.
    EFI_STATUS Status;
    
    // Register HdaController Driver Binding.
    Status = EfiLibInstallDriverBindingComponentName2(ImageHandle, SystemTable, &gHdaControllerDriverBinding,
        ImageHandle, &gHdaControllerComponentName, &gHdaControllerComponentName2);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;

    // Register HdaCodec Driver Binding.
    Status = EfiLibInstallDriverBindingComponentName2(ImageHandle, SystemTable, &gHdaCodecDriverBinding,
        NULL, &gHdaCodecComponentName, &gHdaCodecComponentName2);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        return Status;
    return Status;
}

CHAR16*
EFIAPI
AudioDxeGetCodecName(
    IN UINT16 VendorId,
    IN UINT16 DeviceId) {
    // Create variables.
    CHAR16 *VendorString;

    // Determine vendor ID.
    switch (VendorId) {
        case VEN_AMD_ID:
            VendorString = VEN_AMD_STR;
            break;

        case VEN_IDT_ID:
            VendorString = VEN_IDT_STR;
            break;
        
        case VEN_INTEL_ID:
            VendorString = VEN_INTEL_STR;
            break;

        case VEN_NVIDIA_ID:
            VendorString = VEN_NVIDIA_STR;
            break;

        case VEN_QEMU_ID:
            VendorString = VEN_QEMU_STR;
            break;

        case VEN_REALTEK_ID:
            VendorString = VEN_REALTEK_STR;
            break;

        default:
            return CatSPrint(NULL, L"%s", HDA_CODEC_STR);
    }

    // Create string.
    return CatSPrint(NULL, L"%s %s", VendorString, HDA_CODEC_STR);
}
