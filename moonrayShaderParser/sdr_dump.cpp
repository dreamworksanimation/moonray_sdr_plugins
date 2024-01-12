// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

/// @file sdr_dump.cc

// show contents of sdr registry


#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/sdr/declare.h>
#include <pxr/usd/sdr/shaderProperty.h>

#include <string>
#include <iostream>

using namespace pxr;

const char* TABSTR = "    ";

int usage(const char* prog)
{
    std::cout << "Usage:" << std::endl;
    std::cout << "    " << prog << " CLASSNAME" << std::endl;
    return -1;
}
void dumpProperty(SdrShaderPropertyConstPtr prop)
{
    if (!prop) {
        std::cout << "** BAD PROPERTY **" << std::endl;
        return;
    }
    SdfValueTypeName sdfTypeName;
    TfToken backupType;
    std::tie<SdfValueTypeName,TfToken>(sdfTypeName,backupType) = prop->GetTypeAsSdfType();
    TfToken sdfType = sdfTypeName.GetAsToken();
    TfToken propType = prop->GetType();
    const NdrOptionVec& options = prop->GetOptions();
    if (!options.empty()) propType = TfToken("enum");

    std::cout << TABSTR;
    if (prop->IsConnectable()) std::cout << "<-> ";
    if (prop->IsOutput()) std::cout << "O ";
    std::cout << propType;
    if (prop->GetArraySize() > 0) std::cout << "["
                                            << prop->GetArraySize() << "]";
    if (prop->IsDynamicArray()) std::cout << "[n]";
    std::cout << " " << prop->GetName() << " = " << prop->GetDefaultValue();
    std::cout << " (Sdf type: " << sdfType << ")" << std::endl;

    if (!options.empty()) {
        std::cout << TABSTR << TABSTR << "enum options: ";
        for (const auto& it : options) {
            std::cout << it.first << "; ";
        }
        std::cout << std::endl;
    }
    const NdrTokenMap&  metadata = prop->GetMetadata();
    for (const auto& item : metadata) {
        std::cout << TABSTR << TABSTR << "* " << item.first << " = " << item.second << std::endl;
    }
}

void dumpNode(SdrShaderNodeConstPtr node)
{
    std::cout << "Identifier: " << node->GetIdentifier() << std::endl
              << "Version:    " << node->GetVersion().GetString() << std::endl
              << "Name:       " << node->GetName() << std::endl
              << "Family:     " << node->GetFamily() << std::endl
              << "Context:    " << node->GetContext() << std::endl
              << "SourceType: " << node->GetSourceType() << std::endl
#if PXR_VERSION < 2008
              << "SourceURI:  " << node->GetSourceURI() << std::endl
              << "Resolved:   " << node->GetResolvedSourceURI() << std::endl
#else
              // these APIs changed in 0.20.8
              << "DefURI:     " << node->GetResolvedDefinitionURI() << std::endl
              << "ImplURI:    " << node->GetResolvedImplementationURI() << std::endl
#endif
              << "IsValid:    " << node->IsValid() << std::endl;

    std::cout << "INPUTS:" << std::endl;
    const NdrTokenVec& inputNames = node->GetInputNames();
    for (const TfToken& name : inputNames) {
        dumpProperty(node->GetShaderInput(name));
    }
    std::cout << "OUTPUTS:" << std::endl;
    const NdrTokenVec& outputNames = node->GetOutputNames();
    for (const TfToken& name : outputNames) {
        dumpProperty(node->GetShaderOutput(name));
    }
    std::cout << "METADATA:" << std::endl;
    const NdrTokenMap& metadata = node->GetMetadata();
    for (const auto& item : metadata) {
        std::cout << TABSTR << "* " << item.first << " = " << item.second << std::endl;
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) return usage(argv[0]);
    std::string name(argv[1]);
    SdrShaderNodeConstPtr node = SdrRegistry::GetInstance().GetShaderNodeByName(name);
    if (node) {
        dumpNode(node);
    } else {
        std::cout << "Cannot find a node called '" << name << "' "
                  << "in the sdr registry." << std::endl;
    }
    return 0;
}

