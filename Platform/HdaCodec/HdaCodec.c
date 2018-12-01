/*
 * File: HdaCodec.c
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
#include "HdaCodecProtocol.h"
#include "HdaCodecComponentName.h"
#include "HdaVerbs.h"

EFI_STATUS
EFIAPI
HdaCodecGetDefaultData(
    HDA_CODEC_DEV *HdaCodecDev) {

    EFI_HDA_CODEC_PROTOCOL *HdaCodec = HdaCodecDev->HdaCodecProto;
    EFI_STATUS Status;

    // Get function group count.
    HDA_SUBNODE_COUNT FuncGroupCount;
    Status = HdaCodec->SendCommand(HdaCodec, HDA_NID_ROOT, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUBNODE_COUNT), (UINT32*)&FuncGroupCount);
    ASSERT_EFI_ERROR(Status);

    DEBUG((DEBUG_INFO, "%u function groups. Starting node: 0x%X\n", FuncGroupCount.NodeCount, FuncGroupCount.StartNode));
    
    // Allocate function groups array.
    HdaCodecDev->FuncGroupsLength = FuncGroupCount.NodeCount;
    HdaCodecDev->FuncGroups = AllocateZeroPool(sizeof(HDA_FUNC_GROUP*) * HdaCodecDev->FuncGroupsLength);

    // Go through function groups.
    for (UINT8 funcIndex = 0; funcIndex < HdaCodecDev->FuncGroupsLength; funcIndex++) {
        UINT8 fNid = FuncGroupCount.StartNode + funcIndex;
        DEBUG((DEBUG_INFO, "Probing function group 0x%X\n", fNid));
        UINT32 Tmp;
        Status = HdaCodec->SendCommand(HdaCodec, fNid, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_POWER_STATE, 0), &Tmp);
        ASSERT_EFI_ERROR(Status);

        // Get type.
        HDA_FUNC_GROUP_TYPE fType;
        Status = HdaCodec->SendCommand(HdaCodec, fNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_FUNC_GROUP_TYPE), (UINT32*)&fType);
        ASSERT_EFI_ERROR(Status);

        // Get caps.
        UINT32 fCaps;
        Status = HdaCodec->SendCommand(HdaCodec, fNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_FUNC_GROUP_CAPS), &fCaps);
        ASSERT_EFI_ERROR(Status);

        // Get node count.
        HDA_SUBNODE_COUNT fNodes;
        Status = HdaCodec->SendCommand(HdaCodec, fNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUBNODE_COUNT), (UINT32*)&fNodes);
        ASSERT_EFI_ERROR(Status);

        DEBUG((DEBUG_INFO, "Function Type: 0x%X Caps: 0x%X Nodes: %u (0x%X)\n", fType.NodeType, fCaps, fNodes.NodeCount, fNodes.StartNode));

        // Allocate function group.
        HdaCodecDev->FuncGroups[funcIndex] = AllocateZeroPool(sizeof(HDA_FUNC_GROUP));
        HdaCodecDev->FuncGroups[funcIndex]->NodeId = fNid;
        HdaCodecDev->FuncGroups[funcIndex]->Type = fType;
        HdaCodecDev->FuncGroups[funcIndex]->WidgetsLength = fNodes.NodeCount;
        HdaCodecDev->FuncGroups[funcIndex]->Widgets = AllocateZeroPool(sizeof(HDA_WIDGET*) * HdaCodecDev->FuncGroups[funcIndex]->WidgetsLength);
        if (HdaCodecDev->FuncGroups[funcIndex]->Type.NodeType == HDA_FUNC_GROUP_TYPE_AUDIO)
            HdaCodecDev->AudioFuncGroup = funcIndex;

        // Go through each widget.
        for (UINT8 wIndex = 0; wIndex < HdaCodecDev->FuncGroups[funcIndex]->WidgetsLength; wIndex++) {
            UINT8 wNid = wIndex + fNodes.StartNode;
            Status = HdaCodec->SendCommand(HdaCodec, wNid, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_POWER_STATE, 0), &Tmp);
        ASSERT_EFI_ERROR(Status);

            // Get caps.
            HDA_WIDGET_CAPS wCaps;
            Status = HdaCodec->SendCommand(HdaCodec, wNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_WIDGET_CAPS), (UINT32*)&wCaps);
            ASSERT_EFI_ERROR(Status);

            // Determine type.
            UINTN HdaWidgetSize;
            switch (wCaps.Type) {
                case HDA_WIDGET_TYPE_OUTPUT:
                    HdaWidgetSize = sizeof(HDA_WIDGET_INPUT_OUTPUT);
                    break;

                case HDA_WIDGET_TYPE_INPUT:
                    HdaWidgetSize = sizeof(HDA_WIDGET_INPUT_OUTPUT);
                    break;

                case HDA_WIDGET_TYPE_MIXER:
                    HdaWidgetSize = sizeof(HDA_WIDGET_MIXER);
                    break;

                case HDA_WIDGET_TYPE_SELECTOR:
                    HdaWidgetSize = sizeof(HDA_WIDGET_SELECTOR);
                    break;

                case HDA_WIDGET_TYPE_PIN_COMPLEX:
                    HdaWidgetSize = sizeof(HDA_WIDGET_PIN_COMPLEX);
                    break;

                case HDA_WIDGET_TYPE_POWER:
                    HdaWidgetSize = sizeof(HDA_WIDGET_POWER);
                    break;

                case HDA_WIDGET_TYPE_VOLUME_KNOB:
                    HdaWidgetSize = sizeof(HDA_WIDGET_VOLUME_KNOB);
                    break;

                case HDA_WIDGET_TYPE_BEEP_GEN:
                    HdaWidgetSize = sizeof(HDA_WIDGET_BEEP_GEN);
                    break;

                default:
                    HdaWidgetSize = sizeof(HDA_WIDGET);
            }

            // Allocate widget.
            HdaCodecDev->FuncGroups[funcIndex]->Widgets[wIndex] = AllocateZeroPool(HdaWidgetSize);
            HDA_WIDGET *HdaWidget = HdaCodecDev->FuncGroups[funcIndex]->Widgets[wIndex];
            HdaWidget->NodeId = wNid;
            HdaWidget->Capabilities = wCaps;
            HdaWidget->Length = HdaWidgetSize;

            // Get PCM sizes/rates and stream format.
            if (HdaWidget->Capabilities.Type == HDA_WIDGET_TYPE_OUTPUT
                || HdaWidget->Capabilities.Type == HDA_WIDGET_TYPE_INPUT) {
                HDA_WIDGET_INPUT_OUTPUT *wInOutWidget = (HDA_WIDGET_INPUT_OUTPUT*)HdaWidget;

                Status = HdaCodec->SendCommand(HdaCodec, wNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES), (UINT32*)&wInOutWidget->SizesRates);
                ASSERT_EFI_ERROR(Status);
                Status = HdaCodec->SendCommand(HdaCodec, wNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUPPORTED_STREAM_FORMATS), (UINT32*)&wInOutWidget->StreamFormats);
                ASSERT_EFI_ERROR(Status);

                // Get Amp caps.
                if (HdaWidget->Capabilities.Type == HDA_WIDGET_TYPE_OUTPUT)
                    Status = HdaCodec->SendCommand(HdaCodec, wNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_AMP_CAPS_OUTPUT), (UINT32*)&wInOutWidget->AmpCapabilities);
                else
                    Status = HdaCodec->SendCommand(HdaCodec, wNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_AMP_CAPS_INPUT), (UINT32*)&wInOutWidget->AmpCapabilities);
                ASSERT_EFI_ERROR(Status);
            } else if (HdaWidget->Capabilities.Type == HDA_WIDGET_TYPE_PIN_COMPLEX) {
                HDA_WIDGET_PIN_COMPLEX *wPinWidget = (HDA_WIDGET_PIN_COMPLEX*)HdaWidget;

                // Get Amp caps.
                if (HdaWidget->Capabilities.InAmpPresent) {
                    Status = HdaCodec->SendCommand(HdaCodec, wNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_AMP_CAPS_INPUT), (UINT32*)&wPinWidget->InAmpCapabilities);
                    ASSERT_EFI_ERROR(Status);
                }
                if (HdaWidget->Capabilities.OutAmpPresent) {
                    Status = HdaCodec->SendCommand(HdaCodec, wNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_AMP_CAPS_OUTPUT), (UINT32*)&wPinWidget->OutAmpCapabilities);
                    ASSERT_EFI_ERROR(Status);
                }

                // Get pin caps and config.
                Status = HdaCodec->SendCommand(HdaCodec, wNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_PIN_CAPS), (UINT32*)&wPinWidget->PinCapabilities);
                ASSERT_EFI_ERROR(Status);
                Status = HdaCodec->SendCommand(HdaCodec, wNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PIN_WIDGET_CONTROL, 0), (UINT32*)&wPinWidget->PinControl);
                ASSERT_EFI_ERROR(Status);
                Status = HdaCodec->SendCommand(HdaCodec, wNid, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONFIGURATION_DEFAULT, 0), (UINT32*)&wPinWidget->ConfigurationDefault);
                ASSERT_EFI_ERROR(Status);
            }


        }
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaCodecDriverBindingSupported(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL) {
    //DEBUG((DEBUG_INFO, "HdaCodecDriverBindingSupported():start \n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_HDA_CODEC_PROTOCOL *HdaCodec;
    UINT8 CodecAddress;

    // Attempt to open the HDA codec protocol. If it can be opened, we can support it.
    Status = gBS->OpenProtocol(ControllerHandle, &gEfiHdaCodecProtocolGuid, (VOID**)&HdaCodec,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    if (EFI_ERROR(Status))
        return Status;
    
    // Get address of codec.
    Status = HdaCodec->GetAddress(HdaCodec, &CodecAddress);
    if (EFI_ERROR(Status))
        goto CLOSE_HDA;

    DEBUG((DEBUG_INFO, "HdaCodecDriverBindingSupported(): attaching to codec 0x%X\n", CodecAddress));
    Status = EFI_SUCCESS;

CLOSE_HDA:
    // Close protocol.
    gBS->CloseProtocol(ControllerHandle, &gEfiHdaCodecProtocolGuid, This->DriverBindingHandle, ControllerHandle);
    return Status;
}

EFI_STATUS
EFIAPI
HdaCodecDriverBindingStart(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL) {
    DEBUG((DEBUG_INFO, "HdaCodecDriverBindingStart(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_HDA_CODEC_PROTOCOL *HdaCodecProto;
    EFI_DEVICE_PATH_PROTOCOL *HdaCodecDevicePath;
    HDA_CODEC_DEV *HdaCodecDev;

    Status = gBS->OpenProtocol(ControllerHandle, &gEfiHdaCodecProtocolGuid, (VOID**)&HdaCodecProto,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    if (EFI_ERROR(Status))
        return Status;

    // Get Device Path protocol.
    Status = gBS->OpenProtocol(ControllerHandle, &gEfiDevicePathProtocolGuid, (VOID**)&HdaCodecDevicePath,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    if (EFI_ERROR (Status))
        goto CLOSE_HDA;

    // Create codec device.
    HdaCodecDev = AllocateZeroPool(sizeof(HDA_CODEC_DEV));
    HdaCodecDev->HdaCodecProto = HdaCodecProto;
    HdaCodecDev->DevicePath = HdaCodecDevicePath;
    
    // Get default values for codec nodes.
    Status = HdaCodecGetDefaultData(HdaCodecDev);
    if (EFI_ERROR(Status))
        goto FREE_DEVICE;
   // Status = HdaCodecPrintDefaults(HdaCodecDev);
   // if (EFI_ERROR(Status))
  //      goto FREE_DEVICE;

    // Get address of codec.
    UINT8 CodecAddress;
    Status = HdaCodecProto->GetAddress(HdaCodecProto, &CodecAddress);
    
    if (CodecAddress > 0)
        return EFI_SUCCESS;

  // stream
  DEBUG((DEBUG_INFO, "Set data\n"));
    UINT32 Tmp;


    /*Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x16 speaker amp 0x%X\n", Tmp));
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x16 speaker amp 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PIN_WIDGET_CONTROL, 0x0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x16 speaker pin-ctls 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_EAPD_BTL_ENABLE, 0x0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x16 speaker eapd 0x%X\n", Tmp));
    
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0e, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xF01f), &Tmp);
    ASSERT_EFI_ERROR(Status);

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_CONVERTER_FORMAT, 0x4011), &Tmp);
    ASSERT_EFI_ERROR(Status);

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_CONVERTER_STREAM_CHANNEL, 0x0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONVERTER_STREAM_CHANNEL, 0x0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x2 output stream 0x%X\n", Tmp));*/
    

    //Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0e, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    //ASSERT_EFI_ERROR(Status);
   // DEBUG((DEBUG_INFO, "0x0e mixer amp 0x%X\n", Tmp));
    
    //DEBUG((DEBUG_INFO, "0x0e mixer amp 0x%X\n", Tmp));
   // Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_CONVERTER_CHANNEL_COUNT, 0x0), &Tmp);
  //  ASSERT_EFI_ERROR(Status);
   // Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB030), &Tmp);
    //ASSERT_EFI_ERROR(Status);
  //  Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x17, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0x0), &Tmp);
    //ASSERT_EFI_ERROR(Status);*/

  /*  Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x3, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_CONVERTER_FORMAT, 0x4011), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x3, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_CONVERTER_STREAM_CHANNEL, 0x10), &Tmp);
    ASSERT_EFI_ERROR(Status);
    //Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x3, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_CONVERTER_CHANNEL_COUNT, 0x0), &Tmp);
   // ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x3, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB030), &Tmp);
    ASSERT_EFI_ERROR(Status);
   // Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x3, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB030), &Tmp);
    //ASSERT_EFI_ERROR(Status);
   // Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x14, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0x0), &Tmp);
   // ASSERT_EFI_ERROR(Status);*/


   // Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_CONVERTER_FORMAT, 0x4011), &Tmp);
   // ASSERT_EFI_ERROR(Status);
   // Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_CONVERTER_STREAM_CHANNEL, 0x10), &Tmp);
    //ASSERT_EFI_ERROR(Status);


    /*Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x1b, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x1b, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x1b line out amp 0x%X\n", Tmp));
    
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x1b, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONN_LIST_ENTRY, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x1b line out conn list entries: 0x%X\n", Tmp));
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x1b, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONN_SELECT_CONTROL, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x1b line out conn list selected: %u\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x1b, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_PIN_WIDGET_CONTROL, 0x40), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x1b, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PIN_WIDGET_CONTROL, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x1b line out pin ctls: 0x%X\n", Tmp));



    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x16 line out amp 0x%X\n", Tmp));
    
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONN_LIST_ENTRY, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x16 line out conn list entries: 0x%X\n", Tmp));
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONN_SELECT_CONTROL, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x16 line out conn list selected: %u\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_PIN_WIDGET_CONTROL, 0x40), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PIN_WIDGET_CONTROL, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x16 line out pin ctls: 0x%X\n", Tmp));
    
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0c, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB01F), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0c, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x0c mixer amp-out 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0c, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0x7000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0c, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0x2000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x0c mixer amp-in index 0 0x%X\n", Tmp));
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0c, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0x2001), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x0c mixer amp-in index 1 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x02 output amp 0x%X\n", Tmp));


    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_CONVERTER_FORMAT, 0x4011), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_CONVERTER_FORMAT, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x02 output format 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_CONVERTER_STREAM_CHANNEL, 0x10), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONVERTER_STREAM_CHANNEL, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x02 output stream 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_CONVERTER_CHANNEL_COUNT, 1), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONVERTER_CHANNEL_COUNT, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x02 output channels 0x%X\n", Tmp));*/

Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x17, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x17, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x17 speaker amp 0x%X\n", Tmp));
    
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x17, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONN_LIST_ENTRY, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x17 speaker conn list entries: 0x%X\n", Tmp));
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x17, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONN_SELECT_CONTROL, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x17 speaker conn list selected: %u\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x17, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_PIN_WIDGET_CONTROL, 0x40), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x17, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PIN_WIDGET_CONTROL, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x17 speaker pin ctls: 0x%X\n", Tmp));


    
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0f, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB01F), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0f, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x0f mixer amp-out 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0f, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0x7000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0f, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0x2000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x0f mixer amp-in index 0 0x%X\n", Tmp));
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0f, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0x2001), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x0f mixer amp-in index 1 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB030), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x02 output amp 0x%X\n", Tmp));


    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_CONVERTER_FORMAT, 0x4011), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_CONVERTER_FORMAT, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x02 output format 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_CONVERTER_STREAM_CHANNEL, 0x60), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONVERTER_STREAM_CHANNEL, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x02 output stream 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_CONVERTER_CHANNEL_COUNT, 1), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONVERTER_CHANNEL_COUNT, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x02 output channels 0x%X\n", Tmp));



  /* Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0c, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0c, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x0c speaker amp 0x%X\n", Tmp));
    
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0c, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONN_LIST_ENTRY, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x0c speaker conn list entries: 0x%X\n", Tmp));
    //Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0d, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_CONN_SELECT_CONTROL, 0), &Tmp);
    //ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0c, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONN_SELECT_CONTROL, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x0c speaker conn list selected: %u\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0c, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_EAPD_BTL_ENABLE, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x0c speaker eapd: %u\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0c, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_PIN_WIDGET_CONTROL, 0x40), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0c, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PIN_WIDGET_CONTROL, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x0c speaker pin ctls: 0x%X\n", Tmp));


    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x3, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB020), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x3, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x3 output amp 0x%X\n", Tmp));


    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x3, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_CONVERTER_FORMAT, 0x4011), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x3, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_CONVERTER_FORMAT, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x3 output format 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x3, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_CONVERTER_STREAM_CHANNEL, 0x10), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x3, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONVERTER_STREAM_CHANNEL, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x3 output stream 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x3, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_CONVERTER_CHANNEL_COUNT, 1), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x3, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONVERTER_CHANNEL_COUNT, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x3 output channels 0x%X\n", Tmp));*/


    /*Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x14, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x14, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x14 speaker amp 0x%X\n", Tmp));
    
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x14, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONN_LIST_ENTRY, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x14 speaker conn list entries: 0x%X\n", Tmp));
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x14, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_CONN_SELECT_CONTROL, 1), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x14, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONN_SELECT_CONTROL, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x14 speaker conn list selected: %u\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x14, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_PIN_WIDGET_CONTROL, 0x40), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x17, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PIN_WIDGET_CONTROL, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x14 speaker pin ctls: 0x%X\n", Tmp));


    

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0d, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0x7000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0d, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0x2000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x0d mixer amp-in index 0 0x%X\n", Tmp));
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0d, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0x2001), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x0d mixer amp-in index 1 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB03f), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x02 output amp 0x%X\n", Tmp));


    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_CONVERTER_FORMAT, 0x4011), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_CONVERTER_FORMAT, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x02 output format 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_CONVERTER_STREAM_CHANNEL, 0x10), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONVERTER_STREAM_CHANNEL, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x02 output stream 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_CONVERTER_CHANNEL_COUNT, 1), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONVERTER_CHANNEL_COUNT, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x02 output channels 0x%X\n", Tmp));*/

    // Success.
    return EFI_SUCCESS;

FREE_DEVICE:
    // Free device.
    FreePool(HdaCodecDev);

CLOSE_HDA:
    // Close protocols.
    gBS->CloseProtocol(ControllerHandle, &gEfiHdaCodecProtocolGuid, This->DriverBindingHandle, ControllerHandle);
    gBS->CloseProtocol(ControllerHandle, &gEfiHdaCodecProtocolGuid, This->DriverBindingHandle, ControllerHandle);
    return Status;
}

EFI_STATUS
EFIAPI
HdaCodecDriverBindingStop(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN UINTN NumberOfChildren,
    IN EFI_HANDLE *ChildHandleBuffer OPTIONAL) {
    DEBUG((DEBUG_INFO, "HdaCodecDriverBindingStop()\n"));
    return EFI_UNSUPPORTED;
}
