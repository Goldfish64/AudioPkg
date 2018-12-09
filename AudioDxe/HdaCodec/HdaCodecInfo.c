/*
 * File: HdaCodecInfo.c
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

/**                                                                 
  Gets the codec's vendor and device ID.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] VendorId          The vendor and device ID of the codec.

  @retval EFI_SUCCESS           The vendor and device ID was retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                    
**/
EFI_STATUS
EFIAPI
HdaCodecInfoGetVendorId(
    IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
    OUT UINT32 *VendorId) {
    DEBUG((DEBUG_INFO, "HdaCodecInfoGetVendorId(): start\n"));
    
    // Create variables.
    HDA_CODEC_INFO_PRIVATE_DATA *HdaPrivateData;

    // If parameters are null, fail.
    if ((This == NULL) || (VendorId == NULL))
        return EFI_INVALID_PARAMETER;

    // Get private data and fill vendor ID parameter.
    HdaPrivateData = HDA_CODEC_INFO_PRIVATE_DATA_FROM_THIS(This);
    *VendorId = HdaPrivateData->HdaCodecDev->VendorId;
    return EFI_SUCCESS;
}

/**                                                                 
  Gets the codec's revision ID.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] RevisionId        The revision ID of the codec.

  @retval EFI_SUCCESS           The revision ID was retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                    
**/
EFI_STATUS
EFIAPI
HdaCodecInfoGetRevisionId(
    IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
    OUT UINT32 *RevisionId) {
    DEBUG((DEBUG_INFO, "HdaCodecInfoGetRevisionId(): start\n"));
    
    // Create variables.
    HDA_CODEC_INFO_PRIVATE_DATA *HdaPrivateData;

    // If parameters are null, fail.
    if ((This == NULL) || (RevisionId == NULL))
        return EFI_INVALID_PARAMETER;

    // Get private data and fill revision ID parameter.
    HdaPrivateData = HDA_CODEC_INFO_PRIVATE_DATA_FROM_THIS(This);
    *RevisionId = HdaPrivateData->HdaCodecDev->RevisionId;
    return EFI_SUCCESS;
}

/**                                                                 
  Gets the node ID of the codec's audio function.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] AudioFuncId       The node ID of the codec's audio function.

  @retval EFI_SUCCESS           The node ID was retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                    
**/
EFI_STATUS
EFIAPI
HdaCodecInfoGetAudioFuncId(
    IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
    OUT UINT8 *AudioFuncId,
    OUT BOOLEAN *UnsolCapable) {
    DEBUG((DEBUG_INFO, "HdaCodecInfoGetAudioFuncId(): start\n"));
    
    // Create variables.
    HDA_CODEC_INFO_PRIVATE_DATA *HdaPrivateData;

    // If parameters are null, fail.
    if ((This == NULL) || (AudioFuncId == NULL) || (UnsolCapable == NULL))
        return EFI_INVALID_PARAMETER;

    // Get private data and fill node ID parameter.
    HdaPrivateData = HDA_CODEC_INFO_PRIVATE_DATA_FROM_THIS(This);
    *AudioFuncId = HdaPrivateData->HdaCodecDev->AudioFuncGroup->NodeId;
    *UnsolCapable = HdaPrivateData->HdaCodecDev->AudioFuncGroup->UnsolCapable;
    return EFI_SUCCESS;
}

/**                                                                 
  Gets the codec's default supported stream rates and formats.

  @param[in]  This              A pointer to the EFI_HDA_CODEC_INFO_PROTOCOL instance.
  @param[out] Rates             The default supported rates.
  @param[out] Formats           The default supported formats.

  @retval EFI_SUCCESS           The stream rates and formats were retrieved.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.                    
**/
EFI_STATUS
EFIAPI
HdaCodecInfoGetDefaultRatesFormats(
    IN  EFI_HDA_CODEC_INFO_PROTOCOL *This,
    OUT UINT32 *Rates,
    OUT UINT32 *Formats) {
    DEBUG((DEBUG_INFO, "HdaCodecInfoGetDefaultRatesFormats(): start\n"));
    
    // Create variables.
    HDA_CODEC_INFO_PRIVATE_DATA *HdaPrivateData;

    // If parameters are null, fail.
    if ((This == NULL) || (Rates == NULL) || (Formats == NULL))
        return EFI_INVALID_PARAMETER;

    // Get private data and fill rates and formats parameters.
    HdaPrivateData = HDA_CODEC_INFO_PRIVATE_DATA_FROM_THIS(This);
    *Rates = HdaPrivateData->HdaCodecDev->AudioFuncGroup->SupportedPcmRates;
    *Formats = HdaPrivateData->HdaCodecDev->AudioFuncGroup->SupportedFormats;
    return EFI_SUCCESS;
}
