// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#ifndef PXR_USD_PLUGIN_MOONRAY_DISCOVERY_PLUGIN_H
#define PXR_USD_PLUGIN_MOONRAY_DISCOVERY_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"

#include "pxr/usd/ndr/declare.h"
#include "pxr/usd/ndr/discoveryPlugin.h"
#include "pxr/usd/ndr/parserPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

class MoonrayDiscoveryPlugin : public NdrDiscoveryPlugin {
public:
    MoonrayDiscoveryPlugin();

    ~MoonrayDiscoveryPlugin() override = default;

    virtual NdrNodeDiscoveryResultVec DiscoverNodes(const Context &context)
        override;

    virtual const NdrStringVec& GetSearchURIs() const override;

private:
    NdrStringVec _searchPaths;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
