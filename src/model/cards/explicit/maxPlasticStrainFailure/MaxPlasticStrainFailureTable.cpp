#include "oor/model/cards/explicit/maxPlasticStrainFailure/MaxPlasticStrainFailureTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<MaxPlasticStrainFailureCard> MaxPlasticStrainFailureFormat::read(FieldReader& reader)
{
    MaxPlasticStrainFailureCard card;
    card.id = reader.requiredInteger(1, "id");
    card.maxPlasticStrain = reader.requiredReal(2, "maxPlasticStrain");
    if (reader.lastUsedField() > 2)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 2 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> MaxPlasticStrainFailureFormat::check(const MaxPlasticStrainFailureCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    return checkReal(card.maxPlasticStrain, "maxPlasticStrain", RealRule::Finite);
}

void MaxPlasticStrainFailureFormat::write(const MaxPlasticStrainFailureCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.real(2, card.maxPlasticStrain);
}

std::uint32_t MaxPlasticStrainFailureTable::append(const MaxPlasticStrainFailureCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void MaxPlasticStrainFailureTable::store(std::uint32_t row, const MaxPlasticStrainFailureCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void MaxPlasticStrainFailureTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void MaxPlasticStrainFailureTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

} // namespace oor::model
