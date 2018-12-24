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

VOID
EFIAPI
HdaCodecDumpPrintRatesFormats(
    IN UINT32 Rates,
    IN UINT32 Formats) {
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
}

VOID
EFIAPI
HdaCodecDumpPrintAmpCaps(
    IN UINT32 AmpCaps) {
    if (AmpCaps) {
        Print(L"ofs=0x%2X, nsteps=0x%2X, stepsize=0x%2, mute=%u\n",
            HDA_PARAMETER_AMP_CAPS_OFFSET(AmpCaps), HDA_PARAMETER_AMP_CAPS_NUM_STEPS(AmpCaps),
            HDA_PARAMETER_AMP_CAPS_STEP_SIZE(AmpCaps), (AmpCaps & HDA_PARAMETER_AMP_CAPS_MUTE) != 0);
    } else
        Print(L"N/A\n");
}

VOID
EFIAPI
HdaCodecDumpPrintWidgets(
    IN HDA_WIDGET *Widgets,
    IN UINTN WidgetCount) {
    // Print each widget.
    for (UINTN w = 0; w < WidgetCount; w++) {
        // Determine name of widget.
        CHAR16 *WidgetNames[HDA_WIDGET_TYPE_VENDOR + 1] = {
            L"Audio Output", L"Audio Input", L"Audio Mixer",
            L"Audio Selector", L"Pin Complex", L"Power Widget",
            L"Volume Knob Widget", L"Beep Generator Widget",
            L"Reserved", L"Reserved", L"Reserved", L"Reserved",
            L"Reserved", L"Reserved", L"Reserved",
            L"Vendor Defined Widget"
        };

        // Print header and capabilities.
        Print(L"Node 0x%2X [%s] wcaps 0x%X:", Widgets[w].NodeId,
            WidgetNames[HDA_PARAMETER_WIDGET_CAPS_TYPE(Widgets[w].Capabilities)], Widgets[w].Capabilities);
        if (Widgets[w].Capabilities & HDA_PARAMETER_WIDGET_CAPS_STEREO)
            Print(L" Stereo");
        else
            Print(L" Mono");
        if (Widgets[w].Capabilities & HDA_PARAMETER_WIDGET_CAPS_DIGITAL)
            Print(L" Digital");
        if (Widgets[w].Capabilities & HDA_PARAMETER_WIDGET_CAPS_IN_AMP)
            Print(L" Amp-In");
        if (Widgets[w].Capabilities & HDA_PARAMETER_WIDGET_CAPS_OUT_AMP)
            Print(L" Amp-Out");
        if (Widgets[w].Capabilities & HDA_PARAMETER_WIDGET_CAPS_L_R_SWAP)
            Print(L" R/L");
        Print(L"\n");

        // Print input amp info.
        if (Widgets[w].Capabilities & HDA_PARAMETER_WIDGET_CAPS_IN_AMP) {
            // Print caps.
            Print(L"  Amp-in caps: ");
            HdaCodecDumpPrintAmpCaps(Widgets[w].AmpInCapabilities);

            // Print default values.
            Print(L"  Amp-In vals:");
            for (UINT8 i = 0; i < HDA_PARAMETER_CONN_LIST_LENGTH_LEN(Widgets[w].ConnectionListLength); i++) {
                if (Widgets[w].Capabilities & HDA_PARAMETER_WIDGET_CAPS_STEREO)
                    Print(L" [0x%2X 0x%2X]", Widgets[w].AmpInLeftDefaultGainMute[i], Widgets[w].AmpInRightDefaultGainMute[i]);
                else
                    Print(L" [0x%2X]", Widgets[w].AmpInLeftDefaultGainMute[i]);
            }
            Print(L"\n");
        }

        // Print output amp info.
        if (Widgets[w].Capabilities & HDA_PARAMETER_WIDGET_CAPS_OUT_AMP) {
            // Print caps.
            Print(L"  Amp-Out caps: ");
            HdaCodecDumpPrintAmpCaps(Widgets[w].AmpOutCapabilities);

            // Print default values.
            Print(L"  Amp-Out vals:");
            if (Widgets[w].Capabilities & HDA_PARAMETER_WIDGET_CAPS_STEREO)
                Print(L" [0x%2X 0x%2X]\n", Widgets[w].AmpOutLeftDefaultGainMute, Widgets[w].AmpOutRightDefaultGainMute);
            else
                Print(L" [0x%2X]\n", Widgets[w].AmpOutLeftDefaultGainMute);
        }

        // Print pin complexe info.
        if (HDA_PARAMETER_WIDGET_CAPS_TYPE(Widgets[w].Capabilities) == HDA_WIDGET_TYPE_PIN_COMPLEX) {
            // Print pin capabilities.
            Print(L"  Pincap 0x%8X:", Widgets[w].PinCapabilities);
            if (Widgets[w].PinCapabilities & HDA_PARAMETER_PIN_CAPS_INPUT)
                Print(L" IN");
            if (Widgets[w].PinCapabilities & HDA_PARAMETER_PIN_CAPS_OUTPUT)
                Print(L" OUT");
            if (Widgets[w].PinCapabilities & HDA_PARAMETER_PIN_CAPS_HEADPHONE)
                Print(L" HP");
            if (Widgets[w].PinCapabilities & HDA_PARAMETER_PIN_CAPS_EAPD)
                Print(L" EAPD");
            if (Widgets[w].PinCapabilities & HDA_PARAMETER_PIN_CAPS_TRIGGER)
                Print(L" Trigger");
            if (Widgets[w].PinCapabilities & HDA_PARAMETER_PIN_CAPS_PRESENCE)
                Print(L" Detect");
            if (Widgets[w].PinCapabilities & HDA_PARAMETER_PIN_CAPS_HBR)
                Print(L" HBR");
            if (Widgets[w].PinCapabilities & HDA_PARAMETER_PIN_CAPS_HDMI)
                Print(L" HDMI");
            if (Widgets[w].PinCapabilities & HDA_PARAMETER_PIN_CAPS_DISPLAYPORT)
                Print(L" DP");
            Print(L"\n");

            // Print EAPD info.
            if (Widgets[w].PinCapabilities & HDA_PARAMETER_PIN_CAPS_EAPD) {
                Print(L"  EAPD 0x%X:", Widgets[w].DefaultEapd);
                if (Widgets[w].DefaultEapd & HDA_EAPD_BTL_ENABLE_BTL)
                    Print(L" BTL");
                if (Widgets[w].DefaultEapd & HDA_EAPD_BTL_ENABLE_EAPD)
                    Print(L" EAPD");
                if (Widgets[w].DefaultEapd & HDA_EAPD_BTL_ENABLE_L_R_SWAP)
                    Print(L" R/L");
                Print(L"\n");
            }

            // Create pin default names.
            CHAR16 *PortConnectivities[4] = { L"Jack", L"None", L"Fixed", L"Int Jack" };
            CHAR16 *DefaultDevices[HDA_CONFIG_DEFAULT_DEVICE_OTHER + 1] = {
                L"Line Out", L"Speaker", L"HP Out", L"CD", L"SPDIF Out",
                L"Digital Out", L"Modem Line", L"Modem Handset", L"Line In", L"Aux",
                L"Mic", L"Telephone", L"SPDIF In", L"Digital In", L"Reserved", L"Other" };
            CHAR16 *Surfaces[4] = { L"Ext", L"Int", L"Ext", L"Other" };
            CHAR16 *Locations[0xF + 1] = {
                L"N/A", L"Rear", L"Front", L"Left", L"Right", L"Top", L"Bottom", L"Special",
                L"Special", L"Special", L"Reserved", L"Reserved", L"Reserved", L"Reserved" };
            CHAR16 *ConnTypes[HDA_CONFIG_DEFAULT_CONN_OTHER + 1] = {
                L"Unknown", L"1/8", L"1/4", L"ATAPI", L"RCA", L"Optical", L"Digital",
                L"Analog", L"Multi", L"XLR", L"RJ11", L"Combo", L"Other", L"Other", L"Other", L"Other" };
            CHAR16 *Colors[HDA_CONFIG_DEFAULT_COLOR_OTHER + 1] = {
                L"Unknown", L"Black", L"Grey", L"Blue", L"Green", L"Red", L"Orange",
                L"Yellow", L"Purple", L"Pink", L"Reserved", L"Reserved", L"Reserved",
                L"Reserved", L"White", L"Other" };

            // Print pin default header.
            Print(L"  Pin Default 0x%8X: [%s] %s at %s %s\n", Widgets[w].DefaultConfiguration,
                PortConnectivities[HDA_VERB_GET_CONFIGURATION_DEFAULT_PORT_CONN(Widgets[w].DefaultConfiguration)],
                DefaultDevices[HDA_VERB_GET_CONFIGURATION_DEFAULT_DEVICE(Widgets[w].DefaultConfiguration)],
                Surfaces[HDA_VERB_GET_CONFIGURATION_DEFAULT_SURF(Widgets[w].DefaultConfiguration)],
                Locations[HDA_VERB_GET_CONFIGURATION_DEFAULT_LOC(Widgets[w].DefaultConfiguration)]);

            // Print connection type and color.
            Print(L"    Conn = %s, Color = %s\n",
                ConnTypes[HDA_VERB_GET_CONFIGURATION_DEFAULT_CONN_TYPE(Widgets[w].DefaultConfiguration)],
                Colors[HDA_VERB_GET_CONFIGURATION_DEFAULT_COLOR(Widgets[w].DefaultConfiguration)]);

            // Print default association and sequence.
            Print(L"    DefAssociation = 0x%X, Sequence = 0x%X\n",
                HDA_VERB_GET_CONFIGURATION_DEFAULT_ASSOCIATION(Widgets[w].DefaultConfiguration),
                HDA_VERB_GET_CONFIGURATION_DEFAULT_SEQUENCE(Widgets[w].DefaultConfiguration));

            // Print default pin control.
            Print(L"Pin-ctls: 0x%2X:", Widgets[w].DefaultPinControl);
            if (Widgets[w].DefaultPinControl & HDA_PIN_WIDGET_CONTROL_VREF_EN)
                Print(L" VREF");
            if (Widgets[w].DefaultPinControl & HDA_PIN_WIDGET_CONTROL_IN_EN)
                Print(L" IN");
            if (Widgets[w].DefaultPinControl & HDA_PIN_WIDGET_CONTROL_OUT_EN)
                Print(L" OUT");
            if (Widgets[w].DefaultPinControl & HDA_PIN_WIDGET_CONTROL_HP_EN)
                Print(L" HP");
            Print(L"\n");
        }

        // Print connections.
        if (Widgets[w].Capabilities & HDA_PARAMETER_WIDGET_CAPS_CONN_LIST) {
            Print(L"  Connection: %u\n    ", HDA_PARAMETER_CONN_LIST_LENGTH_LEN(Widgets[w].ConnectionListLength));
            for (UINT8 i = 0; i < HDA_PARAMETER_CONN_LIST_LENGTH_LEN(Widgets[w].ConnectionListLength); i++)
                Print(L" 0x%2X", Widgets[w].Connections[i]);
            Print(L"\n");
        }

    }
}

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

    // Print each codec found.
    for (UINTN i = 0; i < HdaCodecHandleCount; i++) {
        Status = gBS->OpenProtocol(HdaCodecHandles[i], &gEfiHdaCodecInfoProtocolGuid, (VOID**)&HdaCodecInfo, NULL, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        ASSERT_EFI_ERROR(Status);

        // Get name.
        CHAR16 *Name;
        Status = HdaCodecInfo->GetName(HdaCodecInfo, &Name);
        Print(L"Codec: %s\n", Name);

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

        if ((Rates != 0) || (Formats != 0)) {
            Print(L"Default PCM:\n");
            HdaCodecDumpPrintRatesFormats(Rates, Formats);
        } else
            Print(L"Default PCM: N/A\n");

        // Get default amp caps.
        UINT32 AmpInCaps, AmpOutCaps;
        Status = HdaCodecInfo->GetDefaultAmpCaps(HdaCodecInfo, &AmpInCaps, &AmpOutCaps);
        Print(L"Default Amp-In caps: ");
        HdaCodecDumpPrintAmpCaps(AmpInCaps);
        Print(L"Default Amp-Out caps: ");
        HdaCodecDumpPrintAmpCaps(AmpOutCaps);

        // Get widgets.
        HDA_WIDGET *Widgets;
        UINTN WidgetCount;
        Status = HdaCodecInfo->GetWidgets(HdaCodecInfo, &Widgets, &WidgetCount);
        HdaCodecDumpPrintWidgets(Widgets, WidgetCount);
        Status = HdaCodecInfo->FreeWidgetsBuffer(Widgets, WidgetCount);
    }
    return EFI_SUCCESS;
}
