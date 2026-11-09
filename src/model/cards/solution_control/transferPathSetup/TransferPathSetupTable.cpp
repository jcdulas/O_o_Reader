#include "oor/model/cards/solution_control/transferPathSetup/TransferPathSetupTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <limits>

namespace oor::model
{

namespace
{

constexpr int NODE_SET_FIELD = 2;
constexpr int UNIT_FIELD = 3;
constexpr int FIRST_RESPONSE_FIELD = 9; // RESPTYP1, then OUTSET1 in the next field, and so on, four pairs to a line
constexpr int PAIR_FIELDS = 2;
constexpr int NO_LAST_FIELD = std::numeric_limits<int>::max();

} // namespace

std::optional<TransferPathSetupCard> TransferPathSetupFormat::read(FieldReader& reader)
{
    TransferPathSetupCard card;
    card.id = reader.requiredInteger(1, "id");
    card.nodeSetId = reader.requiredInteger(NODE_SET_FIELD, "nodeSetId");
    card.unit = reader.integer(UNIT_FIELD, "unit", TransferPathSetupCard::DEFAULT_UNIT);
    requireBlankFields(reader, {4, 5, 6, 7, 8}, NO_LAST_FIELD);
    // The responses are pairs of fields; a pair with both blank is padding, a blank type is D.
    const int last = reader.lastUsedField();
    for (int field = FIRST_RESPONSE_FIELD; field <= last && !reader.failed(); field += PAIR_FIELDS)
    {
        if (reader.isBlank(field) && reader.isBlank(field + 1))
            continue;
        TransferPathSetupResponse response;
        if (!reader.isBlank(field))
            response.type = base::toUpperAscii(reader.text(field));
        response.outputSet = reader.requiredInteger(field + 1, "outputSet");
        card.responses.push_back(response);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> TransferPathSetupFormat::check(const TransferPathSetupCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.nodeSetId, "nodeSetId"))
        return problem;
    if (const auto problem = checkRequiredId(card.unit, "unit"))
        return problem;
    if (card.responses.empty())
        return CardProblem{"card.missing-field", "the card needs at least one response (RESPTYP1 and OUTSET1)",
                           "outputSet"};
    for (const TransferPathSetupResponse& response : card.responses)
    {
        if (response.type.empty())
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " is a combination of the letters D, V, A and P",
                               "responseKind"};
        for (const char letter : response.type)
        {
            if (TRANSFER_PATH_RESPONSE_LETTERS.find(letter) == std::string_view::npos)
                return CardProblem{"card.bad-value",
                                   std::string(CardProblem::FIELD) + " is a combination of the letters D, V, A and P",
                                   "responseKind"};
        }
        if (const auto problem = checkRequiredId(response.outputSet, "outputSet"))
            return problem;
    }
    return std::nullopt;
}

void TransferPathSetupFormat::write(const TransferPathSetupCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(NODE_SET_FIELD, card.nodeSetId);
    writer.integerUnlessDefault(UNIT_FIELD, card.unit, TransferPathSetupCard::DEFAULT_UNIT);
    int field = FIRST_RESPONSE_FIELD;
    for (const TransferPathSetupResponse& response : card.responses)
    {
        writer.textUnlessDefault(field, response.type, TRANSFER_PATH_DEFAULT_RESPONSE);
        writer.integer(field + 1, response.outputSet);
        field += PAIR_FIELDS;
    }
}

std::uint32_t TransferPathSetupTable::append(const TransferPathSetupCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void TransferPathSetupTable::store(std::uint32_t row, const TransferPathSetupCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void TransferPathSetupTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void TransferPathSetupTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void TransferPathSetupTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    TransferPathSetupCard& card = m_cards.edit(row);
    // The interface grid points, then the response grid points of each pair: all SET3 entries.
    visitId(visitor, "nodeSetId", ReferenceTarget::Set, card.nodeSetId);
    for (TransferPathSetupResponse& response : card.responses)
        visitId(visitor, "outputSet", ReferenceTarget::Set, response.outputSet);
}

} // namespace oor::model
