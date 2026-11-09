#include "oor/model/cards/solution_control/crossSpectrumPair/CrossSpectrumPairTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

namespace
{

// A response type is a neutral word of letters and digits.
bool isResponseWord(std::string_view text)
{
    if (text.empty())
        return false;
    for (const char character : text)
    {
        const bool letter = (character >= 'A' && character <= 'Z') || (character >= 'a' && character <= 'z');
        if (!letter && !(character >= '0' && character <= '9'))
            return false;
    }
    return true;
}

} // namespace

std::optional<CrossSpectrumPairCard> CrossSpectrumPairFormat::read(FieldReader& reader)
{
    CrossSpectrumPairCard card;
    card.id = reader.requiredInteger(1, "id");
    card.responseKinds[0] = std::string(reader.text(2));
    card.targetIds[0] = reader.integer(3, "targetId1", 0);
    card.components[0] = reader.integer(4, "component1", 0);
    card.responseKinds[1] = std::string(reader.text(5));
    card.targetIds[1] = reader.integer(6, "targetId2", 0);
    card.components[1] = reader.integer(7, "component2", 0);
    card.curveId = reader.integer(8, "curveId", 0);
    // A blank RTYPE is the other one: stored resolved.
    if (card.responseKinds[0].empty())
        card.responseKinds[0] = card.responseKinds[1];
    if (card.responseKinds[1].empty())
        card.responseKinds[1] = card.responseKinds[0];
    if (reader.lastUsedField() > 8)
        reader.error("card.bad-value",
                     "an " + std::string(reader.cardName()) + " has no field beyond " + std::string("curveId"),
                     "curveId");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CrossSpectrumPairFormat::check(const CrossSpectrumPairCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the ID must be a positive integer", "id"};
    if (card.responseKinds[0].empty() || card.responseKinds[1].empty())
        return CardProblem{"card.missing-field",
                           "at least one of " + std::string(CardProblem::FIELD) + " and " +
                               CardProblem::fieldNamed("responseKind2") + " must be given",
                           "responseKind1"};
    for (std::size_t index = 0; index < 2; ++index)
    {
        const std::string number = std::to_string(index + 1);
        if (!isResponseWord(card.responseKinds[index]))
            return CardProblem{"card.bad-value",
                               "responseKind" + number + " must be a word such as Displacement or Stress",
                               index == 0 ? "responseKind1" : "responseKind2"};
        if (card.targetIds[index] <= 0)
            return CardProblem{"card.bad-id", "targetId2" + number + " must be a positive integer",
                               index == 0 ? "targetId1" : "targetId2"};
        if (card.components[index] <= 0)
            return CardProblem{"card.bad-value", "component2" + number + " must be a positive integer",
                               index == 0 ? "component1" : "component2"};
    }
    if (card.curveId < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer or blank",
                           "curveId"};
    return std::nullopt;
}

void CrossSpectrumPairFormat::write(const CrossSpectrumPairCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.text(2, card.responseKinds[0]);
    writer.integer(3, card.targetIds[0]);
    writer.integer(4, card.components[0]);
    writer.text(5, card.responseKinds[1]); // a blank RTYPE is the other one: always written
    writer.integer(6, card.targetIds[1]);
    writer.integer(7, card.components[1]);
    if (card.curveId != 0)
        writer.integer(8, card.curveId);
}

void CrossSpectrumPairTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    const CrossSpectrumPairCard value = card(row);
    std::array<base::EntityId, 2> targetIds = value.targetIds;
    for (std::size_t index = 0; index < 2; ++index)
    {
        const ReferenceTarget target =
            value.isElementResponse(index) ? ReferenceTarget::Element : ReferenceTarget::ScalarOrGridPoint;
        targetIds[index] = visitor.visit(index == 0 ? "targetId1" : "targetId2", target, targetIds[index]);
    }
    m_targetIds.set(row, targetIds);
}

std::uint32_t CrossSpectrumPairTable::append(const CrossSpectrumPairCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_responseKind1.push(card.responseKinds[0]);
    m_responseKind2.push(card.responseKinds[1]);
    m_targetIds.push(card.targetIds);
    m_components.push(card.components);
    m_curveId.push(card.curveId);
    return row;
}

CrossSpectrumPairCard CrossSpectrumPairTable::card(std::uint32_t row) const
{
    CrossSpectrumPairCard card;
    card.id = m_id.get(row);
    card.responseKinds = {m_responseKind1.get(row), m_responseKind2.get(row)};
    card.targetIds = m_targetIds.get(row);
    card.components = m_components.get(row);
    card.curveId = m_curveId.get(row);
    return card;
}

void CrossSpectrumPairTable::store(std::uint32_t row, const CrossSpectrumPairCard& card)
{
    m_id.set(row, card.id);
    m_responseKind1.set(row, card.responseKinds[0]);
    m_responseKind2.set(row, card.responseKinds[1]);
    m_targetIds.set(row, card.targetIds);
    m_components.set(row, card.components);
    m_curveId.set(row, card.curveId);
}

void CrossSpectrumPairTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_responseKind1.keepRows(rows);
    m_responseKind2.keepRows(rows);
    m_targetIds.keepRows(rows);
    m_components.keepRows(rows);
    m_curveId.keepRows(rows);
}

} // namespace oor::model
