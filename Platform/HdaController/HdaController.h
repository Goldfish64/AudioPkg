/*
 * File: HdaController.h
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

#ifndef EFI_HDA_CONTROLLER_H_
#define EFI_HDA_CONTROLLER_H_

#include "AudioDxe.h"

//
// Consumed protocols.
//
#include <Protocol/PciIo.h>
#include <IndustryStandard/Pci.h>

// Structure used for PCI class code parsing.
#pragma pack(1)
typedef struct {
    UINT8 ProgInterface;
    UINT8 SubClass;
    UINT8 Class;
} HDA_PCI_CLASSREG;
#pragma pack()

#define PCI_CLASS_MEDIA_HDA 0x3

#define HDA_MAX_CODECS 15

#define HDA_CONTROLLER_DEV_SIGNATURE SIGNATURE_32('h','d','a','P')

typedef struct {
    // Signature.
    UINTN Signature;

    // PCI protocol.
    EFI_PCI_IO_PROTOCOL *PciIo;
    EFI_DEVICE_PATH_PROTOCOL *DevicePath;
    UINT64 OriginalPciAttributes;
    EFI_HANDLE ControllerHandle;
    EFI_DRIVER_BINDING_PROTOCOL *DriverBinding;

    // Whether or not 64-bit addressing is supported.
    BOOLEAN Buffer64BitSupported;

    // Command output buffer (CORB).
    UINT32 *CorbBuffer;
    UINT32 CorbEntryCount;
    VOID *CorbMapping;
    EFI_PHYSICAL_ADDRESS CorbPhysAddr;
    UINT16 CorbWritePointer;

    // Response input buffer (RIRB).
    UINT64 *RirbBuffer;
    UINT32 RirbEntryCount;
    VOID *RirbMapping;
    EFI_PHYSICAL_ADDRESS RirbPhysAddr;
    UINT16 RirbReadPointer;

    // Events.
    EFI_EVENT ResponsePollTimer;
    EFI_EVENT ExitBootServiceEvent;
} HDA_CONTROLLER_DEV;

#define HDA_CONTROLLER_DEV_FROM_THIS(This) CR(This, HDA_CONTROLLER_DEV, DiskIo, HDA_CONTROLLER_DEV_SIGNATURE)

VOID
HdaControllerResponsePollTimerHandler(
    IN EFI_EVENT Event,
    IN VOID *Context);

EFI_STATUS
EFIAPI
HdaControllerRegisterDriver(VOID);

#endif
