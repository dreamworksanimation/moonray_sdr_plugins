# Copyright 2023-2024 DreamWorks Animation LLC
# SPDX-License-Identifier: Apache-2.0

set(component moonrayShaderDiscovery)

set(buildIncludeDir ${CMAKE_CURRENT_SOURCE_DIR})

add_library(${component} SHARED "")

# no "lib" prefix on the plugin lib
set_target_properties(${component} PROPERTIES PREFIX "")

target_sources(${component}
    PRIVATE
        discoveryPlugin.cpp
        moduleDeps.cpp
)

target_include_directories(${component}
    PUBLIC
        $<BUILD_INTERFACE:${buildIncludeDir}>
        $<INSTALL_INTERFACE:include/hdMoonray>
)

target_link_libraries(${component}
    PUBLIC
        # pxr
        ar ndr sdr
        Boost::headers
        # Python3::Module
)

if(IsDarwinPlatform)
    target_compile_features(${component} PRIVATE cxx_std_17)

    target_compile_definitions(${component}
    PUBLIC
        # Need std::unary_functions
        _LIBCPP_ENABLE_CXX17_REMOVED_FEATURES=1)

else()
    # Use RUNPATH instead of RPATH
    target_link_options(${component} PRIVATE ${GLOBAL_LINK_FLAGS})
endif()

# Configure plugInfo.json file
set(plugInfoTemplate ${CMAKE_CURRENT_SOURCE_DIR}/plugInfo.json.in)
set(plugInfoFile ${CMAKE_CURRENT_BINARY_DIR}/plugInfo.json)
set(LIBRARY_PATH ../../${component}${CMAKE_SHARED_LIBRARY_SUFFIX})
configure_file(${plugInfoTemplate} ${plugInfoFile})

# -------------------------------------
# Install the target and the export set
# -------------------------------------
include(GNUInstallDirs)

# install the target
# .so is installed under plugin, so that it can
# use the RUNPATH ../lib64
install(TARGETS ${component}
    COMPONENT ${component}
    LIBRARY
        DESTINATION plugin
        NAMELINK_SKIP
    RUNTIME
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${plugInfoFile}
        DESTINATION plugin/usd/moonrayShaderDiscovery
)

