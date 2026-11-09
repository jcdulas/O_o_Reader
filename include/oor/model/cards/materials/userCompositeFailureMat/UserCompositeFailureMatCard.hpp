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

// When a transverse shear failure fails the element (TRSFAIL).
enum class UserCompositeFailureMatShearFailure : std::uint8_t
{
    Sublayer, // SUBL (the default): when the sublayer fails
    Element   // ELEM: when the element fails
};

inline constexpr std::array<std::string_view, 2> USER_COMPOSITE_FAILURE_SHEAR_FAILURE_WORDS{"SublaminateShear",
                                                                                            "Elements"};

// How the stresses degrade after a failure (PFD).
enum class UserCompositeFailureMatDegradation : std::uint8_t
{
    Steps,   // STEPS (the default): by time steps
    Time,    // TIME: by time
    Velocity // VELOC: by velocity
};

inline constexpr std::array<std::string_view, 3> USER_COMPOSITE_FAILURE_DEGRADATION_WORDS{"Steps", "Time", "Velocity"};

// When the degradation starts (PFDST).
enum class UserCompositeFailureMatDegradationStart : std::uint8_t
{
    Individual, // INDV (the default): per distinct failure mode
    All         // ALL: when all the elastic constants are zero
};

inline constexpr std::array<std::string_view, 2> USER_COMPOSITE_FAILURE_DEGRADATION_START_WORDS{"IndividualDegradation",
                                                                                                "All"};

// One COMPUDS card (SOL 700 only): an orthotropic material of shell composites with its failure model and property
// degradation, whose failure is computed by a user subroutine. The FMS statement CONNECT SERVICE names the group of the
// subroutines: it is a File Management statement and not part of the card.
//
// Card layout (the fields are numbered across the lines, eight to a line; the format draws one blank cell after the
// ninth field of the first line and after the last field of each line):
//   COMPUDS  MID    GROUP   UNAME                                                    fields 1 to 3
//            E1     E2      N12    G12    G1Z    G2Z    RHO                          9 to 15
//            NV     S       ALPHA  TRSFAIL F12                                        17 to 21
//            XT     XC      YT     YC     PFD    VALUE  PFDST                        25 to 31
//            FBTEN  FBCOM   MXTEN  MXCOM  MXSHR                                      33 to 37
//            PRDFT  PRDFC   PRDMT  PRDMC  PRDSH                                      41 to 45
// The four failure stresses are XT, XC, YT and YC (the layout of the format misprints XC as XT).
// MID, E1, E2, N12, G12 and RHO are required, and so are the four failure stresses XT, XC, YT and YC (all greater than
// zero). G1Z and G2Z (the transverse shear moduli) are G12 when blank: the format says so for G1Z and gives no
// description of G2Z, which follows the MAT8 (the fields are those of the MAT8 and MAT8A); both are stored
// resolved. GROUP is required and UNAME blank when not given (the default routine is EXCOMP), each a name of at
// most eight characters. NV, the number of additional history variables, is 0 to 9 (0 by
// default). VALUE (100.0) is a real here as in the MAT8A: a number of steps, a time interval or a
// velocity according to PFD, greater than zero as the MAT8A says. FBTEN to MXSHR name the failure modes: the format
// gives no list of words for the user model, so each is a word of at most eight characters, or blank. The five
// degradation words are four characters of 0 and 1 each (1 denotes property degradation), kept as text so that the
// leading zeros stay: 1111, 1010, 0110, 0110 and 0001 when blank.
struct UserCompositeFailureMatCard
{
    static constexpr std::int32_t MAX_HISTORY_VARIABLES = 9;
    static constexpr double DEFAULT_VALUE = 100.0;

    base::EntityId id = 0;
    std::string groupName;
    std::string userRoutineName; // empty when blank
    double youngsModulus1 = 0.0;
    double youngsModulus2 = 0.0;
    double poissonRatio12 = 0.0;
    double shearModulus12 = 0.0;
    double shearModulus1Z = 0.0; // stored resolved: a blank G1Z is G12
    double shearModulus2Z = 0.0; // stored resolved: a blank G2Z is G12
    double density = 0.0;
    std::int32_t historyVariableCount = 0;
    double shearStrength = 0.0;
    double nonlinearShearCoefficient = 0.0;
    UserCompositeFailureMatShearFailure transverseShear = UserCompositeFailureMatShearFailure::Sublayer;
    double interactionTerm12 = 0.0;
    double tensionStrength1 = 0.0;
    double compressionStrength1 = 0.0;
    double tensionStrength2 = 0.0;
    double compressionStrength2 = 0.0;
    UserCompositeFailureMatDegradation degradation = UserCompositeFailureMatDegradation::Steps;
    double degradationParameter = DEFAULT_VALUE;
    UserCompositeFailureMatDegradationStart degradationStart = UserCompositeFailureMatDegradationStart::Individual;
    std::string fiberTensionTheory; // the five failure modes; empty when blank
    std::string fiberCompressionTheory;
    std::string matrixTensionTheory;
    std::string matrixCompressionTheory;
    std::string matrixShearTheory;
    std::string fiberTensionDegradation = "1111";
    std::string fiberCompressionDegradation = "1010";
    std::string matrixTensionDegradation = "0110";
    std::string matrixCompressionDegradation = "0110";
    std::string matrixShearDegradation = "0001";
};

} // namespace oor::model
