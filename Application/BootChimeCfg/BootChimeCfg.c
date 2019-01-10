/*
 * File: BootChimeCfg.c
 *
 * Description: BootChimeDxe configuration application.
 *
 * Copyright (c) 2018-2019 John Davis
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

STATIC CHAR16 *DefaultDevices[EfiAudioIoDeviceMaximum] = { L"Line", L"Speaker", L"Headphones", L"SPDIF", L"Mic", L"HDMI", L"Other" };
STATIC CHAR16 *Locations[EfiAudioIoLocationMaximum] = { L"N/A", L"rear", L"front", L"left", L"right", L"top", L"bottom", L"other" };
STATIC CHAR16 *Surfaces[EfiAudioIoSurfaceMaximum] = { L"external", L"internal", L"other" };

VOID
EFIAPI
FlushKeystrokes(
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *SimpleTextIn) {

    // Create variables.
    EFI_STATUS Status;
    EFI_INPUT_KEY InputKey;

    // Check if parameters are valid.
    if (!SimpleTextIn)
        return;

    // Flush any keystrokes.
    do {
        gBS->Stall(100);
        Status = SimpleTextIn->ReadKeyStroke(SimpleTextIn, &InputKey);
    } while (!EFI_ERROR(Status));
}

EFI_STATUS
EFIAPI
WaitForKey(
    IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *SimpleTextIn,
    OUT CHAR16 *KeyValue) {

    // Create variables.
    EFI_STATUS Status;
    UINTN EventIndex;
    EFI_INPUT_KEY InputKey;

    // Check if parameters are valid.
    if (!SimpleTextIn || !KeyValue)
        return EFI_INVALID_PARAMETER;

    // Wait for key.
    Status = gBS->WaitForEvent(1, &(SimpleTextIn->WaitForKey), &EventIndex);
    if (EFI_ERROR(Status))
        return EFI_DEVICE_ERROR;

    // Get key value.
    Status = SimpleTextIn->ReadKeyStroke(SimpleTextIn, &InputKey);
    if (EFI_ERROR(Status))
        return EFI_DEVICE_ERROR;

    // If \n, wait again.
    if (InputKey.UnicodeChar == L'\n')
        return WaitForKey(SimpleTextIn, KeyValue);

    // Success.
    *KeyValue = InputKey.UnicodeChar;
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
GetOutputDevices(
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
        Status = gBS->HandleProtocol(AudioIoHandles[h], &gEfiAudioIoProtocolGuid, (VOID**)&AudioIo);
        ASSERT_EFI_ERROR(Status);
        if (EFI_ERROR(Status))
            continue;

        // Get device path.
        Status = gBS->HandleProtocol(AudioIoHandles[h], &gEfiDevicePathProtocolGuid, (VOID**)&DevicePath);
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
    if (OutputDevices)
        FreePool(OutputDevices);

DONE:
    // Free stuff.
    if (AudioIoHandles)
        FreePool(AudioIoHandles);
    return Status;
}

EFI_STATUS
EFIAPI
PrintDevices(
    IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *SimpleTextIn,
    IN BOOT_CHIME_DEVICE *Devices,
    IN UINTN DevicesCount) {
    // Create variables.
    EFI_STATUS Status;
    CHAR16 KeyValue;

    // Check that parameters are valid.
    if (!SimpleTextIn || !Devices)
        return EFI_INVALID_PARAMETER;

    // Print each device.
    Print(L"Output devices:\n");
    for (UINTN i = 0; i < DevicesCount; i++) {
        // Every 10 devices, wait for keystroke.
        if (i && ((i % 10) == 0)) {
            Print(PROMPT_ANY_KEY);
            Status = WaitForKey(SimpleTextIn, &KeyValue);
            if (EFI_ERROR(Status))
                return Status;
            FlushKeystrokes(SimpleTextIn);

            // Clear prompt line.
            for (UINTN s = 0; s < StrLen(PROMPT_ANY_KEY); s++)
                Print(L"\b");
        }

        // Print device.
        Print(L"%lu. %s - %s %s\n", i + 1, DefaultDevices[Devices[i].OutputPort.Device],
            Locations[Devices[i].OutputPort.Location], Surfaces[Devices[i].OutputPort.Surface]);
    }
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SelectDevice(
    IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *SimpleTextIn,
    IN BOOT_CHIME_DEVICE *Devices,
    IN UINTN DevicesCount) {
    // Create variables.
    EFI_STATUS Status;
    CHAR16 KeyValue;
    BOOLEAN Backspace;
    CHAR16 CurrentBuffer[MAX_CHARS + 1];
    UINTN CurrentCharCount;
    UINTN DeviceIndex;

    // Check that parameters are valid.
    if (!SimpleTextIn || !Devices)
        return EFI_INVALID_PARAMETER;

    // We need a null terminator.
    CurrentBuffer[MAX_CHARS] = 0;

    // Prompt for device number.
    CurrentCharCount = 0;
    Print(L"Enter the device number (0-%lu): ", DevicesCount);
    while (TRUE) {
        // Wait for key.
        Status = WaitForKey(SimpleTextIn, &KeyValue);
        if (EFI_ERROR(Status))
            return Status;
        Backspace = (KeyValue == L'\b');

        // If we are backspacing, clear selection.
        if (CurrentCharCount && Backspace) {
            CurrentCharCount--;
            Print(L"\b \b");
            continue;
        }

        // If enter, break out.
        if (KeyValue == L'\r')
            break;

        // If not a number, ignore.
        if (!Backspace && ((KeyValue < L'0') || (KeyValue > '9')))
            continue;

        // If no selection, we don't want to backspace.
        // If we are at the max, don't accept any more.
        if ((!CurrentCharCount && Backspace) || (CurrentCharCount >= MAX_CHARS))
            continue;

        // Get character.
        CurrentBuffer[CurrentCharCount] = KeyValue;
        Print(L"%c", CurrentBuffer[CurrentCharCount]);
        CurrentCharCount++;
    }
    Print(L"\n");

    // Clear out extra characters.
    SetMem(CurrentBuffer + CurrentCharCount, MAX_CHARS - CurrentCharCount, 0);

    // Get device index.
    DeviceIndex = StrDecimalToUintn(CurrentBuffer);
    if (DeviceIndex == 0)
        DeviceIndex = 1;
    DeviceIndex -= 1;

    // Ensure index is in range.
    if (DeviceIndex >= DevicesCount) {
        Print(L"The selected device is not valid.\n");
        return EFI_SUCCESS;
    }

    // Set device path variable. Add one to length to account for null terminator.
    Status = gRT->SetVariable(BOOT_CHIME_VAR_DEVICE, &gBootChimeVendorVariableGuid, BOOT_CHIME_VAR_ATTRIBUTES,
        GetDevicePathSize(Devices[DeviceIndex].DevicePath), Devices[DeviceIndex].DevicePath);
    if (EFI_ERROR(Status))
        return Status;

    // Set index variable.
    Status = gRT->SetVariable(BOOT_CHIME_VAR_INDEX, &gBootChimeVendorVariableGuid, BOOT_CHIME_VAR_ATTRIBUTES,
        sizeof(Devices[DeviceIndex].OutputPortIndex), &Devices[DeviceIndex].OutputPortIndex);
    if (EFI_ERROR(Status))
        return Status;

    // Success.
    Print(L"Now using output %s - %s %s\n", DefaultDevices[Devices[DeviceIndex].OutputPort.Device],
        Locations[Devices[DeviceIndex].OutputPort.Location], Surfaces[Devices[DeviceIndex].OutputPort.Surface]);
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
SelectVolume(
    IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *SimpleTextIn) {
    // Create variables.
    EFI_STATUS Status;
    CHAR16 KeyValue;
    BOOLEAN Backspace;
    CHAR16 CurrentBuffer[MAX_CHARS + 1];
    UINTN CurrentCharCount;
    UINTN Volume;
    UINT8 DeviceVolume;

    // Check that parameters are valid.
    if (!SimpleTextIn)
        return EFI_INVALID_PARAMETER;

    // We need a null terminator.
    CurrentBuffer[MAX_CHARS] = 0;

    // Prompt for device number.
    CurrentCharCount = 0;
    Print(L"Enter the desired volume (0-100): ");
    while (TRUE) {
        // Wait for key.
        Status = WaitForKey(SimpleTextIn, &KeyValue);
        if (EFI_ERROR(Status))
            return Status;
        Backspace = (KeyValue == L'\b');

        // If we are backspacing, clear selection.
        if (CurrentCharCount && Backspace) {
            CurrentCharCount--;
            Print(L"\b \b");
            continue;
        }

        // If enter, break out.
        if (KeyValue == L'\r')
            break;

        // If not a number, ignore.
        if (!Backspace && ((KeyValue < L'0') || (KeyValue > '9')))
            continue;

        // If no selection, we don't want to backspace.
        // If we are at the max, don't accept any more.
        if ((!CurrentCharCount && Backspace) || (CurrentCharCount >= MAX_CHARS))
            continue;

        // Get character.
        CurrentBuffer[CurrentCharCount] = KeyValue;
        Print(L"%c", CurrentBuffer[CurrentCharCount]);
        CurrentCharCount++;
    }
    Print(L"\n");

    // Clear out extra characters.
    SetMem(CurrentBuffer + CurrentCharCount, MAX_CHARS - CurrentCharCount, 0);

    // Get device index.
    Volume = StrDecimalToUintn(CurrentBuffer);
    if (Volume > EFI_AUDIO_IO_PROTOCOL_MAX_VOLUME)
        Volume = EFI_AUDIO_IO_PROTOCOL_MAX_VOLUME;
    DeviceVolume = (UINT8)Volume;

    // Set volume variable.
    Status = gRT->SetVariable(BOOT_CHIME_VAR_VOLUME, &gBootChimeVendorVariableGuid, BOOT_CHIME_VAR_ATTRIBUTES,
        sizeof(DeviceVolume), &DeviceVolume);
    if (EFI_ERROR(Status))
        return Status;

    // Success.
    Print(L"Volume set to %u%%\n", DeviceVolume);
    return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
TestOutput(VOID) {
    // Create variables.
    EFI_STATUS Status;
    EFI_AUDIO_IO_PROTOCOL *AudioIo;
    UINTN OutputIndex;
    UINT8 OutputVolume;

    // Get stored output.
    Status = BootChimeGetStoredOutput(&AudioIo, &OutputIndex);
    if (EFI_ERROR(Status)) {
        if (Status == EFI_NOT_FOUND) {
            Print(L"Couldn't find stored output, using default output.\n");
            Status = BootChimeGetDefaultOutput(&AudioIo, &OutputIndex);
            if (EFI_ERROR(Status))
                return Status;
        } else {
            return Status;
        }
    }

    // Get stored volume.
    Status = BootChimeGetStoredVolume(&OutputVolume);
    if (EFI_ERROR(Status)) {
        if (Status == EFI_NOT_FOUND) {
            Print(L"Couldn't find stored volume, using default volume.\n");
            OutputVolume = BOOT_CHIME_DEFAULT_VOLUME;
            Status = EFI_SUCCESS;
        } else {
            return Status;
        }
    }

    // Setup playback.
    Print(L"Playing back audio...\n");
    Status = AudioIo->SetupPlayback(AudioIo, OutputIndex, OutputVolume, ChimeDataFreq, ChimeDataBits, ChimeDataChannels);
    if (EFI_ERROR(Status))
        return Status;

    // Play chime.
    return AudioIo->StartPlayback(AudioIo, ChimeData, ChimeDataLength, 0);
}

EFI_STATUS
EFIAPI
ClearVars(VOID) {
    // Create variables.
    EFI_STATUS Status;

    // Delete variables.
    Print(L"Clearing variables...\n");
    Status = gRT->SetVariable(BOOT_CHIME_VAR_DEVICE, &gBootChimeVendorVariableGuid, BOOT_CHIME_VAR_ATTRIBUTES, 0, NULL);
    if (EFI_ERROR(Status) && (Status != EFI_NOT_FOUND))
        return Status;
    Status = gRT->SetVariable(BOOT_CHIME_VAR_INDEX, &gBootChimeVendorVariableGuid, BOOT_CHIME_VAR_ATTRIBUTES, 0, NULL);
    if (EFI_ERROR(Status) && (Status != EFI_NOT_FOUND))
        return Status;
    Status = gRT->SetVariable(BOOT_CHIME_VAR_VOLUME, &gBootChimeVendorVariableGuid, BOOT_CHIME_VAR_ATTRIBUTES, 0, NULL);
    if (EFI_ERROR(Status) && (Status != EFI_NOT_FOUND))
        return Status;

    // Success.
    Print(L"Variables cleared!\n");
    return EFI_SUCCESS;
}

VOID
EFIAPI
DisplayMenu(VOID) {
    // Clear screen.
    if (gST->ConOut)
        gST->ConOut->ClearScreen(gST->ConOut);

    // Print menu.
    Print(L"\n");
    Print(L"Configures the BootChimeDxe EFI driver.\n");
    Print(L"=========================================\n");
    Print(L"%c - List all audio outputs\n", BCFG_ARG_LIST);
    Print(L"%c - Select audio output\n", BCFG_ARG_SELECT);
    Print(L"%c - Change volume\n", BCFG_ARG_VOLUME);
    Print(L"%c - Test current audio output\n", BCFG_ARG_TEST);
    Print(L"%c - Clear stored NVRAM variables\n", BCFG_ARG_CLEAR);
    Print(L"%c - Quit\n", BCFG_ARG_QUIT);
    Print(L"\n");
    Print(L"Enter an option: ");
}

EFI_STATUS
EFIAPI
BootChimeCfgMain(
    IN EFI_HANDLE ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable) {

    // Create variables.
    EFI_STATUS Status;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *SimpleTextIn = NULL;
    BOOLEAN Backspace;
    CHAR16 KeyValue;
    CHAR16 Selection;

    // Devices.
    BOOT_CHIME_DEVICE *Devices = NULL;
    UINTN DevicesCount = 0;

    // Ensure ConIn is valid.
    if (!gST->ConIn) {
        Print(L"There is no console input device.\n");
        Status = EFI_UNSUPPORTED;
        goto DONE;
    }
    SimpleTextIn = gST->ConIn;

    // Get devices.
    Status = GetOutputDevices(&Devices, &DevicesCount);
    if (EFI_ERROR(Status))
        goto DONE;

    // Command loop.
    while (TRUE) {
        // Show menu.
        DisplayMenu();

        // Flush any keystrokes.
        FlushKeystrokes(SimpleTextIn);

        // Handle keyboard input.
        Selection = 0;
        while (TRUE) {
            // Wait for key.
            Status = WaitForKey(SimpleTextIn, &KeyValue);
            if (EFI_ERROR(Status))
                goto DONE;
            Backspace = (KeyValue == L'\b');

            // If we are backspacing, clear selection.
            if (Selection && Backspace) {
                Selection = 0;
                Print(L"\b \b");
                continue;
            }

            // If enter, break out.
            if (KeyValue == L'\r')
                break;

            // If lowercase letter, convert to uppercase.
            if ((KeyValue >= 'a') && (KeyValue <= 'z'))
                KeyValue -= 32;

            // If not a letter, ignore.
            if (!Backspace && ((KeyValue < L'A') || (KeyValue > 'Z')))
                continue;

            // If no selection, we don't want to backspace.
            // If we already have a selection, don't accept any more.
            if ((!Selection && Backspace) || Selection)
                continue;

            // Get selection.
            Selection = KeyValue;
            Print(L"%c", Selection);
        }
        Print(L"\n\n");

        // Flush any keystrokes.
        FlushKeystrokes(SimpleTextIn);

        // Execute command.
        switch (Selection) {
            // List devices.
            case BCFG_ARG_LIST:
                Status = PrintDevices(SimpleTextIn, Devices, DevicesCount);
                if (EFI_ERROR(Status))
                    goto DONE;
                break;

            // Select device.
            case BCFG_ARG_SELECT:
                Status = SelectDevice(SimpleTextIn, Devices, DevicesCount);
                if (EFI_ERROR(Status))
                    goto DONE;
                break;

            // Select volume.
            case BCFG_ARG_VOLUME:
                Status = SelectVolume(SimpleTextIn);
                if (EFI_ERROR(Status))
                    goto DONE;
                break;

            // Test playback.
            case BCFG_ARG_TEST:
                Status = TestOutput();
                if (EFI_ERROR(Status))
                    goto DONE;
                break;

            // Clear variables.
            case BCFG_ARG_CLEAR:
                Status = ClearVars();
                if (EFI_ERROR(Status))
                    goto DONE;
                break;

            // Quit.
            case BCFG_ARG_QUIT:
                Status = EFI_SUCCESS;
                goto DONE;

            default:
                Print(L"Invalid option.\n");
        }

        // Wait for keystroke.
        Print(L"\n");
        Print(PROMPT_ANY_KEY);
        WaitForKey(SimpleTextIn, &KeyValue);
        FlushKeystrokes(SimpleTextIn);
    }

DONE:
    if (Devices)
        FreePool(Devices);

    // Show error.
    if (EFI_ERROR(Status)) {
        if (Status == EFI_NOT_FOUND)
            Print(L"No audio outputs were found. Ensure AudioDxe is loaded.\n", Status);
        else
            Print(L"The command failed with error: %r\n", Status);
    }
    return Status;
}
