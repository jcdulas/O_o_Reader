#pragma once

#include "oor/base/diagnostics/DiagnosticSink.hpp"
#include "oor/deck/raw/RawCard.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/SlotTrace.hpp"
#include "oor/model/format/FieldValue.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace oor::model
{

// Reads typed values from the internal slots supplied by CardLayouts, reporting neutral field names.
//
// A bad field never throws: it reports an error, returns the fallback and marks the reader as failed(); the card's
// reader checks failed() once at the end and drops the card (tolerant mode keeps it as an unknown card).
class FieldReader
{
public:
    FieldReader(const deck::RawCardBlock& block, const deck::RawCard& card, base::DiagnosticSink& sink,
                std::string_view fileName) noexcept;
    FieldReader(std::span<const FieldValue> fields, const deck::RawCard& card, base::DiagnosticSink& sink,
                std::string_view fileName);

    [[nodiscard]] std::string_view cardName() const noexcept { return m_card.name.view(); }

    // Neutral field names are used directly in diagnostics.
    [[nodiscard]] std::string fieldLabel(const char* name) const;
    [[nodiscard]] std::string fieldLabel(std::string_view name) const;
    // Number of field slots (a multiple of 4 or 8).
    [[nodiscard]] int slotCount() const noexcept
    {
        return static_cast<int>(m_typed ? m_typedFields.size() : m_fields.size());
    }
    // The field as text (trimmed); empty when blank or beyond the card.
    [[nodiscard]] std::string_view text(int field) const
    {
#ifdef OOR_TRACE_SLOTS
        slot_trace::noteRead(cardName(), field, "text", {});
#endif
        if (field < 1 || field > slotCount())
            return {};
        return m_typed ? typedText(field) : m_fields[static_cast<std::size_t>(field - 1)];
    }
    [[nodiscard]] bool isBlank(int field) const noexcept;
    // Number of the last field that is not blank (0 for an empty card).
    [[nodiscard]] int lastUsedField() const noexcept;

    // Integers: blank gives `fallback`. A value that is not an integer, or does not fit in 32 bits, is an error.
    [[nodiscard]] std::int32_t integer(int field, const char* name, std::int32_t fallback);
    [[nodiscard]] std::int32_t requiredInteger(int field, const char* name);
    [[nodiscard]] std::optional<std::int32_t> optionalInteger(int field, const char* name);

    // Reals: blank gives `fallback`. Integers are accepted where a real is expected ("1" is 1.0).
    [[nodiscard]] double real(int field, const char* name, double fallback);
    [[nodiscard]] double requiredReal(int field, const char* name);
    [[nodiscard]] std::optional<double> optionalReal(int field, const char* name);

    // A field that is an integer (an ID) or a real (an angle): THETA/MCID. Blank gives nullopt.
    [[nodiscard]] std::optional<std::variant<std::int32_t, double>> integerOrReal(int field, const char* name);

    // A text value occupies its first internal slot; remaining slots in its declared span are padding.
    // The compact model record carries the complete string as one value.
    [[nodiscard]] std::string spanText(int first, int last) const;

    // Text: blank gives `fallback`.
    [[nodiscard]] std::string_view word(int field, std::string_view fallback = {}) const
    {
        const std::string_view value = text(field);
        return value.empty() ? fallback : value;
    }

    // Reports an error or a warning about a field (or about the card when `name` is empty).
    void error(const char* code, const std::string& message, const char* name = "");
    void warning(const char* code, const std::string& message, const char* name = "");
    // Reports a card check problem using its neutral field name.
    void report(const std::optional<CardProblem>& problem);
    // True once any error was reported for this card.
    [[nodiscard]] bool failed() const noexcept { return m_failed; }

    // From now on, every field read as a real (real, requiredReal, optionalReal, the real of integerOrReal) notes in
    // `digits` the number of significant digits it was written with, at index field - 1 (the vector grows as needed;
    // the other entries stay 0). The model reader records this precision for numeric source fields.
    void recordDigits(std::vector<std::uint8_t>& digits) noexcept { m_digits = &digits; }

    // From now on, every field read as an integer or a real notes in `names` its number and the name the card's reader
    // gives it ({3, "thickness"} for ShellProp): what lets a program set a field by its name.
    void recordNames(std::vector<std::pair<int, std::string>>& names) noexcept { m_names = &names; }

private:
    [[nodiscard]] std::string_view typedText(int field) const;
    [[nodiscard]] const FieldValue* typedValue(int field) const noexcept;
    [[nodiscard]] base::SourceLocation location(const char* name) const;
    void noteDigits(int field, std::string_view value);
    void noteName(int field, const char* name, const char* accessor);

    const deck::RawCard& m_card;
    std::span<const std::string_view> m_fields;
    bool m_typed = false;
    std::span<const FieldValue> m_typedFields;
    mutable std::vector<std::string> m_formatted;
    base::DiagnosticSink& m_sink;
    std::string_view m_file;
    bool m_failed = false;
    std::vector<std::uint8_t>* m_digits = nullptr;
    std::vector<std::pair<int, std::string>>* m_names = nullptr;
};

} // namespace oor::model
