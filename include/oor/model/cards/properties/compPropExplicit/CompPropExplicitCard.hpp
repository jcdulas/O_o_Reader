#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace oor::model
{

// The enumerators of this file are numbered in the order of the array of words that follows each, so the position of a
// word is the value of its enumerator (`keywordText` and `parseKeyword` of explicit/ExplicitSupport.hpp).

// The reference surface of the laminate (REF).
enum class CompPropExplicitReference : std::uint8_t
{
    Top,    // TOP
    Middle, // MID (the default)
    Bottom  // BOT
};

inline constexpr std::array<std::string_view, 3> COMP_PROP_EXPLICIT_REFERENCE_WORDS{"Top", "MidSurface", "Bottom"};

// The directions in which the stresses and strains are given (STRDEF).
enum class CompPropExplicitStressDefinition : std::uint8_t
{
    Fiber,  // FIBER (the default): the fibre and matrix directions
    Element // ELEM: the element coordinate system
};

inline constexpr std::array<std::string_view, 2> COMP_PROP_EXPLICIT_STRESS_DEFINITION_WORDS{"Fiber", "Elements"};

// The fields DT1D, STRNOUT and SPINCOR hold YES or NO; each has a default of its own.
enum class CompPropExplicitYesNo : std::uint8_t
{
    No,
    Yes
};

inline constexpr std::array<std::string_view, 2> COMP_PROP_EXPLICIT_YES_NO_WORDS{"No", "Yes"};

// One PCOMPA card (SOL 700 only): the additional data of a PCOMP, with the options of the explicit shell elements. The
// card extends the PCOMP of the same number, so its ID space is its own (one PCOMPA per PCOMP) and its PID is a
// reference to a property.
//
// Card layout (the fields are numbered across the lines, eight to a line):
//   PCOMPA  PID  FORM  SHFACT  REF  STRDEF  DT1D  STRNOUT  ICLT                      fields 1 to 8
//           SPINCOR                                                                  field 9
// The format names the eighth field both CLT and ICLT: the card uses ICLT. FORM is the element formulation. The
// format lists no word for it (it only gives the defaults: KEYHOFF for a CQUAD4, C0-TRIA for a CTRIA3, which depend on
// the element and stay blank), and BLT is a common value: the words of the same field of the PSHELL1 are HUGHES, BLT,
// KEYHOFF, C0-TRIA, MEMB and DUMMY, but since the format does not say that they are the set of this card, FORM is kept
// as a word of at most eight characters and no word is refused. SHFACT is not negative (0.83333 when blank) and is
// ignored by the classical lamination theory (ICLT = 1). ICLT is 0 (the integration technique, the default) or 1 (the
// classical lamination theory): the format asks for a positive integer and lists the values 0 and 1. The format has
// SPINCOR on by default and gives the default NO in the description of the field: the card follows the description.
struct CompPropExplicitCard
{
    static constexpr double DEFAULT_SHEAR_FACTOR = 0.83333;

    base::EntityId id = 0;   // the PCOMP that the card extends
    std::string formulation; // blank (empty): the formulation depends on the element
    double shearFactor = DEFAULT_SHEAR_FACTOR;
    CompPropExplicitReference referenceSurface = CompPropExplicitReference::Middle;
    CompPropExplicitStressDefinition stressDefinition = CompPropExplicitStressDefinition::Fiber;
    CompPropExplicitYesNo skipFiberTimeStep =
        CompPropExplicitYesNo::No; // YES: skip the one-dimensional failure modes in the time step
    CompPropExplicitYesNo strainOutput = CompPropExplicitYesNo::Yes; // NO: no strain is kept in memory
    std::int32_t laminationTheory = 0;
    CompPropExplicitYesNo spinCorrection = CompPropExplicitYesNo::No;
};

} // namespace oor::model
