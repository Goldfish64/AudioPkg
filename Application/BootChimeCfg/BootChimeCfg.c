/*
 * File: BootChimeCfg.c
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

#include "BootChimeCfg.h"
#include <Library/ShellLib.h>

STATIC CHAR16 *DefaultDevices[EfiAudioIoDeviceMaximum] = { L"Line", L"Speaker", L"Headphones", L"SPDIF", L"Mic", L"HDMI", L"Other" };
STATIC CHAR16 *Locations[EfiAudioIoLocationMaximum] = { L"N/A", L"rear", L"front", L"left", L"right", L"top", L"bottom", L"other" };
STATIC CHAR16 *Surfaces[EfiAudioIoSurfaceMaximum] = { L"external", L"internal", L"other" };

// Parameter list.
STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
    { BCFG_ARG_HELP, TypeFlag },
    { BCFG_ARG_LIST, TypeFlag },
    { BCFG_ARG_SELECT, TypeValue },
    { BCFG_ARG_VOLUME, TypeValue },
    { BCFG_ARG_TEST, TypeFlag },
    { NULL, TypeMax }
};

EFI_STATUS
EFIAPI
GetOutputDevices(
    IN  EFI_HANDLE ImageHandle,
    OUT BOOT_CHIME_DEVICE **Devices,
    OUT UINTN *DevicesCount) {
    // Create variables.
    EFI_STATUS Status;
    EFI_HANDLE *AudioIoHandles = NULL;
    UINTN AudioIoHandleCount = 0;
    EFI_AUDIO_IO_PROTOCOL *AudioIo;
    EFI_DEVICE_PATH_PROTOCOL *DevicePath;
    EFI_AUDIO_IO_PROTOCOL_PORT *OutputPorts;
    UINTN OutputPortsCount;

    // Devices.
    BOOT_CHIME_DEVICE *OutputDevices = NULL;
    BOOT_CHIME_DEVICE *OutputDevicesNew = NULL;
    UINTN OutputDevicesCount = 0;
    UINTN OutputDeviceIndex = 0;

    // Get Audio I/O protocols in system.
    Status = gBS->LocateHandleBuffer(ByProtocol, &gEfiAudioIoProtocolGuid, NULL, &AudioIoHandleCount, &AudioIoHandles);
    if (EFI_ERROR(Status))
        return Status;

    // Discover audio outputs in system.
    for (UINTN h = 0; h < AudioIoHandleCount; h++) {
        // Open Audio I/O protocol.
        Status = gBS->OpenProtocol(AudioIoHandles[h], &gEfiAudioIoProtocolGuid, (VOID**)&AudioIo,
            NULL, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        ASSERT_EFI_ERROR(Status);
        if (EFI_ERROR(Status))
            continue;
        
        // Get device path.
        Status = gBS->OpenProtocol(AudioIoHandles[h], &gEfiDevicePathProtocolGuid, (VOID**)&DevicePath,
            NULL, ImageHandle, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        ASSERT_EFI_ERROR(Status);
        if (EFI_ERROR(Status))
            continue;

        // Get output devices.
        Status = AudioIo->GetOutputs(AudioIo, &OutputPorts, &OutputPortsCount);
        ASSERT_EFI_ERROR(Status);
        if (EFI_ERROR(Status))
            continue;

        // Increase total output devices.
        OutputDevicesNew = ReallocatePool(OutputDevicesCount * sizeof(BOOT_CHIME_DEVICE),
            (OutputDevicesCount + OutputPortsCount) * sizeof(BOOT_CHIME_DEVICE), OutputDevices);
        if (OutputDevicesNew == NULL) {
            FreePool(OutputPorts);
            Status = EFI_OUT_OF_RESOURCES;
            goto DONE_ERROR;
        }
        OutputDevices = OutputDevicesNew;
        OutputDevicesCount += OutputPortsCount;

        // Get devices on this protocol.
        for (UINTN o = 0; o < OutputPortsCount; o++) {
            OutputDevices[OutputDeviceIndex].AudioIo = AudioIo;
            OutputDevices[OutputDeviceIndex].DevicePath = DevicePath;
            OutputDevices[OutputDeviceIndex].OutputPort = OutputPorts[o];
            OutputDevices[OutputDeviceIndex].OutputPortIndex = o;
            OutputDeviceIndex++;
        }

        // Free output ports.
        FreePool(OutputPorts);
    }

    // Success.
    *Devices = OutputDevices;
    *DevicesCount = OutputDevicesCount;
    Status = EFI_SUCCESS;
    goto DONE;

DONE_ERROR:
    // Free devices.
    if (OutputDevices != NULL)
        FreePool(OutputDevices);

DONE:
    // Free stuff.
    if (AudioIoHandles != NULL)
        FreePool(AudioIoHandles);
    return Status;
}

VOID
EFIAPI
PrintHelp(VOID) {
    // Print help.
    ShellPrintEx(-1, -1, L"Configures the BootChimeDxe EFI driver.\n\n");
    ShellPrintEx(-1, -1, L"%s [%s] [%s X] [%s X] [%s] [%s]\n\n", BCFG_NAME,
        BCFG_ARG_LIST, BCFG_ARG_SELECT, BCFG_ARG_VOLUME, BCFG_ARG_TEST, BCFG_ARG_HELP);
    ShellPrintEx(-1, -1, L"    %s - List all audio outputs\n", BCFG_ARG_LIST);
    ShellPrintEx(-1, -1, L"    %s - Select audio output\n", BCFG_ARG_SELECT);
    ShellPrintEx(-1, -1, L"    %s - Change volume\n", BCFG_ARG_VOLUME);
    ShellPrintEx(-1, -1, L"    %s - Test current audio output\n", BCFG_ARG_TEST);
    ShellPrintEx(-1, -1, L"    %s - Show this help\n", BCFG_ARG_HELP);
}

VOID
EFIAPI
PrintDevices(
    IN BOOT_CHIME_DEVICE *Devices,
    IN UINTN DevicesCount) {
    // Print each device.
    ShellPrintEx(-1, -1, L"Output devices:\n");
    for (UINTN i = 0; i < DevicesCount; i++)
        ShellPrintEx(-1, -1, L"  %lu. %s - %s %s\n", i + 1, DefaultDevices[Devices[i].OutputPort.Device],
            Locations[Devices[i].OutputPort.Location], Surfaces[Devices[i].OutputPort.Surface]);
}

EFI_STATUS
EFIAPI
TestOutput(
    IN EFI_HANDLE ImageHandle) {
    // Create variables.
    EFI_STATUS Status;
    EFI_AUDIO_IO_PROTOCOL *AudioIo;
    UINTN OutputIndex;
    UINT8 OutputVolume;

    // Get stored settings.
    Status = BootChimeGetStoredOutput(ImageHandle, &AudioIo, &OutputIndex, &OutputVolume);
    if (EFI_ERROR(Status))
        return Status;
    
    // Setup playback.
    Status = AudioIo->SetupPlayback(AudioIo, OutputIndex, OutputVolume, ChimeDataFreq, ChimeDataBits, ChimeDataChannels);
    if (EFI_ERROR(Status))
        return Status;

    // Play chime.
    return AudioIo->StartPlayback(AudioIo, ChimeData, ChimeDataLength, 0);

}

EFI_STATUS
EFIAPI
BootChimeCfgMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable) {

    // Create variables.
    EFI_STATUS Status;
    LIST_ENTRY *Package;
    CHAR16 *ProblemParam;
    CONST CHAR16 *SelectArgStrValue;
    UINT64 SelectArgValue = 0;
    CONST CHAR16 *VolumeArgStrValue;
    UINT64 VolumeArgValue = 0;

    // Devices.
    BOOT_CHIME_DEVICE *Devices = NULL;
    UINTN DevicesCount = 0;
    UINTN DeviceIndex = 0;
    UINT8 DeviceVolume = 0;
    CHAR16 *DevicePathText = NULL;

    // Get command line arguments.
    Status = ShellCommandLineParse(ParamList, &Package, &ProblemParam, TRUE);
    if (EFI_ERROR(Status)) {
        if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
            ShellPrintEx(-1, -1, L"Error: The command argument %s is invalid.\n\n", ProblemParam);
            FreePool(ProblemParam);

            // Show help.
            PrintHelp();
            Status = EFI_INVALID_PARAMETER;
        }
        goto DONE;
    }

    // Check to see if any supported flags are on the command line. If none, or the help arg, show help.
    if ((!(ShellCommandLineGetFlag(Package, BCFG_ARG_LIST) || ShellCommandLineGetFlag(Package, BCFG_ARG_SELECT) ||
        ShellCommandLineGetFlag(Package, BCFG_ARG_VOLUME) || ShellCommandLineGetFlag(Package, BCFG_ARG_TEST))) ||
        ShellCommandLineGetFlag(Package, BCFG_ARG_HELP)) {
        PrintHelp();
        goto DONE;
    }

    // Get devices.
    Status = GetOutputDevices(ImageHandle, &Devices, &DevicesCount);
    if (EFI_ERROR(Status))
        goto DONE;

    // Is the list flag present? If so, list devices.
    if (ShellCommandLineGetFlag(Package, BCFG_ARG_LIST)) {
        PrintDevices(Devices, DevicesCount);
        goto DONE;
    }

    // If the select flag is present, store selected device.
    SelectArgStrValue = ShellCommandLineGetValue(Package, BCFG_ARG_SELECT);
    if (SelectArgStrValue != NULL) {
        Status = ShellConvertStringToUint64(SelectArgStrValue, &SelectArgValue, FALSE, FALSE);
        if (EFI_ERROR(Status) || (SelectArgValue == 0) || (SelectArgValue > DevicesCount)) {
            ShellPrintEx(-1, -1, L"Error: %s is not a valid output index.\n", SelectArgStrValue);
            goto DONE;
        }
        DeviceIndex = SelectArgValue - 1;

        // Get device path text.
        DevicePathText = ConvertDevicePathToText(Devices[DeviceIndex].DevicePath, FALSE, FALSE);
        if (DevicePathText == NULL) {
            ShellPrintEx(-1, -1, L"Error: The device path couldn't be retrieved.\n");
            goto DONE;
        }

        // Set device path variable. Add one to length to account for null terminator.
        Status = gRT->SetVariable(BOOT_CHIME_VAR_DEVICE, &gBootChimeVendorVariableGuid, BOOT_CHIME_VAR_ATTRIBUTES,
            (StrLen(DevicePathText) + 1) * sizeof(CHAR16), DevicePathText);
        if (EFI_ERROR(Status))
            goto DONE;

        // Set index variable.
        Status = gRT->SetVariable(BOOT_CHIME_VAR_INDEX, &gBootChimeVendorVariableGuid, BOOT_CHIME_VAR_ATTRIBUTES,
            sizeof(UINTN), &Devices[DeviceIndex].OutputPortIndex);
        if (EFI_ERROR(Status))
            goto DONE;

        // Success.
        ShellPrintEx(-1, -1, L"Now using output %s - %s %s\n", DefaultDevices[Devices[DeviceIndex].OutputPort.Device],
            Locations[Devices[DeviceIndex].OutputPort.Location], Surfaces[Devices[DeviceIndex].OutputPort.Surface]);
        Status = EFI_SUCCESS;
    }

    // If the volume flag is present, store volume.
    VolumeArgStrValue = ShellCommandLineGetValue(Package, BCFG_ARG_VOLUME);
    if (VolumeArgStrValue != NULL) {
        Status = ShellConvertStringToUint64(VolumeArgStrValue, &VolumeArgValue, FALSE, FALSE);
        if (EFI_ERROR(Status)) {
            ShellPrintEx(-1, -1, L"Error: %s is not a valid volume setting.\n", VolumeArgStrValue);
            goto DONE;
        }

        // Cast volume.
        if (VolumeArgValue > EFI_AUDIO_IO_PROTOCOL_MAX_VOLUME)
            VolumeArgValue = EFI_AUDIO_IO_PROTOCOL_MAX_VOLUME;
        DeviceVolume = (UINT8)VolumeArgValue;

        // Set volume variable.
        Status = gRT->SetVariable(BOOT_CHIME_VAR_VOLUME, &gBootChimeVendorVariableGuid, BOOT_CHIME_VAR_ATTRIBUTES,
            sizeof(UINT8), &VolumeArgValue);
        if (EFI_ERROR(Status))
            goto DONE;

        // Success.
        ShellPrintEx(-1, -1, L"Volume set to %u%%\n", DeviceVolume);
        Status = EFI_SUCCESS;
    }

    // If the test flag is present, test output.
    if (ShellCommandLineGetFlag(Package, BCFG_ARG_TEST)) {
        Status = TestOutput(ImageHandle);
        if (EFI_ERROR(Status))
            goto DONE;
    }

    return EFI_SUCCESS;

DONE:
    if (DevicePathText != NULL)
        FreePool(DevicePathText);
    if (Devices != NULL)
        FreePool(Devices);

    ShellCommandLineFreeVarList(Package);

    // Show error.
    if (EFI_ERROR(Status))
        ShellPrintEx(-1, -1, L"The command failed with error: %r\n", Status);
    return Status;
}
