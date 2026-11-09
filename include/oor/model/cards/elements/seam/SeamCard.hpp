#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace oor::model
{

// CTYPE of a CSEAM: what the four patch numbers IDAS, IDBS, IDAE and IDBE name. A blank CTYPE is PSHELL.
enum class SeamPatchType : std::uint8_t
{
    Property, // PSHELL (the default): property IDs of PSHELL cards
    Element   // ELEM: element IDs
};

inline constexpr std::array<std::string_view, 2> SEAM_PATCH_TYPE_WORDS{"ShellProp", "Elements"};

// How a CSEAM gives the start and the end of the seam: by the grid points GS and GE, or by the coordinates XS YS ZS and
// XE YE ZE (the alternate format of the format reference).
enum class SeamEnds : std::uint8_t
{
    GridPoints,
    Coordinates
};

// One CSEAM card: a seam that connects two surface patches (shell elements).
//
// Card layout (the first form, with the grid points):
//   CSEAM  EID  PID  SMLN  CTYPE  IDAS  IDBS  IDAE  IDBE
//          GS   GE
// and the alternate form, which gives the ends as coordinates and leaves SMLN out:
//   CSEAM  EID  PID  (blank)  CTYPE  IDAS  IDBS  IDAE  IDBE
//          XS   YS   ZS   XE   YE   ZE
//
// PID names a PSEAM. SMLN names the seam line (up to eight characters, no blank; blank is nothing): the seam elements
// of one line are continuous where their ends coincide. The alternate form has no SMLN in the format reference; the
// card keeps one that a deck writes there rather than refuse it. CTYPE says what the patch numbers are (PSHELL by
// default).
//
// IDAS and IDBS define the patches A and B, or their start for a tailored blank; IDAE and IDBE their end. IDAS is
// required. With CTYPE PSHELL an IDBS that is blank or equal to IDAS makes the patch two-sided, and so for
// the end; with CTYPE ELEM IDBS is required and differs from IDAS, and IDAE and IDBE differ when both are given. A
// blank IDBS, IDAE or IDBE is 0.
//
// GS and GE are the piercing points on the patches of the start and the end (positive when given). In the alternate
// form the six coordinates are reals or blank and the format reference gives no default for a blank one, so the card
// keeps a blank as nothing; at least one of the six must be given, because a card with none would read back as the
// first form. `ends` says which form the card is: the other form's members are blank (0 for the grid points, nothing
// for the coordinates).
struct SeamCard
{
    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::string seamLine;
    SeamPatchType patchKind = SeamPatchType::Property;
    base::EntityId patchAStart = 0;
    base::EntityId patchBStart = 0;
    base::EntityId patchAEnd = 0;
    base::EntityId patchBEnd = 0;
    SeamEnds ends = SeamEnds::GridPoints;
    std::array<base::EntityId, 2> nodes = {};      // GS and GE; 0 in the coordinate form
    std::array<std::optional<double>, 3> startXyz; // XS, YS, ZS; blank in the grid-point form
    std::array<std::optional<double>, 3> endXyz;   // XE, YE, ZE; blank in the grid-point form

    [[nodiscard]] bool hasCoordinates() const noexcept { return ends == SeamEnds::Coordinates; }
};

} // namespace oor::model
