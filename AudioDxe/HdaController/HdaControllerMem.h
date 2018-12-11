/*
 * File: HdaControllerMem.h
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

#ifndef _EFI_HDA_CONTROLLER_MEM_H_
#define _EFI_HDA_CONTROLLER_MEM_H_

#include "HdaController.h"

HDA_CONTROLLER_DEV *HdaControllerAllocDevice(
    IN EFI_PCI_IO_PROTOCOL *PciIo,
    IN EFI_DEVICE_PATH_PROTOCOL *DevicePath,
    IN UINT64 OriginalPciAttributes);

EFI_STATUS
EFIAPI
HdaControllerInitCorb(
    IN HDA_CONTROLLER_DEV *HdaDev);

EFI_STATUS
EFIAPI
HdaControllerCleanupCorb(
    IN HDA_CONTROLLER_DEV *HdaDev);

EFI_STATUS
EFIAPI
HdaControllerSetCorb(
    IN HDA_CONTROLLER_DEV *HdaDev,
    IN BOOLEAN Enable);

EFI_STATUS
EFIAPI
HdaControllerInitRirb(
    IN HDA_CONTROLLER_DEV *HdaDev);

EFI_STATUS
EFIAPI
HdaControllerCleanupRirb(
    IN HDA_CONTROLLER_DEV *HdaDev);

EFI_STATUS
EFIAPI
HdaControllerSetRirb(
    IN HDA_CONTROLLER_DEV *HdaDev,
    IN BOOLEAN Enable);

EFI_STATUS
EFIAPI
HdaControllerInitStreams(
    IN HDA_CONTROLLER_DEV *HdaDev);

EFI_STATUS
EFIAPI
HdaControllerSetStream(
    IN HDA_STREAM *HdaStream,
    IN BOOLEAN Run,
    IN UINT8 Index);

#endif
