#include "oor/model/cards/dynamics/frfFlexibleConnection/FrfFlexibleConnectionTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <array>
#include <cmath>
#include <string>
#include <variant>

namespace oor::model
{

namespace
{

constexpr std::array<FrfPointNames, 2> GROUP_NAMES{{{"frfComponent1", "point1"}, {"frfComponent2", "point2"}}};

constexpr int FIRST_GROUP_FIELD = 3; // the groups are fields 3-4 and 5-6
constexpr int STIFFNESS_FIELD = 7;
constexpr int DAMPING_FIELD = 8;
constexpr int STRUCTURAL_DAMPING_FIELD = 9; // the first field of the continuation line

// A field that is the value of a property (a real) or the ID of the TABLED that gives it (an integer), or blank.
FrfConnectionValue readFlexValue(FieldReader& reader, int field, const char* valueName, const char* tableName)
{
    FrfConnectionValue result;
    const auto entry = reader.integerOrReal(field, valueName);
    if (!entry)
        return result;
    if (const auto* table = std::get_if<std::int32_t>(&*entry))
    {
        if (*table <= 0)
            reader.error("card.bad-id", std::string(tableName) + " must be a positive integer", tableName);
        result.table = *table;
    }
    else
    {
        result.value = std::get<double>(*entry);
        // 0.0 would read back as a blank field, so it is refused here rather than turned into one.
        if (!(result.value > 0.0))
            reader.error("card.bad-value", std::string(valueName) + " must be greater than zero", valueName);
    }
    return result;
}

void writeFlexValue(FieldWriter& writer, int field, const FrfConnectionValue& value)
{
    if (value.table != 0)
        writer.integer(field, value.table);
    else if (value.value != 0.0)
        writer.real(field, value.value);
}

std::optional<CardProblem> checkFlexValue(const FrfConnectionValue& value, const char* valueName, const char* tableName)
{
    if (value.value != 0.0 && value.table != 0)
    {
        return CardProblem{
            "card.bad-value",
            "give " + CardProblem::fieldNamed(valueName) + " or " + CardProblem::fieldNamed(tableName) + ", not both",
            valueName};
    }
    if (!std::isfinite(value.value) || value.value < 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero", valueName};
    if (value.table < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", tableName};
    return std::nullopt;
}

} // namespace

std::optional<FrfFlexibleConnectionCard> FrfFlexibleConnectionFormat::read(FieldReader& reader)
{
    FrfFlexibleConnectionCard card;
    card.id = reader.requiredInteger(1, "id");
    card.components = reader.integer(2, "components", 0);
    for (std::size_t group = 0; group < card.points.size(); ++group)
        card.points[group] =
            readFrfComponentPoint(reader, FIRST_GROUP_FIELD + 2 * static_cast<int>(group), GROUP_NAMES[group]);
    card.stiffness = readFlexValue(reader, STIFFNESS_FIELD, "stiffness", "stiffnessTableId");
    card.damping = readFlexValue(reader, DAMPING_FIELD, "damping", "dampingTableId");
    card.structuralDamping =
        readFlexValue(reader, STRUCTURAL_DAMPING_FIELD, "structuralDamping", "structuralDampingTableId");
    if (reader.lastUsedField() > STRUCTURAL_DAMPING_FIELD)
        reader.error("card.bad-value",
                     "an " + std::string(reader.cardName()) + " has no field beyond " +
                         std::string("structuralDamping/structuralDampingTableId"),
                     "structuralDamping");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> FrfFlexibleConnectionFormat::check(const FrfFlexibleConnectionCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkSingleComponent(card.components, "components"))
        return problem;
    for (std::size_t group = 0; group < card.points.size(); ++group)
    {
        if (const auto problem = checkFrfComponentPoint(card.points[group], GROUP_NAMES[group], true))
            return problem;
    }
    if (const auto problem = checkFlexValue(card.stiffness, "stiffness", "stiffnessTableId"))
        return problem;
    if (const auto problem = checkFlexValue(card.damping, "damping", "dampingTableId"))
        return problem;
    if (const auto problem = checkFlexValue(card.structuralDamping, "structuralDamping", "structuralDampingTableId"))
        return problem;
    // K or B is given, and GE only with K.
    if (card.stiffness.isBlank() && card.damping.isBlank())
    {
        return CardProblem{"card.missing-field",
                           CardProblem::fieldNamed("stiffness/stiffnessTableId") + " and " +
                               CardProblem::fieldNamed("damping/dampingTableId") + " may not both be blank",
                           "stiffness"};
    }
    if (!card.structuralDamping.isBlank() && card.stiffness.isBlank())
    {
        return CardProblem{"card.bad-value",
                           CardProblem::fieldNamed("structuralDamping/structuralDampingTableId") +
                               " may not be given unless " + CardProblem::fieldNamed("stiffness/stiffnessTableId") +
                               " is",
                           "structuralDamping"};
    }
    return std::nullopt;
}

void FrfFlexibleConnectionFormat::write(const FrfFlexibleConnectionCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.components, 0);
    for (std::size_t group = 0; group < card.points.size(); ++group)
        writeFrfComponentPoint(writer, FIRST_GROUP_FIELD + 2 * static_cast<int>(group), card.points[group]);
    writeFlexValue(writer, STIFFNESS_FIELD, card.stiffness);
    writeFlexValue(writer, DAMPING_FIELD, card.damping);
    writeFlexValue(writer, STRUCTURAL_DAMPING_FIELD, card.structuralDamping);
}

std::uint32_t FrfFlexibleConnectionTable::append(const FrfFlexibleConnectionCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_components.push(card.components);
    m_cards.push(card);
    return row;
}

void FrfFlexibleConnectionTable::store(std::uint32_t row, const FrfFlexibleConnectionCard& card)
{
    m_id.set(row, card.id);
    m_components.set(row, card.components);
    m_cards.set(row, card);
}

void FrfFlexibleConnectionTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_components.keepRows(rows);
    m_cards.keepRows(rows);
}

void FrfFlexibleConnectionTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void FrfFlexibleConnectionTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    FrfFlexibleConnectionCard& card = m_cards.edit(row);
    visitFrfComponent(visitor, "frfComponent1", card.points[0].component);
    visitFrfComponent(visitor, "frfComponent2", card.points[1].component);
    visitId(visitor, "stiffnessTableId", ReferenceTarget::DynamicTable, card.stiffness.table);
    visitId(visitor, "dampingTableId", ReferenceTarget::DynamicTable, card.damping.table);
    visitId(visitor, "structuralDampingTableId", ReferenceTarget::DynamicTable, card.structuralDamping.table);
}

} // namespace oor::model
