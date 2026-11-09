#include "oor/model/cards/nonlinear_contact/nonlinearBushForceTables/NonlinearBushForceTablesTable.hpp"

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/materials/MaterialSupport.hpp"

#include <string>
#include <vector>

namespace oor::model
{

namespace
{

constexpr int FORM_FIELD = 2;
constexpr int MIN_FREQ_FIELD = 3;
constexpr int MAX_FREQ_FIELD = 4;

// The name of the term in row `row` and column `column` (from 0): termTable11 for the first, termTable66 for the last.
std::string termName(std::size_t row, std::size_t column)
{
    return std::string("termTable") + static_cast<char>('1' + row) + static_cast<char>('1' + column);
}

// The diagnostic name of a term. A CardProblem keeps a pointer to its field name, so the names have static storage.
const char* termFieldName(std::size_t row, std::size_t column)
{
    static const std::vector<std::string> NAMES = []
    {
        std::vector<std::string> names;
        for (std::size_t line = 0; line < BUSHING_MATRIX_SIZE; ++line)
        {
            for (std::size_t place = 0; place < BUSHING_MATRIX_SIZE; ++place)
                names.push_back(termName(line, place));
        }
        return names;
    }();
    return NAMES[row * BUSHING_MATRIX_SIZE + column].c_str();
}

std::optional<CardProblem> checkTables(const NonlinearBushForceTablesCard& card)
{
    for (std::size_t row = 0; row < BUSHING_MATRIX_SIZE; ++row)
    {
        for (std::size_t column = 0; column < BUSHING_MATRIX_SIZE; ++column)
        {
            const base::EntityId table = card.tables[row][column];
            if (table < 0)
                return CardProblem{"card.bad-id", "a table ID must be a positive integer or blank",
                                   termFieldName(row, column)};
            if (table != 0 && !bushingTermIsUsed(card.form, row, column))
                return CardProblem{"card.bad-value",
                                   "this term has no place in the form " +
                                       std::string(keywordText(NONLINEAR_BUSH_FORM_WORDS, card.form)) + " of the card",
                                   termFieldName(row, column)};
        }
    }
    return std::nullopt;
}

} // namespace

std::optional<NonlinearBushForceTablesCard> NonlinearBushForceTablesFormat::read(FieldReader& reader)
{
    NonlinearBushForceTablesCard card;
    card.id = reader.requiredInteger(1, "id");
    card.form = readKeyword(reader, FORM_FIELD, "form", NONLINEAR_BUSH_FORM_WORDS, BushingForm::Symmetric);
    card.minimumFreq = reader.real(MIN_FREQ_FIELD, "minimumFreq", 0.0);
    card.maximumFreq = reader.requiredReal(MAX_FREQ_FIELD, "maximumFreq");
    // The terms of the form, from the line of their row: the other fields of the card are blank cells of its grid.
    std::vector<int> used{1, FORM_FIELD, MIN_FREQ_FIELD, MAX_FREQ_FIELD};
    for (std::size_t row = 0; row < BUSHING_MATRIX_SIZE; ++row)
    {
        for (std::size_t column = 0; column < BUSHING_MATRIX_SIZE; ++column)
        {
            if (!bushingTermIsUsed(card.form, row, column))
                continue;
            const int field = bushingTermField(card.form, row, column);
            card.tables[row][column] = reader.integer(field, termFieldName(row, column), 0);
            used.push_back(field);
        }
    }
    requireOnlyFields(reader, used);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> NonlinearBushForceTablesFormat::check(const NonlinearBushForceTablesCard& card)
{
    if (const auto problem = checkRequiredId(card.id, "id"))
        return problem;
    if (const auto problem = checkReal(card.minimumFreq, "minimumFreq", RealRule::Finite))
        return problem;
    if (const auto problem = checkReal(card.maximumFreq, "maximumFreq", RealRule::Positive))
        return problem;
    if (card.minimumFreq > card.maximumFreq)
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must not be greater than " + CardProblem::fieldNamed("maximumFreq"),
            "minimumFreq"};
    return checkTables(card);
}

void NonlinearBushForceTablesFormat::write(const NonlinearBushForceTablesCard& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writeKeywordUnlessDefault(writer, FORM_FIELD, NONLINEAR_BUSH_FORM_WORDS, card.form, BushingForm::Symmetric);
    writer.realUnlessDefault(MIN_FREQ_FIELD, card.minimumFreq, 0.0);
    writer.real(MAX_FREQ_FIELD, card.maximumFreq);
    for (std::size_t row = 0; row < BUSHING_MATRIX_SIZE; ++row)
    {
        for (std::size_t column = 0; column < BUSHING_MATRIX_SIZE; ++column)
        {
            if (bushingTermIsUsed(card.form, row, column) && card.tables[row][column] != 0)
                writer.integer(bushingTermField(card.form, row, column), card.tables[row][column]);
        }
    }
}

std::uint32_t NonlinearBushForceTablesTable::append(const NonlinearBushForceTablesCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_cards.push(card);
    return row;
}

void NonlinearBushForceTablesTable::store(std::uint32_t row, const NonlinearBushForceTablesCard& card)
{
    m_id.set(row, card.id);
    m_cards.set(row, card);
}

void NonlinearBushForceTablesTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_cards.keepRows(rows);
}

void NonlinearBushForceTablesTable::setIdAtRow(std::uint32_t row, base::EntityId id)
{
    m_id.set(row, id);
    m_cards.edit(row).id = id;
}

void NonlinearBushForceTablesTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    NonlinearBushForceTablesCard& card = m_cards.edit(row);
    for (std::size_t line = 0; line < BUSHING_MATRIX_SIZE; ++line)
    {
        for (std::size_t column = 0; column < BUSHING_MATRIX_SIZE; ++column)
            visitId(visitor, termName(line, column), ReferenceTarget::DynamicTable, card.tables[line][column]);
    }
}

} // namespace oor::model
