# CMake toolchain for tvOS (arm64): the Simulator by default, devices with
# -DCMAKE_OSX_SYSROOT=appletvos. Needs Xcode on macOS.
# cmake -S cmake/TvOSCore -B build-tvos -G Xcode -DCMAKE_TOOLCHAIN_FILE=cmake/TvOSToolchain.cmake
set(CMAKE_SYSTEM_NAME tvOS)
if(NOT CMAKE_OSX_SYSROOT)
    set(CMAKE_OSX_SYSROOT appletvsimulator)
endif()
set(CMAKE_OSX_ARCHITECTURES arm64)
set(CMAKE_OSX_DEPLOYMENT_TARGET 17.0)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO)
