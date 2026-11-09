#pragma once

#include "oor/model/cards/explicit/ExplicitSupport.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

// What the two free-body cards (FBODYLD and FBODYSB) have in common: the text of a name and of a LABEL. One kind of
// value (a short text), two small rules; nothing here describes the fields of a card, each card still lays out, reads,
// writes and checks its own fields by hand.
//
// The format reference draws the LABEL over the eight fields of the second line, a string of at most 64 characters. In
// the small format the lexer cuts the line into pieces of eight characters, one per field, and trims each piece; in the
// large and free formats the label is the first field of the line (a longer text than the field takes moves the card to
// the free format). The reader puts the pieces together as written (FieldReader::spanText) and the writer cuts the
// label into pieces again (FieldWriter::spanText), so the fixed layout of the format comes back as it was.
namespace oor::model
{

[[nodiscard]] inline std::optional<CardProblem> checkFreeBodyName(std::string_view name, const char* field)
{
    return checkNameWord(name, field);
}

inline constexpr int FREE_BODY_LABEL_FIRST_FIELD = 9;
inline constexpr int FREE_BODY_LABEL_LAST_FIELD = 16;

[[nodiscard]] inline std::optional<CardProblem> checkFreeBodyLabel(std::string_view label)
{
    return label.empty() ? std::nullopt : checkNameWord(label, "label");
}

[[nodiscard]] inline std::string readFreeBodyLabel(const FieldReader& reader)
{
    return reader.spanText(FREE_BODY_LABEL_FIRST_FIELD, FREE_BODY_LABEL_LAST_FIELD);
}

// Writes the label; an empty one writes nothing, so a card with no label has no second line.
inline void writeFreeBodyLabel(FieldWriter& writer, const std::string& label)
{
    if (label.empty())
        return;
    writer.spanText(FREE_BODY_LABEL_FIRST_FIELD, FREE_BODY_LABEL_LAST_FIELD - FREE_BODY_LABEL_FIRST_FIELD + 1, label);
}

} // namespace oor::model
