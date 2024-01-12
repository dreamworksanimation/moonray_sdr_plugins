# Copyright 2023-2024 DreamWorks Animation LLC
# SPDX-License-Identifier: Apache-2.0

# TODO: setting this/these should not be necessary before calling find_package.
# see: https://stackoverflow.com/questions/55217771/boostcmake-no-suitable-build-variant
set(BUILD_SHARED_LIBS ON)
find_package(Boost REQUIRED)

# the DWA installation of pxrConfig.cmake lives in
# the usd_imaging Rez package. To work correctly, it
# requires env vars REZ_USD_CORE_ROOT, REZ_USD_IMAGING_ROOT
# and PXR_INCLUDE_DIRS to be set (e.g. in CMake preset).
# A standard installation should not require these...
find_package(pxr REQUIRED)

find_package(Python REQUIRED
    COMPONENTS
        Development)

add_subdirectory(moonrayShaderDiscovery)
add_subdirectory(moonrayShaderParser)
