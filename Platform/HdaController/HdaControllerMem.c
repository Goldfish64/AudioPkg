/*
 * File: HdaControllerMem.c
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

#include "HdaControllerMem.h"
#include "HdaController.h"
#include "HdaRegisters.h"

HDA_CONTROLLER_DEV *HdaControllerAllocDevice(
    IN EFI_PCI_IO_PROTOCOL *PciIo,
    IN EFI_DEVICE_PATH_PROTOCOL *DevicePath,
    IN UINT64 OriginalPciAttributes) {
    HDA_CONTROLLER_DEV *HdaDev;
   // EFI_STATUS Status;

    // Allocate memory.
    HdaDev = (HDA_CONTROLLER_DEV*)AllocateZeroPool(sizeof(HDA_CONTROLLER_DEV));
    if (HdaDev == NULL)
        return NULL;

    // Initialize fields.
    HdaDev->Signature = HDA_CONTROLLER_DEV_SIGNATURE;
    HdaDev->PciIo = PciIo;
    HdaDev->DevicePath = DevicePath;
    HdaDev->OriginalPciAttributes = OriginalPciAttributes;
    return HdaDev;
}

EFI_STATUS
EFIAPI
HdaControllerAllocBuffers(
    HDA_CONTROLLER_DEV *HdaDev) {
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;

    // HDA register values.
    UINT8 HdaCorbSize;
    UINT8 HdaRirbSize;
    UINT8 HdaCorbCtl;
    UINT32 HdaLowerCorbBaseAddr;
    UINT32 HdaUpperCorbBaseAddr;
    UINT8 HdaRirbCtl;
    UINT32 HdaLowerRirbBaseAddr;
    UINT32 HdaUpperRirbBaseAddr;
    UINT16 HdaCorbRp;
    UINT16 HdaCorbWp;
    UINT16 HdaRirbWp;

    // CORB buffer.
    VOID *OutboundHostBuffer = NULL;
    UINTN OutboundLength;
    UINTN OutboundLengthActual;
    VOID *OutboundMapping = NULL;
    EFI_PHYSICAL_ADDRESS OutboundPhysAddr;

    // RIRB buffer.
    VOID *InboundHostBuffer = NULL;
    UINTN InboundLength;
    UINTN InboundLengthActual;
    VOID *InboundMapping = NULL;
    EFI_PHYSICAL_ADDRESS InboundPhysAddr;

    // Get value of CORBSIZE and RIRBSIZE registers.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_CORBSIZE, 1, &HdaCorbSize);
    if (EFI_ERROR(Status))
        return Status;
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_RIRBSIZE, 1, &HdaRirbSize);
    if (EFI_ERROR(Status))
        return Status;

    // Determine size of CORB.
    if (HdaCorbSize & HDA_REG_CORBSIZE_CORBSZCAP_256) {
        OutboundLength = 256 * HDA_CORB_ENTRY_SIZE;
        HdaCorbSize = (HdaCorbSize & ~HDA_REG_CORBSIZE_MASK) | HDA_REG_CORBSIZE_ENT256;
    } else if (HdaCorbSize & HDA_REG_CORBSIZE_CORBSZCAP_16) {
        OutboundLength = 16 * HDA_CORB_ENTRY_SIZE;
        HdaCorbSize = (HdaCorbSize & ~HDA_REG_CORBSIZE_MASK) | HDA_REG_CORBSIZE_ENT16;
    } else if (HdaCorbSize & HDA_REG_CORBSIZE_CORBSZCAP_2) {
        OutboundLength = 2 * HDA_CORB_ENTRY_SIZE;
        HdaCorbSize = (HdaCorbSize & ~HDA_REG_CORBSIZE_MASK) | HDA_REG_CORBSIZE_ENT2;
    } else {
        // Unsupported size.
        return EFI_UNSUPPORTED;
    }

    // Determine size of RIRB.
    if (HdaRirbSize & HDA_REG_RIRBSIZE_RIRBSZCAP_256) {
        InboundLength = 256 * HDA_RIRB_ENTRY_SIZE;
        HdaRirbSize = (HdaRirbSize & ~HDA_REG_RIRBSIZE_MASK) | HDA_REG_RIRBSIZE_ENT256;
    } else if (HdaRirbSize & HDA_REG_RIRBSIZE_RIRBSZCAP_16) {
        InboundLength = 16 * HDA_RIRB_ENTRY_SIZE;
        HdaRirbSize = (HdaRirbSize & ~HDA_REG_RIRBSIZE_MASK) | HDA_REG_RIRBSIZE_ENT16;
    } else if (HdaRirbSize & HDA_REG_RIRBSIZE_RIRBSZCAP_2) {
        InboundLength = 2 * HDA_RIRB_ENTRY_SIZE;
        HdaRirbSize = (HdaRirbSize & ~HDA_REG_RIRBSIZE_MASK) | HDA_REG_RIRBSIZE_ENT2;
    } else {
        // Unsupported size.
        return EFI_UNSUPPORTED;
    }

    // Allocate outbound buffer.
    Status = PciIo->AllocateBuffer(PciIo, AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES(OutboundLength), &OutboundHostBuffer, 0);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;
    
    // Map outbound buffer.
    OutboundLengthActual = OutboundLength;
    Status = PciIo->Map(PciIo, EfiPciIoOperationBusMasterCommonBuffer, OutboundHostBuffer, &OutboundLengthActual, &OutboundPhysAddr, &OutboundMapping);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;
    if (OutboundLengthActual != OutboundLength) {
        Status = EFI_OUT_OF_RESOURCES;
        goto FREE_BUFFER;
    }

    // Allocate inbound buffer.
    Status = PciIo->AllocateBuffer(PciIo, AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES(InboundLength), &InboundHostBuffer, 0);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;
    
    // Map inbound buffer.
    InboundLengthActual = InboundLength;
    Status = PciIo->Map(PciIo, EfiPciIoOperationBusMasterCommonBuffer, InboundHostBuffer, &InboundLengthActual, &InboundPhysAddr, &InboundMapping);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;
    if (InboundLengthActual != InboundLength) {
        Status = EFI_OUT_OF_RESOURCES;
        goto FREE_BUFFER;
    }

    // Zero out buffers.
    ZeroMem(OutboundHostBuffer, OutboundLength);
    ZeroMem(InboundHostBuffer, InboundLength);

    // Get value of CORBCTL register and ensure CORB is stopped.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_CORBCTL, 1, &HdaCorbCtl);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;
    if (HdaCorbCtl & HDA_REG_CORBCTL_CORBRUN) {
        // If not, stop CORB.
        HdaCorbCtl &= ~HDA_REG_CORBCTL_CORBRUN;
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_CORBCTL, 1, &HdaCorbCtl);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;
    }
    
    // Set outbound buffer lower base address.
    HdaLowerCorbBaseAddr = (UINT32)(OutboundPhysAddr & 0xFFFFFFFF);
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_CORBLBASE, 1, &HdaLowerCorbBaseAddr);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // If 64-bit supported, set upper base address.
    if (HdaDev->Buffer64BitSupported) {
        HdaUpperCorbBaseAddr = (UINT32)((OutboundPhysAddr >> 32) & 0xFFFFFFF);
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_CORBUBASE, 1, &HdaUpperCorbBaseAddr);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;
    }

    // Get value of RIRBCTL register and ensure RIRB is stopped.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_RIRBCTL, 1, &HdaRirbCtl);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;
    if (HdaRirbCtl & HDA_REG_RIRBCTL_RIRBDMAEN) {
        // If not, stop RIRB.
        HdaRirbCtl &= ~HDA_REG_RIRBCTL_RIRBDMAEN;
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_RIRBCTL, 1, &HdaRirbCtl);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;
    }
    
    // Set inbound buffer lower base address.
    HdaLowerRirbBaseAddr = (UINT32)(InboundPhysAddr & 0xFFFFFFFF);
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_RIRBLBASE, 1, &HdaLowerRirbBaseAddr);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // If 64-bit supported, set upper base address.
    if (HdaDev->Buffer64BitSupported) {
        HdaUpperRirbBaseAddr = (UINT32)((InboundPhysAddr >> 32) & 0xFFFFFFF);
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_RIRBUBASE, 1, &HdaUpperRirbBaseAddr);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;
    }

    // Reset CORB read pointer.
    HdaCorbRp = HDA_REG_CORBRP_RST;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, 1, &HdaCorbRp);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // Reset write pointers.
    HdaCorbWp = 0;
    HdaRirbWp = HDA_REG_RIRBWP_RST;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBWP, 1, &HdaCorbWp);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_RIRBWP, 1, &HdaRirbWp);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // Populate device object.
    HdaDev->CommandOutboundBuffer = OutboundHostBuffer;
    HdaDev->CommandOutboundBufferEntryCount = OutboundLength / HDA_CORB_ENTRY_SIZE;
    HdaDev->CommandOutboundMapping = OutboundMapping;
    HdaDev->CommandOutboundPhysAddr = OutboundPhysAddr;
    HdaDev->ResponseInboundBuffer = InboundHostBuffer;
    HdaDev->ResponseInboundBufferEntryCount = InboundLength / HDA_RIRB_ENTRY_SIZE;
    HdaDev->ResponseInboundMapping = InboundMapping;
    HdaDev->ResponseInboundPhysAddr = InboundPhysAddr;

    // Buffer allocation successful.
    DEBUG((DEBUG_INFO, "HDA controller buffers allocated:\nCORB @ 0x%p (%u entries) RIRB @ 0x%p (%u entries)\n",
        HdaDev->CommandOutboundBuffer, HdaDev->CommandOutboundBufferEntryCount,
        HdaDev->ResponseInboundBuffer, HdaDev->ResponseInboundBufferEntryCount));
    return EFI_SUCCESS;

FREE_BUFFER:
    // Unmap if needed.
    if (OutboundMapping)
        PciIo->Unmap(PciIo, OutboundMapping);
    if (InboundMapping)
        PciIo->Unmap(PciIo, InboundMapping);

    // Free buffers.
    PciIo->FreeBuffer(PciIo, EFI_SIZE_TO_PAGES(OutboundLength), OutboundHostBuffer);
    PciIo->FreeBuffer(PciIo, EFI_SIZE_TO_PAGES(InboundLength), InboundHostBuffer);
    return Status;
}
