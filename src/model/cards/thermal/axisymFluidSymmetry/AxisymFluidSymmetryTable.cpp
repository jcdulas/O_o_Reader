#include "oor/model/cards/thermal/axisymFluidSymmetry/AxisymFluidSymmetryTable.hpp"

#include "oor/model/cards/thermal/ThermalSupport.hpp"
#include "oor/model/core/Model.hpp"

namespace oor::model
{

std::optional<AxisymFluidSymmetryCard> AxisymFluidSymmetryFormat::read(FieldReader& reader)
{
    AxisymFluidSymmetryCard card;
    card.sectionCount = reader.requiredInteger(1, "sectionCount");
    card.planeSymmetry1 = readKeyword(reader, 2, "planeSymmetry1", {"Symmetric", "Antisymmetric"}, "");
    card.planeSymmetry2 = readKeyword(reader, 3, "planeSymmetry2", {"Symmetric", "Antisymmetric"}, "");
    if (!reader.failed() && card.planeSymmetry1.empty())
        reader.error("card.missing-field", std::string("planeSymmetry1") + " (S or A) is required", "planeSymmetry1");
    if (!reader.failed() && card.planeSymmetry2.empty())
        reader.error("card.missing-field", std::string("planeSymmetry2") + " (S or A) is required", "planeSymmetry2");
    requireUnusedBlank(reader, {}, 3);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> AxisymFluidSymmetryFormat::check(const AxisymFluidSymmetryCard& card)
{
    if (card.sectionCount < 2 || card.sectionCount % 2 != 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be an even integer of at least 2",
                           "sectionCount"};
    if (const auto problem = checkKeyword(card.planeSymmetry1, "planeSymmetry1", {"Symmetric", "Antisymmetric"}))
        return problem;
    return checkKeyword(card.planeSymmetry2, "planeSymmetry2", {"Symmetric", "Antisymmetric"});
}

void AxisymFluidSymmetryFormat::write(const AxisymFluidSymmetryCard& card, FieldWriter& writer)
{
    writer.integer(1, card.sectionCount);
    writer.text(2, card.planeSymmetry1);
    writer.text(3, card.planeSymmetry2);
}

std::uint32_t AxisymFluidSymmetryTable::append(const AxisymFluidSymmetryCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_sectionCount.push(card.sectionCount);
    m_planeSymmetry1.push(card.planeSymmetry1);
    m_planeSymmetry2.push(card.planeSymmetry2);
    return row;
}

AxisymFluidSymmetryCard AxisymFluidSymmetryTable::card(std::uint32_t row) const
{
    AxisymFluidSymmetryCard card;
    card.sectionCount = m_sectionCount.get(row);
    card.planeSymmetry1 = m_planeSymmetry1.get(row);
    card.planeSymmetry2 = m_planeSymmetry2.get(row);
    return card;
}

void AxisymFluidSymmetryTable::store(std::uint32_t row, const AxisymFluidSymmetryCard& card)
{
    m_sectionCount.set(row, card.sectionCount);
    m_planeSymmetry1.set(row, card.planeSymmetry1);
    m_planeSymmetry2.set(row, card.planeSymmetry2);
}

void AxisymFluidSymmetryTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_sectionCount.keepRows(rows);
    m_planeSymmetry1.keepRows(rows);
    m_planeSymmetry2.keepRows(rows);
}

void AxisymFluidSymmetryTable::finishRead(Model&, const ReadContext& context)
{
    if (m_rows.aliveCount() > 1)
        context.sink.error("deck.duplicate-axisym-fluid-symmetry",
                           "only one " + std::string(NAME) + " may appear in the Bulk Data; the first one is used");
}

} // namespace oor::model
