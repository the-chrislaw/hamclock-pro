# CMake toolchain for the Qt-free core on iOS (arm64), used by the iPhone/iPad
# widget extension (apple/widgets): the Simulator by default, devices with
# -DCMAKE_OSX_SYSROOT=iphoneos. Same core as tvOS (cmake/TvOSCore). Needs Xcode.
# cmake -S cmake/TvOSCore -B build-ios-core -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/IOSCoreToolchain.cmake
set(CMAKE_SYSTEM_NAME iOS)
if(NOT CMAKE_OSX_SYSROOT)
    set(CMAKE_OSX_SYSROOT iphonesimulator)
endif()
set(CMAKE_OSX_ARCHITECTURES arm64)
# Matches IPHONEOS_DEPLOYMENT_TARGET of the widget extension (apple/widgets/project.yml).
set(CMAKE_OSX_DEPLOYMENT_TARGET 17.0)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO)
