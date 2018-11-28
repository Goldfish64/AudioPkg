/*
 * File: HdaVerbs.h
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

#ifndef _EFI_HDA_VERBS_H_
#define _EFI_HDA_VERBS_H_

// Macros for building verbs.
#define HDA_CODEC_VERB_4BIT(Verb, Payload)  ((((UINT32)Verb) << 16) | (Payload & 0xFFFF))
#define HDA_CODEC_VERB_12BIT(Verb, Payload) ((((UINT32)Verb) << 8) | (Payload & 0xFF))

//
// "Get" Verbs.
//
#define HDA_VERB_GET_CONVERTER_FORMAT           0xA   // Get Converter Format.
#define HDA_VERB_GET_AMP_GAIN_MUTE              0xB   // Get Amplifier Gain/Mute.
#define HDA_VERB_GET_PROCESSING_COEFFICIENT     0xC   // Get Processing Coefficient.
#define HDA_VERB_GET_COEFFICIENT_INDEX          0xD   // Get Coefficient Index.

#define HDA_VERB_GET_PARAMETER                  0xF00 // Get Parameter.
#define HDA_VERB_GET_CONN_SELECT_CONTROL        0xF01 // Get Connection Select Control.
#define HDA_VERB_GET_CONN_LIST_ENTRY            0xF02 // Get Connection List Entry.
#define HDA_VERB_GET_PROCESSING_STATE           0xF03 // Get Processing State.
#define HDA_VERB_GET_INPUT_CONV_SDI_SELECT      0xF04 // Get Input Converter SDI Select.
#define HDA_VERB_GET_POWER_STATE                0xF05 // Get Power State.
#define HDA_VERB_GET_CONVERTER_STREAM_CHANNEL   0xF06 // Get Converter Stream, Channel.
#define HDA_VERB_GET_PIN_WIDGET_CONTROL         0xF07 // Get Pin Widget Control.
#define HDA_VERB_GET_UNSOL_RESPONSE             0xF08 // Get Unsolicited Response.
#define HDA_VERB_GET_PIN_SENSE                  0xF09 // Get Pin Sense.
#define HDA_VERB_GET_BEEP_GENERATION            0xF0A // Get Beep Generation.
#define HDA_VERB_GET_EAPD_BTL_ENABLE            0xF0C // Get EAPD/BTL Enable.
#define HDA_VERB_GET_DIGITAL_CONV_CONTROL       0xF0D // Get Digital Converter Control.
#define HDA_VERB_GET_VOLUME_KNOB                0xF0F // Get Volume Knob.

#define HDA_VERB_GET_GPI_DATA                   0xF10 // Get GPI Data.
#define HDA_VERB_GET_GPI_WAKE_ENABLE_MASK       0xF11 // Get GPI Wake Enable Mask.
#define HDA_VERB_GET_GPI_UNSOL_ENABLE_MASK      0xF12 // Get GPI Unsolicited Enable Mask.
#define HDA_VERB_GET_GPI_STICK_MASK             0xF13 // Get GPI Sticky Mask.
#define HDA_VERB_GET_GPO_DATA                   0xF14 // Get GPO Data.
#define HDA_VERB_GET_GPIO_DATA                  0xF15 // Get GPIO Data.
#define HDA_VERB_GET_GPIO_ENABLE_MASK           0xF16 // Get GPIO Enable Mask.
#define HDA_VERB_GET_GPIO_DIRECTION             0xF17 // Get GPIO Direction.
#define HDA_VERB_GET_GPIO_WAKE_ENABLE_MASK      0xF18 // Get GPIO Wake Enable Mask.
#define HDA_VERB_GET_GPIO_UNSOL_ENABLE_MASK     0xF19 // Get GPIO Unsolicited Enable Mask.
#define HDA_VERB_GET_GPIO_STICKY_MASK           0xF1A // Get GPIO Sticky Mask.
#define HDA_VERB_GET_CONFIGURATION_DEFAULT      0xF1C // Get Configuration Default.

#define HDA_VERB_GET_IMPLEMENTATION_ID          0xF20 // Get Implementation Identification.
#define HDA_VERB_GET_STRIPE_CONTROL             0xF24 // Get Stripe Control.
#define HDA_VERB_GET_CONVERTER_CHANNEL_COUNT    0xF2D // Get Converter Channel Count.
#define HDA_VERB_GET_DATA_ISLAND_PACKET_SIZE    0xF2E // Get Data Island Packet - Size Info.
#define HDA_VERB_GET_EDID_LIKE_DATA             0xF2F // Get EDID-Like Data (ELD).
#define HDA_VERB_GET_DATA_ISLAND_PACKET_INDEX   0xF30 // Get Data Island Packet - Index.
#define HDA_VERB_GET_DATA_ISLAND_PACKET_DATA    0xF31 // Get Data Island Packet – Data.
#define HDA_VERB_GET_DATA_ISLAND_PACKET_XMIT    0xF32 // Get Data Island Packet – Transmit Control.
#define HDA_VERB_GET_CP_CONTROL                 0xF33 // Get Content Protection Control (CP_CONTROL).
#define HDA_VERB_GET_ASP_MAPPING                0xF34 // Get ASP Channel Mapping.

//
// "Set" Verbs.
//
#define HDA_VERB_SET_CONVERTER_FORMAT           0x2   // Set Converter Format.
#define HDA_VERB_SET_AMP_GAIN_MUTE              0x3   // Set Amplifier Gain/Mute.
#define HDA_VERB_SET_PROCESSING_COEFFICIENT     0x4   // Set Processing Coefficient.
#define HDA_VERB_SET_COEFFICIENT_INDEX          0x5   // Set Coefficient Index.

#define HDA_VERB_SET_CONN_SELECT_CONTROL        0x701 // Set Connection Select Control.
#define HDA_VERB_SET_PROCESSING_STATE           0x703 // Set Processing State.
#define HDA_VERB_SET_INPUT_CONV_SDI_SELECT      0x704 // Set Input Converter SDI Select.
#define HDA_VERB_SET_POWER_STATE                0x705 // Set Power State.
#define HDA_VERB_SET_CONVERTER_STREAM_CHANNEL   0x706 // Set Converter Stream, Channel.
#define HDA_VERB_SET_PIN_WIDSET_CONTROL         0x707 // Set Pin Widget Control.
#define HDA_VERB_SET_UNSOL_RESPONSE             0x708 // Set Unsolicited Response.
#define HDA_VERB_SET_PIN_SENSE                  0x709 // Set Pin Sense.
#define HDA_VERB_SET_BEEP_GENERATION            0x70A // Set Beep Generation.
#define HDA_VERB_SET_EAPD_BTL_ENABLE            0x70C // Set EAPD/BTL Enable.
#define HDA_VERB_SET_DIGITAL_CONV_CONTROL1      0x70D // Set Digital Converter Control 1.
#define HDA_VERB_SET_DIGITAL_CONV_CONTROL2      0x70E // Set Digital Converter Control 2.
#define HDA_VERB_SET_VOLUME_KNOB                0x70F // Set Volume Knob.

#define HDA_VERB_SET_GPI_DATA                   0x710 // Set GPI Data.
#define HDA_VERB_SET_GPI_WAKE_ENABLE_MASK       0x711 // Set GPI Wake Enable Mask.
#define HDA_VERB_SET_GPI_UNSOL_ENABLE_MASK      0x712 // Set GPI Unsolicited Enable Mask.
#define HDA_VERB_SET_GPI_STICK_MASK             0x713 // Set GPI Sticky Mask.
#define HDA_VERB_SET_GPO_DATA                   0x714 // Set GPO Data.
#define HDA_VERB_SET_GPIO_DATA                  0x715 // Set GPIO Data.
#define HDA_VERB_SET_GPIO_ENABLE_MASK           0x716 // Set GPIO Enable Mask.
#define HDA_VERB_SET_GPIO_DIRECTION             0x717 // Set GPIO Direction.
#define HDA_VERB_SET_GPIO_WAKE_ENABLE_MASK      0x718 // Set GPIO Wake Enable Mask.
#define HDA_VERB_SET_GPIO_UNSOL_ENABLE_MASK     0x719 // Set GPIO Unsolicited Enable Mask.
#define HDA_VERB_SET_GPIO_STICKY_MASK           0x71A // Set GPIO Sticky Mask.
#define HDA_VERB_SET_CONFIGURATION_DEFAULT0     0x71C // Set Configuration Default Byte 0.
#define HDA_VERB_SET_CONFIGURATION_DEFAULT1     0x71D // Set Configuration Default Byte 1.
#define HDA_VERB_SET_CONFIGURATION_DEFAULT2     0x71E // Set Configuration Default Byte 2.
#define HDA_VERB_SET_CONFIGURATION_DEFAULT3     0x71F // Set Configuration Default Byte 3.

#define HDA_VERB_SET_IMPLEMENTATION_ID          0x720 // Set Implementation Identification.
#define HDA_VERB_SET_STRIPE_CONTROL             0x724 // Set Stripe Control.
#define HDA_VERB_SET_CONVERTER_CHANNEL_COUNT    0x72D // Set Converter Channel Count.
#define HDA_VERB_SET_DATA_ISLAND_PACKET_INDEX   0x730 // Set Data Island Packet - Index.
#define HDA_VERB_SET_DATA_ISLAND_PACKET_DATA    0x731 // Set Data Island Packet – Data.
#define HDA_VERB_SET_DATA_ISLAND_PACKET_XMIT    0x732 // Set Data Island Packet – Transmit Control.
#define HDA_VERB_SET_CP_CONTROL                 0x733 // Set Content Protection Control (CP_CONTROL).
#define HDA_VERB_SET_ASP_MAPPING                0x734 // Set ASP Channel Mapping.
#define HDA_VERB_SET_DIGITAL_CONV_CONTROL3      0x73E // Set Digital Converter Control 3.
#define HDA_VERB_SET_DIGITAL_CONV_CONTROL4      0x73F // Set Digital Converter Control 4.

#define HDA_VERB_FUNC_RESET                     0x7FF // Function Reset.

//
// Parameters for HDA_VERB_GET_PARAMETER.
//
#define HDA_PARAMETER_VENDOR_ID                 0x00 // Vendor ID.
#define HDA_PARAMETER_REVISION_ID               0x02 // Revision ID.
#define HDA_PARAMETER_SUBNODE_COUNT             0x04 // Subordinate Node Count.
#define HDA_PARAMETER_FUNC_GROUP_TYPE           0x05 // Function Group Type.
#define HDA_PARAMETER_FUNC_GROUP_CAPS           0x08 // Audio Function Group Capabilities.
#define HDA_PARAMETER_WIDGET_CAPS               0x09 // Audio Widget Capabilities.
#define HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES  0x0A // Sample Size, Rate CAPs.
#define HDA_PARAMETER_SUPPORTED_STREAM_FORMATS  0x0B // Stream Formats.
#define HDA_PARAMETER_PIN_CAPS                  0x0C // Pin Capabilities.
#define HDA_PARAMETER_AMP_CAPS_INPUT            0x0D // Input Amp Capabilities.
#define HDA_PARAMETER_CONN_LIST_LENGTH          0x0E // Connection List Length.
#define HDA_PARAMETER_SUPPORTED_POWER_STATES    0x0F // Supported Power States.

#define HDA_PARAMETER_PROCESSING_CAPS           0x10 // Processing Capabilities.
#define HDA_PARAMETER_GPIO_COUNT                0x11 // GPIO Count.
#define HDA_PARAMETER_AMP_CAPS_OUTPUT           0x12 // Output Amp Capabilities.
#define HDA_PARAMETER_VOLUME_KNOB_CAPS          0x13 // Volume Knob Capabilities.
#define HDA_PARAMETER_HDMI_LPCM_CAD             0x20 // HDMI LPCM CAD (Obsolete).

#endif
