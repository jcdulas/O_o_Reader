#pragma once

#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

// What the aeroelastic cards have in common. Each helper moves one kind of value used by several cards of the family:
// the USAGE word of a spline, the symmetry word of a parametric force, a list of reals. No helper describes the fields
// of a card: every card still lays out, reads, writes and checks its own fields by hand. The rule of an ID, a real or a
// name is the one of `explicit/ExplicitSupport.hpp` (`checkRequiredId`, `checkOptionalId`, `checkReal`,
// `checkNameWord`, the keyword readers), which other families use as well.
namespace oor::model
{

// ---------------------------------------------------------------------------------------------------------------------
// The USAGE of a spline: which transformation the interpolation is part of (SPLINE1 to SPLINE5 and
// SPLINE7). BOTH is the default, so a blank field is BOTH.
// ---------------------------------------------------------------------------------------------------------------------

enum class SplineUsage : std::uint8_t
{
    Force,
    Displacement,
    Both
};

// The words of the format reference, in the order of the enumerators.
inline constexpr std::array<std::string_view, 3> SPLINE_USAGE_WORDS{"Force", "Displacement", "Both"};

// ---------------------------------------------------------------------------------------------------------------------
// The symmetry word of a parametric force (SYMXZ and SYMXY of AEDW, AEFORCE and AEPRESS).
// ---------------------------------------------------------------------------------------------------------------------

enum class ForceSymmetry : std::uint8_t
{
    Symmetric,
    Asymmetric,
    Antisymmetric
};

inline constexpr std::array<std::string_view, 3> FORCE_SYMMETRY_WORDS{"Symmetric", "Asymmetric", "Antisymmetric"};

// ---------------------------------------------------------------------------------------------------------------------
// The reserved labels of a trim (AESTAT, TRIM, TRIM2, AELINK, UXVEC): the rigid body motions (the angle of attack,
// the side slip, the roll, pitch and yaw rates and the six rigid body accelerations) and the constant term of a control
// vector. Any other label is a name of the user (an AESURF, an AEPARM, an AESTAT of its own).
// ---------------------------------------------------------------------------------------------------------------------

inline constexpr std::array<std::string_view, 12> TRIM_RESERVED_LABELS{
    "AngleOfAttack",      "SideSlip",    "RollRate",    "PitchRate",          "YawRate",
    "RigidAccelX",        "RigidAccelY", "RigidAccelZ", "RigidAngularAccelX", "RigidAngularAccelY",
    "RigidAngularAccelZ", "Intercept"};

// The problem with a trim label, if there is one: a reserved label of TRIM_RESERVED_LABELS, or a name that
// `checkNameWord` accepts.
[[nodiscard]] std::optional<CardProblem> checkTrimLabel(std::string_view label, const char* field);

// ---------------------------------------------------------------------------------------------------------------------
// A list of reals in consecutive fields (AEFACT, FLFACT, DIVERG, the two lists of MKAERO1)
// ---------------------------------------------------------------------------------------------------------------------

// Reads the reals of fields `first` to `last`, in order, up to the last used field of that range. A field that is not a
// real is an error, and so is a blank field between two values (AEFACT and FLFACT refuse a blank between
// values; on MKAERO1 and DIVERG a blank closes the list: a value after it would be lost by a reader that stops
// there, so it is refused here too). Pass `reader.slotCount()` as `last` for a list that runs to the end of the card.
// `name` is the field name of the list (`values` for D1, D2, ...): the diagnostics show its keyword.
[[nodiscard]] std::vector<double> readRealList(FieldReader& reader, int first, int last, const char* name);

// Writes the values from field `first` on, one to a field, and returns the next free field.
int writeRealList(FieldWriter& writer, int first, std::span<const double> values);

// The first value that is not a finite number, as a problem on field `name` (nothing for an empty list).
[[nodiscard]] std::optional<CardProblem> checkRealListFinite(std::span<const double> values, const char* name);

} // namespace oor::model
