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

#ifndef __EFI_HDA_REGS_H__
#define __EFI_HDA_REGS_H__

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
#define HDA_REG_GCTL_CRST   0x001
#define HDA_REG_GCTL_FCNTRL 0x002
#define HDA_REG_GCTL_UNSOL  0x100

#define HDA_REG_WAKEEN  0x0C
#define HDA_REG_STATESTS 0x0E

#define HDA_REG_INTCTL      0x20
#define HDA_REG_INTSTS      0x24
#define HDA_REG_CORBLBASE   0x40
#define HDA_REG_CORBUBASE   0x44
#define HDA_REG_CORBWP      0x48
#define HDA_REG_CORBRP      0x4A
#define HDA_REG_CORBCTL     0x4C
#define HDA_REG_CORBSTS     0x4D
#define HDA_REG_CORBSIZE    0x4E


#endif
