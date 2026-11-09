#include "oor/model/cards/elements/seamProp/SeamPropTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 6;

} // namespace

std::optional<SeamPropCard> SeamPropFormat::read(FieldReader& reader)
{
    SeamPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    // The word is kept as written: the format reference names only the default, Line.
    card.kind = std::string(reader.word(3, SeamPropCard::DEFAULT_TYPE));
    card.width = reader.requiredReal(4, "width");
    card.thickness = reader.optionalReal(5, "thickness");
    card.integration = reader.integer(6, "integration", SeamPropCard::DEFAULT_INTEGRATION);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SeamPropFormat::check(const SeamPropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    if (const auto problem = checkNameWord(card.kind, "kind"))
        return problem;
    if (const auto problem = checkReal(card.width, "width", RealRule::Positive))
        return problem;
    if (const auto problem = checkOptionalReal(card.thickness, "thickness", RealRule::Positive))
        return problem;
    if (card.integration != 0 && card.integration != 2)
        return CardProblem{"card.bad-value", "{field} must be 0 or 2", "integration"};
    return std::nullopt;
}

void SeamPropFormat::write(const SeamPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.textUnlessDefault(3, card.kind, SeamPropCard::DEFAULT_TYPE);
    writer.real(4, card.width);
    if (card.thickness)
        writer.real(5, *card.thickness);
    writer.integerUnlessDefault(6, card.integration, SeamPropCard::DEFAULT_INTEGRATION);
}

std::uint32_t SeamPropTable::append(const SeamPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_kind.push(card.kind);
    m_width.push(card.width);
    m_thickness.push(realToColumn(card.thickness));
    m_integration.push(card.integration);
    return row;
}

SeamPropCard SeamPropTable::card(std::uint32_t row) const
{
    SeamPropCard card;
    card.id = m_id.get(row);
    card.matId = m_matId.get(row);
    card.kind = m_kind.get(row);
    card.width = m_width.get(row);
    card.thickness = realFromColumn(m_thickness.get(row));
    card.integration = m_integration.get(row);
    return card;
}

void SeamPropTable::store(std::uint32_t row, const SeamPropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_kind.set(row, card.kind);
    m_width.set(row, card.width);
    m_thickness.set(row, realToColumn(card.thickness));
    m_integration.set(row, card.integration);
}

void SeamPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_kind.keepRows(rows);
    m_width.keepRows(rows);
    m_thickness.keepRows(rows);
    m_integration.keepRows(rows);
}

void SeamPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
