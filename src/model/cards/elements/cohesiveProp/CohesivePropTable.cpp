#include "oor/model/cards/elements/cohesiveProp/CohesivePropTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

namespace oor::model
{

namespace
{

constexpr int LAST_FIELD = 6; // SECANT

} // namespace

std::optional<CohesivePropCard> CohesivePropFormat::read(FieldReader& reader)
{
    CohesivePropCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.integration = reader.integer(3, "integration", COHESIVE_PROP_INTEGRATION_GAUSS);
    card.thickness = reader.real(4, "thickness", 1.0);
    card.output = readKeyword(reader, 5, "output", COHESIVE_PROP_OUTPUT_WORDS, CohesivePropOutput::Grid);
    card.tangentStiffness = reader.integer(6, "tangentStiffness", 0);
    requireBlankFields(reader, {}, LAST_FIELD);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> CohesivePropFormat::check(const CohesivePropCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    if (card.integration != COHESIVE_PROP_INTEGRATION_GAUSS && card.integration != COHESIVE_PROP_INTEGRATION_LOBATTO)
        return CardProblem{"card.bad-value", "{field} must be 0 (Gauss) or 1 (Newton-Cotes/Lobatto)", "integration"};
    if (const auto problem = checkReal(card.thickness, "thickness", RealRule::Positive))
        return problem;
    if (card.tangentStiffness != 0 && card.tangentStiffness != 1)
        return CardProblem{"card.bad-value", "{field} must be 0 (secant matrix) or 1 (tangent matrix)",
                           "tangentStiffness"};
    return std::nullopt;
}

void CohesivePropFormat::write(const CohesivePropCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.integerUnlessDefault(3, card.integration, COHESIVE_PROP_INTEGRATION_GAUSS);
    writer.realUnlessDefault(4, card.thickness, 1.0);
    writeKeywordUnlessDefault(writer, 5, COHESIVE_PROP_OUTPUT_WORDS, card.output, CohesivePropOutput::Grid);
    writer.integerUnlessDefault(6, card.tangentStiffness, 0);
}

std::uint32_t CohesivePropTable::append(const CohesivePropCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void CohesivePropTable::store(std::uint32_t row, const CohesivePropCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void CohesivePropTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void CohesivePropTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void CohesivePropTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitId(visitor, "matId", ReferenceTarget::Material, m_cards.edit(row).matId);
}

} // namespace oor::model
