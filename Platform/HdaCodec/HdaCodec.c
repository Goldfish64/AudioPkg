/*
 * File: HdaController.c
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

#include "HdaCodec.h"
#include "HdaCodecProtocol.h"
#include "HdaCodecComponentName.h"
#include "HdaVerbs.h"

EFI_STATUS
EFIAPI
HdaCodecDriverBindingSupported(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL) {
    //DEBUG((DEBUG_INFO, "HdaCodecDriverBindingSupported():start \n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_HDA_CODEC_PROTOCOL *HdaCodec;
    UINT8 CodecAddress;

    // Attempt to open the HDA codec protocol. If it can be opened, we can support it.
    Status = gBS->OpenProtocol(ControllerHandle, &gEfiHdaCodecProtocolGuid, (VOID**)&HdaCodec,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    if (EFI_ERROR(Status))
        return Status;
    
    // Get address of codec.
    Status = HdaCodec->GetAddress(HdaCodec, &CodecAddress);
    if (EFI_ERROR(Status))
        goto CLOSE_HDA;

    DEBUG((DEBUG_INFO, "HdaCodecDriverBindingSupported(): attaching to codec 0x%X\n", CodecAddress));
    Status = EFI_SUCCESS;

CLOSE_HDA:
    // Close protocol.
    gBS->CloseProtocol(ControllerHandle, &gEfiHdaCodecProtocolGuid, This->DriverBindingHandle, ControllerHandle);
    return Status;
}

EFI_STATUS
EFIAPI
HdaCodecDriverBindingStart(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL) {
    DEBUG((DEBUG_INFO, "HdaCodecDriverBindingStart(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_HDA_CODEC_PROTOCOL *HdaCodec;
    EFI_DEVICE_PATH_PROTOCOL *HdaDevicePath;
    UINT8 CodecAddress;
    UINT32 VendorId;

    Status = gBS->OpenProtocol(ControllerHandle, &gEfiHdaCodecProtocolGuid, (VOID**)&HdaCodec,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    if (EFI_ERROR(Status))
        return Status;

    // Get address.
    HdaCodec->GetAddress(HdaCodec, &CodecAddress);

    // Get vendor/device IDs.
    Status = HdaCodec->SendCommand(HdaCodec, HDA_NID_ROOT, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_VENDOR_ID), &VendorId);
    if (EFI_ERROR(Status))
        goto CLOSE_HDA;

    DEBUG((DEBUG_INFO, "HdaCodecDriverBindingStart(): attached to codec %4X:%4X @ 0x%X\n",
        (VendorId >> 16) & 0xFFFF, VendorId & 0xFFFF, CodecAddress));

    // Open Device Path protocol.
    Status = gBS->OpenProtocol(ControllerHandle, &gEfiDevicePathProtocolGuid, (VOID**)&HdaDevicePath,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    //ASSERT_EFI_ERROR(Status);
    if (EFI_ERROR (Status))
        return Status;

    // Get function group count.
    HDA_SUBNODE_COUNT FuncGroupCount;
    Status = HdaCodec->SendCommand(HdaCodec, HDA_NID_ROOT, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUBNODE_COUNT), (UINT32*)&FuncGroupCount);
    ASSERT_EFI_ERROR(Status);

    DEBUG((DEBUG_INFO, "%u function groups. Starting node: 0x%X\n", FuncGroupCount.NodeCount, FuncGroupCount.StartNode));

    // Go through function groups.
    for (UINT8 fNid = FuncGroupCount.StartNode; fNid < FuncGroupCount.StartNode + FuncGroupCount.NodeCount; fNid++) {
        DEBUG((DEBUG_INFO, "Probing function group 0x%X\n", fNid));

        // Get type.
        HDA_FUNC_GROUP_TYPE fType;
        Status = HdaCodec->SendCommand(HdaCodec, fNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_FUNC_GROUP_TYPE), (UINT32*)&fType);
        ASSERT_EFI_ERROR(Status);

        // Get caps.
        UINT32 fCaps;
        Status = HdaCodec->SendCommand(HdaCodec, fNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_FUNC_GROUP_CAPS), &fCaps);
        ASSERT_EFI_ERROR(Status);

        // Get node count.
        HDA_SUBNODE_COUNT fNodes;
        Status = HdaCodec->SendCommand(HdaCodec, fNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUBNODE_COUNT), (UINT32*)&fNodes);
        ASSERT_EFI_ERROR(Status);

        DEBUG((DEBUG_INFO, "Function Type: 0x%X Caps: 0x%X Nodes: %u (0x%X)\n", fType.NodeType, fCaps, fNodes.NodeCount, fNodes.StartNode));

        // Go through each widget.
        for (UINT8 wNid = fNodes.StartNode; wNid < fNodes.StartNode + fNodes.NodeCount; wNid++) {
            // Get caps.
            HDA_WIDGET_CAPS wCaps;
            Status = HdaCodec->SendCommand(HdaCodec, wNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_WIDGET_CAPS), (UINT32*)&wCaps);
            ASSERT_EFI_ERROR(Status);

            CHAR16 *wType;
            switch (wCaps.Type) {
                case HDA_WIDGET_TYPE_OUTPUT:
                    wType = L"Output";
                    break;

                case HDA_WIDGET_TYPE_INPUT:
                    wType = L"Input";
                    break;

                case HDA_WIDGET_TYPE_MIXER:
                    wType = L"Mixer";
                    break;

                case HDA_WIDGET_TYPE_SELECTOR:
                    wType = L"Selector";
                    break;

                case HDA_WIDGET_TYPE_PIN_COMPLEX:
                    wType = L"Pin Complex";
                    break;

                case HDA_WIDGET_TYPE_POWER:
                    wType = L"Power";
                    break;

                case HDA_WIDGET_TYPE_VOLUME_KNOB:
                    wType = L"Volume Knob";
                    break;

                case HDA_WIDGET_TYPE_BEEP_GEN:
                    wType = L"Beep Generator";
                    break;

                case HDA_WIDGET_TYPE_VENDOR:
                    wType = L"Vendor-defined";
                    break;

                default:
                    wType = L"Unknown";
            }

            CHAR16 *wChannels = L"Mono";
            if (wCaps.ChannelCountLsb)
                wChannels = L"Stereo";

            CHAR16 *wDigital = L"";
            if (wCaps.Digital)
                wDigital = L" Digital";

            CHAR16 *wAmpOut = L"";
            if (wCaps.OutAmpPresent)
                wAmpOut = L" Amp-Out";

            CHAR16 *wAmpIn = L"";
            if (wCaps.InAmpPresent)
                wAmpIn = L" Amp-In";

            CHAR16 *wRlSwapped = L"";
            if (wCaps.LeftRightSwapped)
                wRlSwapped = L" R/L";

            DEBUG((DEBUG_INFO, "Node 0x%X [%s] wcaps 0x%X: %s%s%s%s%s\n",
                wNid, wType, *((UINT32*)&wCaps), wChannels, wDigital, wAmpOut, wAmpIn, wRlSwapped));

            // If in amp is present, show caps.
            if (wCaps.InAmpPresent) {
                HDA_AMP_CAPS wInAmpCaps;
                Status = HdaCodec->SendCommand(HdaCodec, wNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_AMP_CAPS_INPUT), (UINT32*)&wInAmpCaps);
                ASSERT_EFI_ERROR(Status);
                DEBUG((DEBUG_INFO, "  Amp-In caps: ofs=0x%2X, nsteps=0x%2X, stepsize=0x%2X, mute=%u\n",
                    wInAmpCaps.Offset, wInAmpCaps.NumSteps, wInAmpCaps.StepSize, wInAmpCaps.MuteCapable));
            }

            // If out amp is present, show caps.
            if (wCaps.OutAmpPresent) {
                HDA_AMP_CAPS wOutAmpCaps;
                Status = HdaCodec->SendCommand(HdaCodec, wNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_AMP_CAPS_OUTPUT), (UINT32*)&wOutAmpCaps);
                ASSERT_EFI_ERROR(Status);
                DEBUG((DEBUG_INFO, "  Amp-Out caps: ofs=0x%2X, nsteps=0x%2X, stepsize=0x%2X, mute=%u\n",
                    wOutAmpCaps.Offset, wOutAmpCaps.NumSteps, wOutAmpCaps.StepSize, wOutAmpCaps.MuteCapable));
            }

            // If this is an input or output node, show stream data.
            if (wCaps.Type == HDA_WIDGET_TYPE_INPUT || wCaps.Type == HDA_WIDGET_TYPE_OUTPUT) {
                // Get supported PCM bits and rates.
                HDA_SUPPORTED_PCM_SIZE_RATES wPcmRates;
                Status = HdaCodec->SendCommand(HdaCodec, wNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES), (UINT32*)&wPcmRates);
                ASSERT_EFI_ERROR(Status);

                // Get supported stream formats.
                HDA_SUPPORTED_STREAM_FORMATS wStreamFormats;
                Status = HdaCodec->SendCommand(HdaCodec, wNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUPPORTED_STREAM_FORMATS), (UINT32*)&wStreamFormats);
                ASSERT_EFI_ERROR(Status);

                DEBUG((DEBUG_INFO, "  PCM:\n"));
                DEBUG((DEBUG_INFO, "    rates [0x%X]:", ((UINT16*)&wPcmRates)[0]));
                if (wPcmRates.Hz8000)
                    DEBUG((DEBUG_INFO, " 8000"));
                if (wPcmRates.Hz11025)
                    DEBUG((DEBUG_INFO, " 11025"));
                if (wPcmRates.Hz16000)
                    DEBUG((DEBUG_INFO, " 16000"));
                if (wPcmRates.Hz22050)
                    DEBUG((DEBUG_INFO, " 22050"));
                if (wPcmRates.Hz32000)
                    DEBUG((DEBUG_INFO, " 32000"));
                if (wPcmRates.Hz44100)
                    DEBUG((DEBUG_INFO, " 44100"));
                if (wPcmRates.Hz48000)
                    DEBUG((DEBUG_INFO, " 48000"));
                if (wPcmRates.Hz88200)
                    DEBUG((DEBUG_INFO, " 88200"));
                if (wPcmRates.Hz96000)
                    DEBUG((DEBUG_INFO, " 96000"));
                if (wPcmRates.Hz176400)
                    DEBUG((DEBUG_INFO, " 176400"));
                if (wPcmRates.Hz192000)
                    DEBUG((DEBUG_INFO, " 192000"));
                if (wPcmRates.Hz384000)
                    DEBUG((DEBUG_INFO, " 384000"));
                DEBUG((DEBUG_INFO, "\n"));

                DEBUG((DEBUG_INFO, "    bits [0x%X]:", ((UINT16*)&wPcmRates)[1]));
                if (wPcmRates.Bits8)
                    DEBUG((DEBUG_INFO, " 8"));
                if (wPcmRates.Bits16)
                    DEBUG((DEBUG_INFO, " 16"));
                if (wPcmRates.Bits20)
                    DEBUG((DEBUG_INFO, " 20"));
                if (wPcmRates.Bits24)
                    DEBUG((DEBUG_INFO, " 24"));
                if (wPcmRates.Bits32)
                    DEBUG((DEBUG_INFO, " 32"));
                DEBUG((DEBUG_INFO, "\n"));

                DEBUG((DEBUG_INFO, "    formats [0x%X]:", *((UINT32*)&wStreamFormats)));
                if (wStreamFormats.PcmSupported)
                    DEBUG((DEBUG_INFO, " PCM"));
                if (wStreamFormats.Float32Supported)
                    DEBUG((DEBUG_INFO, " FLOAT32"));
                if (wStreamFormats.Ac3Supported)
                    DEBUG((DEBUG_INFO, " AC3"));
                DEBUG((DEBUG_INFO, "\n"));
            }
        }
    }


    return EFI_SUCCESS;
CLOSE_HDA:
    // Close protocol.
    gBS->CloseProtocol(ControllerHandle, &gEfiHdaCodecProtocolGuid, This->DriverBindingHandle, ControllerHandle);
    return Status;
}

EFI_STATUS
EFIAPI
HdaCodecDriverBindingStop(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN UINTN NumberOfChildren,
    IN EFI_HANDLE *ChildHandleBuffer OPTIONAL) {
    DEBUG((DEBUG_INFO, "HdaCodecDriverBindingStop()\n"));
    return EFI_UNSUPPORTED;
}
