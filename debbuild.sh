#!/bin/bash

BUILDDIR=$(dirname "$0")
pushd "$BUILDDIR" >/dev/null
BUILDDIR=$(pwd)
popd >/dev/null

cd "$BUILDDIR"

if [ "$BUILDDIR" != "$(printf "%s\n" $BUILDDIR)" ]; then
  echo "EDK2 build system may still fail to support directories with spaces!"
  exit 1
fi

if [ "$(which gcc)" = "" ] || [ "$(which git)" = "" ] || [ "$(which nasm)" = "" ] || [ "$(which iasl)" = "" ]; then
  echo "One or more tools missing! Installing..."
  sudo apt update || exit 1
  sudo apt install build-essential uuid-dev iasl git gcc-5 nasm || exit 1
fi

if [ ! -d "Binaries" ]; then
  mkdir Binaries || exit 1
  cd Binaries || exit 1
  ln -s ../UDK/Build/AudioPkg/RELEASE_XCODE5/X64 RELEASE || exit 1
  ln -s ../UDK/Build/AudioPkg/DEBUG_XCODE5/X64 DEBUG || exit 1
  cd .. || exit 1
fi

if [ "$CONFIGURATION" != "" ]; then
  MODE="$(echo $CONFIGURATION | tr /a-z/ /A-Z/)"
fi

if [ "$1" != "" ]; then
  echo "Got action $1, skipping!"
  exit 0
fi

if [ ! -f UDK/UDK.ready ]; then
  rm -rf UDK
fi

git clone "https://github.com/tianocore/edk2" -b UDK2018 --depth=1 UDK || exit 1
cd UDK

if [ ! -d AudioPkg ]; then
  ln -s .. AudioPkg || exit 1
fi

source edksetup.sh || exit 1
make -C BaseTools || exit 1
touch UDK.ready

if [ "$MODE" = "" ] || [ "$MODE" = "DEBUG" ] || [ "$MODE" = "SANITIZE" ]; then
  build -a X64 -b DEBUG -t GCC5 -p AudioPkg/AudioPkg.dsc || exit 1
fi

if [ "$MODE" = "" ] || [ "$MODE" = "RELEASE" ]; then
  build -a X64 -b RELEASE -t GCC5 -p AudioPkg/AudioPkg.dsc || exit 1
fi
