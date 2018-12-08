/*
 * File: HdaCodecInfo.h
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

#ifndef _EFI_HDA_CODEC_INFO_H_
#define _EFI_HDA_CODEC_INFO_H_

#include <Uefi.h>

// HDA Codec Info protocol GUID.
#define EFI_HDA_CODEC_INFO_PROTOCOL_GUID { \
    0x6C9CDDE1, 0xE8A5, 0x43E5, { 0xBE, 0x88, 0xDA, 0x15, 0xBC, 0x1C, 0x02, 0x50 } \
}
extern EFI_GUID gEfiHdaCodecInfoProtocolGuid;
typedef struct _EFI_HDA_CODEC_INFO_PROTOCOL EFI_HDA_CODEC_INFO_PROTOCOL;

struct _EFI_HDA_CODEC_INFO_PROTOCOL {

};

#endif
