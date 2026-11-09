#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"

#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>

// What the thermal and acoustic cards have in common: the check of the cells a fixed layout leaves blank and of the end
// of the card, and the reading of a keyword field. Each card still lays out its own fields by hand.
namespace oor::model
{

// Reports an error when one of `fields` is not blank, or when a field after `lastField` is used: the gaps of a card the
// format reference draws with blank cells, and its end. The message names the card and the field.
void requireUnusedBlank(FieldReader& reader, std::initializer_list<int> fields, int lastField);

// The keyword in `field`, in upper case; a blank field gives `fallback`. A word that is not in `words` is an error on
// `name` (the message lists the words) and gives the fallback.
[[nodiscard]] std::string readKeyword(FieldReader& reader, int field, const char* name,
                                      std::initializer_list<std::string_view> words, std::string_view fallback);

// The problem with `word` as the value of the keyword field `name` (a word of `words`), if there is one.
[[nodiscard]] std::optional<CardProblem> checkKeyword(std::string_view word, const char* name,
                                                      std::initializer_list<std::string_view> words);

} // namespace oor::model
