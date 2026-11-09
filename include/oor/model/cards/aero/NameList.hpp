#pragma once

#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <optional>
#include <span>
#include <string>
#include <vector>

// A list of names in consecutive fields: the component names of an AECOMPL and the character items of an AELISTC. One
// kind of value shared by two cards of the family; each card still lays out its own fields and says what its names
// stand for. A name is one to eight characters that read back from every format (`checkNameWord`).
namespace oor::model
{

// Reads the names of fields `first` to the last used field, as written. A blank field between two names is an error
// (both format reference entries forbid it, and a reader that stops at the blank would lose what follows). `name` is
// the field name of the list (`labels` for LABEL1, LABEL2, ...): the diagnostics show its keyword. It must be a string
// literal.
[[nodiscard]] std::vector<std::string> readNameList(FieldReader& reader, int first, const char* name);

// Writes the names from field `first` on, one to a field, and returns the next free field.
int writeNameList(FieldWriter& writer, int first, std::span<const std::string> names);

// The first problem of a list: at least one name, each a valid name. `name` is a string literal: it stays in the
// problem.
[[nodiscard]] std::optional<CardProblem> checkNameList(std::span<const std::string> names, const char* name);

} // namespace oor::model
