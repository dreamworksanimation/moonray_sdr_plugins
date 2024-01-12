// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "parserPlugin.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/js/value.h"
#include "pxr/base/js/json.h"
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec2d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/vec4f.h>
#include <pxr/base/gf/vec4d.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/vt/value.h>
#include <pxr/base/vt/array.h>

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ndr/nodeDiscoveryResult.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"

#include <iostream>
#include <fstream>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

TfToken nullSceneObjectPtr;

// Get the most specific available SdrPropertyType token for an RDL type.
// We can also return a fixed array size : in some cases
// SdrProperty will internally map an array to a single Sdf type
// (e.g. float[2] will map to float2)
std::pair<TfToken,  // SdrPropertyType
          size_t>   // array size
getSdrTypeAndSize(const std::string& attrType)
{
    // The rdl "vector" types are dynamic arrays, and therefore
    // get the same conversion as the base type. The metadata
    // value SdrPropertyMetadata->IsDynamicArray will mark
    // them as dynamic arrays

    if (attrType == "Bool" || attrType == "BoolVector" ||
        attrType == "Int" || attrType == "IntVector" ||
        attrType == "Long" || attrType == "LongVector") {
        return std::pair<TfToken,size_t>(SdrPropertyTypes->Int,0);
    }
    if (attrType == "Float" || attrType == "FloatVector" ||
        attrType == "Double" || attrType == "DoubleVector") {
        return std::pair<TfToken,size_t>(SdrPropertyTypes->Float,0);
    }
    if (attrType == "String" || attrType == "StringVector") {
        return std::pair<TfToken,size_t>(SdrPropertyTypes->String,0);
    }
    if (attrType == "Rgb" || attrType == "RgbVector") {
        return std::pair<TfToken,size_t>(SdrPropertyTypes->Float,3);
    }
    if (attrType == "Rgba" || attrType == "RgbaVector") {
        return std::pair<TfToken,size_t>(SdrPropertyTypes->Float,4);
     }
    if (attrType == "Vec2f" || attrType == "Vec2fVector" ||
        attrType == "Vec2d" || attrType == "Vec2dVector") {
        return std::pair<TfToken,size_t>(SdrPropertyTypes->Float,2);
    }
    if (attrType == "Vec3f" || attrType == "Vec3fVector" ||
        attrType == "Vec3d" || attrType == "Vec3dVector") {
        return std::pair<TfToken,size_t>(SdrPropertyTypes->Float,3);
    }
    if (attrType == "Vec4f" || attrType == "Vec4fVector" ||
        attrType == "Vec4d" || attrType == "Vec4dVector") {
        return std::pair<TfToken,size_t>(SdrPropertyTypes->Float,4);
    }
    if (attrType == "Mat4f" || attrType == "Mat4fVector" ||
        attrType == "Mat4d" || attrType == "Mat4dVector") {
        return std::pair<TfToken,size_t>(SdrPropertyTypes->Matrix,0);
    }
    return std::pair<TfToken,size_t>(SdrPropertyTypes->Unknown,0);
}

// Convert a default value in JSON to VtValue
template <typename T>
VtValue convertVector(const JsValue& val,
                      const std::string& baseType);
VtValue convertDefault(const JsValue& val,
                       const std::string& attrType)
{
    if (attrType == "Bool") return VtValue(val.GetBool() ? 0 : 1);
    if (attrType == "Int") return VtValue(val.GetInt());
    if (attrType == "Long") return VtValue(val.GetInt64());
    if (attrType == "Float") return VtValue((float)val.GetReal());
    if (attrType == "Double") return VtValue(val.GetReal());
    if (attrType == "String") return VtValue(val.GetString());
    if (attrType == "Rgb" || attrType == "Vec3f") {
        std::vector<double> v = val.GetArrayOf<double>();
        return VtValue(GfVec3f((float)v.at(0), (float)v.at(1), (float)v.at(2)));
    }
    if (attrType == "Rgba" || attrType == "Vec4f") {
        std::vector<double> v = val.GetArrayOf<double>();
        return VtValue(GfVec4f((float)v.at(0), (float)v.at(1), (float)v.at(2),(float)v.at(3)));
    }
    if (attrType == "Vec2f") {
        std::vector<double> v = val.GetArrayOf<double>();
        return VtValue(GfVec2f((float)v.at(0), (float)v.at(1)));
    }
    if (attrType == "Vec2d") {
        std::vector<double> v = val.GetArrayOf<double>();
        return VtValue(GfVec2d(v.at(0), v.at(1)));
    }
    if (attrType == "Vec3d") {
        std::vector<double> v = val.GetArrayOf<double>();
        return VtValue(GfVec3d(v.at(0), v.at(1),v.at(2)));
    }
    if (attrType == "Vec4d") {
        std::vector<double> v = val.GetArrayOf<double>();
        return VtValue(GfVec4d(v.at(0), v.at(1),v.at(2),v.at(3)));
    }
    if (attrType == "Mat4f") {
        std::vector<std::vector<double>> data;
        const JsArray& arr = val.GetJsArray();
        for (const JsValue& val : arr) {
            data.emplace_back(val.GetArrayOf<double>());
        }
        return VtValue(GfMatrix4f(data));
    }
    if (attrType == "Mat4d") {
        std::vector<std::vector<double>> data;
        const JsArray& arr = val.GetJsArray();
        for (const JsValue& val : arr) {
            data.emplace_back(val.GetArrayOf<double>());
        }
        return VtValue(GfMatrix4d(data));
    }
    if (attrType == "SceneObject*") {
        // can't initialize to anything except null
        return VtValue(nullSceneObjectPtr);
    }
    if (attrType == "BoolVector") return convertVector<int>(val,"Bool");
    if (attrType == "IntVector") return convertVector<int>(val,"Int");
    if (attrType == "LongVector") return convertVector<int64_t>(val,"Long");
    if (attrType == "FloatVector") return convertVector<float>(val,"Float");
    if (attrType == "DoubleVector") return convertVector<double>(val,"Double");
    if (attrType == "StringVector") return convertVector<std::string>(val,"String");
    if (attrType == "RgbVector") return convertVector<GfVec3f>(val,"Rgb");
    if (attrType == "Vec3fVector") return convertVector<GfVec3f>(val,"Vec3f");
    if (attrType == "RgbaVector") return convertVector<GfVec3f>(val,"Rgba");
    if (attrType == "Vec4f") return convertVector<GfVec4f>(val,"Vec4f");
    if (attrType == "Vec2fVector") return convertVector<GfVec2f>(val,"Vec2f");
    if (attrType == "Vec2dVector") return convertVector<GfVec2d>(val,"Vec2d");
    if (attrType == "Vec3dVector") return convertVector<GfVec3d>(val,"Vec3d");
    if (attrType == "Vec4fVector") return convertVector<GfVec4f>(val,"Vec4f");
    if (attrType == "Vec4dVector") return convertVector<GfVec4d>(val,"Vec4d");
    if (attrType == "Mat4fVector") return convertVector<GfMatrix4f>(val,"Mat4f");
    if (attrType == "Mat4dVector") return convertVector<GfMatrix4d>(val,"Mat4d");
    if (attrType == "SceneObjectVector" || attrType == "SceneObjectIndexable")
        return convertVector<TfToken>(val,"SceneObject");
    return VtValue();
}

template<typename T>
VtValue convertVector(const JsValue& val,
                       const std::string& baseType)
{
    VtArray<T> arrayOut;
    const JsArray& arrayIn = val.GetJsArray();
    for (const JsValue& elem : arrayIn) {
        VtValue vtElem = convertDefault(elem,baseType);
        arrayOut.push_back(vtElem.Get<T>());
    }
    return VtValue(arrayOut);
}

bool isDynamicVector(const std::string& type)
{
    return (type.size() > 6) &&
        (type.substr(type.size()-6,std::string::npos) == "Vector");
}

const TfToken getNodeContext(const JsObject& definition)
{
    std::string nodeType = definition.at("type").GetString();
    // map supported types to those defined in SdrNode.h
    if (nodeType == "Material") return SdrNodeContext->Surface;
    if (nodeType == "Volume") return SdrNodeContext->Volume;
    if (nodeType == "Map") return SdrNodeContext->Pattern;
    if (nodeType == "Light") return SdrNodeContext->Light;
    if (nodeType == "LightFilter") return SdrNodeContext->LightFilter;
    if (nodeType == "Displacement") return SdrNodeContext->Displacement;
    // otherwise use the moonray name directly
    return TfToken(nodeType);
}

NdrTokenMap getNodeMetadata(const NdrTokenMap &baseMetadata,
                            const JsObject& definition)
{
    // we don't have any special metadata
    return baseMetadata;
}

SdrShaderProperty* makeOutputProperty(const std::string& nodeType)
{
    if (nodeType == "Material" || nodeType == "Volume") {
        return new SdrShaderProperty(TfToken("out"), SdrPropertyTypes->Terminal, VtValue(TfToken()),
                                     true, 0, NdrTokenMap(), NdrTokenMap(), NdrOptionVec());
    }
    if (nodeType == "Map" || nodeType == "Displacement") {
        return new SdrShaderProperty(TfToken("out"), SdrPropertyTypes->Float, VtValue(GfVec3f(0,0,0)),
                                     true, 3, NdrTokenMap(), NdrTokenMap(), NdrOptionVec());
    }
    return nullptr;
}

NdrPropertyUniquePtrVec
getNodeProperties(const NdrNodeDiscoveryResult& discoveryResult,
                  const JsObject& definition)
{
    // groups are defined by listing the attributes in them : we need
    // the inverse map to get the group for each attribute
    std::map<std::string,std::string> attrNameToGroup;
    try {  // sometimes no grouping is defined
         const JsObject& groups = definition.at("grouping").GetJsObject().at("groups").GetJsObject();
        for (const auto& group : groups) {
            const std::string& groupName = group.first;
            std::vector<std::string> attrsInGroup = group.second.GetArrayOf<std::string>();
            for (const std::string& attrName : attrsInGroup) {
                attrNameToGroup[attrName] = groupName;
            }
        }
    } catch (std::out_of_range&) {
        // no grouping data is ok
    }

    size_t numAttributes = 0;
    JsObject attributes;
    if (!definition.at("attributes").IsNull()) {
        // it is possible for a shader to have no attributes
        attributes = definition.at("attributes").GetJsObject();
        numAttributes = attributes.size();
    }
    NdrPropertyUniquePtrVec properties(numAttributes);

    for (const auto& attribute : attributes) {
        const std::string& attrName = attribute.first;
        const JsObject& attrData = attribute.second.GetJsObject();
        const std::string& attrType = attrData.at("attrType").GetString();
        const JsValue& attrDefault = attrData.at("default");

        TfToken sdrType;
        size_t arraySize;
        std::tie(sdrType,arraySize) = getSdrTypeAndSize(attrType);

        VtValue propDefault = convertDefault(attrDefault,attrType);

        NdrTokenMap metadata;
        auto mdIt = attrData.find("metadata");
        if (mdIt != attrData.end()) {
            const JsObject& attrMetadata = mdIt->second.GetJsObject();
            mdIt = attrMetadata.find("label");
            if (mdIt != attrMetadata.end()) metadata[SdrPropertyMetadata->Label] = mdIt->second.GetString();
            mdIt = attrMetadata.find("comment");
            if (mdIt != attrMetadata.end()) metadata[SdrPropertyMetadata->Help] = mdIt->second.GetString();
        }

        // "page" metadata is set from group name
        auto groupIt = attrNameToGroup.find(attrName);
        if (groupIt != attrNameToGroup.end()) {
            metadata[SdrPropertyMetadata->Page] = groupIt->second;
        }

        if (isDynamicVector(attrType))
            metadata[SdrPropertyMetadata->IsDynamicArray] = TfToken("true");

        auto bindIt = attrData.find("bindable");
        if (bindIt != attrData.end() &&
            bindIt->second.GetBool()) {
            metadata[SdrPropertyMetadata->Connectable] = TfToken("true");
        } else {
            // default is connectable, so must set to false if it isn't
            metadata[SdrPropertyMetadata->Connectable] = TfToken("false");
        }
 
        auto fileIt = attrData.find("filename");
        if (fileIt != attrData.end() &&
            fileIt->second.GetBool()) {
            metadata[SdrPropertyMetadata->IsAssetIdentifier] = TfToken("true");
            // probably a bug : shaderMetadataHelpers.cpp identifies assets
            // using the Widget metadata instead of "IsAssetIdentifier".
            // without this, the default value will not be correctly conformed to
            // an SdfAssetPath
            metadata[SdrPropertyMetadata->Widget] = TfToken("fileInput");
        }

        // we don't have any additional UI hints
        NdrTokenMap hints;

        NdrOptionVec options;
        auto enumIt = attrData.find("enum");
        if (enumIt != attrData.end()) {
            // type for an enum should be string (per Usd), not int (per RDL)
            sdrType = SdrPropertyTypes->String;
            // we will also need to update propDefault...
            int dfltInt = propDefault.Get<int>();
            const JsObject& enumItems = enumIt->second.GetJsObject();
            for (const auto& option : enumItems) {
                // RDL enums have int values, whereas Sdr
                // uses strings, so we have to leave it to the shader
                // implementation to look up the strings...
                TfToken name(option.first);
                options.emplace_back(name,name);
                if (option.second.GetInt() == dfltInt) {
                    propDefault = VtValue(name.GetText());
                }
            }
        }

        int index = attrData.at("order").GetInt();
        properties.at(index) = SdrShaderPropertyUniquePtr(
            new SdrShaderProperty(
                TfToken(attrName),
                sdrType,
                propDefault,
                false,    // is output
                arraySize,
                metadata,
                hints,
                options)
            );
    }

    SdrShaderProperty *output = makeOutputProperty(definition.at("type").GetString());
    if (output) {
        properties.push_back(SdrShaderPropertyUniquePtr(output));
    }
    return properties;
}

} // namespace {

NDR_REGISTER_PARSER_PLUGIN(MoonrayParserPlugin);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Discovery and source type
    ((discoveryType, "moonrayClass"))
    ((sourceType, "moonrayClass"))

);

const NdrTokenVec&
MoonrayParserPlugin::GetDiscoveryTypes() const
{
    static const NdrTokenVec _DiscoveryTypes = {_tokens->discoveryType};
    return _DiscoveryTypes;
}

const TfToken&
MoonrayParserPlugin::GetSourceType() const
{
    return _tokens->sourceType;
}

NdrNodeUniquePtr
MoonrayParserPlugin::Parse(const NdrNodeDiscoveryResult& discoveryResult)
{
    if (discoveryResult.uri.empty()) {
        TF_WARN("Invalid NdrNodeDiscoveryResult with identifier %s: uri is empty.",
                discoveryResult.identifier.GetText());
         return NdrParserPlugin::GetInvalidNode(discoveryResult);
    }

#if AR_VERSION == 1
    // Get the resolved URI to a location that it can be read
    bool localFetchSuccessful = ArGetResolver().FetchToLocalResolvedPath(
        discoveryResult.uri,
        discoveryResult.resolvedUri
        );

    if (!localFetchSuccessful) {
        TF_WARN("Could not localize the Moonray shader definition at URI [%s] into a local path.",
                discoveryResult.uri.c_str());
        return NdrParserPlugin::GetInvalidNode(discoveryResult);
    }
#endif

    // load the json file
    std::ifstream ifs(discoveryResult.resolvedUri);
    if (ifs.fail()) {
        TF_WARN("Could not open the Moonray shader definition at URI [%s]. ",
                discoveryResult.resolvedUri.c_str());
        return NdrParserPlugin::GetInvalidNode(discoveryResult);
    }

    try {
        JsParseError error;
        JsValue jsDef = JsParseStream(ifs,&error);
        if (jsDef.IsNull()) {
            TF_WARN("JSON error parsing Moonray shader definition at URI [%s]: line %d col %d : %s",
                    discoveryResult.resolvedUri.c_str(),
                    error.line,error.column,error.reason.c_str());
            return NdrParserPlugin::GetInvalidNode(discoveryResult);
        }

        const JsObject& definition = jsDef.GetJsObject().at("scene_classes").
            GetJsObject().at(discoveryResult.name).GetJsObject();
        return NdrNodeUniquePtr(new SdrShaderNode(
                                    discoveryResult.identifier,
                                    discoveryResult.version,
                                    discoveryResult.name,
                                    discoveryResult.family,
                                    getNodeContext(definition),
                                    _tokens->sourceType,
                                    discoveryResult.uri,
                                    discoveryResult.resolvedUri,
                                    getNodeProperties(discoveryResult,definition),
                                    getNodeMetadata(discoveryResult.metadata,definition),
                                    discoveryResult.sourceCode));
    } catch (std::exception& e) {
        TF_WARN("Could not parse the Moonray shader definition at URI [%s] : [%s]"
                "An invalid Sdr node definition will be created.",
                e.what(), discoveryResult.resolvedUri.c_str());
    }
    return NdrParserPlugin::GetInvalidNode(discoveryResult);
}

PXR_NAMESPACE_CLOSE_SCOPE
