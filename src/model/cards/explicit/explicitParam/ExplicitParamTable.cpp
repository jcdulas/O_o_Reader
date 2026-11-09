#include "oor/model/cards/explicit/explicitParam/ExplicitParamTable.hpp"

#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/cards/explicit/explicitParam/ExplicitParamCatalogue.hpp"

#include <cmath>
#include <limits>

namespace oor::model
{

namespace
{

// Unknown entries retain exact text while numeric values retain their numeric kind.
std::optional<CardProblem> textProblem(const std::string& text, const char* field, bool mayLookLikeNumber)
{
    if (const auto problem = checkNameWord(text, field))
        return problem;
    if (!mayLookLikeNumber && (base::NumberParser::parseInteger(text) || base::NumberParser::parseReal(text)))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " is a word, not a number: " + text,
                           field};
    return std::nullopt;
}

ExplicitParamValue readValue(FieldReader& reader, int field)
{
    const std::string_view text = reader.text(field);
    if (text.empty())
        return ExplicitParamValue::blank();
    if (const auto integer = base::NumberParser::parseInteger(text))
    {
        if (*integer < std::numeric_limits<std::int32_t>::min() || *integer > std::numeric_limits<std::int32_t>::max())
        {
            reader.error("card.bad-value", "the integer value does not fit in 32 bits", "fieldValue");
            return ExplicitParamValue::blank();
        }
        return ExplicitParamValue::integer(static_cast<std::int32_t>(*integer));
    }
    if (const auto real = base::NumberParser::parseReal(text))
        return ExplicitParamValue::real(*real);
    return ExplicitParamValue::text(std::string(text));
}

} // namespace

std::optional<ExplicitParamCard> ExplicitParamFormat::read(FieldReader& reader)
{
    ExplicitParamCard card;
    card.entryName = std::string(reader.text(1));
    if (card.entryName.empty())
    {
        reader.error("card.missing-field", "a " + std::string(reader.cardName()) + " card needs a name", "entryName");
        return card;
    }
    // Every field up to the last used one is a value: the layout of the entry (how many there may be, which are
    // optional) is the catalogue's to judge, and a name it does not list keeps whatever it was given.
    const int lastField = reader.lastUsedField();
    for (int field = 2; field <= lastField; ++field)
        card.values.push_back(readValue(reader, field));
    normaliseExplicitParam(card);
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> ExplicitParamFormat::check(const ExplicitParamCard& card)
{
    // The name is checked as the deck writes it: the keyword of a neutral name, or the word as it was read.
    if (const auto problem = textProblem(card.entryName, "entryName", true);
        problem && ExplicitParamCatalogue::instance().find(card.entryName) == nullptr)
        return problem;
    const ExplicitParamCatalogue& catalogue = ExplicitParamCatalogue::instance();
    // The catalogue diagnoses a missing selector before the general text checks: without its form, valid neutral
    // entity names would otherwise be mistaken for user words limited to eight characters.
    if (const auto problem = catalogue.problemWith(card))
        return problem;
    for (std::size_t index = 0; index < card.values.size(); ++index)
    {
        const ExplicitParamValue& value = card.values[index];
        const ExplicitParamSlot* slot = catalogue.slotOf(card, index);
        const char* field = slot != nullptr ? slot->name : "fieldValue";
        if (value.kind == ExplicitParamValueKind::Real && !std::isfinite(value.realValue))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", field};
        if (value.kind == ExplicitParamValueKind::Text && !(slot != nullptr && slotWord(*slot, value.textValue)))
        {
            if (const auto problem = textProblem(value.textValue, field, false))
                return problem;
        }
    }
    return catalogue.problemWith(card);
}

void ExplicitParamFormat::write(const ExplicitParamCard& card, FieldWriter& writer)
{
    writer.text(1, std::string(card.entryName));
    const ExplicitParamCatalogue& catalogue = ExplicitParamCatalogue::instance();
    for (std::size_t index = 0; index < card.values.size(); ++index)
    {
        const ExplicitParamValue& value = card.values[index];
        const int field = static_cast<int>(index) + 2;
        switch (value.kind)
        {
        case ExplicitParamValueKind::Blank:
            break;
        case ExplicitParamValueKind::Integer:
        {
            // An integer where the entry takes a real is the real of the same value: the text reads back as a real.
            const ExplicitParamSlot* slot = catalogue.slotOf(card, index);
            if (slot != nullptr && slot->kind == ExplicitParamSlotKind::Real)
                writer.real(field, static_cast<double>(value.integerValue));
            else
                writer.integer(field, value.integerValue);
            break;
        }
        case ExplicitParamValueKind::Real:
            writer.real(field, value.realValue);
            break;
        case ExplicitParamValueKind::Text:
            writer.text(field, value.textValue);
            break;
        }
    }
}

std::uint32_t ExplicitParamTable::append(const ExplicitParamCard& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_entryName.push(card.entryName);
    m_cards.push(card);
    return row;
}

void ExplicitParamTable::store(std::uint32_t row, const ExplicitParamCard& card)
{
    m_entryName.set(row, card.entryName);
    m_cards.set(row, card);
}

void ExplicitParamTable::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_entryName.keepRows(rows);
    m_cards.keepRows(rows);
}

void ExplicitParamTable::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    visitExplicitParamReferences(m_cards.edit(row), visitor);
}

} // namespace oor::model
