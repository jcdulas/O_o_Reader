#include "oor/model/cards/properties/rodPropNonlinear/RodPropNonlinearTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_KEYWORD_FIELD = 9; // the C2 line is a continuation line

} // namespace

std::optional<RodPropNonlinearCard> RodPropNonlinearFormat::read(FieldReader& reader)
{
    RodPropNonlinearCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.integer(2, "matId", 0);
    card.analysisType =
        readKeyword(reader, 5, "analysisType", PROPERTY_ANALYSIS_WORDS, PropertyAnalysis::StructuralAndHeat);
    requireUnusedFields(reader, {3, 4, 6, 7, 8});
    readBehaviorLines(reader, FIRST_KEYWORD_FIELD, ROD_PROP_NONLINEAR_GROUPS, card.groups);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> RodPropNonlinearFormat::check(const RodPropNonlinearCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.matId, "matId"))
        return problem;
    return checkBehaviorLines(ROD_PROP_NONLINEAR_GROUPS, card.groups);
}

void RodPropNonlinearFormat::write(const RodPropNonlinearCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.matId, 0);
    writeKeywordUnlessDefault(writer, 5, PROPERTY_ANALYSIS_WORDS, card.analysisType,
                              PropertyAnalysis::StructuralAndHeat);
    writeBehaviorLines(writer, FIRST_KEYWORD_FIELD, ROD_PROP_NONLINEAR_GROUPS, card.groups);
}

std::uint32_t RodPropNonlinearTable::append(const RodPropNonlinearCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void RodPropNonlinearTable::store(std::uint32_t row, const RodPropNonlinearCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void RodPropNonlinearTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void RodPropNonlinearTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void RodPropNonlinearTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    RodPropNonlinearCard& card = m_cards.edit(row);
    // The ID is the one of the PROD that the card extends.
    card.id = visitor.visit("id", ReferenceTarget::Property, card.id);
    m_id.set(row, card.id);
    // The material may be a MAT4 or MAT5 for the heat analysis: the Material target falls back to them.
    visitId(visitor, "matId", ReferenceTarget::Material, card.matId);
}

} // namespace oor::model
