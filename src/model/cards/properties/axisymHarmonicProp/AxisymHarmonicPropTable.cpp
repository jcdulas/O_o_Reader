#include "oor/model/cards/properties/axisymHarmonicProp/AxisymHarmonicPropTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 5;

} // namespace

std::optional<AxisymHarmonicPropCard> AxisymHarmonicPropFormat::read(FieldReader& reader)
{
    AxisymHarmonicPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.frameId = reader.integer(3, "frameId", 0);
    card.harmonicIndex = reader.integer(4, "harmonicIndex", AxisymHarmonicPropCard::DEFAULT_HARMONIC_INDEX);
    card.integration = reader.integer(5, "integration", 0);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymHarmonicPropFormat::check(const AxisymHarmonicPropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    if (card.frameId < 0)
        return CardProblem{
            "card.bad-id",
            std::string(CardProblem::FIELD) + " must be 0 (the basic system) or the ID of a coordinate system",
            "frameId"};
    if (card.harmonicIndex < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or greater",
                           "harmonicIndex"};
    if (card.integration < 0 || card.integration > AxisymHarmonicPropCard::MAX_INTEGRATION)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0, 1, 2 or 3", "integration"};
    return std::nullopt;
}

void AxisymHarmonicPropFormat::write(const AxisymHarmonicPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.integerUnlessDefault(3, card.frameId, 0);
    writer.integerUnlessDefault(4, card.harmonicIndex, AxisymHarmonicPropCard::DEFAULT_HARMONIC_INDEX);
    writer.integerUnlessDefault(5, card.integration, 0);
}

std::uint32_t AxisymHarmonicPropTable::append(const AxisymHarmonicPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_frameId.push(card.frameId);
    m_harmonicIndex.push(card.harmonicIndex);
    m_integration.push(card.integration);
    return row;
}

AxisymHarmonicPropCard AxisymHarmonicPropTable::card(std::uint32_t row) const
{
    AxisymHarmonicPropCard card;
    card.id = m_id.get(row);
    card.matId = m_matId.get(row);
    card.frameId = m_frameId.get(row);
    card.harmonicIndex = m_harmonicIndex.get(row);
    card.integration = m_integration.get(row);
    return card;
}

void AxisymHarmonicPropTable::store(std::uint32_t row, const AxisymHarmonicPropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_frameId.set(row, card.frameId);
    m_harmonicIndex.set(row, card.harmonicIndex);
    m_integration.set(row, card.integration);
}

void AxisymHarmonicPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_frameId.keepRows(rows);
    m_harmonicIndex.keepRows(rows);
    m_integration.keepRows(rows);
}

void AxisymHarmonicPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
    // A blank CID is the basic system: nothing to look up.
    if (const base::EntityId frameId = m_frameId.get(row); frameId > 0)
        m_frameId.set(row, visitor.visit("frameId", ReferenceTarget::Coordinate, frameId));
}

} // namespace oor::model
