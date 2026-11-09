#pragma once

#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// What the fatigue and output-file cards of the solution control family (FatigueDefinition, FatigueEvent, FatigueLoad,
// FatigueParams, FatigueSequence, TimeToSpectrum, UserFileName, ResultFileModules, ModuleResultFileParams and the
// TableTimeHistoryFile of the tables) have in common: the arithmetic of the lines of a card, the cutting of the
// continuation lines into the sections that start with a keyword and the rule of a text that has to read back from
// every format. Each helper moves one kind of value or one kind of layout; every card still lays out, reads, writes and
// checks its own fields by hand.
namespace oor::model
{

// A line of a card has eight fields in every format (a large format line is half of one, the field numbers do not
// change), so the continuation line `line` starts at field 8 * line + 1 of the reader numbering.
inline constexpr int FATIGUE_FIELDS_PER_LINE = 8;

// The field of the reader numbering that the format reference calls field `formatField` (2 to 9) of line `line` (0 is
// the line of the card name, 1 the first continuation line). The format reference counts the card name as field 1, the
// reader does not: the ID of a card is field 1 of the reader and field 2 of the format reference, the keyword that
// starts a section is field 2 of its line.
[[nodiscard]] constexpr int fatigueField(int line, int formatField) noexcept
{
    return line * FATIGUE_FIELDS_PER_LINE + formatField - 1;
}

// The number of the last line that has a field in use (0 when only the line of the card name has one).
[[nodiscard]] inline int lastUsedLine(const FieldReader& reader) noexcept
{
    const int last = reader.lastUsedField();
    return last <= 0 ? 0 : (last - 1) / FATIGUE_FIELDS_PER_LINE;
}

// True when a field of the line, from format reference field 2 to 9, is not blank.
[[nodiscard]] inline bool lineHasData(const FieldReader& reader, int line) noexcept
{
    for (int formatField = 2; formatField <= 9; ++formatField)
    {
        if (!reader.isBlank(fatigueField(line, formatField)))
            return true;
    }
    return false;
}

// The lines of a section: the line that holds its keyword in field 2 and the lines after it with a blank field 2, up to
// the next keyword.
struct LineSection
{
    std::string keyword; // upper case
    int firstLine = 0;
    int lastLine = 0;
};

// Cuts the continuation lines of a card into sections at the lines that have a keyword in field 2. Every keyword must
// be one of `keywords` (compared without regard to the case). A line with a blank field 2 goes on with the section
// above it; before the first keyword only blank lines are allowed (a lone comma of a free format card). Reports the
// first fault and returns what it has cut so far.
[[nodiscard]] inline std::vector<LineSection> cutLineSections(FieldReader& reader,
                                                              std::span<const std::string_view> keywords)
{
    std::vector<LineSection> sections;
    const int last = lastUsedLine(reader);
    for (int line = 1; line <= last && !reader.failed(); ++line)
    {
        const std::string_view keyword = reader.text(fatigueField(line, 2));
        if (keyword.empty())
        {
            if (!sections.empty())
                sections.back().lastLine = line;
            else if (lineHasData(reader, line))
                reader.error("card.bad-value",
                             "a continuation line of a " + std::string(reader.cardName()) +
                                 " that has data starts with a keyword in field 2",
                             "");
            continue;
        }
        const auto known = std::find(keywords.begin(), keywords.end(), keyword);
        if (known == keywords.end())
        {
            std::string expected;
            for (const std::string_view word : keywords)
                expected += (expected.empty() ? "" : ", ") + std::string(word);
            reader.error("card.bad-value",
                         "\"" + std::string(keyword) + "\" is not a keyword of a " + std::string(reader.cardName()) +
                             " (" + expected + ")",
                         "");
            break;
        }
        sections.push_back(LineSection{std::string(*known), line, line});
    }
    return sections;
}

// Reports an error when one of the format reference fields `firstFormatField` to `lastFormatField` of `line` has a
// value: the cells that a layout draws blank. `section` names the keyword of the line for the message.
inline void requireBlankLineFields(FieldReader& reader, int line, int firstFormatField, int lastFormatField,
                                   std::string_view section)
{
    for (int formatField = firstFormatField; formatField <= lastFormatField; ++formatField)
    {
        if (!reader.isBlank(fatigueField(line, formatField)))
        {
            reader.error("card.bad-value",
                         "field " + std::to_string(formatField) + " of the " + std::string(section) + " line of a " +
                             std::string(reader.cardName()) + " is not used and must be blank",
                         "");
            return;
        }
    }
}

// A required text value with exact spelling and no field-width limit.
[[nodiscard]] inline std::optional<CardProblem> checkSpannedText(std::string_view text, const char* field)
{
    if (text.empty())
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be blank", field};
    for (const char character : text)
        if (static_cast<unsigned char>(character) < 0x20 || character == 0x7F)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " must not contain control characters", field};
    return std::nullopt;
}

} // namespace oor::model
