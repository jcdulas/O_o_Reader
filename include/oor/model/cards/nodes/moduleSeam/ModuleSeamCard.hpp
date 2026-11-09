#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace oor::model
{

// CTYPE of an MDSEAM: what the four patch numbers IDAS, IDBS, IDAE and IDBE name. A blank CTYPE is PSHELL. The
// enumerators are numbered in the order of MODULE_SEAM_PATCH_TYPE_WORDS.
enum class ModuleSeamPatchType : std::uint8_t
{
    Property, // PSHELL (the default): property IDs of PSHELL cards
    Element   // ELEM: element IDs
};

inline constexpr std::array<std::string_view, 2> MODULE_SEAM_PATCH_TYPE_WORDS{"ShellProp", "Elements"};

// How an MDSEAM gives the start and the end of the seam: by the grid points GS and GE, or by the coordinates XS YS ZS
// and XE YE ZE (the alternate format of the format reference).
enum class ModuleSeamEnds : std::uint8_t
{
    GridPoints,
    Coordinates
};

// One MDSEAM card: a seam that connects two surface patches (shell elements) that lie in two modules (the multi-module
// counterpart of the CSEAM, whose rules apply). The patch A is in the module MODA and the patch B in MODB; a module
// is a BEGIN MODULE section of the deck, a part of the deck, and module 0 (a blank field) is the main Bulk Data.
//
// Card layout (the first form, with the grid points):
//   MDSEAM  EID  PID  SMLN  CTYPE  IDAS  IDBS  IDAE  IDBE         fields 1 to 8
//           GS   GE                                 MODA  MODB     fields 9 and 10, and 15 and 16
// and the alternate form, which gives the ends as coordinates and leaves SMLN out:
//   MDSEAM  EID  PID  (blank)  CTYPE  IDAS  IDBS  IDAE  IDBE
//           XS   YS   ZS   XE   YE   ZE   MODA  MODB             fields 9 to 14, 15 and 16
// In the first form fields 11 to 14 are blank. The card belongs in the main Bulk Data (a rule of the deck,
// not of the card).
//
// PID names a PSEAM of the main Bulk Data. SMLN names the seam line (up to eight characters, no blank; blank is
// nothing); the alternate form has no SMLN in the format reference, and the card keeps one that a deck writes there
// rather than refuse it (as the CSEAM does). CTYPE says what the patch numbers are (PSHELL by default). IDAS and IDBS
// define the patches A and B, or their start for a tailored blank, in the modules MODA and MODB; IDAE and IDBE their
// end, in the same modules. IDAS is required. With CTYPE PSHELL an IDBS that is blank or equal to IDAS makes the patch
// two-sided, and so for the end; with CTYPE ELEM IDBS is required and differs from IDAS, and IDAE and
// IDBE differ when both are given. The format reference says "differ" with no regard to the modules; two patches of two
// modules may have the same number, so the rule is asked of the numbers of one module. A blank IDBS, IDAE or IDBE is 0.
//
// GS and GE are the piercing points on the patches of the start and the end, in the main Bulk Data (module 0): positive
// when the first form is used. In the alternate form the six coordinates are reals or blank and the format reference
// gives no default for a blank one, so the card keeps a blank as nothing; at least one of the six must be given,
// because a card with none would read back as the first form. `ends` says which form the card is: the other form's
// members are blank (0 for the grid points, nothing for the coordinates).
struct ModuleSeamCard
{
    base::EntityId id = 0;
    base::EntityId propId = 0;
    std::string seamLineName;
    ModuleSeamPatchType patchKind = ModuleSeamPatchType::Property;
    base::EntityId startPatchIdA = 0;
    base::EntityId startPatchIdB = 0;
    base::EntityId endPatchIdA = 0;
    base::EntityId endPatchIdB = 0;
    ModuleSeamEnds ends = ModuleSeamEnds::GridPoints;
    base::EntityId startNode = 0;                  // the first form; 0 in the coordinate form
    base::EntityId endNode = 0;                    // the first form; 0 in the coordinate form
    std::array<std::optional<double>, 3> startXyz; // XS, YS, ZS; blank in the grid-point form
    std::array<std::optional<double>, 3> endXyz;   // XE, YE, ZE; blank in the grid-point form
    base::EntityId moduleIdA = 0;                  // the module of IDAS and IDAE, 0 for the main Bulk Data
    base::EntityId moduleIdB = 0;                  // the module of IDBS and IDBE

    [[nodiscard]] bool hasCoordinates() const noexcept { return ends == ModuleSeamEnds::Coordinates; }
};

} // namespace oor::model
