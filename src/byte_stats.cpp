#include "byte_stats.h"

#include "rive/core/field_types/core_bool_type.hpp"
#include "rive/core/field_types/core_color_type.hpp"
#include "rive/core/field_types/core_double_type.hpp"
#include "rive/core/field_types/core_string_type.hpp"
#include "rive/core/field_types/core_uint_type.hpp"
#ifndef _RIVE_INTERNAL_
#define _RIVE_INTERNAL_
#endif
#include "rive/generated/core_registry.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace
{
enum class ByteCategory
{
    Keyframes,
    KeyedScaffolding,
    AnimationState,
    VectorGeometryPaint,
    SceneHierarchyConstraints,
    DataBindingViewModels,
    AssetRefsMetadataContents,
    Other,
};

struct TypeDescriptor
{
    std::string name;
    ByteCategory category;
};

class ByteReader
{
public:
    explicit ByteReader(std::vector<uint8_t> bytes) : m_bytes(std::move(bytes)) {}

    size_t position() const { return m_position; }
    size_t size() const { return m_bytes.size(); }
    bool reachedEnd() const { return m_position >= m_bytes.size(); }

    uint8_t readByte()
    {
        ensureAvailable(1);
        return m_bytes[m_position++];
    }

    uint32_t readUint32()
    {
        ensureAvailable(4);
        uint32_t value = static_cast<uint32_t>(m_bytes[m_position]) |
                         (static_cast<uint32_t>(m_bytes[m_position + 1]) << 8) |
                         (static_cast<uint32_t>(m_bytes[m_position + 2]) << 16) |
                         (static_cast<uint32_t>(m_bytes[m_position + 3]) << 24);
        m_position += 4;
        return value;
    }

    uint64_t readVarUint()
    {
        uint64_t result = 0;
        uint32_t shift = 0;
        for (int i = 0; i < 10; i++)
        {
            uint8_t byte = readByte();
            result |= static_cast<uint64_t>(byte & 0x7F) << shift;
            if ((byte & 0x80) == 0)
            {
                return result;
            }
            shift += 7;
        }
        throw std::runtime_error("varuint is too long");
    }

    void skip(size_t byteCount)
    {
        ensureAvailable(byteCount);
        m_position += byteCount;
    }

    void skipString()
    {
        auto length = readVarUint();
        skip(static_cast<size_t>(length));
    }

    void skipBytes()
    {
        auto length = readVarUint();
        skip(static_cast<size_t>(length));
    }

private:
    void ensureAvailable(size_t byteCount) const
    {
        if (m_position + byteCount > m_bytes.size())
        {
            throw std::runtime_error("unexpected end of file");
        }
    }

    std::vector<uint8_t> m_bytes;
    size_t m_position = 0;
};

std::vector<uint8_t> readFileBytes(const std::string& path)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
    {
        throw std::runtime_error("unable to open file");
    }
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(stream), {});
}

std::string categoryName(ByteCategory category)
{
    switch (category)
    {
        case ByteCategory::Keyframes:                 return "keyframes";
        case ByteCategory::KeyedScaffolding:          return "keyed object/property scaffolding";
        case ByteCategory::AnimationState:            return "animation/state machine objects";
        case ByteCategory::VectorGeometryPaint:       return "vector geometry/paint";
        case ByteCategory::SceneHierarchyConstraints: return "scene hierarchy/constraints";
        case ByteCategory::DataBindingViewModels:     return "data binding/view models";
        case ByteCategory::AssetRefsMetadataContents: return "asset refs/metadata/contents";
        case ByteCategory::Other:                     return "other";
    }
}

#define TYPE_DESC(Type, Category) \
    case rive::Type##Base::typeKey: return {#Type, ByteCategory::Category}

TypeDescriptor describeType(uint32_t typeKey)
{
    switch (typeKey)
    {
        TYPE_DESC(KeyFrameUint, Keyframes);
        TYPE_DESC(KeyFrameId, Keyframes);
        TYPE_DESC(KeyFrameBool, Keyframes);
        TYPE_DESC(KeyFrameString, Keyframes);
        TYPE_DESC(KeyFrameDouble, Keyframes);
        TYPE_DESC(KeyFrameColor, Keyframes);
        TYPE_DESC(KeyFrameCallback, Keyframes);

        TYPE_DESC(KeyedObject, KeyedScaffolding);
        TYPE_DESC(KeyedProperty, KeyedScaffolding);

        TYPE_DESC(ListenerFireEvent, AnimationState);
        TYPE_DESC(TransitionSelfComparator, AnimationState);
        TYPE_DESC(StateMachineFireTrigger, AnimationState);
        TYPE_DESC(TransitionValueTriggerComparator, AnimationState);
        TYPE_DESC(NestedSimpleAnimation, AnimationState);
        TYPE_DESC(AnimationState, AnimationState);
        TYPE_DESC(NestedTrigger, AnimationState);
        TYPE_DESC(Animation, AnimationState);
        TYPE_DESC(BlendAnimationDirect, AnimationState);
        TYPE_DESC(StateMachineNumber, AnimationState);
        TYPE_DESC(CubicValueInterpolator, AnimationState);
        TYPE_DESC(TransitionTriggerCondition, AnimationState);
        TYPE_DESC(StateMachineListener, AnimationState);
        TYPE_DESC(TransitionPropertyArtboardComparator, AnimationState);
        TYPE_DESC(TransitionPropertyViewModelComparator, AnimationState);
        TYPE_DESC(ListenerBoolChange, AnimationState);
        TYPE_DESC(ListenerAlignTarget, AnimationState);
        TYPE_DESC(TransitionNumberCondition, AnimationState);
        TYPE_DESC(TransitionValueBooleanComparator, AnimationState);
        TYPE_DESC(TransitionViewModelCondition, AnimationState);
        TYPE_DESC(TransitionArtboardCondition, AnimationState);
        TYPE_DESC(AnyState, AnimationState);
        TYPE_DESC(BlendState1DInput, AnimationState);
        TYPE_DESC(CubicInterpolatorComponent, AnimationState);
        TYPE_DESC(StateMachineLayer, AnimationState);
        TYPE_DESC(ListenerNumberChange, AnimationState);
        TYPE_DESC(CubicEaseInterpolator, AnimationState);
        TYPE_DESC(TransitionValueIdComparator, AnimationState);
        TYPE_DESC(StateTransition, AnimationState);
        TYPE_DESC(NestedBool, AnimationState);
        TYPE_DESC(StateMachine, AnimationState);
        TYPE_DESC(StateMachineFireEvent, AnimationState);
        TYPE_DESC(EntryState, AnimationState);
        TYPE_DESC(LinearAnimation, AnimationState);
        TYPE_DESC(StateMachineTrigger, AnimationState);
        TYPE_DESC(TransitionValueColorComparator, AnimationState);
        TYPE_DESC(ListenerTriggerChange, AnimationState);
        TYPE_DESC(BlendStateDirect, AnimationState);
        TYPE_DESC(ListenerViewModelChange, AnimationState);
        TYPE_DESC(TransitionValueNumberComparator, AnimationState);
        TYPE_DESC(NestedStateMachine, AnimationState);
        TYPE_DESC(ElasticInterpolator, AnimationState);
        TYPE_DESC(ExitState, AnimationState);
        TYPE_DESC(NestedNumber, AnimationState);
        TYPE_DESC(TransitionValueEnumComparator, AnimationState);
        TYPE_DESC(TransitionValueStringComparator, AnimationState);
        TYPE_DESC(NestedRemapAnimation, AnimationState);
        TYPE_DESC(TransitionValueAssetComparator, AnimationState);
        TYPE_DESC(TransitionBoolCondition, AnimationState);
        TYPE_DESC(BlendState1DViewModel, AnimationState);
        TYPE_DESC(BlendStateTransition, AnimationState);
        TYPE_DESC(StateMachineBool, AnimationState);
        TYPE_DESC(BlendAnimation1D, AnimationState);

        TYPE_DESC(DrawTarget, VectorGeometryPaint);
        TYPE_DESC(DashPath, VectorGeometryPaint);
        TYPE_DESC(LinearGradient, VectorGeometryPaint);
        TYPE_DESC(RadialGradient, VectorGeometryPaint);
        TYPE_DESC(Dash, VectorGeometryPaint);
        TYPE_DESC(Stroke, VectorGeometryPaint);
        TYPE_DESC(SolidColor, VectorGeometryPaint);
        TYPE_DESC(GradientStop, VectorGeometryPaint);
        TYPE_DESC(Feather, VectorGeometryPaint);
        TYPE_DESC(TrimPath, VectorGeometryPaint);
        TYPE_DESC(Fill, VectorGeometryPaint);
        TYPE_DESC(MeshVertex, VectorGeometryPaint);
        TYPE_DESC(Shape, VectorGeometryPaint);
        TYPE_DESC(StraightVertex, VectorGeometryPaint);
        TYPE_DESC(CubicAsymmetricVertex, VectorGeometryPaint);
        TYPE_DESC(Mesh, VectorGeometryPaint);
        TYPE_DESC(PointsPath, VectorGeometryPaint);
        TYPE_DESC(ContourMeshVertex, VectorGeometryPaint);
        TYPE_DESC(Rectangle, VectorGeometryPaint);
        TYPE_DESC(CubicMirroredVertex, VectorGeometryPaint);
        TYPE_DESC(Triangle, VectorGeometryPaint);
        TYPE_DESC(Ellipse, VectorGeometryPaint);
        TYPE_DESC(ListPath, VectorGeometryPaint);
        TYPE_DESC(ClippingShape, VectorGeometryPaint);
        TYPE_DESC(Polygon, VectorGeometryPaint);
        TYPE_DESC(Star, VectorGeometryPaint);
        TYPE_DESC(Image, VectorGeometryPaint);
        TYPE_DESC(CubicDetachedVertex, VectorGeometryPaint);
        TYPE_DESC(DrawRules, VectorGeometryPaint);

        TYPE_DESC(DistanceConstraint, SceneHierarchyConstraints);
        TYPE_DESC(FollowPathConstraint, SceneHierarchyConstraints);
        TYPE_DESC(ListFollowPathConstraint, SceneHierarchyConstraints);
        TYPE_DESC(IKConstraint, SceneHierarchyConstraints);
        TYPE_DESC(TranslationConstraint, SceneHierarchyConstraints);
        TYPE_DESC(ClampedScrollPhysics, SceneHierarchyConstraints);
        TYPE_DESC(ScrollConstraint, SceneHierarchyConstraints);
        TYPE_DESC(ElasticScrollPhysics, SceneHierarchyConstraints);
        TYPE_DESC(ScrollBarConstraint, SceneHierarchyConstraints);
        TYPE_DESC(TransformConstraint, SceneHierarchyConstraints);
        TYPE_DESC(ScaleConstraint, SceneHierarchyConstraints);
        TYPE_DESC(RotationConstraint, SceneHierarchyConstraints);
        TYPE_DESC(Node, SceneHierarchyConstraints);
        TYPE_DESC(ForegroundLayoutDrawable, SceneHierarchyConstraints);
        TYPE_DESC(NestedArtboard, SceneHierarchyConstraints);
        TYPE_DESC(ArtboardComponentList, SceneHierarchyConstraints);
        TYPE_DESC(Solo, SceneHierarchyConstraints);
        TYPE_DESC(NestedArtboardLayout, SceneHierarchyConstraints);
        TYPE_DESC(NSlicerTileMode, SceneHierarchyConstraints);
        TYPE_DESC(AxisY, SceneHierarchyConstraints);
        TYPE_DESC(LayoutComponentStyle, SceneHierarchyConstraints);
        TYPE_DESC(AxisX, SceneHierarchyConstraints);
        TYPE_DESC(NSlicer, SceneHierarchyConstraints);
        TYPE_DESC(NSlicedNode, SceneHierarchyConstraints);
        TYPE_DESC(ArtboardComponentListOverride, SceneHierarchyConstraints);
        TYPE_DESC(LayoutComponent, SceneHierarchyConstraints);
        TYPE_DESC(Artboard, SceneHierarchyConstraints);
        TYPE_DESC(Joystick, SceneHierarchyConstraints);
        TYPE_DESC(Backboard, SceneHierarchyConstraints);
        TYPE_DESC(NestedArtboardLeaf, SceneHierarchyConstraints);
        TYPE_DESC(Weight, SceneHierarchyConstraints);
        TYPE_DESC(Bone, SceneHierarchyConstraints);
        TYPE_DESC(RootBone, SceneHierarchyConstraints);
        TYPE_DESC(Skin, SceneHierarchyConstraints);
        TYPE_DESC(Tendon, SceneHierarchyConstraints);
        TYPE_DESC(CubicWeight, SceneHierarchyConstraints);

        TYPE_DESC(ViewModelInstanceListItem, DataBindingViewModels);
        TYPE_DESC(ViewModelComponent, DataBindingViewModels);
        TYPE_DESC(ViewModelProperty, DataBindingViewModels);
        TYPE_DESC(ViewModelPropertyArtboard, DataBindingViewModels);
        TYPE_DESC(ViewModelInstanceColor, DataBindingViewModels);
        TYPE_DESC(ViewModelPropertyEnum, DataBindingViewModels);
        TYPE_DESC(ViewModelPropertyEnumCustom, DataBindingViewModels);
        TYPE_DESC(DataEnum, DataBindingViewModels);
        TYPE_DESC(DataEnumCustom, DataBindingViewModels);
        TYPE_DESC(ViewModelPropertyNumber, DataBindingViewModels);
        TYPE_DESC(ViewModelInstanceEnum, DataBindingViewModels);
        TYPE_DESC(ViewModelPropertySymbolListIndex, DataBindingViewModels);
        TYPE_DESC(ViewModelInstanceArtboard, DataBindingViewModels);
        TYPE_DESC(ViewModelInstanceString, DataBindingViewModels);
        TYPE_DESC(ViewModelPropertyList, DataBindingViewModels);
        TYPE_DESC(ViewModelPropertyEnumSystem, DataBindingViewModels);
        TYPE_DESC(ViewModel, DataBindingViewModels);
        TYPE_DESC(ViewModelPropertyAsset, DataBindingViewModels);
        TYPE_DESC(DataEnumSystem, DataBindingViewModels);
        TYPE_DESC(ViewModelPropertyViewModel, DataBindingViewModels);
        TYPE_DESC(ViewModelInstance, DataBindingViewModels);
        TYPE_DESC(ViewModelPropertyBoolean, DataBindingViewModels);
        TYPE_DESC(ViewModelPropertyColor, DataBindingViewModels);
        TYPE_DESC(ViewModelPropertyAssetImage, DataBindingViewModels);
        TYPE_DESC(ViewModelInstanceBoolean, DataBindingViewModels);
        TYPE_DESC(ViewModelInstanceList, DataBindingViewModels);
        TYPE_DESC(ViewModelInstanceNumber, DataBindingViewModels);
        TYPE_DESC(ViewModelInstanceTrigger, DataBindingViewModels);
        TYPE_DESC(ViewModelInstanceSymbolListIndex, DataBindingViewModels);
        TYPE_DESC(ViewModelPropertyString, DataBindingViewModels);
        TYPE_DESC(ViewModelInstanceViewModel, DataBindingViewModels);
        TYPE_DESC(ViewModelPropertyTrigger, DataBindingViewModels);
        TYPE_DESC(ViewModelInstanceAsset, DataBindingViewModels);
        TYPE_DESC(ViewModelInstanceAssetImage, DataBindingViewModels);
        TYPE_DESC(DataEnumValue, DataBindingViewModels);
        TYPE_DESC(CustomPropertyNumber, DataBindingViewModels);
        TYPE_DESC(CustomPropertyColor, DataBindingViewModels);
        TYPE_DESC(CustomPropertyGroup, DataBindingViewModels);
        TYPE_DESC(CustomPropertyBoolean, DataBindingViewModels);
        TYPE_DESC(OpenUrlEvent, DataBindingViewModels);
        TYPE_DESC(BindablePropertyArtboard, DataBindingViewModels);
        TYPE_DESC(BindablePropertyInteger, DataBindingViewModels);
        TYPE_DESC(BindablePropertyTrigger, DataBindingViewModels);
        TYPE_DESC(BindablePropertyBoolean, DataBindingViewModels);
        TYPE_DESC(DataBind, DataBindingViewModels);
        TYPE_DESC(BindablePropertyAsset, DataBindingViewModels);
        TYPE_DESC(DataConverterNumberToList, DataBindingViewModels);
        TYPE_DESC(DataConverterFormula, DataBindingViewModels);
        TYPE_DESC(DataConverterToNumber, DataBindingViewModels);
        TYPE_DESC(DataConverterOperation, DataBindingViewModels);
        TYPE_DESC(DataConverterOperationValue, DataBindingViewModels);
        TYPE_DESC(DataConverterSystemDegsToRads, DataBindingViewModels);
        TYPE_DESC(DataConverterRangeMapper, DataBindingViewModels);
        TYPE_DESC(DataConverterInterpolator, DataBindingViewModels);
        TYPE_DESC(DataConverterSystemNormalizer, DataBindingViewModels);
        TYPE_DESC(DataConverterListToLength, DataBindingViewModels);
        TYPE_DESC(DataConverterGroupItem, DataBindingViewModels);
        TYPE_DESC(DataConverterGroup, DataBindingViewModels);
        TYPE_DESC(DataConverterStringRemoveZeros, DataBindingViewModels);
        TYPE_DESC(DataConverterRounder, DataBindingViewModels);
        TYPE_DESC(DataConverterStringPad, DataBindingViewModels);
        TYPE_DESC(DataConverterTrigger, DataBindingViewModels);
        TYPE_DESC(DataConverterStringTrim, DataBindingViewModels);
        TYPE_DESC(FormulaToken, DataBindingViewModels);
        TYPE_DESC(FormulaTokenArgumentSeparator, DataBindingViewModels);
        TYPE_DESC(FormulaTokenParenthesis, DataBindingViewModels);
        TYPE_DESC(FormulaTokenParenthesisClose, DataBindingViewModels);
        TYPE_DESC(FormulaTokenOperation, DataBindingViewModels);
        TYPE_DESC(FormulaTokenFunction, DataBindingViewModels);
        TYPE_DESC(FormulaTokenValue, DataBindingViewModels);
        TYPE_DESC(FormulaTokenParenthesisOpen, DataBindingViewModels);
        TYPE_DESC(FormulaTokenInput, DataBindingViewModels);
        TYPE_DESC(DataConverterOperationViewModel, DataBindingViewModels);
        TYPE_DESC(DataConverterBooleanNegate, DataBindingViewModels);
        TYPE_DESC(DataConverterToString, DataBindingViewModels);
        TYPE_DESC(DataBindContext, DataBindingViewModels);
        TYPE_DESC(BindablePropertyList, DataBindingViewModels);
        TYPE_DESC(BindablePropertyString, DataBindingViewModels);
        TYPE_DESC(BindablePropertyNumber, DataBindingViewModels);
        TYPE_DESC(BindablePropertyEnum, DataBindingViewModels);
        TYPE_DESC(BindablePropertyColor, DataBindingViewModels);
        TYPE_DESC(CustomPropertyEnum, DataBindingViewModels);
        TYPE_DESC(CustomPropertyString, DataBindingViewModels);
        TYPE_DESC(CustomPropertyTrigger, DataBindingViewModels);

        TYPE_DESC(Folder, AssetRefsMetadataContents);
        TYPE_DESC(ImageAsset, AssetRefsMetadataContents);
        TYPE_DESC(FontAsset, AssetRefsMetadataContents);
        TYPE_DESC(AudioAsset, AssetRefsMetadataContents);
        TYPE_DESC(FileAssetContents, AssetRefsMetadataContents);
        TYPE_DESC(AudioEvent, AssetRefsMetadataContents);
    }

    return {"Unknown(" + std::to_string(typeKey) + ")", ByteCategory::Other};
}

#undef TYPE_DESC

bool isBytesPropertyKey(uint32_t propertyKey)
{
    switch (propertyKey)
    {
        case rive::NestedArtboardBase::dataBindPathIdsPropertyKey:
        case rive::StateMachineFireTriggerBase::viewModelPathIdsPropertyKey:
        case rive::StateMachineListenerSingleBase::viewModelPathIdsPropertyKey:
        case rive::ListenerInputTypeViewModelBase::viewModelPathIdsPropertyKey:
        case rive::DataBindPathBase::pathPropertyKey:
        case rive::MeshBase::triangleIndexBytesPropertyKey:
        case rive::DataConverterOperationViewModelBase::sourcePathIdsPropertyKey:
        case rive::DataBindContextBase::sourcePathIdsPropertyKey:
        case rive::FileAssetBase::cdnUuidPropertyKey:
        case rive::FileAssetContentsBase::bytesPropertyKey:
            return true;
        default:
            return false;
    }
}

int propertyFieldId(uint32_t propertyKey, const std::unordered_map<uint32_t, int>& headerFields)
{
    if (isBytesPropertyKey(propertyKey))
    {
        return -2;
    }

    int registryFieldId = rive::CoreRegistry::propertyFieldId(static_cast<int>(propertyKey));
    if (registryFieldId != -1)
    {
        return registryFieldId;
    }

    auto it = headerFields.find(propertyKey);
    return it == headerFields.end() ? -1 : it->second;
}

void skipPropertyValue(ByteReader& reader, int fieldId)
{
    if (fieldId == rive::CoreUintType::id)
    {
        reader.readVarUint();
    }
    else if (fieldId == rive::CoreStringType::id)
    {
        reader.skipString();
    }
    else if (fieldId == rive::CoreDoubleType::id)
    {
        reader.skip(4);
    }
    else if (fieldId == rive::CoreColorType::id)
    {
        reader.skip(4);
    }
    else if (fieldId == rive::CoreBoolType::id)
    {
        reader.skip(1);
    }
    else if (fieldId == -2)
    {
        reader.skipBytes();
    }
    else
    {
        throw std::runtime_error("unknown property field id");
    }
}

std::string primitiveName(int fieldId)
{
    if (fieldId == rive::CoreUintType::id)
    {
        return "uint/ids";
    }
    if (fieldId == rive::CoreStringType::id)
    {
        return "strings";
    }
    if (fieldId == rive::CoreDoubleType::id)
    {
        return "floats";
    }
    if (fieldId == rive::CoreColorType::id)
    {
        return "colors";
    }
    if (fieldId == rive::CoreBoolType::id)
    {
        return "booleans";
    }
    return "byte arrays";
}
} // namespace

ByteStats analyzeRiveByteStats(const std::string& riveFilePath)
{
    ByteStats stats;

    try
    {
        ByteReader reader(readFileBytes(riveFilePath));
        stats.fileByteSize = reader.size();

        if (reader.readByte() != 'R' || reader.readByte() != 'I' || reader.readByte() != 'V' ||
            reader.readByte() != 'E')
        {
            throw std::runtime_error("bad Rive header");
        }

        reader.readVarUint(); // major version
        reader.readVarUint(); // minor version
        reader.readVarUint(); // file id

        std::vector<uint32_t> propertyKeys;
        while (true)
        {
            auto propertyKey = static_cast<uint32_t>(reader.readVarUint());
            if (propertyKey == 0)
            {
                break;
            }
            propertyKeys.push_back(propertyKey);
        }

        std::unordered_map<uint32_t, int> headerFields;
        int currentInt = 0;
        int currentBit = 8;
        for (auto propertyKey : propertyKeys)
        {
            if (currentBit == 8)
            {
                currentInt = static_cast<int>(reader.readUint32());
                currentBit = 0;
            }
            headerFields[propertyKey] = (currentInt >> currentBit) & 3;
            currentBit += 2;
        }

        stats.headerByteSize = reader.position();

        std::unordered_map<std::string, ByteStatsBucket> bucketsByName;
        std::unordered_map<uint32_t, ByteStatsType> typesByKey;
        std::unordered_map<std::string, ByteStatsPrimitive> primitivesByName;

        while (!reader.reachedEnd())
        {
            size_t objectStart = reader.position();
            auto typeKey = static_cast<uint32_t>(reader.readVarUint());
            auto descriptor = describeType(typeKey);
            size_t propertyCount = 0;

            while (true)
            {
                auto propertyKey = static_cast<uint32_t>(reader.readVarUint());
                if (propertyKey == 0)
                {
                    break;
                }

                int fieldId = propertyFieldId(propertyKey, headerFields);
                if (fieldId == -1)
                {
                    throw std::runtime_error("missing property field id for property " +
                                             std::to_string(propertyKey));
                }

                size_t valueStart = reader.position();
                skipPropertyValue(reader, fieldId);
                size_t valueByteSize = reader.position() - valueStart;

                auto primitive = primitiveName(fieldId);
                auto& primitiveStats = primitivesByName[primitive];
                primitiveStats.name = primitive;
                primitiveStats.propertyCount++;
                primitiveStats.byteSize += valueByteSize;
                propertyCount++;
            }

            size_t objectByteSize = reader.position() - objectStart;
            std::string category = categoryName(descriptor.category);

            auto& bucket = bucketsByName[category];
            bucket.name = category;
            bucket.objectCount++;
            bucket.byteSize += objectByteSize;

            auto& typeStats = typesByKey[typeKey];
            typeStats.name = descriptor.name;
            typeStats.category = category;
            typeStats.typeKey = typeKey;
            typeStats.objectCount++;
            typeStats.byteSize += objectByteSize;

            stats.objectCount++;
            stats.parsedObjectByteSize += objectByteSize;
            (void)propertyCount;
        }

        for (const auto& [_, bucket] : bucketsByName)
        {
            stats.buckets.push_back(bucket);
        }
        std::sort(stats.buckets.begin(), stats.buckets.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.byteSize > rhs.byteSize;
        });

        for (const auto& [_, typeStats] : typesByKey)
        {
            stats.topTypes.push_back(typeStats);
        }
        std::sort(stats.topTypes.begin(), stats.topTypes.end(), [](const auto& lhs, const auto& rhs) {
            return lhs.byteSize > rhs.byteSize;
        });
        if (stats.topTypes.size() > 12)
        {
            stats.topTypes.resize(12);
        }

        const std::vector<std::string> primitiveOrder = {
            "uint/ids", "strings", "floats", "colors", "booleans", "byte arrays"};
        for (const auto& primitive : primitiveOrder)
        {
            auto it = primitivesByName.find(primitive);
            if (it != primitivesByName.end())
            {
                stats.primitives.push_back(it->second);
            }
        }

        stats.parsed = true;
    }
    catch (const std::exception& exception)
    {
        stats.error = exception.what();
    }

    return stats;
}
