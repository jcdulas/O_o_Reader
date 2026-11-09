#include "oor/model/cards/elements/solidPropNonlinear/SolidPropNonlinearTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_KEYWORD_FIELD = 9; // every keyword line is a continuation line

} // namespace

std::optional<SolidPropNonlinearCard> SolidPropNonlinearFormat::read(FieldReader& reader)
{
    SolidPropNonlinearCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.integer(2, "matId", 0);
    card.direction = reader.integer(3, "direction", 1);
    card.analysisType =
        readKeyword(reader, 5, "analysisType", PROPERTY_ANALYSIS_WORDS, PropertyAnalysis::StructuralAndHeat);
    requireUnusedFields(reader, {4, 6, 7, 8});
    readBehaviorLines(reader, FIRST_KEYWORD_FIELD, SOLID_PROP_NONLINEAR_GROUPS, card.groups);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> SolidPropNonlinearFormat::check(const SolidPropNonlinearCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.matId, "matId"))
        return problem;
    if (card.direction < 1 || card.direction > 3)
        return CardProblem{"card.bad-value", "{field} must be 1, 2 or 3", "direction"};
    if (const auto problem = checkBehaviorLines(SOLID_PROP_NONLINEAR_GROUPS, card.groups))
        return problem;
    // The words of the C8 line go together: the layered solid (SLCOMP) is the 3D gasket with L and the
    // solid shell with ASTN, the plain solid has L or the reduced integration LRIH, and ASTN is for the solid shell
    // only.
    const BehaviorGroup& hexahedron = card.groups[SOLID_PROP_NONLINEAR_C8];
    const bool layered = hexahedron.behavior == ElementBehavior::SolidComposite;
    if (layered && hexahedron.integration == IntegrationScheme::LinearReducedHourglass)
        return CardProblem{"card.bad-value", "{field} of a layered solid (SLCOMP) must be L or ASTN", "integration8"};
    if (!layered && hexahedron.integration == IntegrationScheme::AssumedStrain)
        return CardProblem{"card.bad-value", "{field} of ASTN needs {field:behavior8} of SLCOMP", "integration8"};
    if (hexahedron.integration == IntegrationScheme::AssumedStrain && card.direction != 1)
        return CardProblem{"card.bad-value", "the solid shell (SLCOMP with ASTN) takes {field} 1 only", "direction"};
    return std::nullopt;
}

void SolidPropNonlinearFormat::write(const SolidPropNonlinearCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.matId, 0);
    writer.integerUnlessDefault(3, card.direction, 1);
    writeKeywordUnlessDefault(writer, 5, PROPERTY_ANALYSIS_WORDS, card.analysisType,
                              PropertyAnalysis::StructuralAndHeat);
    writeBehaviorLines(writer, FIRST_KEYWORD_FIELD, SOLID_PROP_NONLINEAR_GROUPS, card.groups);
}

std::uint32_t SolidPropNonlinearTable::append(const SolidPropNonlinearCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void SolidPropNonlinearTable::store(std::uint32_t row, const SolidPropNonlinearCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void SolidPropNonlinearTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void SolidPropNonlinearTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void SolidPropNonlinearTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    SolidPropNonlinearCard& card = m_cards.edit(row);
    // The ID is the one of the PSOLID that the card extends.
    card.id = visitor.visit("id", ReferenceTarget::Property, card.id);
    m_id.set(row, card.id);
    // The material may be a MAT4 or MAT5 for the heat analysis: the Material target falls back to them.
    visitId(visitor, "matId", ReferenceTarget::Material, card.matId);
}

} // namespace oor::model
