#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <string_view>

namespace oor::model
{

// The letters of the XFLAG of an FBODYSB: the types of grid point force that are left out of the subsystem (any
// combination of them): S the forces of the single point constraints, M those of the multipoint
// constraints, A, L and P the applied loads, D the DMIG matrices and every other type the format does not name.
inline constexpr std::string_view FREE_BODY_EXCLUSION_LETTERS = "SMALPD";

// One FBODYSB card: an equilibrated free-body subsystem. The grid point forces that have both an included grid point
// (GRIDSET) and an included element (ELEMSET) or force type make up the loads of the subsystem; XFLAG leaves some types
// of force out.
//
// Card layout: FBODYSB  NAMES  GRIDSET  ELEMSET  XFLAG
//                      .      LABEL (the second line, fields 2 to 9: up to 64 characters)
// In this library's numbering (field 1 is NAMES): GRIDSET is field 2, ELEMSET 3, XFLAG 4 and the LABEL fields 9 to 16,
// the second line (the continuation line is optional). Fields 5 to 8 of the first line must be blank.
//
// A subsystem is named, not numbered (IdSpace::None, category Other): NAMES is the key of the card and is what an
// FBODYLD names in its FBODYSB field; a name is kept as written and matched without regard to the case, and no
// reference resolves it. GRIDSET is required and names a SET1 (the grid points whose forces are included); ELEMSET is a
// SET1 of elements and may be blank, and then no element contributes. Both are `Set` references (the sets of SET1, SET3
// and their kin: the format says SET1 only, and the library does not tell the kinds of set apart). XFLAG is kept in
// upper case, as written; a blank one excludes nothing.
struct FreeBodyLoadSubcaseCard
{
    std::string names;
    base::EntityId nodeSetId = 0;
    base::EntityId elementSetId = 0; // 0: blank
    std::string excludedForces;      // letters of FREE_BODY_EXCLUSION_LETTERS, possibly none
    std::string label;               // up to 64 characters, may be empty

    [[nodiscard]] bool operator==(const FreeBodyLoadSubcaseCard&) const noexcept = default;
};

} // namespace oor::model
