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

#ifndef _EFI_HDA_CODEC_PROTOCOL_H_
#define _EFI_HDA_CODEC_PROTOCOL_H_

#include <Uefi.h>
#include <Protocol/DevicePath.h>

//
// HDA Codec protocol.
//

// HDA Codec protocol GUID.
#define EFI_HDA_CODEC_PROTOCOL_GUID { \
    0xA090D7F9, 0xB50A, 0x4EA1, { 0xBD, 0xE9, 0x1A, 0xA5, 0xE9, 0x81, 0x2F, 0x45 } \
}
extern EFI_GUID gEfiHdaCodecProtocolGuid;
typedef struct _EFI_HDA_CODEC_PROTOCOL EFI_HDA_CODEC_PROTOCOL;

/**                                                                 
  Retrieves this codec's address.
            
  @param  This                  A pointer to the HDA_CODEC_PROTOCOL instance.  
  @param  CodecAddress          The codec's address.
                                  
  @retval EFI_SUCCESS           The codec's address was returned.                                                       
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                              
                                     
**/
typedef
EFI_STATUS
(EFIAPI *EFI_HDA_CODEC_GET_ADDRESS)(
    IN EFI_HDA_CODEC_PROTOCOL *This,
    OUT UINT8 *CodecAddress);

// Send command function.
typedef
EFI_STATUS
(EFIAPI *EFI_HDA_CODEC_SEND_COMMAND)(
    IN EFI_HDA_CODEC_PROTOCOL *This,
    IN UINT8 Node,
    IN UINT32 Verb,
    OUT UINT32 *Response);

// HDA Codec protocol structure.
struct _EFI_HDA_CODEC_PROTOCOL {
    EFI_HDA_CODEC_GET_ADDRESS GetAddress;
    EFI_HDA_CODEC_SEND_COMMAND SendCommand;
};

//
// HDA Codec Device Path protocol.
//

// HDA Codec Device Path GUID.
#define EFI_HDA_CODEC_DEVICE_PATH_GUID { \
    0xA9003FEB, 0xD806, 0x41DB, { 0xA4, 0x91, 0x54, 0x05, 0xFE, 0xEF, 0x46, 0xC3 } \
}
EFI_GUID gEfiHdaCodecDevicePathGuid;

// HDA Codec Device Path structure.
typedef struct {
    // Vendor-specific device path fields.
    EFI_DEVICE_PATH_PROTOCOL Header;
    EFI_GUID Guid;

    // Codec address.
    UINT8 Address;
} EFI_HDA_CODEC_DEVICE_PATH;

// Template for HDA Codec Device Path protocol.
#define EFI_HDA_CODEC_DEVICE_PATH_TEMPLATE { \
    { \
        MESSAGING_DEVICE_PATH, \
        MSG_VENDOR_DP, \
        { \
            (UINT8)(sizeof(EFI_HDA_CODEC_DEVICE_PATH)), \
            (UINT8)((sizeof(EFI_HDA_CODEC_DEVICE_PATH)) >> 8) \
        } \
    }, \
    gEfiHdaCodecDevicePathGuid, \
    0 \
}

#endif
