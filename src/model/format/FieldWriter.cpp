#include "oor/model/format/FieldWriter.hpp"

#include "oor/base/text/NumberFormatter.hpp"

#include <algorithm>
#include <cmath>

namespace oor::model
{

FieldWriter::Slot& FieldWriter::slot(int field)
{
    if (field >= 1 && static_cast<std::size_t>(field) > m_slots.size())
        m_slots.resize(static_cast<std::size_t>(field));
    return m_slots[static_cast<std::size_t>(field - 1)];
}

void FieldWriter::integer(int field, std::int64_t value)
{
    Slot& target = slot(field);
    target.kind = Slot::Kind::Integer;
    target.integer = value;
}

void FieldWriter::real(int field, double value)
{
    Slot& target = slot(field);
    target.kind = Slot::Kind::Real;
    target.real = value;
}

void FieldWriter::text(int field, std::string_view value)
{
    Slot& target = slot(field);
    target.kind = value.empty() ? Slot::Kind::Blank : Slot::Kind::Text;
    target.text = std::string(value);
}

void FieldWriter::spanText(int firstField, int fieldCount, std::string_view value)
{
    (void)slot(firstField + fieldCount - 1); // the fields of the span exist, blank, whatever the text
    Slot& target = slot(firstField);
    target.kind = value.empty() ? Slot::Kind::Blank : Slot::Kind::SpanText;
    target.integer = fieldCount;
    target.text = std::string(value);
}

void FieldWriter::namedReal(int field, std::string_view name, double value)
{
    Slot& target = slot(field);
    target.kind = Slot::Kind::NamedReal;
    target.text = std::string(name);
    target.real = value;
}

void FieldWriter::integerUnlessDefault(int field, std::int64_t value, std::int64_t defaultValue)
{
    if (m_omitDefaults && value == defaultValue)
        (void)slot(field); // keep the field count right: a trailing default is a blank field
    else
        integer(field, value);
}

void FieldWriter::realUnlessDefault(int field, double value, double defaultValue)
{
    if (m_omitDefaults && value == defaultValue && !(m_signOfZero && std::signbit(value) != std::signbit(defaultValue)))
        (void)slot(field);
    else
        real(field, value);
}

void FieldWriter::textUnlessDefault(int field, std::string_view value, std::string_view defaultValue)
{
    if (m_omitDefaults && value == defaultValue)
        (void)slot(field);
    else
        text(field, value);
}

bool FieldWriter::holdsNumber(int field) const noexcept
{
    if (field < 1 || static_cast<std::size_t>(field) > m_slots.size())
        return false;
    const Slot::Kind kind = m_slots[static_cast<std::size_t>(field - 1)].kind;
    return kind == Slot::Kind::Integer || kind == Slot::Kind::Real;
}

bool FieldWriter::holdsReal(int field) const noexcept
{
    return field >= 1 && static_cast<std::size_t>(field) <= m_slots.size() &&
           m_slots[static_cast<std::size_t>(field - 1)].kind == Slot::Kind::Real;
}

std::vector<std::string> FieldWriter::fields() const
{
    std::vector<std::string> texts;
    texts.reserve(m_slots.size());
    const auto realText = [&](double value)
    {
        if (m_signOfZero && value == 0.0 && std::signbit(value))
            return std::string("-0.");
        return m_significantDigits > 0 ? base::NumberFormatter::formatRealWithDigits(value, m_significantDigits).text
                                       : base::NumberFormatter::formatRealFree(value);
    };
    for (const Slot& value : m_slots)
    {
        switch (value.kind)
        {
        case Slot::Kind::Blank:
            texts.emplace_back();
            break;
        case Slot::Kind::Integer:
            texts.push_back(base::NumberFormatter::formatInteger(value.integer));
            break;
        case Slot::Kind::Text:
        case Slot::Kind::SpanText:
            texts.push_back(value.text);
            break;
        case Slot::Kind::Real:
            texts.push_back(realText(value.real));
            break;
        case Slot::Kind::NamedReal:
            texts.push_back(value.text + "=" + realText(value.real));
            break;
        }
    }
    return texts;
}

} // namespace oor::model
