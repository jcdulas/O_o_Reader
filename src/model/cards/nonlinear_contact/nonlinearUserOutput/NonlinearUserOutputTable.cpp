#include "oor/model/cards/nonlinear_contact/nonlinearUserOutput/NonlinearUserOutputTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"
#include "oor/model/cards/thermal/ThermalSupport.hpp"

#include <cmath>

namespace oor::model
{

namespace
{

// The first field of the IDs: the first continuation line.
constexpr int FIRST_ID_FIELD = 9;

} // namespace

std::optional<NonlinearUserOutputCard> NonlinearUserOutputFormat::read(FieldReader& reader)
{
    NonlinearUserOutputCard card;
    card.id = reader.requiredInteger(1, "id");
    card.groupName = reader.word(2);
    card.outputKind = reader.isBlank(3) ? "ElemExplicitOutput" : std::string(reader.text(3));
    card.timeInterval = reader.optionalReal(4, "timeInterval");
    card.stepInterval = reader.optionalInteger(5, "stepInterval");
    card.outputName = reader.word(6);
    requireUnusedBlank(reader, {7, 8}, reader.lastUsedField());
    card.entityIds = readContactIds(reader, FIRST_ID_FIELD, reader.lastUsedField(), "entityIds");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonlinearUserOutputFormat::check(const NonlinearUserOutputCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", std::string(CardProblem::FIELD) + " must be a positive integer", "id"};
    if (card.groupName.empty())
        return CardProblem{"card.missing-field", std::string(CardProblem::FIELD) + " is required", "groupName"};
    if (auto problem = checkKeyword(card.outputKind, "outputKind", {"ElemExplicitOutput", "NodeExplicitOutput"}))
        return problem;
    if (card.timeInterval && (!(*card.timeInterval > 0.0) || !std::isfinite(*card.timeInterval)))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than zero",
                           "timeInterval"};
    if (card.stepInterval && *card.stepInterval <= 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "stepInterval"};
    // A time interval and a step interval exclude each other.
    if (card.timeInterval && card.stepInterval)
        return CardProblem{"card.bad-value",
                           "only one of " + CardProblem::fieldNamed("timeInterval") + " and " +
                               std::string(CardProblem::FIELD) + " may be given",
                           "stepInterval"};
    if (card.outputName.empty())
        return CardProblem{"card.missing-field", std::string(CardProblem::FIELD) + " is required", "outputName"};
    if (card.entityIds.empty())
        return CardProblem{"card.missing-field", "the card needs at least one element or grid point", "entityIds"};
    for (const base::EntityId id : card.entityIds)
    {
        if (id <= 0)
            return CardProblem{"card.bad-id", "an element or grid point ID must be a positive integer", "entityIds"};
    }
    return std::nullopt;
}

void NonlinearUserOutputFormat::write(const NonlinearUserOutputCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.groupName);
    writer.textUnlessDefault(3, card.outputKind, "ElemExplicitOutput");
    if (card.timeInterval)
        writer.real(4, *card.timeInterval);
    if (card.stepInterval)
        writer.integer(5, *card.stepInterval);
    writer.text(6, card.outputName);
    int field = FIRST_ID_FIELD;
    for (const base::EntityId id : card.entityIds)
        writer.integer(field++, id);
}

std::uint32_t NonlinearUserOutputTable::append(const NonlinearUserOutputCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void NonlinearUserOutputTable::store(std::uint32_t row, const NonlinearUserOutputCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void NonlinearUserOutputTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void NonlinearUserOutputTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void NonlinearUserOutputTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    NonlinearUserOutputCard& card = m_cards.edit(row);
    visitContactIds(card.entityIds, visitor, "entityIds",
                    card.outputKind == "NodeExplicitOutput" ? ReferenceTarget::Node : ReferenceTarget::Element);
}

} // namespace oor::model
