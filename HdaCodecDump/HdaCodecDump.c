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
    BOOLEAN Unsol;
    Status = HdaCodecInfo->GetAudioFuncId(HdaCodecInfo, &AudioFuncId, &Unsol);
    Print(L"AFG Function Id: 0x%X (unsol %u)\n", AudioFuncId, Unsol);

    // Get vendor.
    UINT32 VendorId;
    Status = HdaCodecInfo->GetVendorId(HdaCodecInfo, &VendorId);
    Print(L"Vendor ID: 0x%X\n", VendorId);

    // Get revision.
    UINT32 RevisionId;
    Status = HdaCodecInfo->GetRevisionId(HdaCodecInfo, &RevisionId);
    Print(L"Revision ID: 0x%X\n", RevisionId);

    // Get supported rates/formats.
    UINT32 Rates, Formats;
    Status = HdaCodecInfo->GetDefaultRatesFormats(HdaCodecInfo, &Rates, &Formats);
    Print(L"Default PCM:\n");

    if ((Rates != 0) || (Formats != 0)) {
        // Print sample rates.
        Print(L"    rates [0x%X]:", (UINT16)Rates);
        if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_8KHZ)
            Print(L" 8000");
        if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_11KHZ)
            Print(L" 11025");
        if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_16KHZ)
            Print(L" 16000");
        if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_22KHZ)
            Print(L" 22050");
        if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_32KHZ)
            Print(L" 32000");
        if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_44KHZ)
            Print(L" 44100");
        if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_48KHZ)
            Print(L" 48000");
        if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_88KHZ)
            Print(L" 88200");
        if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_96KHZ)
            Print(L" 96000");
        if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_176KHZ)
            Print(L" 176400");
        if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_192KHZ)
            Print(L" 192000");
        if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_384KHZ)
            Print(L" 384000");
        Print(L"\n");

        // Print bits.
        Print(L"    bits [0x%X]:", (UINT16)(Rates >> 16));
        if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_8BIT)
            Print(L" 8");
        if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_16BIT)
            Print(L" 16");
        if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_20BIT)
            Print(L" 20");
        if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_24BIT)
            Print(L" 24");
        if (Rates & HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES_32BIT)
            Print(L" 32");
        Print(L"\n");

        // Print formats.
        Print(L"    formats [0x%X]:", Formats);
        if (Formats & HDA_PARAMETER_SUPPORTED_STREAM_FORMATS_PCM)
            Print(L" PCM");
        if (Formats & HDA_PARAMETER_SUPPORTED_STREAM_FORMATS_FLOAT32)
            Print(L" FLOAT32");
        if (Formats & HDA_PARAMETER_SUPPORTED_STREAM_FORMATS_AC3)
            Print(L" AC3");
        Print(L"\n");
    } else {
        Print(L" N/A\n");
    }

    return EFI_SUCCESS;
}
