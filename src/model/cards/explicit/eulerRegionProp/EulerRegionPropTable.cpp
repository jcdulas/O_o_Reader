#include "oor/model/cards/explicit/eulerRegionProp/EulerRegionPropTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"

namespace oor::model
{

std::optional<EulerRegionPropCard> EulerRegionPropFormat::read(FieldReader& reader)
{
    EulerRegionPropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.type = readKeyword(reader, 2, "solverType", EULER_SOLVER_WORDS, EulerSolverType::Hydro);
    card.initialRegionsId = reader.requiredInteger(3, "initialRegionsId");
    card.bodyForceSetId = reader.integer(4, "bodyForceSetId", 0);
    if (reader.lastUsedField() > 4)
        reader.error("card.bad-value", "a " + std::string(reader.cardName()) + " has 4 fields", "");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> EulerRegionPropFormat::check(const EulerRegionPropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.initialRegionsId, "initialRegionsId"))
        return problem;
    return checkOptionalId(card.bodyForceSetId, "bodyForceSetId");
}

void EulerRegionPropFormat::write(const EulerRegionPropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, 2, EULER_SOLVER_WORDS, card.type, EulerSolverType::Hydro);
    writer.integer(3, card.initialRegionsId);
    writer.integerUnlessDefault(4, card.bodyForceSetId, 0);
}

std::uint32_t EulerRegionPropTable::append(const EulerRegionPropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void EulerRegionPropTable::store(std::uint32_t row, const EulerRegionPropCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void EulerRegionPropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void EulerRegionPropTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void EulerRegionPropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    EulerRegionPropCard& card = m_cards.edit(row);
    visitId(visitor, "initialRegionsId", ReferenceTarget::EulerInitialization, card.initialRegionsId);
    visitId(visitor, "bodyForceSetId", ReferenceTarget::EulerBodyForce, card.bodyForceSetId);
}

} // namespace oor::model
