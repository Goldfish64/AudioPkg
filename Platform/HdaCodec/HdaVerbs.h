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
// "Get" Verbs
//
#define HDA_VERB_GET_PARAMETER                  0xF00 // Get Parameter.

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
