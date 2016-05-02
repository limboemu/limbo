#!/bin/bash
#
# This script generates 'WebP.framework'. An iOS app can decode WebP images
# by including 'WebP.framework'.
#
# Run ./iosbuild.sh to generate 'WebP.framework' under the current directory
# (previous build will be erased if it exists).
#
# This script is inspired by the build script written by Carson McDonald.
# (http://www.ioncannon.net/programming/1483/using-webp-to-reduce-native-ios-app-size/).

set -e

# Extract the latest SDK version from the final field of the form: iphoneosX.Y
declare -r SDK=$(xcodebuild -showsdks \
  | grep iphoneos | sort | tail -n 1 | awk '{print substr($NF, 9)}'
)
declare -r OLDPATH=${PATH}

# Add iPhoneOS-V6 to the list of platforms below if you need armv6 support.
# Note that iPhoneOS-V6 support is not available with the iOS6 SDK.
declare -r PLATFORMS="iPhoneSimulator iPhoneOS-V7 iPhoneOS-V7s"
declare -r SRCDIR=$(dirname $0)
declare -r TOPDIR=$(pwd)
declare -r BUILDDIR="${TOPDIR}/iosbuild"
declare -r TARGETDIR="${TOPDIR}/WebP.framework"
declare -r DEVELOPER=$(xcode-select --print-path)
declare -r PLATFORMSROOT="${DEVELOPER}/Platforms"
declare -r LIPO=$(xcrun -sdk iphoneos${SDK} -find lipo)
LIBLIST=''

if [[ -z "${SDK}" ]]; then
  echo "iOS SDK not available"
  exit 1
elif [[ ${SDK} < 4.0 ]]; then
  echo "You need iOS SDK version 4.0 or above"
  exit 1
else
  echo "iOS SDK Version ${SDK}"
fi

rm -rf ${BUILDDIR}
rm -rf ${TARGETDIR}
mkdir -p ${BUILDDIR}
mkdir -p ${TARGETDIR}/Headers/

[[ -e ${SRCDIR}/configure ]] || (cd ${SRCDIR} && sh autogen.sh)

for PLATFORM in ${PLATFORMS}; do
  if [[ "${PLATFORM}" == "iPhoneOS-V7s" ]]; then
    PLATFORM="iPhoneOS"
    ARCH="armv7s"
  elif [[ "${PLATFORM}" == "iPhoneOS-V7" ]]; then
    PLATFORM="iPhoneOS"
    ARCH="armv7"
  elif [[ "${PLATFORM}" == "iPhoneOS-V6" ]]; then
    PLATFORM="iPhoneOS"
    ARCH="armv6"
  else
    ARCH="i386"
  fi

  ROOTDIR="${BUILDDIR}/${PLATFORM}-${SDK}-${ARCH}"
  mkdir -p "${ROOTDIR}"

  export DEVROOT="${PLATFORMSROOT}/${PLATFORM}.platform/Developer"
  export SDKROOT="${DEVROOT}/SDKs/${PLATFORM}${SDK}.sdk"

  export CFLAGS="-arch ${ARCH} -pipe -isysroot ${SDKROOT}"
  export CXXFLAGS=${CFLAGS}
  export LDFLAGS="-arch ${ARCH} -pipe -isysroot ${SDKROOT}"
  export PATH="${DEVROOT}/usr/bin:${OLDPATH}"

  ${SRCDIR}/configure --host=${ARCH}-apple-darwin --prefix=${ROOTDIR} \
    --build=$(${SRCDIR}/config.guess) \
    --disable-shared --enable-static \
    --enable-libwebpdecoder --enable-swap-16bit-csp

  # run make only in the src/ directory to create libwebpdecoder.a
  cd src/
  make V=0
  make install

  LIBLIST+=" ${ROOTDIR}/lib/libwebpdecoder.a"

  make clean
  cd ..

  export PATH=${OLDPATH}
done

cp -a ${SRCDIR}/src/webp/* ${TARGETDIR}/Headers/
${LIPO} -create ${LIBLIST} -output ${TARGETDIR}/WebP
