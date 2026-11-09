#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

// The two kinds of field that RLOAD1, RLOAD2, TLOAD1 and TLOAD2 have in common. Each card still lays out its own fields
// by hand; these functions only read, write and check a TYPE field and an "ID or real" field the same way everywhere.
namespace oor::model
{

// TYPE of a dynamic load: what the excitation is. A model gives an integer code or an exact neutral name. The codes
// 4, 12 and 13 are for the explicit solver and have no word.
enum class ExcitationType : std::uint8_t
{
    Load = 0, // applied force or moment (the default)
    Displacement = 1,
    Velocity = 2,
    Acceleration = 3,
    Flow = 4,
    RigidBodyVelocity = 12,
    RigidBodyForce = 13
};

// The type with this text (an integer code or Load, Displacement, Velocity, Acceleration); nothing for another value.
[[nodiscard]] std::optional<ExcitationType> parseExcitationType(std::string_view text);
// Load, Displacement, Velocity or Acceleration; empty for the explicit codes, which are written as integers.
[[nodiscard]] const char* excitationTypeWord(ExcitationType type) noexcept;
// Reads the TYPE field: blank is Load. A text that is not a type is an error (and gives Load).
[[nodiscard]] ExcitationType readExcitationType(FieldReader& reader, int field);
// Writes the word of the type, or its code when it has no word; Load is the default and is left blank.
void writeExcitationType(FieldWriter& writer, int field, ExcitationType type);

// What the EXCITEID of a dynamic load refers to: a DAREA or static load set for an applied load; for an enforced
// motion it is the set of an SPCD card, which is not a typed card yet, so nothing is resolved.
[[nodiscard]] ReferenceTarget excitationTarget(ExcitationType type) noexcept;
// Visits a reference field unless it is blank (0), and gives the ID the visitor returns.
[[nodiscard]] base::EntityId visitIfSet(ReferenceVisitor& visitor, const char* field, ReferenceTarget target,
                                        base::EntityId id);

// A field that holds the ID of another card (an integer) or a constant (a real): DELAYI/DELAYR, TC/RC, TID/F, ... The
// two live in separate members of the card: `id` is 0 and `value` 0.0 when the field is blank, and at most one of them
// is set.
void readIdOrReal(FieldReader& reader, int field, const char* idName, base::EntityId& id, double& value);
// An ID is written as an integer; otherwise the constant is written (blank when it is 0.0, unless defaults are kept).
void writeIdOrReal(FieldWriter& writer, int field, base::EntityId id, double value);
// The problem with the two members of an ID-or-real field, if there is one.
[[nodiscard]] std::optional<CardProblem> checkIdOrReal(base::EntityId id, double value, const char* idName);

} // namespace oor::model
