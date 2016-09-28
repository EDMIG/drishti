#!/bin/bash

. ${DRISHTISDK}/bin/build-common.sh

DRISTHI_ASAN=1

if [ $DRISHTI_ASAN -gt 0 ]; then

    TOOLCHAIN=osx-10-11-sanitize-address
    DRISHTI_BUILD_SHARED_SDK=OFF
    
    # TODO : retrieve this more programmatically ?
    export DYLD_LIBRARY_PATH=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/7.0.2/lib/darwin
else

    DRISHTI_BUILD_SHARED_SDK=ON
    TOOLCHAIN=xcode-sections
fi

EXTRA_ARGS=""
if [ $# -ge 1 ]; then
    EXTRA_ARGS="--reconfig"
    #EXTRA_ARGS="--clear"
fi

#DRISHTI_CONFIGURATION=Release
DRISHTI_CONFIGURATION=MinSizeRel
DRISHTI_BUILD_QT=OFF
DRISHTI_BUILD_OGLES_GPGPU=OFF
DRISHTI_USE_TEXT_ARCHIVES=OFF
DRISHTI_BUILD_C_INTERFACE=ON
DRISHTI_BUILD_WORLD=ON

rename_tab drishti ${TOOLCHAIN}

COMMAND=(
    "--verbose --fwd "
    "HUNTER_CONFIGURATION_TYPES=${DRISHTI_CONFIGURATION} "
    "${DRISHTI_BUILD_ARGS[*]} "
    "${DRISHTI_BUILD_HIDE[*]} "
    "CMAKE_XCODE_ATTRIBUTE_OSX_DEPLOYMENT_TARGET=10.10 "
    "CMAKE_OSX_DEPLOYMENT_TARGET=10.10 "
    "CMAKE_OSX_SYSROOT=${CMAKE_OSX_SYSROOT} "
    "DRISHTI_BUILD_QT=${DRISHTI_BUILD_QT} "
    "DRISHTI_BUILD_OGLES_GPGPU=${DRISHTI_BUILD_OGLES_GPGPU} "
    "DRISHTI_BUILD_MIN_SIZE=${DRISHTI_BUILD_MIN_SIZE} "
    "DRISHTI_USE_TEXT_ARCHIVES=${DRISHTI_USE_TEXT_ARCHIVES} "
    "DRISHTI_BUILD_C_INTERFACE=${DRISHTI_BUILD_C_INTERFACE} "
    "DRISHTI_BUILD_WORLD=${DRISHTI_BUILD_WORLD} "
    "DRISHTI_BUILD_SHARED_SDK=${DRISHTI_BUILD_SHARED_SDK} "
    "--config Release "
    "--jobs 8 "
    "--open "
    "--install "
    "${EXTRA_ARGS} " " --test"
)

build.py --toolchain ${TOOLCHAIN} ${COMMAND[*]}


