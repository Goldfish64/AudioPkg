/*
 * File: HdaCodecDebug.c
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
#include "HdaVerbs.h"

EFI_STATUS
EFIAPI
HdaCodecPrintDefaults(
    HDA_CODEC_DEV *HdaCodecDev) {
    HDA_FUNC_GROUP *AudioFuncGroup = HdaCodecDev->FuncGroups[HdaCodecDev->AudioFuncGroup];
    for (UINT8 i = 0; i < AudioFuncGroup->WidgetsLength; i++) {
        HDA_WIDGET *HdaWidget = AudioFuncGroup->Widgets[i];
        HDA_WIDGET_CAPS wCaps = HdaWidget->Capabilities;

        CHAR16 *wType;
        switch (wCaps.Type) {
            case HDA_WIDGET_TYPE_OUTPUT:
                wType = L"Audio Output";
                break;

            case HDA_WIDGET_TYPE_INPUT:
                wType = L"Audio Input";
                break;

            case HDA_WIDGET_TYPE_MIXER:
                wType = L"Audio Mixer";
                break;

            case HDA_WIDGET_TYPE_SELECTOR:
                wType = L"Audio Selector";
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
                wType = L"Vendor Defined Widget";
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

        // Show header.
        DEBUG((DEBUG_INFO, "Node 0x%2X [%s] wcaps 0x%X: %s%s%s%s%s\n",
            HdaWidget->NodeId, wType, *((UINT32*)&wCaps), wChannels, wDigital, wAmpOut, wAmpIn, wRlSwapped));

        // If this is an input or output node, show stream data.
        if (wCaps.Type == HDA_WIDGET_TYPE_INPUT || wCaps.Type == HDA_WIDGET_TYPE_OUTPUT) {
            HDA_WIDGET_INPUT_OUTPUT *HdaInOutWidget = (HDA_WIDGET_INPUT_OUTPUT*)HdaWidget;
            HDA_SUPPORTED_PCM_SIZE_RATES wPcmRates = HdaInOutWidget->SizesRates;
            HDA_SUPPORTED_STREAM_FORMATS wStreamFormats = HdaInOutWidget->StreamFormats;

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
        } else if (wCaps.Type == HDA_WIDGET_TYPE_PIN_COMPLEX) {
            HDA_WIDGET_PIN_COMPLEX *HdaPinWidget = (HDA_WIDGET_PIN_COMPLEX*)HdaWidget;

            DEBUG((DEBUG_INFO, "  Pincap 0x%8X\n", *((UINT32*)&HdaPinWidget->PinCapabilities)));

            // Determine port connectivity.
            CHAR16 *wPortConnectivities[] = { L"Jack", L"None", L"Fixed", L"Int Jack" };
            CHAR16 *wPortConnectivity = wPortConnectivities[HdaPinWidget->ConfigurationDefault.PortConnectivity];

            // Determine default device.
            CHAR16 *wDefaultDevices[] = {
                L"Line Out", L"Speaker", L"HP Out", L"CD", L"SPDIF Out", L"Digital Out", L"Modem Line",
                L"Modem Handset", L"Line In", L"Aux", L"Mic", L"Telephone", L"SPDIF In", L"Digital In", L"Other", L"Other" };
            CHAR16 *wDefaultDevice = wDefaultDevices[HdaPinWidget->ConfigurationDefault.DefaultDevice];

            // Determine surface.
            CHAR16 *wSurfaces[] = { L"Ext", L"Int", L"Ext", L"Other" };
            CHAR16 *wSurface = wSurfaces[HdaPinWidget->ConfigurationDefault.LocationSurface];

            // Determine location.
            CHAR16 *wLocations[] = { L"N/A", L"Rear", L"Front", L"Left", L"Right", L"Top", L"Bottom" };
            CHAR16 *wLocation = L"Other";
            if (HdaPinWidget->ConfigurationDefault.LocationSpecific <= HDA_CONFIG_DEFAULT_LOC_SPEC_BOTTOM) 
                wLocation = wLocations[HdaPinWidget->ConfigurationDefault.LocationSpecific];

            // Determine connection type.
            CHAR16 *wConnTypes[] = {
                L"Unknown", L"1/8", L"1/4", L"ATAPI", L"RCA", L"Optical", L"Digital",
                L"Analog", L"Multi", L"XLR", L"RJ11", L"Combo", L"Other", L"Other", L"Other", L"Other" };
            CHAR16 *wConnType = wConnTypes[HdaPinWidget->ConfigurationDefault.ConnectionType];

            // Determine color.
            CHAR16 *wColors[] = {
                L"Unknown", L"Black", L"Grey", L"Blue", L"Green", L"Red", L"Orange",
                L"Yellow", L"Purple", L"Pink", L"Other", L"Other", L"Other", L"Other", L"White", L"Other" };
            CHAR16 *wColor = wColors[HdaPinWidget->ConfigurationDefault.Color];
            
            DEBUG((DEBUG_INFO, "  Pin Default 0x%8X: [%s] %s at %s %s\n",
                *((UINT32*)&HdaPinWidget->ConfigurationDefault), wPortConnectivity, wDefaultDevice, wSurface, wLocation));
            DEBUG((DEBUG_INFO, "    Conn = %s, Color = %s\n", wConnType, wColor));
            DEBUG((DEBUG_INFO, "    DefAssociation = 0x%X, Sequence = 0x%X\n",
                HdaPinWidget->ConfigurationDefault.DefaultAssociaton, HdaPinWidget->ConfigurationDefault.Sequence));
            DEBUG((DEBUG_INFO, "  Pin-ctls: 0x%X\n", *((UINT32*)&HdaPinWidget->PinControl)));
        }
    }

    return EFI_SUCCESS;
}
