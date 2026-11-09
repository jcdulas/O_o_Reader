#include "oor/model/cards/solution_control/initialPlasticStrain/InitialPlasticStrainTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"

namespace oor::model
{

std::optional<InitialPlasticStrainCard> InitialPlasticStrainFormat::read(FieldReader& reader)
{
    InitialPlasticStrainCard card;
    card.range = readInitialStateRange(reader);
    card.strain = reader.real(7, "strain", 0.0);
    requireUnusedBlank(reader, {8}, 8);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> InitialPlasticStrainFormat::check(const InitialPlasticStrainCard& card)
{
    if (const auto problem = checkInitialStateRange(card.range))
        return problem;
    return checkReal(card.strain, "strain", RealRule::Finite);
}

void InitialPlasticStrainFormat::write(const InitialPlasticStrainCard& card, FieldWriter& writer)
{
    writeInitialStateRange(writer, card.range);
    writer.realUnlessDefault(7, card.strain, 0.0);
}

std::uint32_t InitialPlasticStrainTable::append(const InitialPlasticStrainCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_cards.push(card);
    return row;
}

void InitialPlasticStrainTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitInitialStateRange(visitor, m_cards.edit(row).range);
}

} // namespace oor::model
