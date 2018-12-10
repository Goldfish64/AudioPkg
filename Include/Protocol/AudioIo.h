/*
 * File: AudioIo.h
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

#ifndef _EFI_AUDIO_IO_H_
#define _EFI_AUDIO_IO_H_

#include <Uefi.h>

// Audio I/O protocol GUID.
#define EFI_AUDIO_IO_PROTOCOL_GUID { \
    0xF05B559C, 0x1971, 0x4AF5, { 0xB2, 0xAE, 0xD6, 0x08, 0x08, 0xF7, 0x4F, 0x70 } \
}
extern EFI_GUID gEfiAudioIoProtocolGuid;
typedef struct _EFI_AUDIO_IO_PROTOCOL EFI_AUDIO_IO_PROTOCOL;

/**                                                                 
  Plays the specified audio data.

  @param[in] This               A pointer to the EFI_AUDIO_IO_PROTOCOL instance.
  @param[in] OutputIndex        The desired audio output index.
  @param[in] Data               A pointer to the buffer containing the audio data to play.
  @param[in] DataLength         The size, in bytes, of the data buffer specified by Data.

  @retval EFI_SUCCESS           The audio data was played successfully.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                    
**/
typedef
EFI_STATUS
(EFIAPI *EFI_AUDIO_IO_PLAY)(
    IN  EFI_AUDIO_IO_PROTOCOL *This,
    IN  UINT32 OutputIndex,
    IN  VOID *Data,
    IN  UINTN DataLength);

// Protocol struct.
struct _EFI_AUDIO_IO_PROTOCOL {
    EFI_AUDIO_IO_PLAY       AudioPlay;
};

#endif
