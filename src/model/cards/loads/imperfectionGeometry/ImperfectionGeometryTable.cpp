#include "oor/model/cards/loads/imperfectionGeometry/ImperfectionGeometryTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <limits>

namespace oor::model
{

namespace
{

// The fields of the first line that carry a value of the card.
constexpr int NODE_SET_ID_FIELD = 4;
constexpr int SCALE_FIELD = 5;
constexpr int UNIT_FIELD = 7;
constexpr int MODULE_ID_FIELD = 8;

// The groups are lines of eight fields from field 9: SUBCASEi, STEPi, MODEINCi, SETIDi, SCALEi, a blank, UNITi, MODIDi.
constexpr int FIRST_GROUP_FIELD = 9;
constexpr int GROUP_FIELDS = 8;
constexpr int GROUP_BLANK_OFFSET = 5;

// Any field of a card may be beyond the last one the format reference draws (the groups repeat), so a blank cell is
// checked alone.
constexpr int NO_LAST_FIELD = std::numeric_limits<int>::max();

// The problem with the four values that the card and each group have alike, if there is one.
std::optional<CardProblem> checkSharedValues(base::EntityId nodeSetId, double scaleFactor, std::int32_t fileUnit,
                                             std::int32_t moduleId)
{
    if (nodeSetId < 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must not be negative", "nodeSetId"};
    if (const auto problem = checkReal(scaleFactor, "scaleFactor", RealRule::Finite))
        return problem;
    if (fileUnit < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "fileUnit"};
    if (moduleId < IMPERFECTION_WHOLE_MODEL)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be -1 or above", "moduleId"};
    return std::nullopt;
}

} // namespace

std::optional<ImperfectionGeometryCard> ImperfectionGeometryFormat::read(FieldReader& reader)
{
    ImperfectionGeometryCard card;
    card.id = reader.requiredInteger(1, "id");
    requireBlankFields(reader, {2, 3, 6}, NO_LAST_FIELD);
    card.nodeSetId = reader.integer(NODE_SET_ID_FIELD, "nodeSetId", 0);
    card.scaleFactor = reader.real(SCALE_FIELD, "scaleFactor", 0.0);
    card.fileUnit = reader.integer(UNIT_FIELD, "fileUnit", 0);
    card.moduleId = reader.integer(MODULE_ID_FIELD, "moduleId", IMPERFECTION_WHOLE_MODEL);
    // The groups: a line each. A field that a group leaves blank takes the value of the card (resolved here).
    const int last = reader.lastUsedField();
    for (int first = FIRST_GROUP_FIELD; first <= last && !reader.failed(); first += GROUP_FIELDS)
    {
        ImperfectionGeometryGroup group;
        group.subcase = reader.integer(first, "subcase", 0);
        group.step = reader.integer(first + 1, "step", 0);
        group.modeIncrement = reader.integer(first + 2, "modeIncrement", 0);
        group.nodeSetId = reader.integer(first + 3, "nodeSetId", card.nodeSetId);
        group.scaleFactor = reader.real(first + 4, "scaleFactor", card.scaleFactor);
        requireBlankFields(reader, {first + GROUP_BLANK_OFFSET}, NO_LAST_FIELD);
        group.fileUnit = reader.integer(first + 6, "fileUnit", card.fileUnit);
        group.moduleId = reader.integer(first + 7, "moduleId", card.moduleId);
        card.groups.push_back(group);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ImperfectionGeometryFormat::check(const ImperfectionGeometryCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkSharedValues(card.nodeSetId, card.scaleFactor, card.fileUnit, card.moduleId))
        return problem;
    if (card.groups.empty())
        return CardProblem{
            "card.missing-field",
            "the card needs at least one group (" + std::string(CardProblem::FIELD) + "1 and the rest of its line)",
            "subcase"};
    for (const ImperfectionGeometryGroup& group : card.groups)
    {
        if (group.subcase < 0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "subcase"};
        if (group.step < 0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "step"};
        if (group.modeIncrement < 0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                               "modeIncrement"};
        if (const auto problem = checkSharedValues(group.nodeSetId, group.scaleFactor, group.fileUnit, group.moduleId))
            return problem;
    }
    return std::nullopt;
}

void ImperfectionGeometryFormat::write(const ImperfectionGeometryCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(NODE_SET_ID_FIELD, card.nodeSetId, 0);
    writer.realUnlessDefault(SCALE_FIELD, card.scaleFactor, 0.0);
    writer.integerUnlessDefault(UNIT_FIELD, card.fileUnit, 0);
    writer.integerUnlessDefault(MODULE_ID_FIELD, card.moduleId, IMPERFECTION_WHOLE_MODEL);
    int first = FIRST_GROUP_FIELD;
    for (const ImperfectionGeometryGroup& group : card.groups)
    {
        writer.integerUnlessDefault(first, group.subcase, 0);
        writer.integerUnlessDefault(first + 1, group.step, 0);
        writer.integerUnlessDefault(first + 2, group.modeIncrement, 0);
        // The four fields that default to the value of the card are resolved: always written.
        writer.integer(first + 3, group.nodeSetId);
        writer.real(first + 4, group.scaleFactor);
        writer.integer(first + 6, group.fileUnit);
        writer.integer(first + 7, group.moduleId);
        first += GROUP_FIELDS;
    }
}

std::uint32_t ImperfectionGeometryTable::append(const ImperfectionGeometryCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ImperfectionGeometryTable::store(std::uint32_t row, const ImperfectionGeometryCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ImperfectionGeometryTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ImperfectionGeometryTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ImperfectionGeometryTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ImperfectionGeometryCard& card = m_cards.edit(row);
    // The default set of the card, then the set of each group: all name a SET1 or a SET3 of grid points, and a zero
    // names nothing.
    visitId(visitor, "nodeSetId", ReferenceTarget::Set, card.nodeSetId);
    for (ImperfectionGeometryGroup& group : card.groups)
        visitId(visitor, "nodeSetId", ReferenceTarget::Set, group.nodeSetId);
}

} // namespace oor::model
