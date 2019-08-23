# AudioPkg
[![Build Status](https://travis-ci.org/Goldfish64/AudioPkg.svg?branch=master)](https://travis-ci.org/Goldfish64/AudioPkg)

AudioPkg is a set of drivers/applications for supporting audio (currently only Intel HD audio) under UEFI.

**Note: This driver is still a work in progress and may change widely before final release.**

## AudioDxe
Main audio driver, supporting HD audio controllers and codecs. This driver exposes an instance of `EFI_AUDIO_IO_PROTOCOL` and `EFI_HDA_CODEC_INFO_PROTOCOL` for each codec for consumption by other drivers and applications in system.

## BootChimeDxe
Driver that produces the signature Mac boot chime at the start of booting macOS. Hackintosh users might find this useful.

##### Features
* Plays boot chime on macOS boot.efi load.
* A wave file at the root of the EFI partition will override the built-in chime data.
* Desired output device and volume is configurable with [BootChimeCfg](#BootChimeCfg), if these are not set the driver defaults to internal speakers or line out.

## BootChimeCfg
Application for configuring the output device and output volume used by [BootChimeDxe](#BootChimeDxe). Settings are stored in NVRAM for now.

## HdaCodecDump
Application that aims to produce dump printouts of HD audio codecs in the system, similar to ALSA's dumps under `/proc/asound`. Still a work in progress.

## WaveLib
This library aims to provide simple WAVE file support.

# Issues
While this driver has been tested on various codecs and controllers, it may still be broken for others. If you encounter a bug or problem with this driver, please use the issues feature in Github.

## Known issues
* HDMI or other digitial outputs don't work.
* Some stuttering on NVIDIA HDA controllers.
