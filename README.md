# AudioPkg
[![Build Status](https://travis-ci.org/Goldfish64/AudioPkg.svg?branch=master)](https://travis-ci.org/Goldfish64/AudioPkg)

AudioPkg is a set of drivers/applications for supporting audio (currently only HD audio) under UEFI.

## AudioDxe ##
Main audio driver, supporting HD audio controllers and codecs. This driver exposes an instance of EFI_AUDIO_IO_PROTOCOL for each codec for consumption by other drivers and applications in system.

## BootChimeDxe ##
Driver that produces the signature Mac boot chime at the start of booting macOS. Hackintosh users might find this useful.

## HdaCodecDump ##
Application that aims to produce dump printouts of HD audio codecs in the system, similar to ALSA's dumps under `/proc/asound`. Still a work in progress.

## WaveLib ##
This library aims to provide simple WAVE file support.
