#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

// What the cards of the explicit solver (SOL 700) have in common. Each helper moves one kind of value: the rule of an
// ID or a real, a field that holds a keyword of the format reference, a quantity that is either a number or a TABLED1,
// and the visit of a reference. No helper describes the fields of a card: every card still lays out, reads, writes and
// checks its own fields by hand.
namespace oor::model
{

// ---------------------------------------------------------------------------------------------------------------------
// The rule of an ID and of a real
// ---------------------------------------------------------------------------------------------------------------------

// The problem with an ID that the card needs (a positive integer), if there is one.
[[nodiscard]] std::optional<CardProblem> checkRequiredId(base::EntityId id, const char* field);

// The problem with an ID that may be blank (0 is a blank field, anything else must be a positive integer).
[[nodiscard]] std::optional<CardProblem> checkOptionalId(base::EntityId id, const char* field);

enum class RealRule : std::uint8_t
{
    Finite,      // any number
    Positive,    // greater than zero
    NonNegative, // zero or greater
};

// The problem with a real, if there is one: never infinite or NaN, and greater than zero or not negative where `rule`
// says so.
[[nodiscard]] std::optional<CardProblem> checkReal(double value, const char* field, RealRule rule);

// The same for a real that may be blank: a blank field has no problem.
[[nodiscard]] std::optional<CardProblem> checkOptionalReal(const std::optional<double>& value, const char* field,
                                                           RealRule rule);

// The problem with a real that must lie in [low, high], if there is one.
[[nodiscard]] std::optional<CardProblem> checkRealRange(double value, const char* field, double low, double high);

// A required literal name. Its exact spelling, case and length are preserved; control characters are refused.
[[nodiscard]] std::optional<CardProblem> checkNameWord(std::string_view word, const char* field);

// A name that is either a neutral name of the format (a word: letters and digits with a lower-case letter, any length,
// "ExternalFlow", "VelocityX") or a name of the user that checkNameWord accepts: the problem, if there is one.
[[nodiscard]] std::optional<CardProblem> checkWordOrName(std::string_view word, const char* field);

// A user routine name as a card keeps it: the neutral name of the routine the card defaults to as it is written, any
// other user routine name exactly as supplied.
[[nodiscard]] std::string routineNameOf(std::string_view text, std::string_view defaultName);

// The problem with a user routine name, if there is one: the default routine, or a name checkNameWord accepts.
[[nodiscard]] std::optional<CardProblem> checkRoutineName(std::string_view name, std::string_view defaultName,
                                                          const char* field);

// A field that names a type of card in free text (the TYPE of a DVLREL1, the PTYPE of a TOPVAR): the card keeps the
// neutral name of a card type ("PointForceLoad"), and the word in upper case for any other.
[[nodiscard]] std::string readCardTypeWord(FieldReader& reader, int field);

// The problem with such a field, if there is one: a neutral name (it has lower-case letters) is taken as it is, a word
// of the deck follows checkNameWord.
[[nodiscard]] std::optional<CardProblem> checkCardTypeWord(std::string_view word, const char* field);

// ---------------------------------------------------------------------------------------------------------------------
// Keywords
// ---------------------------------------------------------------------------------------------------------------------
//
// A field that holds one of the words the format reference lists is an enumeration in the card value. Its enumerators
// are numbered from 0 in the order of the array of words that the card declares next to the enumeration, so the
// position of a word is the value of its enumerator. Neutral words are case-sensitive; the card keeps the enumerator
// and writes its canonical neutral name.

template <typename Keyword, std::size_t COUNT>
[[nodiscard]] constexpr std::string_view keywordText(const std::array<std::string_view, COUNT>& words,
                                                     Keyword value) noexcept
{
    return words[static_cast<std::size_t>(value)];
}

template <typename Keyword, std::size_t COUNT>
[[nodiscard]] constexpr std::optional<Keyword> parseKeyword(const std::array<std::string_view, COUNT>& words,
                                                            std::string_view text) noexcept
{
    for (std::size_t index = 0; index < COUNT; ++index)
    {
        if ((words[index] == text))
            return static_cast<Keyword>(index);
    }
    return std::nullopt;
}

// The words as a phrase for a message: "INSIDE, OUTSIDE or NONE".
template <std::size_t COUNT>
[[nodiscard]] std::string keywordList(const std::array<std::string_view, COUNT>& words)
{
    std::string phrase;
    for (std::size_t index = 0; index < COUNT; ++index)
    {
        if (index > 0)
            phrase += index + 1 == COUNT ? " or " : ", ";
        phrase += words[index];
    }
    return phrase;
}

// A keyword field with a default: blank gives `fallback`; a word that is not in the list is an error.
template <typename Keyword, std::size_t COUNT>
[[nodiscard]] Keyword readKeyword(FieldReader& reader, int field, const char* name,
                                  const std::array<std::string_view, COUNT>& words, Keyword fallback)
{
    const std::string_view text = reader.text(field);
    if (text.empty())
        return fallback;
    if (const std::optional<Keyword> keyword = parseKeyword<Keyword>(words, text))
        return *keyword;
    reader.error("card.bad-value",
                 std::string(name) + " must be " + keywordList(words) + ", not \"" + std::string(text) + "\"", name);
    return fallback;
}

// A keyword field that may be blank and has no default: blank gives nothing.
template <typename Keyword, std::size_t COUNT>
[[nodiscard]] std::optional<Keyword> readOptionalKeyword(FieldReader& reader, int field, const char* name,
                                                         const std::array<std::string_view, COUNT>& words)
{
    if (reader.isBlank(field))
        return std::nullopt;
    return readKeyword<Keyword>(reader, field, name, words, static_cast<Keyword>(0));
}

// A keyword field that the card requires.
template <typename Keyword, std::size_t COUNT>
[[nodiscard]] Keyword readRequiredKeyword(FieldReader& reader, int field, const char* name,
                                          const std::array<std::string_view, COUNT>& words)
{
    if (reader.isBlank(field))
    {
        reader.error("card.missing-field", std::string(name) + " is required: " + keywordList(words), name);
        return static_cast<Keyword>(0);
    }
    return readKeyword<Keyword>(reader, field, name, words, static_cast<Keyword>(0));
}

template <typename Keyword, std::size_t COUNT>
void writeKeyword(FieldWriter& writer, int field, const std::array<std::string_view, COUNT>& words, Keyword value)
{
    writer.text(field, keywordText(words, value));
}

// A default that does not depend on another field is a blank when it is the default (FieldWriter::omitsDefaults).
template <typename Keyword, std::size_t COUNT>
void writeKeywordUnlessDefault(FieldWriter& writer, int field, const std::array<std::string_view, COUNT>& words,
                               Keyword value, Keyword fallback)
{
    writer.textUnlessDefault(field, keywordText(words, value), keywordText(words, fallback));
}

template <typename Keyword, std::size_t COUNT>
void writeOptionalKeyword(FieldWriter& writer, int field, const std::array<std::string_view, COUNT>& words,
                          const std::optional<Keyword>& value)
{
    if (value)
        writer.text(field, keywordText(words, *value));
}

// ---------------------------------------------------------------------------------------------------------------------
// A quantity that is a number, a TABLED1 or the value of the element
// ---------------------------------------------------------------------------------------------------------------------
//
// Many entries of the airbag and coupling cards give a quantity with a keyword in one field and its value in the next
// one: CONSTANT and a number, TABLE and the ID of a TABLED1 (the quantity varies with time), or ELEMENT (the value of
// the Euler element: no value at all). The card says which of the three it accepts and which one a blank keyword means.

enum class ValueSource : std::uint8_t
{
    Element,
    Constant,
    Table,
};

inline constexpr std::array<std::string_view, 3> VALUE_SOURCE_WORDS{"Element", "Constant", "Table"};

// The bit of a source in `SourcedValueLayout::allowed`.
[[nodiscard]] constexpr std::uint8_t sourceBit(ValueSource source) noexcept
{
    return static_cast<std::uint8_t>(1U << static_cast<unsigned>(source));
}

struct SourcedValue
{
    ValueSource source = ValueSource::Constant;
    double constant = 0.0;    // the number, when the source is Constant
    base::EntityId table = 0; // the TABLED1, when the source is Table

    [[nodiscard]] bool operator==(const SourcedValue&) const noexcept = default;
};

// Where a sourced quantity is in its card and what the card accepts.
struct SourcedValueLayout
{
    int sourceField;        // the field of the keyword
    int valueField;         // the field of the number or the table
    const char* sourceName; // the names of the format reference, for the messages and the diagnostics
    const char* valueName;
    std::uint8_t allowed;   // the sources the card accepts, as the bits of sourceBit
    ValueSource fallback;   // the source a blank keyword means
    double defaultConstant; // the number a blank value field means when the source is Constant
};

// Reads the two fields. A source the card does not accept, a value with ELEMENT and a table that is not a positive
// integer are errors.
[[nodiscard]] SourcedValue readSourcedValue(FieldReader& reader, const SourcedValueLayout& layout);

// Writes the two fields: the keyword is left blank when it is the one a blank means, and so is a constant that is the
// default.
void writeSourcedValue(FieldWriter& writer, const SourcedValueLayout& layout, const SourcedValue& value);

// The problem with the values, if there is one: a source the card does not accept, a constant that breaks `rule`, a
// table that is not positive.
[[nodiscard]] std::optional<CardProblem> checkSourcedValue(const SourcedValue& value, const SourcedValueLayout& layout,
                                                           RealRule rule);

// ---------------------------------------------------------------------------------------------------------------------
// References
// ---------------------------------------------------------------------------------------------------------------------

// Visits an ID field that names another card, and stores what the visitor returns. A blank field (0) is not visited.
inline void visitId(ReferenceVisitor& visitor, std::string_view field, ReferenceTarget target, base::EntityId& id)
{
    if (id > 0)
        id = visitor.visit(field, target, id);
}

// Visits the TABLED1 of a quantity whose source is Table.
inline void visitSourcedTable(ReferenceVisitor& visitor, std::string_view field, SourcedValue& value)
{
    if (value.source == ValueSource::Table)
        visitId(visitor, field, ReferenceTarget::DynamicTable, value.table);
}

} // namespace oor::model
