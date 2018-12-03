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

/*EFI_STATUS
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
}*/

EFI_STATUS
EFIAPI
HdaCodecProbeWidget(
    HDA_WIDGET *HdaWidget) {
    DEBUG((DEBUG_INFO, "HdaCodecProbeWidget(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_HDA_CODEC_PROTOCOL *HdaCodecIo = HdaWidget->FuncGroup->HdaCodecDev->HdaCodecIo;
    //UINT32 Response;

    // Get widget capabilities.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, HdaWidget->NodeId,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_WIDGET_CAPS), &HdaWidget->Capabilities);
    if (EFI_ERROR(Status))
        return Status;
    HdaWidget->Type = HDA_PARAMETER_WIDGET_CAPS_TYPE(HdaWidget->Capabilities);
    DEBUG((DEBUG_INFO, "Widget @ 0x%X type: 0x%X\n", HdaWidget->NodeId, HdaWidget->Type));
    DEBUG((DEBUG_INFO, "Widget @ 0x%X capabilities: 0x%X\n", HdaWidget->NodeId, HdaWidget->Capabilities));

    // Get supported PCM sizes/rates.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, HdaWidget->NodeId,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES), &HdaWidget->SupportedPcmRates);
    if (EFI_ERROR(Status))
        return Status;
    DEBUG((DEBUG_INFO, "Widget @ 0x%X supported PCM sizes/rates: 0x%X\n", HdaWidget->NodeId, HdaWidget->SupportedPcmRates));

    // Get supported stream formats.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, HdaWidget->NodeId,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUPPORTED_STREAM_FORMATS), &HdaWidget->SupportedFormats);
    if (EFI_ERROR(Status))
        return Status;
    DEBUG((DEBUG_INFO, "Widget @ 0x%X supported formats: 0x%X\n", HdaWidget->NodeId, HdaWidget->SupportedFormats));

    // Get input amp capabilities.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, HdaWidget->NodeId,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_AMP_CAPS_INPUT), &HdaWidget->AmpInCapabilities);
    if (EFI_ERROR(Status))
        return Status;
    DEBUG((DEBUG_INFO, "Widget @ 0x%X input amp capabilities: 0x%X\n", HdaWidget->NodeId, HdaWidget->AmpInCapabilities));

    // Get output amp capabilities.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, HdaWidget->NodeId,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_AMP_CAPS_OUTPUT), &HdaWidget->AmpOutCapabilities);
    if (EFI_ERROR(Status))
        return Status;
    DEBUG((DEBUG_INFO, "Widget @ 0x%X output amp capabilities: 0x%X\n", HdaWidget->NodeId, HdaWidget->AmpOutCapabilities));

    // Get pin capabilities.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, HdaWidget->NodeId,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_PIN_CAPS), &HdaWidget->PinCapabilities);
    if (EFI_ERROR(Status))
        return Status;
    DEBUG((DEBUG_INFO, "Widget @ 0x%X pin capabilities: 0x%X\n", HdaWidget->NodeId, HdaWidget->PinCapabilities));

    // Get connection list length.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, HdaWidget->NodeId,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_CONN_LIST_LENGTH), &HdaWidget->ConnectionListLength);
    if (EFI_ERROR(Status))
        return Status;
    DEBUG((DEBUG_INFO, "Widget @ 0x%X connection list length: 0x%X\n", HdaWidget->NodeId, HdaWidget->ConnectionListLength));

    // Get supported power states.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, HdaWidget->NodeId,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUPPORTED_POWER_STATES), &HdaWidget->SupportedPowerStates);
    if (EFI_ERROR(Status))
        return Status;
    DEBUG((DEBUG_INFO, "Widget @ 0x%X supported power states: 0x%X\n", HdaWidget->NodeId, HdaWidget->SupportedPowerStates));

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaCodecProbeFuncGroup(
    HDA_FUNC_GROUP *FuncGroup) {
    DEBUG((DEBUG_INFO, "HdaCodecProbeFuncGroup(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_HDA_CODEC_PROTOCOL *HdaCodecIo = FuncGroup->HdaCodecDev->HdaCodecIo;
    UINT32 Response;

    UINT8 WidgetStart;
    UINT8 WidgetEnd;
    UINT8 WidgetCount;

    // Get function group type.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, FuncGroup->NodeId,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_FUNC_GROUP_TYPE), &Response);
    if (EFI_ERROR(Status))
        return Status;
    FuncGroup->Type = HDA_PARAMETER_FUNC_GROUP_TYPE_NODETYPE(Response);

    // Determine if function group is an audio one. If not, we cannot support it.
    DEBUG((DEBUG_INFO, "Function group @ 0x%X is of type 0x%X\n", FuncGroup->NodeId, FuncGroup->Type));
    if (FuncGroup->Type != HDA_FUNC_GROUP_TYPE_AUDIO)
        return EFI_UNSUPPORTED;

    // Get function group capabilities.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, FuncGroup->NodeId,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_FUNC_GROUP_CAPS), &FuncGroup->Capabilities);
    if (EFI_ERROR(Status))
        return Status;
    DEBUG((DEBUG_INFO, "Function group @ 0x%X capabilities: 0x%X\n", FuncGroup->NodeId, FuncGroup->Capabilities));

    // Get default supported PCM sizes/rates.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, FuncGroup->NodeId,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUPPORTED_PCM_SIZE_RATES), &FuncGroup->SupportedPcmRates);
    if (EFI_ERROR(Status))
        return Status;
    DEBUG((DEBUG_INFO, "Function group @ 0x%X supported PCM sizes/rates: 0x%X\n", FuncGroup->NodeId, FuncGroup->SupportedPcmRates));

    // Get default supported stream formats.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, FuncGroup->NodeId,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUPPORTED_STREAM_FORMATS), &FuncGroup->SupportedFormats);
    if (EFI_ERROR(Status))
        return Status;
    DEBUG((DEBUG_INFO, "Function group @ 0x%X supported formats: 0x%X\n", FuncGroup->NodeId, FuncGroup->SupportedFormats));

    // Get default input amp capabilities.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, FuncGroup->NodeId,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_AMP_CAPS_INPUT), &FuncGroup->AmpInCapabilities);
    if (EFI_ERROR(Status))
        return Status;
    DEBUG((DEBUG_INFO, "Function group @ 0x%X input amp capabilities: 0x%X\n", FuncGroup->NodeId, FuncGroup->AmpInCapabilities));

    // Get default output amp capabilities.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, FuncGroup->NodeId,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_AMP_CAPS_OUTPUT), &FuncGroup->AmpOutCapabilities);
    if (EFI_ERROR(Status))
        return Status;
    DEBUG((DEBUG_INFO, "Function group @ 0x%X output amp capabilities: 0x%X\n", FuncGroup->NodeId, FuncGroup->AmpOutCapabilities));

    // Get supported power states.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, FuncGroup->NodeId,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUPPORTED_POWER_STATES), &FuncGroup->SupportedPowerStates);
    if (EFI_ERROR(Status))
        return Status;
    DEBUG((DEBUG_INFO, "Function group @ 0x%X supported power states: 0x%X\n", FuncGroup->NodeId, FuncGroup->SupportedPowerStates));

    // Get GPIO capabilities.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, FuncGroup->NodeId,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_GPIO_COUNT), &FuncGroup->GpioCapabilities);
    if (EFI_ERROR(Status))
        return Status;
    DEBUG((DEBUG_INFO, "Function group @ 0x%X GPIO capabilities: 0x%X\n", FuncGroup->NodeId, FuncGroup->GpioCapabilities));

    // Get number of widgets in function group.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, FuncGroup->NodeId,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUBNODE_COUNT), &Response);
    if (EFI_ERROR(Status))
        return Status;
    WidgetStart = HDA_PARAMETER_SUBNODE_COUNT_START(Response);
    WidgetCount = HDA_PARAMETER_SUBNODE_COUNT_TOTAL(Response);
    WidgetEnd = WidgetStart + WidgetCount - 1;
    DEBUG((DEBUG_INFO, "Function group @ 0x%X contains %u widgets, start @ 0x%X, end @ 0x%X\n",
        FuncGroup->NodeId, WidgetCount, WidgetStart, WidgetEnd));

    // Ensure there are widgets.
    if (WidgetCount == 0)
        return EFI_UNSUPPORTED;

    // Allocate space for widgets.
    FuncGroup->Widgets = AllocateZeroPool(sizeof(HDA_WIDGET) * WidgetCount);
    FuncGroup->WidgetsCount = WidgetCount;

    // Probe widgets.
    for (UINT8 i = 0; i < WidgetCount; i++) {
        FuncGroup->Widgets[i].FuncGroup = FuncGroup;
        FuncGroup->Widgets[i].NodeId = WidgetStart + i;
        Status = HdaCodecProbeWidget(FuncGroup->Widgets + i);
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaCodecProbeCodec(
    HDA_CODEC_DEV *HdaCodecDev) {
    DEBUG((DEBUG_INFO, "HdaCodecProbeCodec(): start\n"));

    // Create variables.
    EFI_STATUS Status;
    EFI_HDA_CODEC_PROTOCOL *HdaCodecIo = HdaCodecDev->HdaCodecIo;
    UINT32 Response;
    UINT8 FuncStart;
    UINT8 FuncEnd;
    UINT8 FuncCount;

    // Get vendor and device ID.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, HDA_NID_ROOT,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_VENDOR_ID), &Response);
    if (EFI_ERROR(Status))
        return Status;
    HdaCodecDev->VendorId = HDA_PARAMETER_VENDOR_ID_VEN(Response);
    HdaCodecDev->DeviceId = HDA_PARAMETER_VENDOR_ID_DEV(Response);
    DEBUG((DEBUG_INFO, "Codec ID: 0x%X:0x%X\n", HdaCodecDev->VendorId, HdaCodecDev->DeviceId));

    // Get revision ID.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, HDA_NID_ROOT,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_REVISION_ID), &Response);
    if (EFI_ERROR(Status))
        return Status;
    HdaCodecDev->RevisionId = HDA_PARAMETER_REVISION_ID_REV_ID(Response);
    HdaCodecDev->SteppindId = HDA_PARAMETER_REVISION_ID_STEPPING(Response);
    
    // Get function group count.
    Status = HdaCodecIo->SendCommand(HdaCodecIo, HDA_NID_ROOT,
        HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PARAMETER, HDA_PARAMETER_SUBNODE_COUNT), &Response);
    if (EFI_ERROR(Status))
        return Status;
    FuncStart = HDA_PARAMETER_SUBNODE_COUNT_START(Response);
    FuncCount = HDA_PARAMETER_SUBNODE_COUNT_TOTAL(Response);
    FuncEnd = FuncStart + FuncCount - 1;
    DEBUG((DEBUG_INFO, "Codec contains %u function groups, start @ 0x%X, end @ 0x%X\n", FuncCount, FuncStart, FuncEnd));

    // Ensure there are functions.
    if (FuncCount == 0)
        return EFI_UNSUPPORTED;

    // Allocate space for function groups.
    HdaCodecDev->FuncGroups = AllocateZeroPool(sizeof(HDA_FUNC_GROUP) * FuncCount);
    HdaCodecDev->FuncGroupsCount = FuncCount;

    // Probe functions.
    for (UINT8 i = 0; i < FuncCount; i++) {
        HdaCodecDev->FuncGroups[i].HdaCodecDev = HdaCodecDev;
        HdaCodecDev->FuncGroups[i].NodeId = FuncStart + i;
        Status = HdaCodecProbeFuncGroup(HdaCodecDev->FuncGroups + i);
    }

    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
HdaCodecDriverBindingSupported(
    IN EFI_DRIVER_BINDING_PROTOCOL *This,
    IN EFI_HANDLE ControllerHandle,
    IN EFI_DEVICE_PATH_PROTOCOL *RemainingDevicePath OPTIONAL) {

    // Create variables.
    EFI_STATUS Status;
    EFI_HDA_CODEC_PROTOCOL *HdaCodecIo;
    UINT8 CodecAddress;

    // Attempt to open the HDA codec protocol. If it can be opened, we can support it.
    Status = gBS->OpenProtocol(ControllerHandle, &gEfiHdaCodecProtocolGuid, (VOID**)&HdaCodecIo,
        This->DriverBindingHandle, ControllerHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
    if (EFI_ERROR(Status))
        return Status;
    
    // Get address of codec.
    Status = HdaCodecIo->GetAddress(HdaCodecIo, &CodecAddress);
    if (EFI_ERROR(Status))
        goto CLOSE_CODEC;

    // Codec can be supported.
    DEBUG((DEBUG_INFO, "HdaCodecDriverBindingSupported(): attaching to codec 0x%X\n", CodecAddress));
    Status = EFI_SUCCESS;

CLOSE_CODEC:
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
    EFI_HDA_CODEC_PROTOCOL *HdaCodecIo;
    EFI_DEVICE_PATH_PROTOCOL *HdaCodecDevicePath;
    HDA_CODEC_DEV *HdaCodecDev;

    Status = gBS->OpenProtocol(ControllerHandle, &gEfiHdaCodecProtocolGuid, (VOID**)&HdaCodecIo,
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
    HdaCodecDev->HdaCodecIo = HdaCodecIo;
    HdaCodecDev->DevicePath = HdaCodecDevicePath;
    
    // Get default values for codec nodes.
    //Status = HdaCodecGetDefaultData(HdaCodecDev);
  //  if (EFI_ERROR(Status))
   //     goto FREE_DEVICE;
   // Status = HdaCodecPrintDefaults(HdaCodecDev);
   // if (EFI_ERROR(Status))
  //      goto FREE_DEVICE;

    // Get address of codec.
    UINT8 CodecAddress;
    Status = HdaCodecIo->GetAddress(HdaCodecIo, &CodecAddress);
    
    if (CodecAddress > 0)
        return EFI_SUCCESS;

    // Probe codec.
    Status = HdaCodecProbeCodec(HdaCodecDev);

  // stream
  DEBUG((DEBUG_INFO, "Set data\n"));
  //  UINT32 Tmp;


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

   /* Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x17, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x17, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x17 speaker amp 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x17, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_EAPD_BTL_ENABLE, 0x2), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x17, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_EAPD_BTL_ENABLE, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x17 speaker eapd 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x17, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_PIN_WIDGET_CONTROL, 0x40), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x17, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PIN_WIDGET_CONTROL, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x17 speaker pin ctls: 0x%X\n", Tmp));   8300*/
/*
    // dc7700
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x16 speaker amp 0x%X\n", Tmp));

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_12BIT(HDA_VERB_SET_PIN_WIDGET_CONTROL, 0x40), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_PIN_WIDGET_CONTROL, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x16 speaker pin ctls: 0x%X\n", Tmp));
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x16, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONN_LIST_ENTRY, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x16 speaker conn list: 0x%X\n", Tmp));


    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0e, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_CONN_LIST_ENTRY, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x0e mixer list: 0x%X\n", Tmp));
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0e, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB01F), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0e, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x0e mixer amp-out 0x%X\n", Tmp));

    //Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0e, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0x7000), &Tmp);
    //ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0e, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0x2000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x0e mixer amp-in index 0 0x%X\n", Tmp));
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x0e, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0x2001), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x0e mixer amp-in index 1 0x%X\n", Tmp));*/

  /*  Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_4BIT(HDA_VERB_SET_AMP_GAIN_MUTE, 0xB057), &Tmp);
    ASSERT_EFI_ERROR(Status);
    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x2, HDA_CODEC_VERB_4BIT(HDA_VERB_GET_AMP_GAIN_MUTE, 0xA000), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x02 output amp 0x%X\n", Tmp));*/

/*
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

    Status = HdaCodecProto->SendCommand(HdaCodecProto, 0x1, HDA_CODEC_VERB_12BIT(HDA_VERB_GET_POWER_STATE, 0), &Tmp);
    ASSERT_EFI_ERROR(Status);
    DEBUG((DEBUG_INFO, "0x01 poewr state 0x%X\n", Tmp));

*/

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

//FREE_DEVICE:
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
