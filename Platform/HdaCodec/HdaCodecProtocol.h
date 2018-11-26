/*
 * File: HdaCodecProtocol.h
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

#ifndef _HDA_CODEC_PROTOCOL_H_
#define _HDA_CODEC_PROTOCOL_H_

#include <Uefi.h>
#include <Protocol/DevicePath.h>

#define HDA_CODEC_PROTOCOL_GUID \
    { \
        0xA090D7F9, 0xB50A, 0x4EA1, { 0xBD, 0xE9, 0x1A, 0xA5, 0xE9, 0x81, 0x2F, 0x45 } \
    }

typedef struct _HDA_CODEC_PROTOCOL HDA_CODEC_PROTOCOL;

struct _HDA_CODEC_PROTOCOL {
    UINT8 Address;
};
extern EFI_GUID gHdaCodecProtocolGuid;

#define HDA_CODEC_DEVICE_PATH_GUID \
    { \
        0xA9003FEB, 0xD806, 0x41DB, { 0xA4, 0x91, 0x54, 0x05, 0xFE, 0xEF, 0x46, 0xC3 } \
    }

typedef struct {
    // Vendor-specific device path fields.
    EFI_DEVICE_PATH_PROTOCOL Header;
    EFI_GUID Guid;

    // Codec address.
    UINT8 Address;
} HDA_CODEC_DEVICE_PATH;

EFI_GUID gHdaCodecDevicePathGuid;

#define gHdaCodecDevicePath \
    { \
        { \
            MESSAGING_DEVICE_PATH, \
            MSG_VENDOR_DP, \
            { \
                (UINT8)(sizeof(HDA_CODEC_DEVICE_PATH)), \
                (UINT8)((sizeof(HDA_CODEC_DEVICE_PATH)) >> 8) \
            } \
        }, \
        gHdaCodecDevicePathGuid, \
        0 \
    }

#endif
