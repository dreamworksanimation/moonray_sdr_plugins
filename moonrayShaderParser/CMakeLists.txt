# Copyright 2023-2024 DreamWorks Animation LLC
# SPDX-License-Identifier: Apache-2.0

set(component moonrayShaderParser)

set(buildIncludeDir ${CMAKE_CURRENT_SOURCE_DIR})

add_library(${component} SHARED "")

# no "lib" prefix on the plugin lib
set_target_properties(${component} PROPERTIES PREFIX "")

target_sources(${component}
    PRIVATE
        parserPlugin.cpp
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
        Python::Module
)

# Use RUNPATH instead of RPATH
target_link_options(${component} PRIVATE -Wl,--enable-new-dtags)

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
install(FILES plugInfo.json
        DESTINATION plugin/usd/moonrayShaderParser)


