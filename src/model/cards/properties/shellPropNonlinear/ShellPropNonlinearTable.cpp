#include "oor/model/cards/properties/shellPropNonlinear/ShellPropNonlinearTable.hpp"

namespace oor::model
{

namespace
{

constexpr int FIRST_KEYWORD_FIELD = 9; // every keyword line is a continuation line

} // namespace

std::optional<ShellPropNonlinearCard> ShellPropNonlinearFormat::read(FieldReader& reader)
{
    ShellPropNonlinearCard card;
    card.id = reader.requiredInteger(1, "id");
    card.matId1 = reader.integer(2, "matId1", 0);
    card.matId2 = reader.integer(3, "matId2", 0);
    card.analysisType =
        readKeyword(reader, 5, "analysisType", PROPERTY_ANALYSIS_WORDS, PropertyAnalysis::StructuralAndHeat);
    requireUnusedFields(reader, {4, 6, 7, 8});
    readShellLines(reader, FIRST_KEYWORD_FIELD, SHELL_PROP_NONLINEAR_LINES, card.lines);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ShellPropNonlinearFormat::check(const ShellPropNonlinearCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkOptionalId(card.matId1, "matId1"))
        return problem;
    if (const auto problem = checkOptionalId(card.matId2, "matId2"))
        return problem;
    return checkShellLines(SHELL_PROP_NONLINEAR_LINES, card.lines);
}

void ShellPropNonlinearFormat::write(const ShellPropNonlinearCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integerUnlessDefault(2, card.matId1, 0);
    writer.integerUnlessDefault(3, card.matId2, 0);
    writeKeywordUnlessDefault(writer, 5, PROPERTY_ANALYSIS_WORDS, card.analysisType,
                              PropertyAnalysis::StructuralAndHeat);
    writeShellLines(writer, FIRST_KEYWORD_FIELD, SHELL_PROP_NONLINEAR_LINES, card.lines);
}

std::uint32_t ShellPropNonlinearTable::append(const ShellPropNonlinearCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void ShellPropNonlinearTable::store(std::uint32_t row, const ShellPropNonlinearCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void ShellPropNonlinearTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void ShellPropNonlinearTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void ShellPropNonlinearTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ShellPropNonlinearCard& card = m_cards.edit(row);
    // The ID is the one of the PSHELL, PCOMP or PCOMPG that the card extends.
    card.id = visitor.visit("id", ReferenceTarget::Property, card.id);
    m_id.set(row, card.id);
    // The materials may be a MAT4 or MAT5 for the heat analysis: the Material target falls back to them.
    visitId(visitor, "matId1", ReferenceTarget::Material, card.matId1);
    visitId(visitor, "matId2", ReferenceTarget::Material, card.matId2);
}

} // namespace oor::model
