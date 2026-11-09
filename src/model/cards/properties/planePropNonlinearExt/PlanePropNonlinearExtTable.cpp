#include "oor/model/cards/properties/planePropNonlinearExt/PlanePropNonlinearExtTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_KEYWORD_FIELD = 9; // every keyword line is a continuation line

} // namespace

std::optional<PlanePropNonlinearExtCard> PlanePropNonlinearExtFormat::read(FieldReader& reader)
{
    PlanePropNonlinearExtCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");
    card.direction = reader.integer(3, "direction", 1);
    card.thickness = reader.real(4, "thickness", 1.0);
    card.analysisType =
        readKeyword(reader, 5, "analysisType", PROPERTY_ANALYSIS_WORDS, PropertyAnalysis::StructuralAndHeat);
    requireUnusedFields(reader, {6, 7, 8});
    readShellLines(reader, FIRST_KEYWORD_FIELD, PLANE_PROP_NONLINEAR_EXT_LINES, card.lines);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> PlanePropNonlinearExtFormat::check(const PlanePropNonlinearExtCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkRequiredId(card.matId, "matId"))
        return problem;
    if (card.direction < 1 || card.direction > 2)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1 or 2", "direction"};
    if (const auto problem = checkReal(card.thickness, "thickness", RealRule::Finite))
        return problem;
    return checkShellLines(PLANE_PROP_NONLINEAR_EXT_LINES, card.lines);
}

void PlanePropNonlinearExtFormat::write(const PlanePropNonlinearExtCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    writer.integerUnlessDefault(3, card.direction, 1);
    writer.realUnlessDefault(4, card.thickness, 1.0);
    writeKeywordUnlessDefault(writer, 5, PROPERTY_ANALYSIS_WORDS, card.analysisType,
                              PropertyAnalysis::StructuralAndHeat);
    writeShellLines(writer, FIRST_KEYWORD_FIELD, PLANE_PROP_NONLINEAR_EXT_LINES, card.lines);
}

std::uint32_t PlanePropNonlinearExtTable::append(const PlanePropNonlinearExtCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void PlanePropNonlinearExtTable::store(std::uint32_t row, const PlanePropNonlinearExtCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void PlanePropNonlinearExtTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void PlanePropNonlinearExtTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void PlanePropNonlinearExtTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    PlanePropNonlinearExtCard& card = m_cards.edit(row);
    // The ID is the one of the PLPLANE that the card extends.
    card.id = visitor.visit("id", ReferenceTarget::Property, card.id);
    m_id.set(row, card.id);
    // The material may be a MAT4 or MAT5 for the heat analysis: the Material target falls back to them.
    visitId(visitor, "matId", ReferenceTarget::Material, card.matId);
}

} // namespace oor::model
