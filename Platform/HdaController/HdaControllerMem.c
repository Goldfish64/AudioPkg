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
    EFI_STATUS Status;

    // Allocate memory.
    HdaDev = (HDA_CONTROLLER_DEV*)AllocateZeroPool(sizeof(HDA_CONTROLLER_DEV));
    if (HdaDev == NULL)
        return NULL;

    // Initialize fields.
    HdaDev->Signature = HDA_CONTROLLER_DEV_SIGNATURE;
    HdaDev->PciIo = PciIo;
    HdaDev->DevicePath = DevicePath;
    HdaDev->OriginalPciAttributes = OriginalPciAttributes;

    // Initialize events.
    Status = gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_NOTIFY,
        HdaControllerResponsePollTimerHandler, HdaDev, &HdaDev->ResponsePollTimer);
    if (EFI_ERROR(Status))
        return NULL;

    return HdaDev;
}

EFI_STATUS
EFIAPI
HdaControllerInitCorb(
    HDA_CONTROLLER_DEV *HdaDev) {
    DEBUG((DEBUG_INFO, "HdaControllerInitCorb(): start\n"));

    // Status and PCI I/O protocol.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;

    // HDA register values.
    UINT8 HdaCorbSize;
    UINT32 HdaLowerCorbBaseAddr;
    UINT32 HdaUpperCorbBaseAddr;
    UINT16 HdaCorbWp;
    UINT16 HdaCorbRp;
    UINT64 HdaCorbRpPollResult;

    // CORB buffer.
    VOID *CorbBuffer = NULL;
    UINTN CorbLength;
    UINTN CorbLengthActual;
    VOID *CorbMapping = NULL;
    EFI_PHYSICAL_ADDRESS CorbPhysAddr;

    // Get value of CORBSIZE register.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_CORBSIZE, 1, &HdaCorbSize);
    if (EFI_ERROR(Status))
        return Status;

    // Determine size of CORB.
    if (HdaCorbSize & HDA_REG_CORBSIZE_CORBSZCAP_256) {
        CorbLength = 256 * HDA_CORB_ENTRY_SIZE;
        HdaCorbSize = (HdaCorbSize & ~HDA_REG_CORBSIZE_MASK) | HDA_REG_CORBSIZE_ENT256;
    } else if (HdaCorbSize & HDA_REG_CORBSIZE_CORBSZCAP_16) {
        CorbLength = 16 * HDA_CORB_ENTRY_SIZE;
        HdaCorbSize = (HdaCorbSize & ~HDA_REG_CORBSIZE_MASK) | HDA_REG_CORBSIZE_ENT16;
    } else if (HdaCorbSize & HDA_REG_CORBSIZE_CORBSZCAP_2) {
        CorbLength = 2 * HDA_CORB_ENTRY_SIZE;
        HdaCorbSize = (HdaCorbSize & ~HDA_REG_CORBSIZE_MASK) | HDA_REG_CORBSIZE_ENT2;
    } else {
        // Unsupported size.
        return EFI_UNSUPPORTED;
    }

    // Allocate outbound buffer.
    Status = PciIo->AllocateBuffer(PciIo, AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES(CorbLength), &CorbBuffer, 0);
    if (EFI_ERROR(Status))
        return Status;
    ZeroMem(CorbBuffer, CorbLength);
    
    // Map outbound buffer.
    CorbLengthActual = CorbLength;
    Status = PciIo->Map(PciIo, EfiPciIoOperationBusMasterCommonBuffer, CorbBuffer, &CorbLengthActual, &CorbPhysAddr, &CorbMapping);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;
    if (CorbLengthActual != CorbLength) {
        Status = EFI_OUT_OF_RESOURCES;
        goto FREE_BUFFER;
    }

    // Disable CORB.
    Status = HdaControllerDisableCorb(HdaDev);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // Set outbound buffer lower base address.
    HdaLowerCorbBaseAddr = (UINT32)CorbPhysAddr;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_CORBLBASE, 1, &HdaLowerCorbBaseAddr);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // If 64-bit supported, set upper base address.
    if (HdaDev->Buffer64BitSupported) {
        HdaUpperCorbBaseAddr = (UINT32)(CorbPhysAddr >> 32);
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_CORBUBASE, 1, &HdaUpperCorbBaseAddr);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;
    }

    // Reset write pointer to zero.
    HdaCorbWp = 0;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBWP, 1, &HdaCorbWp);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // Reset read pointer by setting bit, waiting until bit is set.
    HdaCorbRp = HDA_REG_CORBRP_RST;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, 1, &HdaCorbRp);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;
    Status = PciIo->PollMem(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, HDA_REG_CORBRP_RST, HDA_REG_CORBRP_RST, 10, &HdaCorbRpPollResult);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // Docs state we need to clear the bit and wait for it to clear.
    HdaCorbRp = 0;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, 1, &HdaCorbRp);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;
    Status = PciIo->PollMem(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, HDA_REG_CORBRP_RST, 0, 10, &HdaCorbRpPollResult);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // Populate device object properties.
    HdaDev->CorbBuffer = CorbBuffer;
    HdaDev->CorbEntryCount = CorbLength / HDA_CORB_ENTRY_SIZE;
    HdaDev->CorbMapping = CorbMapping;
    HdaDev->CorbPhysAddr = CorbPhysAddr;
    HdaDev->CorbWritePointer = HdaCorbWp;

    // Buffer allocation successful.
    DEBUG((DEBUG_INFO, "HDA controller CORB allocated @ 0x%p (0x%p) (%u entries)\n",
        HdaDev->CorbBuffer, HdaDev->CorbPhysAddr, HdaDev->CorbEntryCount));
    return EFI_SUCCESS;

FREE_BUFFER:
    // Unmap if needed.
    if (CorbMapping)
        PciIo->Unmap(PciIo, CorbMapping);

    // Free buffer.
    PciIo->FreeBuffer(PciIo, EFI_SIZE_TO_PAGES(CorbLength), CorbBuffer);
    return Status;
}

EFI_STATUS
EFIAPI
HdaControllerCleanupCorb(
    HDA_CONTROLLER_DEV *HdaDev) {
    DEBUG((DEBUG_INFO, "HdaControllerCleanupCorb(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;

    // Stop CORB.
    Status = HdaControllerDisableCorb(HdaDev);
    if (EFI_ERROR(Status))
        return Status;
    
    // Unmap CORB and free buffer.
    if (HdaDev->CorbMapping)
        PciIo->Unmap(PciIo, HdaDev->CorbMapping);
    PciIo->FreeBuffer(PciIo, EFI_SIZE_TO_PAGES(HdaDev->CorbEntryCount * HDA_CORB_ENTRY_SIZE), HdaDev->CorbBuffer);

    // Clear device object properties.
    HdaDev->CorbBuffer = NULL;
    HdaDev->CorbEntryCount = 0;
    HdaDev->CorbMapping = NULL;
    HdaDev->CorbPhysAddr = 0;
    HdaDev->CorbWritePointer = 0;
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerEnableCorb(
    HDA_CONTROLLER_DEV *HdaDev) {
    DEBUG((DEBUG_INFO, "HdaControllerEnableCorb(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;
    UINT8 HdaCorbCtl;

    // Get current value of CORBCTL.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_CORBCTL, 1, &HdaCorbCtl);
    if (EFI_ERROR(Status))
        return Status;
    
    // Enable CORB operation.
    HdaCorbCtl |= HDA_REG_CORBCTL_CORBRUN;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_CORBCTL, 1, &HdaCorbCtl);
    if (EFI_ERROR(Status))
        return Status;

    // Success.
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerDisableCorb(
    HDA_CONTROLLER_DEV *HdaDev) {
    DEBUG((DEBUG_INFO, "HdaControllerDisableCorb(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;
    UINT8 HdaCorbCtl;

    // Get current value of CORBCTL.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_CORBCTL, 1, &HdaCorbCtl);
    if (EFI_ERROR(Status))
        return Status;
    
    // Disable CORB operation.
    HdaCorbCtl &= ~HDA_REG_CORBCTL_CORBRUN;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_CORBCTL, 1, &HdaCorbCtl);
    if (EFI_ERROR(Status))
        return Status;

    // Success.
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerInitRirb(
    HDA_CONTROLLER_DEV *HdaDev) {
    DEBUG((DEBUG_INFO, "HdaControllerInitRirb(): start\n"));

    // Status and PCI I/O protocol.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;

    // HDA register values.
    UINT8 HdaRirbSize;
    UINT32 HdaLowerRirbBaseAddr;
    UINT32 HdaUpperRirbBaseAddr;
    UINT16 HdaRirbWp;

    // RIRB buffer.
    VOID *RirbBuffer = NULL;
    UINTN RirbLength;
    UINTN RirbLengthActual;
    VOID *RirbMapping = NULL;
    EFI_PHYSICAL_ADDRESS RirbPhysAddr;

    // Get value of RIRBSIZE register.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_RIRBSIZE, 1, &HdaRirbSize);
    if (EFI_ERROR(Status))
        return Status;

    // Determine size of RIRB.
    if (HdaRirbSize & HDA_REG_RIRBSIZE_RIRBSZCAP_256) {
        RirbLength = 256 * HDA_RIRB_ENTRY_SIZE;
        HdaRirbSize = (HdaRirbSize & ~HDA_REG_RIRBSIZE_MASK) | HDA_REG_RIRBSIZE_ENT256;
    } else if (HdaRirbSize & HDA_REG_RIRBSIZE_RIRBSZCAP_16) {
        RirbLength = 16 * HDA_RIRB_ENTRY_SIZE;
        HdaRirbSize = (HdaRirbSize & ~HDA_REG_RIRBSIZE_MASK) | HDA_REG_RIRBSIZE_ENT16;
    } else if (HdaRirbSize & HDA_REG_RIRBSIZE_RIRBSZCAP_2) {
        RirbLength = 2 * HDA_RIRB_ENTRY_SIZE;
        HdaRirbSize = (HdaRirbSize & ~HDA_REG_RIRBSIZE_MASK) | HDA_REG_RIRBSIZE_ENT2;
    } else {
        // Unsupported size.
        return EFI_UNSUPPORTED;
    }

    // Allocate outbound buffer.
    Status = PciIo->AllocateBuffer(PciIo, AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES(RirbLength), &RirbBuffer, 0);
    if (EFI_ERROR(Status))
        return Status;
    ZeroMem(RirbBuffer, RirbLength);
    
    // Map outbound buffer.
    RirbLengthActual = RirbLength;
    Status = PciIo->Map(PciIo, EfiPciIoOperationBusMasterCommonBuffer, RirbBuffer, &RirbLengthActual, &RirbPhysAddr, &RirbMapping);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;
    if (RirbLengthActual != RirbLength) {
        Status = EFI_OUT_OF_RESOURCES;
        goto FREE_BUFFER;
    }

    // Disable RIRB.
    Status = HdaControllerDisableRirb(HdaDev);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // Set outbound buffer lower base address.
    HdaLowerRirbBaseAddr = (UINT32)RirbPhysAddr;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_RIRBLBASE, 1, &HdaLowerRirbBaseAddr);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // If 64-bit supported, set upper base address.
    if (HdaDev->Buffer64BitSupported) {
        HdaUpperRirbBaseAddr = (UINT32)(RirbPhysAddr >> 32);
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_RIRBUBASE, 1, &HdaUpperRirbBaseAddr);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;
    }

    // Reset write pointer by setting reset bit.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_RIRBWP, 1, &HdaRirbWp);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;
    HdaRirbWp |= HDA_REG_RIRBWP_RST;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_RIRBWP, 1, &HdaRirbWp);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // Populate device object properties.
    HdaDev->RirbBuffer = RirbBuffer;
    HdaDev->RirbEntryCount = RirbLength / HDA_RIRB_ENTRY_SIZE;
    HdaDev->RirbMapping = RirbMapping;
    HdaDev->RirbPhysAddr = RirbPhysAddr;
    HdaDev->RirbReadPointer = 0;

    // Buffer allocation successful.
    DEBUG((DEBUG_INFO, "HDA controller RIRB allocated @ 0x%p (0x%p) (%u entries)\n",
        HdaDev->RirbBuffer, HdaDev->RirbPhysAddr, HdaDev->RirbEntryCount));
    return EFI_SUCCESS;

FREE_BUFFER:
    // Unmap if needed.
    if (RirbMapping)
        PciIo->Unmap(PciIo, RirbMapping);

    // Free buffer.
    PciIo->FreeBuffer(PciIo, EFI_SIZE_TO_PAGES(RirbLength), RirbBuffer);
    return Status;
}

EFI_STATUS
EFIAPI
HdaControllerCleanupRirb(
    HDA_CONTROLLER_DEV *HdaDev) {
    DEBUG((DEBUG_INFO, "HdaControllerCleanupRirb(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;

    // Stop RIRB.
    Status = HdaControllerDisableRirb(HdaDev);
    if (EFI_ERROR(Status))
        return Status;
    
    // Unmap RIRB and free buffer.
    if (HdaDev->RirbMapping)
        PciIo->Unmap(PciIo, HdaDev->RirbMapping);
    PciIo->FreeBuffer(PciIo, EFI_SIZE_TO_PAGES(HdaDev->RirbEntryCount * HDA_RIRB_ENTRY_SIZE), HdaDev->RirbBuffer);

    // Clear device object properties.
    HdaDev->RirbBuffer = NULL;
    HdaDev->RirbEntryCount = 0;
    HdaDev->RirbMapping = NULL;
    HdaDev->RirbPhysAddr = 0;
    HdaDev->RirbReadPointer = 0;
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerEnableRirb(
    HDA_CONTROLLER_DEV *HdaDev) {
    DEBUG((DEBUG_INFO, "HdaControllerEnableRirb(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;
    UINT8 HdaRirbCtl;

    // Get current value of RIRBCTL.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_RIRBCTL, 1, &HdaRirbCtl);
    if (EFI_ERROR(Status))
        return Status;
    
    // Enable RIRB operation.
    HdaRirbCtl |= HDA_REG_RIRBCTL_RIRBDMAEN;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_RIRBCTL, 1, &HdaRirbCtl);
    if (EFI_ERROR(Status))
        return Status;

    // Success.
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerDisableRirb(
    HDA_CONTROLLER_DEV *HdaDev) {
    DEBUG((DEBUG_INFO, "HdaControllerDisableRirb(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;
    UINT8 HdaRirbCtl;

    // Get current value of RIRBCTL.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_RIRBCTL, 1, &HdaRirbCtl);
    if (EFI_ERROR(Status))
        return Status;
    
    // Disable RIRB operation.
    HdaRirbCtl &= ~HDA_REG_RIRBCTL_RIRBDMAEN;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_RIRBCTL, 1, &HdaRirbCtl);
    if (EFI_ERROR(Status))
        return Status;

    // Success.
    return EFI_SUCCESS;
}
