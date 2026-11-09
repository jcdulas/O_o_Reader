#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"

#include <initializer_list>
#include <optional>
#include <string>
#include <vector>

// What the material cards and their companions (MATT1 to MATT11, MATS1, CREEP, MATEP, MATVE) have in common: the rule
// of a material ID, of a table ID and the check of the fields a fixed layout leaves blank. Each card still lays out its
// own fields by hand.
namespace oor::model
{

// The problem with the ID of a material (MAT3, MAT9, ... and the MID of a companion that extends one), if there is one.
[[nodiscard]] std::optional<CardProblem> checkMaterialId(base::EntityId id);

// The problem with a table ID field of a companion (T(E) of a MATT1, TID of a MATS1, ...), if there is one: 0 is a
// blank field, a negative ID is allowed only where `allowNegative` says so (the T(A) fields: a negative TABLEMi ID is a
// table of thermal strains).
[[nodiscard]] std::optional<CardProblem> checkTableReference(base::EntityId id, const char* field,
                                                             bool allowNegative = false);

// Reports an error when one of `fields` is not blank, or when a field after `lastField` is used: the gaps of a card
// the format reference draws with blank cells, and its end. The message names the card and the field.
void requireBlankFields(FieldReader& reader, std::initializer_list<int> fields, int lastField);

// Reports an error when a field that is not in `usedFields` has a value: the layout of a card with variants lists the
// fields its variant uses, and everything else up to the end of the card must be blank.
void requireOnlyFields(FieldReader& reader, const std::vector<int>& usedFields);

// The keywords of the fields `names` of the card being read, for a message: "REFFECT, ANISO or PRESS" (the keywords
// that start the lines of a card made of keyword lines).
[[nodiscard]] std::string fieldNameList(std::initializer_list<const char*> names);

} // namespace oor::model
