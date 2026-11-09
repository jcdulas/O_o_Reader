#include "oor/model/cards/properties/solidPropHyperelastic/SolidPropHyperelasticTable.hpp"

#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

std::optional<SolidPropHyperelasticCard> SolidPropHyperelasticFormat::read(FieldReader& reader)
{
    SolidPropHyperelasticCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    if (const std::string_view location = reader.text(3); (location == "Gauss"))
        card.gaussOutput = true;
    else if (!location.empty() && !(location == "Node"))
        reader.error(
            "card.bad-value",
            std::string("stressLocation") + " must be GAUS, GRID or blank, found \"" + std::string(location) + "\"",
            "stressLocation");
    if (reader.lastUsedField() > 3)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has three fields", "stressLocation");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SolidPropHyperelasticFormat::check(const SolidPropHyperelasticCard& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.matId <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "matId"};
    return std::nullopt;
}

void SolidPropHyperelasticFormat::write(const SolidPropHyperelasticCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.textUnlessDefault(3, card.gaussOutput ? "Gauss" : "Node", "Node");
}

std::uint32_t SolidPropHyperelasticTable::append(const SolidPropHyperelasticCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_gaussOutput.push(card.gaussOutput ? 1 : 0);
    return row;
}

SolidPropHyperelasticCard SolidPropHyperelasticTable::card(std::uint32_t row) const
{
    return SolidPropHyperelasticCard{m_id.get(row), m_matId.get(row), m_gaussOutput.get(row) != 0};
}

void SolidPropHyperelasticTable::store(std::uint32_t row, const SolidPropHyperelasticCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_gaussOutput.set(row, card.gaussOutput ? 1 : 0);
}

void SolidPropHyperelasticTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_gaussOutput.keepRows(rows);
}

void SolidPropHyperelasticTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
