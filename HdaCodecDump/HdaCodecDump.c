/*
 * File: HdaCodecDump.c
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

#include "HdaCodecDump.h"

EFI_STATUS
EFIAPI
HdaCodecDumpMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable) {
    Print(L"HdaCodecDump start\n");

    // Create variables.
    EFI_STATUS Status;
    EFI_HANDLE *HdaCodecHandles;
    UINTN HdaCodecHandleCount;
    EFI_HDA_CODEC_INFO_PROTOCOL *HdaCodecInfo;

    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiHdaCodecInfoProtocolGuid, NULL, &HdaCodecHandleCount, &HdaCodecHandles);
    ASSERT_EFI_ERROR(Status);

    Status = gBS->OpenProtocol(HdaCodecHandles[0], &gEfiHdaCodecInfoProtocolGuid, (VOID**)&HdaCodecInfo, NULL, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    ASSERT_EFI_ERROR(Status);

    // Get AFG ID.
    UINT8 AudioFuncId;
    Status = HdaCodecInfo->GetAudioFuncId(HdaCodecInfo, &AudioFuncId);
    Print(L"AFG Function Id: 0x%X\n", AudioFuncId);

    // Get vendor.
    UINT32 VendorId;
    Status = HdaCodecInfo->GetVendorId(HdaCodecInfo, &VendorId);
    Print(L"Vendor ID: 0x%X\n", VendorId);

    // Get revision.
    UINT32 RevisionId;
    Status = HdaCodecInfo->GetRevisionId(HdaCodecInfo, &RevisionId);
    Print(L"Revision ID: 0x%X\n", RevisionId);



    return EFI_SUCCESS;
}
