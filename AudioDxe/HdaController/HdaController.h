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

#ifndef _EFI_HDA_CONTROLLER_H_
#define _EFI_HDA_CONTROLLER_H_

#include "AudioDxe.h"
#include "HdaRegisters.h"

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


// HDA controller is accessed via MMIO on BAR #0.
#define PCI_HDA_BAR 0

// Min supported version.
#define HDA_VERSION_MIN_MAJOR   0x1
#define HDA_VERSION_MIN_MINOR   0x0

#define HDA_MAX_CODECS 15

#define PCI_HDA_TCSEL_OFFSET    0x44
#define PCI_HDA_TCSEL_TC0_MASK  ~(BIT0 | BIT1 | BIT2)
#define PCI_HDA_DEVC_OFFSET     0x78
#define PCI_HDA_DEVC_NOSNOOPEN  BIT11

#define HDA_CORB_VERB(Cad, Nid, Verb) ((((UINT32)Cad) << 28) | (((UINT32)Nid) << 20) | (Verb & 0xFFFFF))

#define HDA_RIRB_RESP(Response)     ((UINT32)Response)
#define HDA_RIRB_CAD(Response)      ((Response >> 32) & 0xF)
#define HDA_RIRB_UNSOL(Response)    ((Response >> 36) & 0x1)

//
// Streams.
//
// Buffer Descriptor List Entry.
#pragma pack(1)
typedef struct {
    UINT32 Address;
    UINT32 AddressHigh;
    UINT32 Length;
    UINT32 Reserved;
} HDA_BDL_ENTRY;
#pragma pack()
#define HDA_NUM_OF_BDL_ENTRIES  256
#define HDA_BDL_SIZE            (sizeof(HDA_BDL_ENTRY) * HDA_NUM_OF_BDL_ENTRIES)

typedef struct _HDA_CONTROLLER_DEV HDA_CONTROLLER_DEV;

#define HDA_STREAM_TYPE_BIDIR   0
#define HDA_STREAM_TYPE_IN      1
#define HDA_STREAM_TYPE_OUT     2

// Stream.
typedef struct {
    HDA_CONTROLLER_DEV *HdaDev;

    UINT8 Type;
    UINT8 Index;
    HDA_BDL_ENTRY *BufferList;
    VOID *BufferListMapping;
    EFI_PHYSICAL_ADDRESS BufferListPhysAddr;
} HDA_STREAM;

typedef struct _HDA_CONTROLLER_INFO_PRIVATE_DATA HDA_CONTROLLER_INFO_PRIVATE_DATA;

typedef struct _HDA_CONTROLLER_PRIVATE_DATA HDA_CONTROLLER_PRIVATE_DATA;
struct _HDA_CONTROLLER_DEV {
    // PCI protocol.
    EFI_PCI_IO_PROTOCOL *PciIo;
    EFI_DEVICE_PATH_PROTOCOL *DevicePath;
    UINT64 OriginalPciAttributes;
    EFI_HANDLE ControllerHandle;
    EFI_DRIVER_BINDING_PROTOCOL *DriverBinding;

    // Published info protocol.
    HDA_CONTROLLER_INFO_PRIVATE_DATA *HdaControllerInfoData;

    // Capabilites.
    UINT32 VendorId;
    CHAR16 *Name;
    UINT8 MajorVersion;
    UINT8 MinorVersion;
    UINT16 Capabilities;

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

    // Streams.
    UINT8 TotalStreamsCount;
    UINT8 BidirStreamsCount;
    UINT8 InputStreamsCount;
    UINT8 OutputStreamsCount;
    HDA_STREAM *BidirStreams;
    HDA_STREAM *InputStreams;
    HDA_STREAM *OutputStreams;
    

    // Events.
    EFI_EVENT ResponsePollTimer;
    EFI_EVENT ExitBootServiceEvent;
    SPIN_LOCK SpinLock;
    HDA_CONTROLLER_PRIVATE_DATA *PrivateDatas[HDA_MAX_CODECS];

    UINT32 *dmaList;
};

// HDA Codec Info private data.
#define HDA_CONTROLLER_PRIVATE_DATA_SIGNATURE SIGNATURE_32('H','d','a','C')
struct _HDA_CONTROLLER_INFO_PRIVATE_DATA {
    // Signature.
    UINTN Signature;

    // HDA Codec Info protocol and codec device.
    EFI_HDA_CONTROLLER_INFO_PROTOCOL HdaControllerInfo;
    HDA_CONTROLLER_DEV *HdaControllerDev;
};

#define HDA_CONTROLLER_INFO_PRIVATE_DATA_FROM_THIS(This) \
    CR(This, HDA_CONTROLLER_INFO_PRIVATE_DATA, HdaControllerInfo, HDA_CONTROLLER_PRIVATE_DATA_SIGNATURE)

struct _HDA_CONTROLLER_PRIVATE_DATA {
    // Signature.
    UINTN Signature;

    // HDA Codec protocol and address.
    EFI_HDA_IO_PROTOCOL HdaCodec;
    UINT8 HdaCodecAddress;

    // HDA controller.
    HDA_CONTROLLER_DEV *HdaDev;
};

#define HDA_CONTROLLER_PRIVATE_DATA_FROM_THIS(This) CR(This, HDA_CONTROLLER_PRIVATE_DATA, HdaCodec, HDA_CONTROLLER_PRIVATE_DATA_SIGNATURE)

EFI_STATUS
EFIAPI
HdaControllerCodecProtocolSendCommand(
    IN EFI_HDA_IO_PROTOCOL *This,
    IN UINT8 Node,
    IN UINT32 Verb,
    OUT UINT32 *Response);

EFI_STATUS
EFIAPI
HdaControllerCodecProtocolSendCommands(
    IN EFI_HDA_IO_PROTOCOL *This,
    IN UINT8 Node,
    IN EFI_HDA_IO_VERB_LIST *Verbs);

EFI_STATUS
EFIAPI
HdaControllerSendCommands(
    IN HDA_CONTROLLER_DEV *HdaDev,
    IN UINT8 CodecAddress,
    IN UINT8 Node,
    IN EFI_HDA_IO_VERB_LIST *Verbs);

VOID
HdaControllerResponsePollTimerHandler(
    IN EFI_EVENT Event,
    IN VOID *Context);

EFI_STATUS
EFIAPI
HdaControllerDriverBindingSupported(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL);

EFI_STATUS
EFIAPI
HdaControllerDriverBindingStart(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL);

EFI_STATUS
EFIAPI
HdaControllerDriverBindingStop(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN UINTN NumberOfChildren,
    IN EFI_HANDLE *ChildHandleBuffer OPTIONAL);

EFI_STATUS
EFIAPI
HdaControllerInfoGetName(
    IN  EFI_HDA_CONTROLLER_INFO_PROTOCOL *This,
    OUT CHAR16 **ControllerName);

#endif
