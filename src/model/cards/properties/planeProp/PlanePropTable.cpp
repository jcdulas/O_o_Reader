#include "oor/model/cards/properties/planeProp/PlanePropTable.hpp"

#include "oor/model/cards/aero/BlankReal.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 5;

} // namespace

std::optional<PlanePropCard> PlanePropFormat::read(FieldReader& reader)
{
    PlanePropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.thickness = reader.optionalReal(3, "thickness");
    card.nonStructMass = reader.real(4, "nonStructMass", 0.0);
    card.formulationOption = reader.integer(5, "formulationOption", 0);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has five fields, " + std::string("id") + " to " +
                         std::string("formulationOption"),
                     "formulationOption");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PlanePropFormat::check(const PlanePropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    if (const auto problem = checkOptionalReal(card.thickness, "thickness", RealRule::NonNegative))
        return problem;
    if (const auto problem = checkReal(card.nonStructMass, "nonStructMass", RealRule::Finite))
        return problem;
    if (card.formulationOption < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                           "formulationOption"};
    return std::nullopt;
}

void PlanePropFormat::write(const PlanePropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    if (card.thickness)
        writer.real(3, *card.thickness);
    writer.realUnlessDefault(4, card.nonStructMass, 0.0);
    writer.integerUnlessDefault(5, card.formulationOption, 0);
}

std::uint32_t PlanePropTable::append(const PlanePropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    m_t.push(realToColumn(card.thickness));
    m_nonStructMass.push(card.nonStructMass);
    m_formulationOption.push(card.formulationOption);
    return row;
}

PlanePropCard PlanePropTable::card(std::uint32_t row) const
{
    return PlanePropCard{m_id.get(row), m_matId.get(row), realFromColumn(m_t.get(row)), m_nonStructMass.get(row),
                         m_formulationOption.get(row)};
}

void PlanePropTable::store(std::uint32_t row, const PlanePropCard& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    m_t.set(row, realToColumn(card.thickness));
    m_nonStructMass.set(row, card.nonStructMass);
    m_formulationOption.set(row, card.formulationOption);
}

void PlanePropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_t.keepRows(rows);
    m_nonStructMass.keepRows(rows);
    m_formulationOption.keepRows(rows);
}

void PlanePropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The material is a MAT1 and the like; a plane element may also have a heat material, so the Material target (which
    // falls back to the heat materials) is the right one.
    m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, m_matId.get(row)));
}

} // namespace oor::model
