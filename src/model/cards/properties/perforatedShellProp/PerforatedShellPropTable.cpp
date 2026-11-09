#include "oor/model/cards/properties/perforatedShellProp/PerforatedShellPropTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 9; // HOMG, on the second line

} // namespace

std::optional<PerforatedShellPropCard> PerforatedShellPropFormat::read(FieldReader& reader)
{
    PerforatedShellPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.thickness = reader.requiredReal(3, "thickness");
    card.holeSpacing = reader.requiredReal(4, "holeSpacing");
    card.holeRadius = reader.requiredReal(5, "holeRadius");
    card.topology =
        readRequiredKeyword<PerforatedShellPropTopology>(reader, 6, "topology", PERFORATED_SHELL_PROP_TOPOLOGY_WORDS);
    card.fluidDensity = reader.requiredReal(7, "fluidDensity");
    card.fluidViscosity = reader.requiredReal(8, "fluidViscosity");
    card.homogenization = reader.integer(LAST_FIELD, "homogenization", 1);
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has no field beyond " + std::string("homogenization"),
                     "homogenization");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PerforatedShellPropFormat::check(const PerforatedShellPropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    if (const auto problem = checkReal(card.thickness, "thickness", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.holeSpacing, "holeSpacing", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.holeRadius, "holeRadius", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.fluidDensity, "fluidDensity", RealRule::Positive))
        return problem;
    if (const auto problem = checkReal(card.fluidViscosity, "fluidViscosity", RealRule::Positive))
        return problem;
    if (card.homogenization != 0 && card.homogenization != 1)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0 or 1", "homogenization"};
    return std::nullopt;
}

void PerforatedShellPropFormat::write(const PerforatedShellPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.real(3, card.thickness);
    writer.real(4, card.holeSpacing);
    writer.real(5, card.holeRadius);
    writeKeyword(writer, 6, PERFORATED_SHELL_PROP_TOPOLOGY_WORDS, card.topology);
    writer.real(7, card.fluidDensity);
    writer.real(8, card.fluidViscosity);
    writer.integerUnlessDefault(LAST_FIELD, card.homogenization, 1);
}

std::uint32_t PerforatedShellPropTable::append(const PerforatedShellPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void PerforatedShellPropTable::store(std::uint32_t row, const PerforatedShellPropCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void PerforatedShellPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void PerforatedShellPropTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void PerforatedShellPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    // The material may also be a MAT4 or MAT5 for a heat analysis: the Material target falls back to them.
    visitId(visitor, "matId", ReferenceTarget::Material, m_cards.edit(row).matId);
}

} // namespace oor::model
