#pragma once

#include "oor/base/text/DigitSets.hpp"
#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

// What the cards of the load closure have in common. Two kinds of value, one helper each: the
// name of a part or of a load in a PAA combination (LOADCLID, LOADCNAM, LOADCSUB) and the set of components of a grid
// point of a heat shell element (TEMPN1, SLOADN1). Nothing here describes the fields of a card: every card still lays
// out, reads, writes and checks its own fields by hand.
namespace oor::model
{

// ---------------------------------------------------------------------------------------------------------------------
// The names of a PAA load combination
// ---------------------------------------------------------------------------------------------------------------------
//
// A PARTNAME or a LOADNAME is up to 64 characters, which the format reference draws over the eight fields of a line of
// its own (the name starts in the first field of the line and may run to the last one). In the small format the lexer
// cuts it into pieces of eight characters, one per field; in the large and free formats it is the first field of the
// line. The reader puts the pieces together (FieldReader::spanText) and the writer writes the whole name in the first
// field, so the card formatter moves a card with a long name to the large or the free format by itself.

inline constexpr int PAA_NAME_FIELDS = 8;

[[nodiscard]] inline std::optional<CardProblem> checkPaaName(std::string_view name, const char* field)
{
    return checkNameWord(name, field);
}

[[nodiscard]] inline std::string readPaaName(FieldReader& reader, int firstField, const char* name)
{
    std::string text = reader.spanText(firstField, firstField + PAA_NAME_FIELDS - 1);
    if (text.empty())
        reader.error("card.missing-field", std::string(name) + " is required", name);
    return text;
}

// Writes the name in the first field of its line.
inline void writePaaName(FieldWriter& writer, int firstField, const std::string& name)
{
    writer.text(firstField, name);
}

// ---------------------------------------------------------------------------------------------------------------------
// The components of a grid point of a heat shell element
// ---------------------------------------------------------------------------------------------------------------------
//
// The C of a TEMPN1 and an SLOADN1 says where in the thickness the value acts: 1 the top, 2 the bottom, 3 the middle,
// as the digits of one integer written with no blank ("13" is the top and the middle, whatever the order of the
// digits). A blank field is 1. The library keeps the digits in ascending order.

inline constexpr std::int32_t SHELL_DEFAULT_COMPONENTS = 1;

[[nodiscard]] constexpr bool isShellComponentSet(std::int32_t value) noexcept
{
    if (value <= 0)
        return false;
    unsigned seen = 0;
    for (std::int32_t rest = value; rest > 0; rest /= 10)
    {
        const unsigned digit = static_cast<unsigned>(rest % 10);
        if (digit < 1 || digit > 3 || (seen & (1U << digit)) != 0)
            return false;
        seen |= 1U << digit;
    }
    return true;
}

[[nodiscard]] inline std::optional<CardProblem> checkShellComponents(std::int32_t components, const char* field)
{
    if (!isShellComponentSet(components))
        return CardProblem{"card.bad-value",
                           std::string(CardProblem::FIELD) +
                               " must be some of the digits 1 (top), 2 (bottom) and 3 (middle), once each",
                           field};
    return std::nullopt;
}

[[nodiscard]] constexpr std::int32_t sortedShellComponents(std::int32_t value) noexcept
{
    return isShellComponentSet(value) ? base::sortedDigits(value) : value;
}

[[nodiscard]] inline std::int32_t readShellComponents(FieldReader& reader, int field, const char* name)
{
    return sortedShellComponents(reader.integer(field, name, SHELL_DEFAULT_COMPONENTS));
}

// Writes the components, a blank when they are the default.
inline void writeShellComponents(FieldWriter& writer, int field, std::int32_t components)
{
    writer.integerUnlessDefault(field, components, SHELL_DEFAULT_COMPONENTS);
}

} // namespace oor::model
