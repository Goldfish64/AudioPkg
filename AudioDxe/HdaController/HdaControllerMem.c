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
#include <Library/HdaRegisters.h>

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
    HdaDev->PciIo = PciIo;
    HdaDev->DevicePath = DevicePath;
    HdaDev->OriginalPciAttributes = OriginalPciAttributes;
    InitializeSpinLock(&HdaDev->SpinLock);

    return HdaDev;
}

EFI_STATUS
EFIAPI
HdaControllerInitCorb(
    IN HDA_CONTROLLER_DEV *HdaDev) {
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
    Status = HdaControllerSetCorb(HdaDev, FALSE);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // Set outbound buffer lower base address.
    HdaLowerCorbBaseAddr = (UINT32)CorbPhysAddr;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_CORBLBASE, 1, &HdaLowerCorbBaseAddr);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // If 64-bit supported, set upper base address.
    if (HdaDev->Capabilities & HDA_REG_GCAP_64OK) {
        HdaUpperCorbBaseAddr = (UINT32)(CorbPhysAddr >> 32);
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_CORBUBASE, 1, &HdaUpperCorbBaseAddr);
        ASSERT_EFI_ERROR(Status);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;
    }

    // Reset write pointer to zero.
    HdaCorbWp = 0;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBWP, 1, &HdaCorbWp);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // Reset read pointer by setting bit.
    HdaCorbRp = HDA_REG_CORBRP_RST;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, 1, &HdaCorbRp);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // Docs state we need to clear the bit and wait for it to clear.
    HdaCorbRp = 0;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, 1, &HdaCorbRp);
    ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;
    Status = PciIo->PollMem(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_CORBRP, HDA_REG_CORBRP_RST, 0, 50000000, &HdaCorbRpPollResult);
    ASSERT_EFI_ERROR(Status);
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
    IN HDA_CONTROLLER_DEV *HdaDev) {
    DEBUG((DEBUG_INFO, "HdaControllerCleanupCorb(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;

    // Stop CORB.
    Status = HdaControllerSetCorb(HdaDev, FALSE);
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
HdaControllerSetCorb(
    IN HDA_CONTROLLER_DEV *HdaDev,
    IN BOOLEAN Enable) {
    DEBUG((DEBUG_INFO, "HdaControllerSetCorb(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;
    UINT8 HdaCorbCtl;

    // Get current value of CORBCTL.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_CORBCTL, 1, &HdaCorbCtl);
    if (EFI_ERROR(Status))
        return Status;
    
    // Change CORB operation.
    if (Enable)
        HdaCorbCtl |= HDA_REG_CORBCTL_CORBRUN;
    else
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
    IN HDA_CONTROLLER_DEV *HdaDev) {
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
    Status = HdaControllerSetRirb(HdaDev, FALSE);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // Set outbound buffer lower base address.
    HdaLowerRirbBaseAddr = (UINT32)RirbPhysAddr;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_RIRBLBASE, 1, &HdaLowerRirbBaseAddr);
    if (EFI_ERROR(Status))
        goto FREE_BUFFER;

    // If 64-bit supported, set upper base address.
    if (HdaDev->Capabilities & HDA_REG_GCAP_64OK) {
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
    IN HDA_CONTROLLER_DEV *HdaDev) {
    DEBUG((DEBUG_INFO, "HdaControllerCleanupRirb(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;

    // Stop RIRB.
    Status = HdaControllerSetRirb(HdaDev, FALSE);
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
HdaControllerSetRirb(
    IN HDA_CONTROLLER_DEV *HdaDev,
    IN BOOLEAN Enable) {
    DEBUG((DEBUG_INFO, "HdaControllerSetRirb(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;
    UINT8 HdaRirbCtl;

    // Get current value of RIRBCTL.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_RIRBCTL, 1, &HdaRirbCtl);
    if (EFI_ERROR(Status))
        return Status;
    
    // Change RIRB operation.
    if (Enable)
        HdaRirbCtl |= HDA_REG_RIRBCTL_RIRBDMAEN;
    else
        HdaRirbCtl &= ~HDA_REG_RIRBCTL_RIRBDMAEN;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_RIRBCTL, 1, &HdaRirbCtl);
    if (EFI_ERROR(Status))
        return Status;

    // Success.
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaControllerInitStreams(
    IN HDA_CONTROLLER_DEV *HdaDev) {
    DEBUG((DEBUG_INFO, "HdaControllerInitStreams(): start\n"));

    // Status and PCI I/O protocol.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaDev->PciIo;
    UINT8 HdaStreamCtl1;
    UINT64 Tmp;
    UINT32 LowerBaseAddr;
    UINT32 UpperBaseAddr;
    EFI_PHYSICAL_ADDRESS DataBlockAddr;
    HDA_STREAM *HdaStream;

    // Buffers.
    UINTN BdlLengthActual;
    UINTN DataLengthActual;

    // Stream regs.
    UINT16 StreamLvi;
    UINT32 StreamCbl;

    // Determine number of streams.
    HdaDev->BidirStreamsCount = HDA_REG_GCAP_BSS(HdaDev->Capabilities);
    HdaDev->InputStreamsCount = HDA_REG_GCAP_ISS(HdaDev->Capabilities);
    HdaDev->OutputStreamsCount = HDA_REG_GCAP_OSS(HdaDev->Capabilities);
    HdaDev->TotalStreamsCount = HdaDev->BidirStreamsCount +
        HdaDev->InputStreamsCount + HdaDev->OutputStreamsCount;

    // Initialize stream arrays.
    HdaDev->BidirStreams = AllocateZeroPool(sizeof(HDA_STREAM) * HdaDev->BidirStreamsCount);
    HdaDev->InputStreams = AllocateZeroPool(sizeof(HDA_STREAM) * HdaDev->InputStreamsCount);
    HdaDev->OutputStreams = AllocateZeroPool(sizeof(HDA_STREAM) * HdaDev->OutputStreamsCount);

    // Initialize streams.
    UINT8 InputStreamsOffset = HdaDev->BidirStreamsCount;
    UINT8 OutputStreamsOffset = InputStreamsOffset + HdaDev->InputStreamsCount;
    DEBUG((DEBUG_INFO, "HdaControllerInitStreams(): in offset %u, out offset %u\n", InputStreamsOffset, OutputStreamsOffset));
    for (UINT8 i = 0; i < HdaDev->TotalStreamsCount; i++) {
        // Get pointer to stream and set type.
        if (i < InputStreamsOffset) {
            HdaStream = HdaDev->BidirStreams + i;
            HdaStream->Type = HDA_STREAM_TYPE_BIDIR;
        } else if (i < OutputStreamsOffset) {
            HdaStream = HdaDev->InputStreams + (i - InputStreamsOffset);
            HdaStream->Type = HDA_STREAM_TYPE_IN;
        } else {
            HdaStream = HdaDev->OutputStreams + (i - OutputStreamsOffset);
            HdaStream->Type = HDA_STREAM_TYPE_OUT;
        }

        // Set parent controller and index.
        HdaStream->HdaDev = HdaDev;
        HdaStream->Index = i;
        HdaStream->Output = (HdaStream->Type == HDA_STREAM_TYPE_OUT);

        // Initialize polling timer.
        Status = gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_SIGNAL, TPL_NOTIFY,
            HdaControllerStreamPollTimerHandler, HdaStream, &HdaStream->PollTimer);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;

        // Allocate buffer descriptor list.
        Status = PciIo->AllocateBuffer(PciIo, AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES(HDA_BDL_SIZE),
            (VOID**)&HdaStream->BufferList, 0);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;
        ZeroMem(HdaStream->BufferList, HDA_BDL_SIZE);
        
        // Map buffer descriptor list.
        BdlLengthActual = HDA_BDL_SIZE;
        Status = PciIo->Map(PciIo, EfiPciIoOperationBusMasterCommonBuffer, HdaStream->BufferList, &BdlLengthActual,
            &HdaStream->BufferListPhysAddr, &HdaStream->BufferListMapping);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;
        if (BdlLengthActual != HDA_BDL_SIZE) {
            Status = EFI_OUT_OF_RESOURCES;
            goto FREE_BUFFER;
        }

        // Disable stream.
        Status = HdaControllerSetStream(HdaStream, FALSE, 0);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;

        // Get value of control register.
        Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index), 1, &HdaStreamCtl1);
        ASSERT_EFI_ERROR(Status);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;

        // Reset stream and wait for bit to be set.
        HdaStreamCtl1 |= HDA_REG_SDNCTL1_SRST;
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index), 1, &HdaStreamCtl1);
        ASSERT_EFI_ERROR(Status);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;
        Status = PciIo->PollMem(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index),
            HDA_REG_SDNCTL1_SRST, HDA_REG_SDNCTL1_SRST, MS_TO_NANOSECOND(100), &Tmp);
        ASSERT_EFI_ERROR(Status);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;

        // Get value of control register.
        Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index), 1, &HdaStreamCtl1);
        ASSERT_EFI_ERROR(Status);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;

        // Docs state we need to clear the bit and wait for it to clear.
        HdaStreamCtl1 &= ~HDA_REG_SDNCTL1_SRST;
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index), 1, &HdaStreamCtl1);
        ASSERT_EFI_ERROR(Status);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;
        Status = PciIo->PollMem(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index),
            HDA_REG_SDNCTL1_SRST, 0, MS_TO_NANOSECOND(100), &Tmp);
        ASSERT_EFI_ERROR(Status);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;

        // Set buffer list lower base address.
        LowerBaseAddr = (UINT32)HdaStream->BufferListPhysAddr;
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_SDNBDPL(HdaStream->Index), 1, &LowerBaseAddr);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;

        // If 64-bit supported, set buffer list upper base address.
        if (HdaDev->Capabilities & HDA_REG_GCAP_64OK) {
            UpperBaseAddr = (UINT32)(HdaStream->BufferListPhysAddr >> 32);
            Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_SDNBDPU(HdaStream->Index), 1, &UpperBaseAddr);
            if (EFI_ERROR(Status))
                goto FREE_BUFFER;
        }

        // Allocate buffer for data.
        Status = PciIo->AllocateBuffer(PciIo, AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES(HDA_STREAM_BUF_SIZE),
            (VOID**)&HdaStream->BufferData, 0);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;
        ZeroMem(HdaStream->BufferData, HDA_STREAM_BUF_SIZE);
        
        // Map buffer descriptor list.
        DataLengthActual = HDA_STREAM_BUF_SIZE;
        Status = PciIo->Map(PciIo, EfiPciIoOperationBusMasterCommonBuffer, HdaStream->BufferData, &DataLengthActual,
            &HdaStream->BufferDataPhysAddr, &HdaStream->BufferDataMapping);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;
        if (DataLengthActual != HDA_STREAM_BUF_SIZE) {
            Status = EFI_OUT_OF_RESOURCES;
            goto FREE_BUFFER;
        }

        // Fill buffer list.
        for (UINTN b = 0; b < HDA_BDL_ENTRY_COUNT; b++) {
            // Set address and length of entry.
            DataBlockAddr = HdaStream->BufferDataPhysAddr + (b * HDA_BDL_BLOCKSIZE);
            HdaStream->BufferList[b].Address = (UINT32)DataBlockAddr;
            HdaStream->BufferList[b].AddressHigh = (UINT32)(DataBlockAddr >> 32);
            HdaStream->BufferList[b].Length = HDA_BDL_BLOCKSIZE;

            // We want the stream to signal when its completed either half of the
            // buffer so it can be refilled with fresh data.
            HdaStream->BufferList[b].InterruptOnCompletion =
                (((b == HDA_BDL_ENTRY_HALF) || (b == HDA_BDL_ENTRY_LAST)) ? HDA_BDL_ENTRY_IOC : 0);
        }

        // Set last valid index (LVI).
        StreamLvi = HDA_BDL_ENTRY_LAST;
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint16, PCI_HDA_BAR, HDA_REG_SDNLVI(HdaStream->Index), 1, &StreamLvi);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;

        // Set total buffer length.
        StreamCbl = HDA_STREAM_BUF_SIZE;
        Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint32, PCI_HDA_BAR, HDA_REG_SDNCBL(HdaStream->Index), 1, &StreamCbl);
        if (EFI_ERROR(Status))
            goto FREE_BUFFER;
    }

    // Success.
    return EFI_SUCCESS;

FREE_BUFFER:
    // Free stream arrays. TODO
    FreePool(HdaDev->BidirStreams);
    FreePool(HdaDev->InputStreams);
    FreePool(HdaDev->OutputStreams);
    return Status;
}

EFI_STATUS
EFIAPI
HdaControllerSetStream(
    IN HDA_STREAM *HdaStream,
    IN BOOLEAN Run,
    IN UINT8 Index) {
    if (HdaStream == NULL)
        return EFI_INVALID_PARAMETER;
    DEBUG((DEBUG_INFO, "HdaControllerSetStream(%u, %u, %u): start\n", HdaStream->Index, Run, Index));

    // Create variables.
    EFI_STATUS Status;
    EFI_PCI_IO_PROTOCOL *PciIo = HdaStream->HdaDev->PciIo;
    UINT8 HdaStreamCtl1;
    UINT8 HdaStreamCtl3;

    // Get current value of registers.
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index), 1, &HdaStreamCtl1);
    if (EFI_ERROR(Status))
        return Status;
    Status = PciIo->Mem.Read(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL3(HdaStream->Index), 1, &HdaStreamCtl3);
    if (EFI_ERROR(Status))
        return Status;
    
    // Update stream operation.
    if (Run)
        HdaStreamCtl1 |= HDA_REG_SDNCTL1_RUN;
    else
        HdaStreamCtl1 &= ~HDA_REG_SDNCTL1_RUN;

    // Update stream index.
    HdaStreamCtl3 = HDA_REG_SDNCTL3_STRM_SET(HdaStreamCtl3, Index);

    // Write register.
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL1(HdaStream->Index), 1, &HdaStreamCtl1);
    if (EFI_ERROR(Status))
        return Status;
    Status = PciIo->Mem.Write(PciIo, EfiPciIoWidthUint8, PCI_HDA_BAR, HDA_REG_SDNCTL3(HdaStream->Index), 1, &HdaStreamCtl3);
    if (EFI_ERROR(Status))
        return Status;

    // Success.
    return EFI_SUCCESS;
}
