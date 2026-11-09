#include "oor/model/cards/nodes/moduleDirectMatrix/ModuleDirectMatrixTable.hpp"

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/matrices/MatrixName.hpp"
#include "oor/model/cards/nodes/ModuleBoundarySupport.hpp"

#include <cmath>
#include <string>

namespace oor::model
{

namespace
{

// A term is a line of its own after the first: the first field of the line is blank, the term's five fields follow and
// the last two are blank.
constexpr int FIELDS_PER_LINE = 8;
constexpr int TERM_FIRST_OFFSET = 1; // the MOD of the term, after the blank first field of its line
constexpr int TERM_FIELD_COUNT = 5;  // MOD, G, C, A, B

// True when the header fields still hold what a column keeps.
bool hasHeaderDefaults(const ModuleDirectMatrixCard& card) noexcept
{
    return card.matrixShape == 6 && card.inputType == 1 && card.outputType == 0 && card.polarFlag == 0 &&
           card.columnCount == 0;
}

} // namespace

std::optional<ModuleDirectMatrixCard> ModuleDirectMatrixFormat::read(FieldReader& reader)
{
    ModuleDirectMatrixCard card;
    const std::string_view name = reader.text(1);
    if (!isMatrixName(name))
    {
        reader.error("card.bad-value",
                     std::string("entryName") + " must be a nonempty matrix name without control characters, found \"" +
                         std::string(name) + "\"",
                     "entryName");
        return card;
    }
    card.entryName = std::string(name);
    card.columnModuleId = reader.integer(2, "columnModuleId", 0);
    const int last = reader.lastUsedField();
    if (card.columnModuleId == 0 && last <= FIELDS_PER_LINE)
    {
        // The header: 0 in field 2 and nothing after the first line.
        card.matrixShape = reader.requiredInteger(3, "matrixShape");
        card.inputType = reader.requiredInteger(4, "inputType");
        card.outputType = reader.integer(5, "outputType", 0);
        card.polarFlag = reader.integer(6, "polarFlag", 0);
        if (!reader.isBlank(7))
            reader.error("card.bad-value",
                         "field 7 of an " + std::string(reader.cardName()) + " header is not used and must be blank",
                         "");
        card.columnCount = reader.integer(8, "columnCount", 0);
        if (!reader.failed())
            reader.report(check(card));
        return card;
    }
    card.nodeJ = reader.integer(3, "nodeJ", 0);
    card.componentsJ = reader.integer(4, "componentsJ", 0);
    for (int field = 5; field <= FIELDS_PER_LINE; ++field)
    {
        if (!reader.isBlank(field))
            reader.error("card.bad-value",
                         "field " + std::to_string(field) + " of the first line of an " +
                             std::string(reader.cardName()) +
                             " column is not used and "
                             "must be blank",
                         "");
    }
    for (int start = FIELDS_PER_LINE + 1; start <= last && !reader.failed(); start += FIELDS_PER_LINE)
    {
        const int first = start + TERM_FIRST_OFFSET;
        const int after = first + TERM_FIELD_COUNT;
        bool used = false;
        for (int field = start; field < start + FIELDS_PER_LINE; ++field)
            used = used || !reader.isBlank(field);
        if (!used)
            continue; // a line with nothing on it
        if (!reader.isBlank(start) || !reader.isBlank(after) || !reader.isBlank(after + 1))
        {
            reader.error("card.bad-value",
                         "a term of an " + std::string(reader.cardName()) + " column has " + std::string("moduleId") +
                             ", " + std::string("node") + ", " + std::string("components") + ", " +
                             std::string("realValue") + " and " + std::string("imaginaryValue") + " in fields " +
                             std::to_string(first) + " to " + std::to_string(after - 1) + " of its line",
                         "");
            break;
        }
        ModuleDirectMatrixTerm term;
        term.module = reader.integer(first, "moduleId", 0);
        term.point = reader.requiredInteger(first + 1, "node");
        term.component = reader.integer(first + 2, "components", 0);
        term.real = reader.real(first + 3, "realValue", 0.0);
        if (!reader.isBlank(first + 4))
        {
            term.imaginary = reader.real(first + 4, "imaginaryValue", 0.0);
            card.imaginary = true;
        }
        card.terms.push_back(term);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ModuleDirectMatrixFormat::check(const ModuleDirectMatrixCard& card)
{
    if (!isMatrixName(card.entryName))
        return CardProblem{"card.bad-value", "{field} must be a nonempty matrix name without control characters",
                           "entryName"};
    if (card.isHeader())
    {
        if (card.matrixShape != 1 && card.matrixShape != 2 && card.matrixShape != 6 && card.matrixShape != 9)
            return CardProblem{"card.bad-value", "{field} must be 1 (square), 2 or 9 (rectangular) or 6 (symmetric)",
                               "matrixShape"};
        if (card.inputType < 1 || card.inputType > 4)
            return CardProblem{"card.bad-value", "{field} must be 1, 2, 3 or 4", "inputType"};
        if (card.outputType < 0 || card.outputType > 4)
            return CardProblem{"card.bad-value", "{field} must be 0, 1, 2, 3 or 4", "outputType"};
        if (card.polarFlag < 0)
            return CardProblem{"card.bad-value", "{field} must not be negative", "polarFlag"};
        if (card.columnCount < 0)
            return CardProblem{"card.bad-value", "{field} must not be negative", "columnCount"};
        if (card.nodeJ != 0 || card.componentsJ != 0)
            return CardProblem{"card.bad-value",
                               "a card with " + CardProblem::fieldNamed("columnModuleId") +
                                   " 0 and no terms is a header: it has no {field} or " +
                                   CardProblem::fieldNamed("componentsJ") +
                                   " (a column of module 0 "
                                   "needs its terms)",
                               "nodeJ"};
        if (card.imaginary)
            return CardProblem{"card.bad-value", "a header card has no terms and no {field} fields", "imaginaryValue"};
        return std::nullopt;
    }
    if (const auto problem = checkBoundaryModule(card.columnModuleId, "columnModuleId"))
        return problem;
    // GJ and CJ are ignored when MODJ is a column number (IFO 2 or 9), so a column may leave GJ blank.
    if (card.nodeJ < 0)
        return CardProblem{"card.bad-id",
                           "{field} must be a point ID, or blank where " + CardProblem::fieldNamed("columnModuleId") +
                               " is a column number",
                           "nodeJ"};
    if (card.componentsJ < 0 || card.componentsJ > 6)
        return CardProblem{"card.bad-value",
                           "{field} must be one of the integers 1 to 6, or 0 for a scalar or extra point",
                           "componentsJ"};
    if (!hasHeaderDefaults(card))
        return CardProblem{"card.bad-value",
                           "{field}, " + CardProblem::fieldNamed("inputType") + ", " +
                               CardProblem::fieldNamed("outputType") + ", " + CardProblem::fieldNamed("polarFlag") +
                               " and " + CardProblem::fieldNamed("columnCount") + " belong to the header card",
                           "matrixShape"};
    for (const ModuleDirectMatrixTerm& term : card.terms)
    {
        if (const auto problem = checkBoundaryModule(term.module, "moduleId"))
            return problem;
        if (term.point <= 0)
            return CardProblem{"card.bad-id", "a grid, scalar or extra point ID must be a positive integer", "node"};
        if (term.component < 0 || term.component > 6)
            return CardProblem{"card.bad-value", "{field} must be one of the integers 1 to 6, or 0 for a scalar point",
                               "components"};
        if (!std::isfinite(term.real) || !std::isfinite(term.imaginary))
            return CardProblem{"card.bad-value", "a term of the matrix must be a finite number", "realValue"};
        if (!card.imaginary && term.imaginary != 0.0)
            return CardProblem{"card.bad-value", "a column of a real matrix has no imaginary parts ({field} is blank)",
                               "imaginaryValue"};
    }
    return std::nullopt;
}

void ModuleDirectMatrixFormat::write(const ModuleDirectMatrixCard& card, FieldWriter& writer)
{
    writer.text(1, card.entryName);
    if (card.isHeader())
    {
        writer.integer(2, 0);
        writer.integer(3, card.matrixShape);
        writer.integer(4, card.inputType);
        writer.integerUnlessDefault(5, card.outputType, 0);
        writer.integerUnlessDefault(6, card.polarFlag, 0);
        writer.integerUnlessDefault(8, card.columnCount, 0);
        return;
    }
    writer.integer(2, card.columnModuleId);
    writer.integerUnlessDefault(3, card.nodeJ, 0);
    writer.integerUnlessDefault(4, card.componentsJ, 0);
    int start = FIELDS_PER_LINE + 1;
    for (const ModuleDirectMatrixTerm& term : card.terms)
    {
        const int first = start + TERM_FIRST_OFFSET;
        writer.integerUnlessDefault(first, term.module, 0);
        writer.integer(first + 1, term.point);
        writer.integerUnlessDefault(first + 2, term.component, 0);
        writer.real(first + 3, term.real);
        if (card.imaginary)
            writer.real(first + 4, term.imaginary);
        start += FIELDS_PER_LINE;
    }
}

std::uint32_t ModuleDirectMatrixTable::append(const ModuleDirectMatrixCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_entryName.push(card.entryName);
    m_cards.push(card);
    return row;
}

void ModuleDirectMatrixTable::store(std::uint32_t row, const ModuleDirectMatrixCard& card)
{
    m_entryName.set(row, card.entryName);
    m_cards.set(row, card);
}

void ModuleDirectMatrixTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_entryName.keepRows(rows);
    m_cards.keepRows(rows);
}

void ModuleDirectMatrixTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    ModuleDirectMatrixCard& card = m_cards.edit(row);
    // The point of the column: a module that is not 0 (or a column number, for IFO 2 and 9) is left alone.
    visitBoundaryId(visitor, "nodeJ", ReferenceTarget::ScalarOrGridPoint, card.columnModuleId, card.nodeJ);
    for (ModuleDirectMatrixTerm& term : card.terms)
        visitBoundaryId(visitor, "node", ReferenceTarget::ScalarOrGridPoint, term.module, term.point);
}

} // namespace oor::model
