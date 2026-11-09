#include "oor/model/format/FieldReader.hpp"

#include "oor/base/ids/EntityId.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/NumberFormatter.hpp"
#include "oor/model/format/SignificantDigits.hpp"

#include <functional>
#include <string>

namespace oor::model
{

FieldReader::FieldReader(const deck::RawCardBlock& block, const deck::RawCard& card, base::DiagnosticSink& sink,
                         std::string_view fileName) noexcept
    : m_card(card), m_fields(block.fieldsOf(card)), m_sink(sink), m_file(fileName)
{
}

FieldReader::FieldReader(std::span<const FieldValue> fields, const deck::RawCard& card, base::DiagnosticSink& sink,
                         std::string_view fileName)
    : m_card(card), m_typed(true), m_typedFields(fields), m_sink(sink), m_file(fileName)
{
}

const FieldValue* FieldReader::typedValue(int field) const noexcept
{
    return m_typed && field >= 1 && field <= slotCount() ? &m_typedFields[static_cast<std::size_t>(field - 1)] : nullptr;
}

bool FieldReader::isBlank(int field) const noexcept
{
    if (const FieldValue* value = typedValue(field))
        return value->kind == FieldValue::Kind::Blank ||
               (value->kind == FieldValue::Kind::Text && value->text.empty());
    return field < 1 || field > slotCount() || m_fields[static_cast<std::size_t>(field - 1)].empty();
}

std::string_view FieldReader::typedText(int field) const
{
    const FieldValue& value = *typedValue(field);
    if (value.kind == FieldValue::Kind::Blank)
        return {};
    if (value.kind == FieldValue::Kind::Text)
        return value.text;
    if (m_formatted.empty())
        m_formatted.resize(m_typedFields.size());
    std::string& formatted = m_formatted[static_cast<std::size_t>(field - 1)];
    if (formatted.empty())
        formatted = value.kind == FieldValue::Kind::Integer ? base::NumberFormatter::formatInteger(value.integer)
                                                           : base::NumberFormatter::formatRealFree(value.real);
    return formatted;
}

std::string FieldReader::fieldLabel(const char* name) const
{
    if (name == nullptr)
        return {};
    return fieldLabel(std::string_view(name));
}

std::string FieldReader::fieldLabel(std::string_view name) const
{
    return std::string(name);
}

// Source locations use exact neutral card and field names.
base::SourceLocation FieldReader::location(const char* name) const
{
    return base::SourceLocation{std::string(m_file), m_card.line, std::string(m_card.name.view()), fieldLabel(name)};
}

void FieldReader::error(const char* code, const std::string& message, const char* name)
{
    m_failed = true;
    m_sink.error(code, message, location(name));
}

void FieldReader::report(const std::optional<CardProblem>& problem)
{
    if (!problem)
        return;
    const std::string message = problem->text();
    error(problem->code, message, problem->field);
}

void FieldReader::warning(const char* code, const std::string& message, const char* name)
{
    m_sink.warning(code, message, location(name));
}

std::string FieldReader::spanText(int first, [[maybe_unused]] int last) const
{
#ifdef OOR_TRACE_SLOTS
    slot_trace::noteSpan(cardName(), first, last);
#endif
    return std::string(text(first));
}

int FieldReader::lastUsedField() const noexcept
{
    for (int field = slotCount(); field >= 1; --field)
    {
        if (!isBlank(field))
            return field;
    }
    return 0;
}

void FieldReader::noteName(int field, const char* name, [[maybe_unused]] const char* accessor)
{
#ifdef OOR_TRACE_SLOTS
    slot_trace::noteRead(cardName(), field, accessor, name != nullptr ? name : "");
#endif
    if (m_names != nullptr && name != nullptr && name[0] != '\0')
        m_names->emplace_back(field, name);
}

std::optional<std::int32_t> FieldReader::optionalInteger(int field, const char* name)
{
    noteName(field, name, "integer");
    if (const FieldValue* value = typedValue(field); value && value->kind == FieldValue::Kind::Integer)
    {
        const auto id = base::toEntityId(value->integer);
        if (!id)
            error("card.integer-too-large", "field " + fieldLabel(name) + " does not fit in 32 bits", name);
        return id;
    }
    const std::string_view value = text(field);
    if (value.empty())
        return std::nullopt;
    const auto parsed = base::NumberParser::parseInteger(value);
    if (!parsed)
    {
        error("card.bad-integer",
              "field " + fieldLabel(name) + ": expected an integer, found \"" + std::string(value) + "\"", name);
        return std::nullopt;
    }
    const auto id = base::toEntityId(*parsed);
    if (!id)
    {
        error("card.integer-too-large",
              "field " + fieldLabel(name) + ": " + std::string(value) + " does not fit in 32 bits", name);
        return std::nullopt;
    }
    return *id;
}

std::int32_t FieldReader::integer(int field, const char* name, std::int32_t fallback)
{
    return optionalInteger(field, name).value_or(fallback);
}

std::int32_t FieldReader::requiredInteger(int field, const char* name)
{
    const auto value = optionalInteger(field, name);
    if (!value)
    {
        if (isBlank(field))
            error("card.missing-field", "field " + fieldLabel(name) + " is required", name);
        return 0;
    }
    return *value;
}

std::optional<double> FieldReader::optionalReal(int field, const char* name)
{
    noteName(field, name, "real");
    if (const FieldValue* value = typedValue(field))
    {
        if (value->kind == FieldValue::Kind::Real)
            return value->real;
        if (value->kind == FieldValue::Kind::Integer)
            return static_cast<double>(value->integer);
    }
    const std::string_view value = text(field);
    if (value.empty())
        return std::nullopt;
    const auto parsed = base::NumberParser::parseReal(value);
    if (!parsed)
    {
        error("card.bad-real",
              "field " + fieldLabel(name) + ": expected a real number, found \"" + std::string(value) + "\"", name);
        return std::nullopt;
    }
    noteDigits(field, value);
    return *parsed;
}

void FieldReader::noteDigits(int field, std::string_view value)
{
    if (m_digits == nullptr || field < 1)
        return;
    const auto index = static_cast<std::size_t>(field - 1);
    if (m_digits->size() <= index)
        m_digits->resize(index + 1, 0);
    (*m_digits)[index] = significantDigitsOf(value);
}

double FieldReader::real(int field, const char* name, double fallback)
{
    return optionalReal(field, name).value_or(fallback);
}

double FieldReader::requiredReal(int field, const char* name)
{
    const auto value = optionalReal(field, name);
    if (!value)
    {
        if (isBlank(field))
            error("card.missing-field", "field " + fieldLabel(name) + " is required", name);
        return 0.0;
    }
    return *value;
}

std::optional<std::variant<std::int32_t, double>> FieldReader::integerOrReal(int field, const char* name)
{
    noteName(field, name, "number");
    if (const FieldValue* value = typedValue(field))
    {
        if (value->kind == FieldValue::Kind::Real)
            return std::variant<std::int32_t, double>(value->real);
        if (value->kind == FieldValue::Kind::Integer)
        {
            if (const auto id = base::toEntityId(value->integer))
                return std::variant<std::int32_t, double>(*id);
        }
    }
    const std::string_view value = text(field);
    if (value.empty())
        return std::nullopt;
    // Preserve the distinction between integer identifiers and real values.
    if (const auto integerValue = base::NumberParser::parseInteger(value))
    {
        if (const auto id = base::toEntityId(*integerValue))
            return std::variant<std::int32_t, double>(*id);
    }
    if (const auto realValue = base::NumberParser::parseReal(value))
    {
        noteDigits(field, value);
        return std::variant<std::int32_t, double>(*realValue);
    }
    error("card.bad-number",
          "field " + std::to_string(field + 1) + " (" + fieldLabel(name) +
              "): expected an integer or a real, found \"" + std::string(value) + "\"",
          name);
    return std::nullopt;
}

} // namespace oor::model
