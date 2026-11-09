#include "oor/model/cards/properties/propDistribution/PropDistributionTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cmath>
#include <span>
#include <string>
#include <utility>

namespace oor::model
{

namespace
{

constexpr int FIRST_PAIR_FIELD = 9; // the pairs of (EID, V) begin on the second line

// The lowest value of the coordinate system of a PSOLID (an integer of at least -2).
constexpr double LOWEST_MATERIAL_COORD_SYSTEM = -2.0;

// The names of the supported items for each kind of property. A
// row that gives two names ("Z1, Z2", "I1, I2, I12", "K1, K2") is one entry for each.
constexpr std::array<std::string_view, 8> BAR_PROP_NAMES{
    "area",          "bendingInertia1", "bendingInertia2", "productInertia12", "torsionConstant",
    "nonStructMass", "shearFactor1",    "shearFactor2"};
constexpr std::array<std::string_view, 8> BEAM_PROP_NAMES{
    "area",          "bendingInertia1", "bendingInertia2", "productInertia12", "torsionConstant",
    "nonStructMass", "shearFactor1",    "shearFactor2"};
constexpr std::array<std::string_view, 9> BEAM_PROP_3_NAMES{
    "area",          "bendingInertiaZ", "bendingInertiaY", "productInertiaYZ",  "torsionConstant",
    "nonStructMass", "shearFactorY",    "shearFactorZ",    "warpingCoefficient"};
constexpr std::array<std::string_view, 8> COMP_PROP_NAMES{
    "bottomOffset",      "nonStructMass", "bondAllowable", "referenceTemperature",
    "structuralDamping", "plyThickness",  "plyAngle",      "plyStressOutput"};
constexpr std::array<std::string_view, 3> ROD_PROP_NAMES{"area", "torsionConstant", "nonStructMass"};
constexpr std::array<std::string_view, 4> SHEAR_PANEL_PROP_NAMES{"thickness", "nonStructMass", "extensionalFactor1",
                                                                 "extensionalFactor2"};
constexpr std::array<std::string_view, 6> SHELL_PROP_NAMES{
    "thickness", "bendingStiffnessRatio", "shearThicknessRatio", "nonStructMass", "fiberDistance1", "fiberDistance2"};
constexpr std::array<std::string_view, 1> SOLID_PROP_NAMES{"matFrameId"};
constexpr std::array<std::string_view, 4> TUBE_PROP_NAMES{"outerDiameter", "thickness", "nonStructMass",
                                                          "outerDiameter2"};

std::span<const std::string_view> supportedNames(DistributedPropertyType type)
{
    switch (type)
    {
    case DistributedPropertyType::BarProp:
        return BAR_PROP_NAMES;
    case DistributedPropertyType::BeamProp:
        return BEAM_PROP_NAMES;
    case DistributedPropertyType::BeamProp3:
        return BEAM_PROP_3_NAMES;
    case DistributedPropertyType::CompProp:
    case DistributedPropertyType::CompPropGlobal:
        return COMP_PROP_NAMES; // the format has one set of rows for the two
    case DistributedPropertyType::RodProp:
        return ROD_PROP_NAMES;
    case DistributedPropertyType::ShearPanelProp:
        return SHEAR_PANEL_PROP_NAMES;
    case DistributedPropertyType::ShellProp:
        return SHELL_PROP_NAMES;
    case DistributedPropertyType::SolidProp:
        return SOLID_PROP_NAMES;
    case DistributedPropertyType::TubeProp:
        return TUBE_PROP_NAMES;
    }
    return {};
}

std::string namesAsPhrase(std::span<const std::string_view> names)
{
    std::string phrase;
    for (const std::string_view name : names)
    {
        if (!phrase.empty())
            phrase += ", ";
        phrase += name;
    }
    return phrase;
}

// A value of the entries of a PSOLID: a whole number from -2 up that fits an integer field.
bool isCoordinateSystemValue(double value)
{
    return std::isfinite(value) && value == std::floor(value) && value >= LOWEST_MATERIAL_COORD_SYSTEM &&
           value <= 2.0e9;
}

} // namespace

std::optional<PropDistributionCard> PropDistributionFormat::read(FieldReader& reader)
{
    PropDistributionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.type = readRequiredKeyword<DistributedPropertyType>(reader, 2, "type", DISTRIBUTED_PROPERTY_WORDS);
    card.propId = reader.requiredInteger(3, "propId");
    card.fieldName = std::string(reader.text(4));
    card.locationNumber = reader.optionalInteger(5, "locationNumber");
    card.scaleFactor = reader.real(6, "scaleFactor", 1.0);
    card.valueOffset = reader.real(7, "valueOffset", 0.0);
    if (!reader.isBlank(8))
        reader.error("card.bad-value",
                     "field 8 of a " + std::string(reader.cardName()) + " is blank: the pairs of " +
                         std::string("elemId") + " and " + std::string("elemValue") + " start in field 9",
                     "");
    // Four pairs to a line, from field 9. A pair with both fields blank is a gap of the layout; a pair with one of them
    // is an error (no default for either).
    const bool integers = card.type == DistributedPropertyType::SolidProp;
    for (int field = FIRST_PAIR_FIELD; field <= reader.lastUsedField(); field += 2)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        const base::EntityId elemId = reader.requiredInteger(field, "elemId");
        double value = 0.0;
        if (integers)
            value = static_cast<double>(reader.requiredInteger(field + 1, "elemValue"));
        else
            value = reader.requiredReal(field + 1, "elemValue");
        card.entries.emplace_back(elemId, value);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PropDistributionFormat::check(const PropDistributionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.propId, "propId"))
        return problem;
    const std::span<const std::string_view> names = supportedNames(card.type);
    bool supported = false;
    for (const std::string_view name : names)
        supported = supported || name == card.fieldName;
    if (card.fieldName.empty())
        return CardProblem{"card.missing-field",
                           std::string(CardProblem::FIELD) + " is required: " + namesAsPhrase(names), "fieldName"};
    if (!supported)
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " \"" + card.fieldName + "\" is not one of the names a " +
                               CardProblem::cardNamed(keywordText(DISTRIBUTED_PROPERTY_WORDS, card.type)) +
                               " supports: " + namesAsPhrase(names),
                           "fieldName"};
    if (card.locationNumber && *card.locationNumber < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "locationNumber"};
    if (const auto problem = checkReal(card.scaleFactor, "scaleFactor", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.valueOffset, "valueOffset", RealRule::Finite))
        return problem;
    const bool integers = card.type == DistributedPropertyType::SolidProp;
    if (integers && (card.scaleFactor != 1.0 || card.valueOffset != 0.0))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) + " and " + CardProblem::fieldNamed("valueOffset") +
                               " are left blank for a " + CardProblem::cardNamed("SolidProp"),
                           "scaleFactor"};
    std::vector<base::EntityId> elements;
    elements.reserve(card.entries.size());
    for (const auto& [elemId, value] : card.entries)
    {
        elements.push_back(elemId);
        if (integers && !isCoordinateSystemValue(value))
            return CardProblem{
                "card.bad-value",
                "the values of a " + CardProblem::cardNamed("SolidProp") + " are integers of at least -2", "elemValue"};
        if (const auto problem = checkReal(value, "elemValue", RealRule::Finite))
            return problem;
    }
    return checkDistributedElements(std::move(elements));
}

void PropDistributionFormat::write(const PropDistributionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeyword(writer, 2, DISTRIBUTED_PROPERTY_WORDS, card.type);
    writer.integer(3, card.propId);
    writer.text(4, card.fieldName);
    if (card.locationNumber)
        writer.integer(5, *card.locationNumber);
    writer.realUnlessDefault(6, card.scaleFactor, 1.0);
    writer.realUnlessDefault(7, card.valueOffset, 0.0);
    const bool integers = card.type == DistributedPropertyType::SolidProp;
    int field = FIRST_PAIR_FIELD;
    for (const auto& [elemId, value] : card.entries)
    {
        writer.integer(field, elemId);
        if (integers)
            writer.integer(field + 1, static_cast<std::int64_t>(value));
        else
            writer.real(field + 1, value);
        field += 2;
    }
}

std::uint32_t PropDistributionTable::append(const PropDistributionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void PropDistributionTable::store(std::uint32_t row, const PropDistributionCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void PropDistributionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void PropDistributionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void PropDistributionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    PropDistributionCard& card = m_cards.edit(row);
    visitId(visitor, "propId", ReferenceTarget::Property, card.propId);
    for (auto& entry : card.entries)
        visitId(visitor, "elemId", ReferenceTarget::Element, entry.first);
}

} // namespace oor::model
