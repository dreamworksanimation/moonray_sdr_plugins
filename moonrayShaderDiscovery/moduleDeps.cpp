// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "pxr/pxr.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/scriptModuleLoader.h"
#include "pxr/base/tf/token.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfScriptModuleLoader) {
    // List of direct dependencies for this library.
    const std::vector<TfToken> reqs = {
        TfToken("ar"),
        TfToken("ndr"),
        TfToken("sdr")
    };
    TfScriptModuleLoader::GetInstance().
        RegisterLibrary(TfToken("moonrayShaderDiscovery"), TfToken("pxr.MoonrayShaderDiscovery"), reqs);
}

PXR_NAMESPACE_CLOSE_SCOPE
