#include "oor/model/cards/properties/shearPanelPropNonlinear/ShearPanelPropNonlinearTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_KEYWORD_FIELD = 9; // the C4 line is a continuation line

} // namespace

std::optional<ShearPanelPropNonlinearCard> ShearPanelPropNonlinearFormat::read(FieldReader& reader)
{
    ShearPanelPropNonlinearCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.integer(2, "matId", 0);
    card.analysisType = readKeyword(reader, 5, "analysisType", PROPERTY_ANALYSIS_WORDS, PropertyAnalysis::Structural);
    requireUnusedFields(reader, {3, 4, 6, 7, 8});
    readBehaviorLines(reader, FIRST_KEYWORD_FIELD, SHEAR_PANEL_PROP_NONLINEAR_GROUPS, card.groups);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ShearPanelPropNonlinearFormat::check(const ShearPanelPropNonlinearCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.matId, "matId"))
        return problem;
    // A shear panel has no heat behaviour: the format reference gives IS only.
    if (card.analysisType != PropertyAnalysis::Structural)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be IS", "analysisType"};
    return checkBehaviorLines(SHEAR_PANEL_PROP_NONLINEAR_GROUPS, card.groups);
}

void ShearPanelPropNonlinearFormat::write(const ShearPanelPropNonlinearCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.matId, 0);
    writeKeywordUnlessDefault(writer, 5, PROPERTY_ANALYSIS_WORDS, card.analysisType, PropertyAnalysis::Structural);
    writeBehaviorLines(writer, FIRST_KEYWORD_FIELD, SHEAR_PANEL_PROP_NONLINEAR_GROUPS, card.groups);
}

std::uint32_t ShearPanelPropNonlinearTable::append(const ShearPanelPropNonlinearCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ShearPanelPropNonlinearTable::store(std::uint32_t row, const ShearPanelPropNonlinearCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ShearPanelPropNonlinearTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ShearPanelPropNonlinearTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ShearPanelPropNonlinearTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ShearPanelPropNonlinearCard& card = m_cards.edit(row);
    // The ID is the one of the PSHEAR that the card extends.
    card.id = visitor.visit("id", ReferenceTarget::Property, card.id);
    m_id.set(row, card.id);
    visitId(visitor, "matId", ReferenceTarget::Material, card.matId);
}

} // namespace oor::model
