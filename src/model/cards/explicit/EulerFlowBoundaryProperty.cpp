#include "oor/model/cards/explicit/EulerFlowBoundaryProperty.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <string>

namespace oor::model
{

namespace
{

constexpr std::array<const char*, 16> TYPE_NAMES{
    "propertyKind1",  "propertyKind2",  "propertyKind3",  "propertyKind4",  "propertyKind5",  "propertyKind6",
    "propertyKind7",  "propertyKind8",  "propertyKind9",  "propertyKind10", "propertyKind11", "propertyKind12",
    "propertyKind13", "propertyKind14", "propertyKind15", "propertyKind16"};
constexpr std::array<const char*, 16> VALUE_NAMES{
    "propertyValue1",  "propertyValue2",  "propertyValue3",  "propertyValue4",  "propertyValue5",  "propertyValue6",
    "propertyValue7",  "propertyValue8",  "propertyValue9",  "propertyValue10", "propertyValue11", "propertyValue12",
    "propertyValue13", "propertyValue14", "propertyValue15", "propertyValue16"};

const char* typeName(std::size_t index) noexcept
{
    return index < TYPE_NAMES.size() ? TYPE_NAMES[index] : "propertyKind";
}

const char* valueName(std::size_t index) noexcept
{
    return index < VALUE_NAMES.size() ? VALUE_NAMES[index] : "propertyValue";
}

} // namespace

std::optional<EulerFlowBoundaryProperty> readFlowProperty(FieldReader& reader, int typeField, std::size_t index,
                                                          const EulerFlowBoundaryPropertyRules& rules)
{
    const int valueField = typeField + 1;
    const char* typeLabel = typeName(index);
    const char* valueLabel = valueName(index);
    if (reader.isBlank(typeField))
    {
        if (!reader.isBlank(valueField))
            reader.error("card.missing-field",
                         std::string(valueLabel) + " is there without its " + std::string(typeLabel), typeLabel);
        return std::nullopt;
    }
    const std::optional<FlowPropertyType> type =
        parseKeyword<FlowPropertyType>(FLOW_PROPERTY_WORDS, reader.text(typeField));
    if (!type)
    {
        reader.error("card.bad-value",
                     std::string(typeLabel) + " must be " + keywordList(FLOW_PROPERTY_WORDS) + ", not \"" +
                         std::string(reader.text(typeField)) + "\"",
                     typeLabel);
        return std::nullopt;
    }
    EulerFlowBoundaryProperty property;
    property.type = *type;
    switch (property.type)
    {
    case FlowPropertyType::Material:
    case FlowPropertyType::Hydrostatic:
        property.id = reader.requiredInteger(valueField, valueLabel);
        break;
    case FlowPropertyType::XVelocity:
    case FlowPropertyType::YVelocity:
    case FlowPropertyType::ZVelocity:
    case FlowPropertyType::Pressure:
    case FlowPropertyType::Density:
    case FlowPropertyType::SpecificEnergy:
        property.number = reader.requiredReal(valueField, valueLabel);
        break;
    case FlowPropertyType::Flow:
        property.mode = readKeyword(reader, valueField, valueLabel, FLOW_MODE_WORDS, FlowMode::Both);
        break;
    }
    if (property.type == FlowPropertyType::Flow && property.mode == FlowMode::Symmetry && !rules.allowSymmetry)
        reader.error("card.bad-value", std::string(valueLabel) + " must be BOTH, IN or OUT", valueLabel);
    return property;
}

void writeFlowProperty(FieldWriter& writer, int typeField, const EulerFlowBoundaryProperty& property)
{
    const int valueField = typeField + 1;
    writeKeyword(writer, typeField, FLOW_PROPERTY_WORDS, property.type);
    switch (property.type)
    {
    case FlowPropertyType::Material:
    case FlowPropertyType::Hydrostatic:
        writer.integer(valueField, property.id);
        break;
    case FlowPropertyType::XVelocity:
    case FlowPropertyType::YVelocity:
    case FlowPropertyType::ZVelocity:
    case FlowPropertyType::Pressure:
    case FlowPropertyType::Density:
    case FlowPropertyType::SpecificEnergy:
        writer.real(valueField, property.number);
        break;
    case FlowPropertyType::Flow:
        writeKeywordUnlessDefault(writer, valueField, FLOW_MODE_WORDS, property.mode, FlowMode::Both);
        break;
    }
}

std::optional<CardProblem> checkFlowProperty(const EulerFlowBoundaryProperty& property, std::size_t index,
                                             const EulerFlowBoundaryPropertyRules& rules)
{
    const char* valueLabel = valueName(index);
    switch (property.type)
    {
    case FlowPropertyType::Material:
    case FlowPropertyType::Hydrostatic:
        return checkRequiredId(property.id, valueLabel);
    case FlowPropertyType::XVelocity:
    case FlowPropertyType::YVelocity:
    case FlowPropertyType::ZVelocity:
    case FlowPropertyType::Pressure:
    case FlowPropertyType::Density:
    case FlowPropertyType::SpecificEnergy:
        return checkReal(property.number, valueLabel, RealRule::Finite);
    case FlowPropertyType::Flow:
        if (property.mode == FlowMode::Symmetry && !rules.allowSymmetry)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be BOTH, IN or OUT",
                               valueLabel};
        break;
    }
    return std::nullopt;
}

void visitFlowProperty(ReferenceVisitor& visitor, EulerFlowBoundaryProperty& property, std::size_t index)
{
    const std::string field = "propertyValue" + std::to_string(index + 1);
    if (property.type == FlowPropertyType::Material)
        visitId(visitor, field, ReferenceTarget::Material, property.id);
    else if (property.type == FlowPropertyType::Hydrostatic)
        visitId(visitor, field, ReferenceTarget::HydrostaticPreset, property.id);
}

} // namespace oor::model
