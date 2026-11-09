#include "oor/model/cards/matrices/directTable/DirectTableTable.hpp"

#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"

#include <cmath>
#include <limits>

namespace oor::model
{

std::optional<DirectTableCard> DirectTableFormat::read(FieldReader& reader)
{
    DirectTableCard card;
    card.entryName = std::string(reader.text(1));
    if (const auto problem = checkNameWord(card.entryName, "entryName"))
    {
        reader.report(problem);
        return card;
    }
    card.recordNumber = reader.requiredInteger(2, "recordNumber");
    const int last = reader.lastUsedField();
    for (int field = 3; field <= last && !reader.failed(); ++field)
    {
        DirectTableValue value;
        const std::string_view text = reader.text(field);
        if (text.empty())
        {
            card.values.push_back(value); // a blank in the middle of a record counts
            continue;
        }
        if (const auto integer = base::NumberParser::parseInteger(text))
        {
            if (*integer < std::numeric_limits<std::int32_t>::min() ||
                *integer > std::numeric_limits<std::int32_t>::max())
            {
                reader.error("card.bad-value", "the integer value does not fit in 32 bits", "values");
                break;
            }
            value.kind = DirectTableKind::Integer;
            value.integerValue = static_cast<std::int32_t>(*integer);
        }
        else if (const auto real = base::NumberParser::parseReal(text))
        {
            value.kind = DirectTableKind::Real;
            value.realValue = *real;
        }
        else
        {
            value.kind = DirectTableKind::Text;
            value.textValue = std::string(text);
        }
        card.values.push_back(std::move(value));
    }
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> DirectTableFormat::check(const DirectTableCard& card)
{
    if (const auto problem = checkNameWord(card.entryName, "entryName"))
        return problem;
    if (card.recordNumber < 0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative", "recordNumber"};
    for (const DirectTableValue& value : card.values)
    {
        if (value.kind == DirectTableKind::Real && !std::isfinite(value.realValue))
            return CardProblem{"card.bad-value", "a value must be a finite number", "values"};
        if (value.kind == DirectTableKind::Text && value.textValue.empty())
            return CardProblem{"card.bad-value", "a word value is empty", "values"};
    }
    // Trailing blank values cannot be distinguished from omitted fields. A literal nonblank value may follow them.
    if (!card.values.empty() && card.values.back().kind == DirectTableKind::Blank)
        return CardProblem{"card.bad-value", "a record must end with a nonblank value", "values"};
    return std::nullopt;
}

void DirectTableFormat::write(const DirectTableCard& card, FieldWriter& writer)
{
    writer.text(1, card.entryName);
    writer.integer(2, card.recordNumber);
    int field = 3;
    for (const DirectTableValue& value : card.values)
    {
        switch (value.kind)
        {
        case DirectTableKind::Blank:
            break;
        case DirectTableKind::Integer:
            writer.integer(field, value.integerValue);
            break;
        case DirectTableKind::Real:
            writer.real(field, value.realValue);
            break;
        case DirectTableKind::Text:
            writer.text(field, value.textValue);
            break;
        }
        ++field;
    }
}

std::uint32_t DirectTableTable::append(const DirectTableCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_entryName.push(card.entryName);
    m_recordNumber.push(card.recordNumber);
    std::vector<std::uint8_t> kinds;
    std::vector<std::int32_t> integers;
    std::vector<double> reals;
    std::vector<std::string> texts;
    for (const DirectTableValue& value : card.values)
    {
        kinds.push_back(static_cast<std::uint8_t>(value.kind));
        integers.push_back(value.integerValue);
        reals.push_back(value.realValue);
        texts.push_back(value.textValue);
    }
    m_kind.push(kinds);
    m_integer.push(integers);
    m_real.push(reals);
    m_text.push(texts);
    return row;
}

DirectTableCard DirectTableTable::card(std::uint32_t row) const
{
    DirectTableCard card;
    card.entryName = m_entryName.get(row);
    card.recordNumber = m_recordNumber.get(row);
    const auto kinds = m_kind.get(row);
    const auto integers = m_integer.get(row);
    const auto reals = m_real.get(row);
    const auto texts = m_text.get(row);
    for (std::size_t index = 0; index < kinds.size(); ++index)
    {
        DirectTableValue value;
        value.kind = static_cast<DirectTableKind>(kinds[index]);
        value.integerValue = integers[index];
        value.realValue = reals[index];
        value.textValue = texts[index];
        card.values.push_back(std::move(value));
    }
    return card;
}

void DirectTableTable::store(std::uint32_t row, const DirectTableCard& card)
{
    m_entryName.set(row, card.entryName);
    m_recordNumber.set(row, card.recordNumber);
    std::vector<std::uint8_t> kinds;
    std::vector<std::int32_t> integers;
    std::vector<double> reals;
    std::vector<std::string> texts;
    for (const DirectTableValue& value : card.values)
    {
        kinds.push_back(static_cast<std::uint8_t>(value.kind));
        integers.push_back(value.integerValue);
        reals.push_back(value.realValue);
        texts.push_back(value.textValue);
    }
    m_kind.set(row, kinds);
    m_integer.set(row, integers);
    m_real.set(row, reals);
    m_text.set(row, texts);
}

void DirectTableTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_entryName.keepRows(rows);
    m_recordNumber.keepRows(rows);
    m_kind.keepRows(rows);
    m_integer.keepRows(rows);
    m_real.keepRows(rows);
    m_text.keepRows(rows);
}

} // namespace oor::model
