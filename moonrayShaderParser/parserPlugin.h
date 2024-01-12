// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#ifndef PXR_USD_PLUGIN_MOONRAY_PARSER_PLUGIN_H
#define PXR_USD_PLUGIN_MOONRAY_PARSER_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/base/tf/token.h"

#include "pxr/usd/ndr/declare.h"
#include "pxr/usd/ndr/parserPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

class NdrNode;
class NdrNodeDiscoveryResult;

class MoonrayParserPlugin : public NdrParserPlugin {
public:
    MoonrayParserPlugin() = default;

    ~MoonrayParserPlugin() override = default;

    NdrNodeUniquePtr Parse(const NdrNodeDiscoveryResult &discoveryResult)
        override;

    const NdrTokenVec &GetDiscoveryTypes() const override;

    const TfToken &GetSourceType() const override;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
