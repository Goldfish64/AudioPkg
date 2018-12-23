#!/bin/bash

BUILDDIR=$(dirname "$0")
pushd "$BUILDDIR" >/dev/null
BUILDDIR=$(pwd)
popd >/dev/null

cd "$BUILDDIR"

updaterepo() {
    if [ ! -d "$3" ]; then
        git clone "$1" -b "$2" --depth=1 "$3" || exit 1
    fi
    pushd "$3" >/dev/null
    git pull
    popd >/dev/null
}

package() {
    if [ ! -d "$1" ]; then
        echo "Missing package directory"
        exit 1
    fi

    local ver=$(cat Platform/AudioDxe/AudioDxe.h | grep AUDIODXE_PKG_VERSION | cut -f6 -d' ')
    if [ "$(echo $ver | grep -E '^[0-9]+$')" = "" ]; then
        echo "Invalid version $ver"
    fi

    pushd "$1" || exit 1
    rm -rf tmp || exit 1
    mkdir -p tmp/Drivers || exit 1
    mkdir -p tmp/Tools || exit 1
    cp AudioDxe.efi tmp/Drivers/ || exit 1
    cp BootChimeDxe.efi tmp/Drivers/ || exit 1
    cp BootChimeCfg.efi tmp/Tools/ || exit 1
    cp HdaCodecDump.efi tmp/Tools/ || exit 1
    pushd tmp || exit 1
    zip -qry ../"AudioPkg-R${ver}-${2}.zip" * || exit 1
    popd || exit 1
    rm -rf tmp || exit 1
    popd || exit 1
}

if [ "$BUILDDIR" != "$(printf "%s\n" $BUILDDIR)" ]; then
  echo "EDK2 build system may still fail to support directories with spaces!"
  exit 1
fi

if [ "$(which gcc)" = "" ] || [ "$(which git)" = "" ] || [ "$(which nasm)" = "" ] || [ "$(which iasl)" = "" ] || [ "$(which zip)"]; then
  echo "One or more tools missing! Installing..."
  sudo apt update || exit 1
  sudo apt install build-essential uuid-dev iasl git gcc-5 nasm zip || exit 1
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

# Get mode.
if [ "$1" != "" ]; then
    MODE="$1"
    shift
fi

if [ ! -f UDK/UDK.ready ]; then
  rm -rf UDK
fi

# Clone EDK2.
updaterepo "https://github.com/tianocore/edk2" UDK2018 UDK || exit 1
cd UDK

# EDK2 can only build packages in its folder, so link AudioPkg there.
if [ ! -d AudioPkg ]; then
    ln -s .. AudioPkg || exit 1
fi

# Setup EDK2.
source edksetup.sh || exit 1
make -C BaseTools || exit 1
touch UDK.ready

# Build debug binaries.
if [ "$MODE" = "" ] || [ "$MODE" = "DEBUG" ]; then
    build -a X64 -b DEBUG -t GCC5 -p AudioPkg/AudioPkg.dsc || exit 1
fi

# Build release binaries.
if [ "$MODE" = "" ] || [ "$MODE" = "RELEASE" ]; then
    build -a X64 -b RELEASE -t GCC5 -p AudioPkg/AudioPkg.dsc || exit 1
fi

# Build packages.
cd .. || exit 1
if [ "$PACKAGE" = "" ] || [ "$PACKAGE" = "DEBUG" ]; then
    package "Binaries/DEBUG" "DEBUG" || exit 1
fi
if [ "$PACKAGE" = "" ] || [ "$PACKAGE" = "RELEASE" ]; then
    package "Binaries/RELEASE" "RELEASE" || exit 1
fi
