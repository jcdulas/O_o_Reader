#include "oor/model/cards/matrices/directMatrix/DirectMatrixTable.hpp"

#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/matrices/MatrixName.hpp"
#include "oor/model/core/Model.hpp"

#include <cmath>
#include <limits>
#include <set>

namespace oor::model
{

namespace
{

constexpr int FIRST_TOKEN_FIELD = 4;

} // namespace

std::optional<DirectMatrixCard> DirectMatrixFormat::read(FieldReader& reader)
{
    DirectMatrixCard card;
    const std::string_view name = reader.text(1);
    if (!isMatrixName(name))
    {
        reader.error("card.bad-value",
                     std::string("entryName") + " must be a nonempty matrix name without control characters, found \"" +
                         std::string(name) + "\"",
                     "entryName");
        return card;
    }
    card.entryName = matrixNameText(name);
    card.columnIndex = reader.requiredInteger(2, "columnIndex");
    if (card.isHeader())
    {
        card.matrixForm = reader.requiredInteger(3, "matrixForm");
        card.inputType = reader.requiredInteger(4, "inputType");
        card.outputType = reader.integer(5, "outputType", 0);
        card.rows = reader.requiredInteger(7, "rowCount");
        card.columns = reader.integer(8, "columnCount", 0);
        if (!reader.failed())
            reader.report(check(card));
        return card;
    }
    card.firstRow = reader.requiredInteger(3, "firstRow");
    const int last = reader.lastUsedField();
    for (int field = FIRST_TOKEN_FIELD; field <= last && !reader.failed(); ++field)
    {
        const std::string_view text = reader.text(field);
        if (text.empty())
            continue; // a blank is not a zero: it is skipped
        DirectMatrixToken token;
        if (text == "Through")
        {
            token.kind = DirectMatrixTokenKind::Thru;
            token.row = reader.requiredInteger(field + 1, "row");
            ++field;
        }
        else if (const auto integer = base::NumberParser::parseInteger(text))
        {
            if (*integer < std::numeric_limits<std::int32_t>::min() ||
                *integer > std::numeric_limits<std::int32_t>::max())
            {
                reader.error("card.bad-value", "the row number does not fit in 32 bits", "row");
                break;
            }
            token.kind = DirectMatrixTokenKind::Row;
            token.row = static_cast<std::int32_t>(*integer);
        }
        else if (const auto real = base::NumberParser::parseReal(text))
        {
            token.kind = DirectMatrixTokenKind::Value;
            token.value = *real;
        }
        else
        {
            reader.error("card.bad-value", "the field is neither a number nor THRU: \"" + std::string(text) + "\"",
                         "value");
            break;
        }
        card.tokens.push_back(token);
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DirectMatrixFormat::check(const DirectMatrixCard& card)
{
    if (!isMatrixName(card.entryName))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be a nonempty matrix name without control characters",
            "entryName"};
    if (card.isHeader())
    {
        const bool known = card.matrixForm >= 1 && card.matrixForm <= 6 ? true : card.matrixForm == 8;
        if (!known)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1 to 6 or 8",
                               "matrixForm"};
        if (card.inputType < 1 || card.inputType > 4)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 1, 2, 3 or 4",
                               "inputType"};
        if (card.outputType < 0 || card.outputType > 4)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be 0, 1, 2, 3 or 4",
                               "outputType"};
        if (card.rows <= 0)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " (the number of rows) must be a positive integer",
                               "rowCount"};
        if (card.columns < 0 || (card.columns == 0 && card.matrixForm != 3 && card.matrixForm != 8))
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " (the number of columns) must be a positive integer",
                               "columnCount"};
        if (card.firstRow != 0 || !card.tokens.empty())
            return CardProblem{"card.bad-value", "a header card has no column elements", "columnIndex"};
        return std::nullopt;
    }
    if (card.columnIndex < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "columnIndex"};
    if (card.firstRow <= 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a positive integer",
                           "firstRow"};
    DirectMatrixTokenKind before = DirectMatrixTokenKind::Row; // the row number I1 stands before the first field
    bool first = true;
    for (const DirectMatrixToken& token : card.tokens)
    {
        switch (token.kind)
        {
        case DirectMatrixTokenKind::Value:
            if (!std::isfinite(token.value))
                return CardProblem{"card.bad-value", "an element of the matrix must be a finite number", "value"};
            break;
        case DirectMatrixTokenKind::Row:
            if (token.row <= 0)
                return CardProblem{"card.bad-value", "a row number must be a positive integer", "row"};
            break;
        case DirectMatrixTokenKind::Thru:
            if (first || before != DirectMatrixTokenKind::Value)
                return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must follow an element value",
                                   "through"};
            if (token.row <= 0)
                return CardProblem{"card.bad-value", "the row number after THRU must be a positive integer", "row"};
            break;
        }
        before = token.kind;
        first = false;
    }
    return std::nullopt;
}

void DirectMatrixFormat::write(const DirectMatrixCard& card, FieldWriter& writer)
{
    writer.text(1, card.entryName);
    writer.integer(2, card.columnIndex);
    if (card.isHeader())
    {
        writer.integer(3, card.matrixForm);
        writer.integer(4, card.inputType);
        writer.integer(5, card.outputType);
        writer.integer(7, card.rows);
        writer.integerUnlessDefault(8, card.columns, 0);
        return;
    }
    writer.integer(3, card.firstRow);
    int field = FIRST_TOKEN_FIELD;
    for (const DirectMatrixToken& token : card.tokens)
    {
        switch (token.kind)
        {
        case DirectMatrixTokenKind::Value:
            writer.real(field++, token.value);
            break;
        case DirectMatrixTokenKind::Row:
            writer.integer(field++, token.row);
            break;
        case DirectMatrixTokenKind::Thru:
            writer.text(field++, "Through");
            writer.integer(field++, token.row);
            break;
        }
    }
}

std::uint32_t DirectMatrixTable::append(const DirectMatrixCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_entryName.push(card.entryName);
    m_columnIndex.push(card.columnIndex);
    m_matrixForm.push(card.matrixForm);
    m_inputType.push(card.inputType);
    m_outputType.push(card.outputType);
    m_matrixRows.push(card.rows);
    m_columns.push(card.columns);
    m_firstRow.push(card.firstRow);
    std::vector<std::uint8_t> kinds;
    std::vector<double> values;
    std::vector<std::int32_t> rows;
    for (const DirectMatrixToken& token : card.tokens)
    {
        kinds.push_back(static_cast<std::uint8_t>(token.kind));
        values.push_back(token.value);
        rows.push_back(token.row);
    }
    m_kind.push(kinds);
    m_value.push(values);
    m_row.push(rows);
    return row;
}

DirectMatrixCard DirectMatrixTable::card(std::uint32_t row) const
{
    DirectMatrixCard card;
    card.entryName = m_entryName.get(row);
    card.columnIndex = m_columnIndex.get(row);
    card.matrixForm = m_matrixForm.get(row);
    card.inputType = m_inputType.get(row);
    card.outputType = m_outputType.get(row);
    card.rows = m_matrixRows.get(row);
    card.columns = m_columns.get(row);
    card.firstRow = m_firstRow.get(row);
    const auto kinds = m_kind.get(row);
    const auto values = m_value.get(row);
    const auto rows = m_row.get(row);
    for (std::size_t index = 0; index < kinds.size(); ++index)
        card.tokens.push_back(
            DirectMatrixToken{static_cast<DirectMatrixTokenKind>(kinds[index]), values[index], rows[index]});
    return card;
}

void DirectMatrixTable::store(std::uint32_t row, const DirectMatrixCard& card)
{
    m_entryName.set(row, card.entryName);
    m_columnIndex.set(row, card.columnIndex);
    m_matrixForm.set(row, card.matrixForm);
    m_inputType.set(row, card.inputType);
    m_outputType.set(row, card.outputType);
    m_matrixRows.set(row, card.rows);
    m_columns.set(row, card.columns);
    m_firstRow.set(row, card.firstRow);
    std::vector<std::uint8_t> kinds;
    std::vector<double> values;
    std::vector<std::int32_t> rows;
    for (const DirectMatrixToken& token : card.tokens)
    {
        kinds.push_back(static_cast<std::uint8_t>(token.kind));
        values.push_back(token.value);
        rows.push_back(token.row);
    }
    m_kind.set(row, kinds);
    m_value.set(row, values);
    m_row.set(row, rows);
}

void DirectMatrixTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_entryName.keepRows(rows);
    m_columnIndex.keepRows(rows);
    m_matrixForm.keepRows(rows);
    m_inputType.keepRows(rows);
    m_outputType.keepRows(rows);
    m_matrixRows.keepRows(rows);
    m_columns.keepRows(rows);
    m_firstRow.keepRows(rows);
    m_kind.keepRows(rows);
    m_value.keepRows(rows);
    m_row.keepRows(rows);
}

void DirectMatrixTable::finishRead(Model&, const ReadContext& context)
{
    const std::string card = std::string(NAME);
    std::set<std::string> headers;
    std::set<std::string> reported;
    for (std::uint32_t row = 0; row < m_rows.rowCount(); ++row)
    {
        if (!m_rows.isAlive(row) || m_columnIndex.get(row) != 0)
            continue;
        const std::string name = m_entryName.get(row);
        if (!headers.insert(name).second && reported.insert(name).second)
            context.sink.error("deck.duplicate-direct-matrix",
                               "the " + card + " matrix " + name + " has more than one header card");
    }
    for (std::uint32_t row = 0; row < m_rows.rowCount(); ++row)
    {
        if (!m_rows.isAlive(row) || m_columnIndex.get(row) == 0)
            continue;
        const std::string name = m_entryName.get(row);
        if (!headers.contains(name) && reported.insert(name).second)
            context.sink.warning("deck.direct-matrix-without-header",
                                 "the " + card + " matrix " + name + " has column cards and no header card");
    }
}

} // namespace oor::model
