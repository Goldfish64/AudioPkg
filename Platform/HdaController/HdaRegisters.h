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

// HDA controller is accessed via MMIO on BAR #0.
#define PCI_HDA_BAR 0

// Min supported version.
#define HDA_VERSION_MIN_MAJOR   0x1
#define HDA_VERSION_MIN_MINOR   0x0

//
// HDA registers.
//
#define HDA_REG_GCAP    0x00
#define HDA_REG_GCAP_64BIT 0x1

#define HDA_REG_VMIN    0x02
#define HDA_REG_VMAJ    0x03
#define HDA_REG_OUTPAY  0x04
#define HDA_REG_INPAY   0x06

#define HDA_REG_GCTL    0x08
#define HDA_REG_GCTL_CRST   (1 << 0)
#define HDA_REG_GCTL_FCNTRL (1 << 1)
#define HDA_REG_GCTL_UNSOL  (1 << 8)

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

#endif