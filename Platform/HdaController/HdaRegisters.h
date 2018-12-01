/*
 * File: HdaRegisters.h
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

#ifndef _EFI_HDA_REGS_H__
#define _EFI_HDA_REGS_H__

#include "AudioDxe.h"

// HDA controller is accessed via MMIO on BAR #0.
#define PCI_HDA_BAR 0

// Min supported version.
#define HDA_VERSION_MIN_MAJOR   0x1
#define HDA_VERSION_MIN_MINOR   0x0

//
// HDA registers.
//
#define HDA_REG_GCAP    0x00 // Global Capabilities; 2 bytes.
#define HDA_REG_VMIN    0x02 // Minor Version; 1 byte.
#define HDA_REG_VMAJ    0x03 // Major Version; 1 byte.
#define HDA_REG_OUTPAY  0x04 // Output Payload Capability; 2 bytes.
#define HDA_REG_INPAY   0x06 // Input Payload Capability; 2 bytes.
#define HDA_REG_GCTL    0x08 // Global Control; 4 bytes.
#define HDA_REG_GCTL_CRST   (1 << 0)

#define HDA_REG_WAKEEN  0x0C
#define HDA_REG_STATESTS 0x0E

#define HDA_REG_INTCTL      0x20
#define HDA_REG_INTSTS      0x24

// Command Output Ring Buffer.
#define HDA_REG_CORBLBASE   0x40 // CORB Lower Base Address; 4 bytes.
#define HDA_REG_CORBUBASE   0x44 // CORB Upper Base Address; 4 bytes.
#define HDA_REG_CORBWP      0x48 // CORB Write Pointer; 2 bytes.
#define HDA_REG_CORBRP      0x4A // CORB Read Pointer; 2 bytes.
#define HDA_REG_CORBCTL     0x4C // CORB Control; 1 byte.
#define HDA_REG_CORBSTS     0x4D // CORB Status; 1 byte.
#define HDA_REG_CORBSIZE    0x4E // CORB Size; 1 byte.

#define HDA_REG_CORBWP_MASK         0xFF // CORB Write Pointer; 1 byte.
#define HDA_REG_CORBRP_MASK         0xFF // CORB Read Pointer; 1 byte.
#define HDA_REG_CORBRP_RST          (1 << 15) // CORB Read Pointer Reset bit.
#define HDA_REG_CORBCTL_CMEIE       (1 << 0) // CORB Memory Error Interrupt Enable bit.
#define HDA_REG_CORBCTL_CORBRUN     (1 << 1) // Enable CORB DMA Engine bit.
#define HDA_REG_CORBSTS_CMEI        (1 << 0) // CORB Memory Error Indication bit.

// CORB Size.
#define HDA_REG_CORBSIZE_MASK           ((1 << 0) | (1 << 1))
#define HDA_REG_CORBSIZE_ENT2           0        // 8 B = 2 entries.
#define HDA_REG_CORBSIZE_ENT16          (1 << 0) // 64 B = 16 entries.
#define HDA_REG_CORBSIZE_ENT256         (1 << 1) // 1 KB = 256 entries.
#define HDA_REG_CORBSIZE_CORBSZCAP_2    (1 << 4) // 8 B = 2 entries.
#define HDA_REG_CORBSIZE_CORBSZCAP_16   (1 << 5) // 64 B = 16 entries.
#define HDA_REG_CORBSIZE_CORBSZCAP_256  (1 << 6) // 1 KB = 256 entries.

// Response Input Ring Buffer.
#define HDA_REG_RIRBLBASE   0x50 // RIRB Lower Base Address; 4 bytes.
#define HDA_REG_RIRBUBASE   0x54 // RIRB Upper Base Address; 4 bytes.
#define HDA_REG_RIRBWP      0x58 // RIRB Write Pointer; 2 bytes.
#define HDA_REG_RINTCNT     0x5A // Response Interrupt Count; 2 bytes.
#define HDA_REG_RIRBCTL     0x5C // RIRB Control; 1 byte.
#define HDA_REG_RIRBSTS     0x5D // RIRB Status; 1 byte.
#define HDA_REG_RIRBSIZE    0x5E // RIRB Size; 1 byte.

#define HDA_REG_RIRBWP_MASK         0xFF // RIRB Write Pointer; 1 byte.
#define HDA_REG_RIRBWP_RST          (1 << 15) // RIRB Write Pointer Reset bit.
#define HDA_REG_RINTCNT_MASK        0xFF // N Response Interrupt Count; 0 = 256.
#define HDA_REG_RIRBCTL_RINTCTL     (1 << 0) // Response Interrupt Control bit.
#define HDA_REG_RIRBCTL_RIRBDMAEN   (1 << 1) // RIRB DMA Enable bit.
#define HDA_REG_RIRBCTL_RIRBOIC     (1 << 2) // Response Overrun Interrupt Control bit.
#define HDA_REG_RIRBSTS_RINTFL      (1 << 0) // Response Interrupt bit.
#define HDA_REG_RIRBSTS_RIRBOIS     (1 << 2) // Response Overrun Interrupt Status bit.

// RIRB Size.
#define HDA_REG_RIRBSIZE_MASK           ((1 << 0) | (1 << 1))
#define HDA_REG_RIRBSIZE_ENT2           0        // 16 B = 2 entries.
#define HDA_REG_RIRBSIZE_ENT16          (1 << 0) // 128 B = 16 entries.
#define HDA_REG_RIRBSIZE_ENT256         (1 << 1) // 2 KB = 256 entries.
#define HDA_REG_RIRBSIZE_RIRBSZCAP_2    (1 << 4) // 16 B = 2 entries.
#define HDA_REG_RIRBSIZE_RIRBSZCAP_16   (1 << 5) // 128 B = 16 entries.
#define HDA_REG_RIRBSIZE_RIRBSZCAP_256  (1 << 6) // 2 KB = 256 entries.

// Direct command interface.
#define HDA_REG_ICOI        0x60 // Immediate Command Output Interface; 4 bytes.
#define HDA_REG_ICII        0x64 // Immediate Command Input Interface; 4 bytes.
#define HDA_REG_ICIS        0x68 // Immediate Command Status; 2 bytes.

// Stream descriptors. Streams start at 0 with input streams, then output, and then bidir.
/*#define HDA_REG_SDCTL(s)    (0x80 + (s * 0x20)) // Stream Control; 3 bytes.
#define HDA_REG_SDSTS(s)    (0x83 + (s * 0x20)) // Stream Status; 2 bytes.
#define HDA_REG_SDLPIB(s)   (0x84 + (s * 0x20)) // Stream Link Position in Current Buffer; 4 bytes.
#define HDA_REG_SDCBL(s)    (0x88 + (s * 0x20)) // Stream Cyclic Buffer Length; 4 bytes.
#define HDA_REG_SDLVI(s)    (0x8C + (s * 0x20)) // Stream Last Valid Index; 2 bytes.
#define HDA_REG_SDFIFOD(s)  (0x90 + (s * 0x20)) // Stream FIFO Size; 2 bytes.
#define HDA_REG_SDFMT(s)    (0x90 + (s * 0x20)) // Stream Format; 2 bytes.
#define HDA_REG_SDBDPL(s)   (0x98 + (s * 0x20)) // Stream Buffer Descriptor List Pointer - Lower; 4 bytes.
#define HDA_REG_SDBDPU(s)   (0x9C + (s * 0x20)) // Stream Buffer Descriptor List Pointer - Upper; 4 bytes.*/

#define HDA_REG_SDCTL(s)    0x100 // Stream Control; 3 bytes.
#define HDA_REG_SDSTS(s)    0x103 // Stream Status; 2 bytes.
#define HDA_REG_SDLPIB(s)   0x104 // Stream Link Position in Current Buffer; 4 bytes.
#define HDA_REG_SDCBL(s)    0x108 // Stream Cyclic Buffer Length; 4 bytes.
#define HDA_REG_SDLVI(s)    0x10C // Stream Last Valid Index; 2 bytes.
#define HDA_REG_SDFIFOD(s)  0x110 // Stream FIFO Size; 2 bytes.
#define HDA_REG_SDFMT(s)    0x112 // Stream Format; 2 bytes.
#define HDA_REG_SDBDPL(s)   0x118 // Stream Buffer Descriptor List Pointer - Lower; 4 bytes.
#define HDA_REG_SDBDPU(s)   0x11C // Stream Buffer Descriptor List Pointer - Upper; 4 bytes.

#pragma pack(1)

// GCAP register.
typedef struct {
    BOOLEAN Addressing64Bit : 1;
    UINT8 NumSerialDataOutSignals : 2;
    UINT8 NumBidirStreams : 5;
    UINT8 NumInputStreams : 4;
    UINT8 NumOutputStreams : 4;
} HDA_GCAP;

// GCTL register.
typedef struct {
    BOOLEAN Reset : 1;
    BOOLEAN Flush : 1;
    UINT8 Reserved1 : 6;
    BOOLEAN AcceptUnsolResponses : 1;
    UINT8 Reserved2 : 7;
    UINT8 Reserved3 : 8;
} HDA_GCTL;

typedef struct {
    BOOLEAN Reset : 1;
    BOOLEAN Run : 1;
    BOOLEAN InterruptCompletion : 1;
    BOOLEAN InterruptFifoError : 1;
    BOOLEAN InterruptDescError : 1;
    UINT8 Reserved1 : 3;
    UINT8 Reserved2 : 8;
    UINT8 StripeControl : 2;
    BOOLEAN PriorityTraffic : 1;
    BOOLEAN BidirOutput : 1;
    UINT8 Number : 4;
} HDA_STREAMCTL;
#define HDA_REG_SDCTL_SRST (1 << 0)

#pragma pack()

#endif