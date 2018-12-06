/*
 * File: UsbAudioDesc.c
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

#ifndef _EFI_USB_AUDIO_DESC_H_
#define _EFI_USB_AUDIO_DESC_H_

#include "AudioDxe.h"

// USB interface class codes.
#define USB_AUDIO_CLASS                 0x1
#define USB_AUDIO_SUBCLASS_CONTROL      0x1
#define USB_AUDIO_SUBCLASS_STREAMING    0x2
#define USB_AUDIO_SUBCLASS_MIDI         0x3
#define USB_AUDIO_PROTOCOL_VER_01_00    0x00
#define USB_AUDIO_PROTOCOL_VER_02_00    0x20
#define USB_AUDIO_PROTOCOL_VER_03_00    0x30

// USB audio function class codes.
#define USB_AUDIO_FUNC_CLASS                        USB_AUDIO_CLASS
#define USB_AUDIO_FUNC_SUBCLASS_UNDEFINED           0x00
#define USB_AUDIO_FUNC_SUBCLASS_FULL_ADC_3_0        0x01
#define USB_AUDIO_FUNC_SUBCLASS_GENERIC_IO          0x20
#define USB_AUDIO_FUNC_SUBCLASS_HEADPHONE           0x21
#define USB_AUDIO_FUNC_SUBCLASS_SPEAKER             0x22
#define USB_AUDIO_FUNC_SUBCLASS_MICROPHONE          0x23
#define USB_AUDIO_FUNC_SUBCLASS_HEADSET             0x24
#define USB_AUDIO_FUNC_SUBCLASS_HEADSET_ADAPTER     0x25
#define USB_AUDIO_FUNC_SUBCLASS_SPEAKERPHONE        0x26
#define USB_AUDIO_FUNC_PROTOCOL_VER_01_00           USB_AUDIO_PROTOCOL_VER_01_00
#define USB_AUDIO_FUNC_PROTOCOL_VER_02_00           USB_AUDIO_PROTOCOL_VER_02_00
#define USB_AUDIO_FUNC_PROTOCOL_VER_03_00           USB_AUDIO_PROTOCOL_VER_03_00

// USB audio function category codes.
#define USB_AUDIO_CATEGORY_UNDEFINED                0x00
#define USB_AUDIO_CATEGORY_DESKTOP_SPEAKER          0x01
#define USB_AUDIO_CATEGORY_HOME_THEATER             0x02
#define USB_AUDIO_CATEGORY_MICROPHONE               0x03
#define USB_AUDIO_CATEGORY_HEADSET                  0x04
#define USB_AUDIO_CATEGORY_TELEPHONE                0x05
#define USB_AUDIO_CATEGORY_CONVERTER                0x06
#define USB_AUDIO_CATEGORY_VOICE_SOUND_RECORDER     0x07
#define USB_AUDIO_CATEGORY_IO_BOX                   0x08
#define USB_AUDIO_CATEGORY_MUSICAL_INSTRUMENT       0x09
#define USB_AUDIO_CATEGORY_PRO_AUDIO                0x0A
#define USB_AUDIO_CATEGORY_AUDIO_VIDEO              0x0B
#define USB_AUDIO_CATEGORY_CONTROL_PANEL            0x0C
#define USB_AUDIO_CATEGORY_HEADPHONE                0x0D
#define USB_AUDIO_CATEGORY_GENERIC_SPEAKER          0x0E
#define USB_AUDIO_CATEGORY_HEADSET_ADAPTER          0x0F
#define USB_AUDIO_CATEGORY_SPEAKERPHONE             0x10
#define USB_AUDIO_CATEGORY_OTHER                    0xFF

// USB audio channel purpose codes.
#define USB_AUDIO_PURPOSE_UNDEFINED                 0x00
#define USB_AUDIO_PURPOSE_GENERIC_AUDIO             0x01
#define USB_AUDIO_PURPOSE_VOICE                     0x02
#define USB_AUDIO_PURPOSE_SPEECH                    0x03
#define USB_AUDIO_PURPOSE_AMBIENT                   0x04
#define USB_AUDIO_PURPOSE_REFERENCE                 0x05
#define USB_AUDIO_PURPOSE_ULTRASONIC                0x06
#define USB_AUDIO_PURPOSE_VIBROKINETIC              0x07
#define USB_AUDIO_PURPOSE_NON_AUDIO                 0xFF

#define USB_AUDIO_CLASS_DESCRIPTOR_INTERFACE        0x24

// Descriptors need to be packed.
#pragma pack(1)

// Class-Specific AC Interface Header Descriptor header.
typedef struct {
    UINT8   Length;
    UINT8   DescriptorType;
    UINT8   DescriptorSubtype;
    UINT16  BcdADC;
} USB_AUDIO_CLASS_INTERFACE_DESCRIPTOR_HEADER;

// Class-Specific AC Interface Header Descriptor v1.
typedef struct {
    USB_AUDIO_CLASS_INTERFACE_DESCRIPTOR_HEADER Header;
    UINT16  TotalLength;
    UINT8   InterfaceCollection;
    UINT8   InterfaceNum;
} USB_AUDIO_CLASS_INTERFACE_DESCRIPTOR_V1;

// Class-Specific AC Interface Header Descriptor v2.
typedef struct {
    USB_AUDIO_CLASS_INTERFACE_DESCRIPTOR_HEADER Header;
    UINT8   Category;
    UINT16  TotalLength;
    UINT8   BmControls;
} USB_AUDIO_CLASS_INTERFACE_DESCRIPTOR_V2;

// Clock Source Descriptor.
typedef struct {
    UINT8   Length;
    UINT8   DescriptorType;
    UINT8   DescriptorSubtype;
    UINT8   ClockId;
    UINT8   BmAttributes;
    UINT8   BmControls;
    UINT8   AssocTerminal;
    UINT8   ClockSourceIndex;
} USB_AUDIO_CLOCK_SOURCE_INTERFACE_DESCRIPTOR;

// Clock Multiplier Descriptor.
typedef struct {
    UINT8   Length;
    UINT8   DescriptorType;
    UINT8   DescriptorSubtype;
    UINT8   ClockId;
    UINT8   ClockSourceId;
    UINT8   BmControls;
    UINT8   ClockMultiplierIndex;
} USB_AUDIO_CLOCK_MULTIPLIER_INTERFACE_DESCRIPTOR;

// Input Terminal Descriptor.
typedef struct {
    UINT8   Length;
    UINT8   DescriptorType;
    UINT8   DescriptorSubtype;
    UINT8   TerminalId;
    UINT16  TerminalType;
    UINT8   AssocTerminal;
    UINT8   ClockSourceId;
    UINT8   NumChannels;
    UINT32  BmChannelConfig;
    UINT8   ChannelNamesStrIndex;
    UINT16  BmControls;
    UINT8   StrIndex;
} USB_AUDIO_INPUT_TERMINAL_INTERFACE_DESCRIPTOR;

// Output Terminal Descriptor.
typedef struct {
    UINT8   Length;
    UINT8   DescriptorType;
    UINT8   DescriptorSubtype;
    UINT8   TerminalId;
    UINT16  TerminalType;
    UINT8   AssocTerminal;
    UINT8   SourceId;
    UINT8   ClockSourceId;
    UINT16  BmControls;
    UINT8   StrIndex;
} USB_AUDIO_OUTPUT_TERMINAL_INTERFACE_DESCRIPTOR;

// Sampling Rate Converter Descriptor.
typedef struct {
    UINT8   Length;
    UINT8   DescriptorType;
    UINT8   DescriptorSubtype;
    UINT8   UnitId;
    UINT8   SourceId;
    UINT8   ClockSourceInId;
    UINT8   ClockSourceOutId;
    UINT8   StrIndex;
} USB_AUDIO_SAMPLING_RATE_CONVERTER_INTERFACE_DESCRIPTOR;

#pragma pack()

#endif
