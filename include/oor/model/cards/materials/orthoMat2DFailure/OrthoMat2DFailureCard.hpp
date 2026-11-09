#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace oor::model
{

// The failure theory of a MAT8A card (FT), and the theory that defines one failure mode of the COMBINAT theory.
enum class OrthoMat2DFailureTheory : std::uint8_t
{
    None,         // blank: no failure
    Hill,         // HILL: Tsai-Hill
    Tsai,         // TSAI: Tsai-Wu
    ModifiedTsai, // MODTSAI: modified Tsai-Wu
    MaxStress,    // STRSS: maximum stress
    Chang,        // CHANG: Chang-Chang
    Combination,  // COMBINAT: one theory for each failure mode (not for a mode)
    Hashin        // HASHIN
};

// TRSFAIL: when a transverse shear failure fails the element.
enum class OrthoMat2DFailureShearFailure : std::uint8_t
{
    Sublayer, // SUBL (the default): when the sublayer fails
    Element   // ELEM: when the element fails
};

// PFD: how the stresses degrade after a failure.
enum class OrthoMat2DFailureDegradation : std::uint8_t
{
    Steps,   // STEPS (the default): by time steps
    Time,    // TIME: by time
    Velocity // VELOC: by velocity
};

// PFDST: when the degradation starts.
enum class OrthoMat2DFailureDegradationStart : std::uint8_t
{
    Individual, // INDV (the default): per failure mode
    All         // ALL: when all the elastic constants are zero
};

// One MAT8A card: the failure properties of an orthotropic material for shell elements (SOL 700); the MAT8 with the
// same ID holds the elastic constants.
//
// Card layout (the fields are numbered across the lines, eight to a line):
//   MAT8A  MID    FT     NV     S      ALPHA  TRSFAIL F12                        fields 1 to 7
//          XT     XC     YT     YC     PFD    VALUE   PFDST                      9 to 15
//          FBTEN  FBCOM  MXTEN  MXCOM  MXSHR                                     17 to 21
//          (blank line)
//          PRDFT  PRDFC  PRDMT  PRDMC  PRDSH                                      33 to 37
// S, the failure stress of the in-plane shear, is greater than zero. NV (0 to 9) is the number of additional history
// variables of a user model, ALPHA (0.0) the nonlinear shear coefficient, F12 (0.0) the interaction term of the Tsai-Wu
// theory. XT, XC, YT and YC (0.0 when blank) are the failure stresses of the fibre and matrix directions, greater than
// zero for a card that selects a theory. VALUE (100.0) is the number of steps, the time interval or the velocity of
// PFD. The five failure modes of FT = COMBINAT, in `modes` as (FBTEN, FBCOM, MXTEN, MXCOM, MXSHR), each name one of the
// theories (not COMBINAT) and are all required for that theory. The five degradation words are four characters of 0 and
// 1 each (1 denotes property degradation), kept as text so that the leading zeros stay: `degradation` is (PRDFT, PRDFC,
// PRDMT, PRDMC, PRDSH), which are 1111, 1010, 0110, 0110 and 0001 when blank.
struct OrthoMat2DFailureCard
{
    static constexpr std::size_t MODES = 5;

    base::EntityId id = 0; // the MAT8 that the card extends
    OrthoMat2DFailureTheory theory = OrthoMat2DFailureTheory::None;
    int historyVariableCount = 0;
    double shearStrength = 0.0;
    double nonlinearShearCoefficient = 0.0;
    OrthoMat2DFailureShearFailure transverseShear = OrthoMat2DFailureShearFailure::Sublayer;
    double interactionTerm12 = 0.0;
    double tensionStrength1 = 0.0;
    double compressionStrength1 = 0.0;
    double tensionStrength2 = 0.0;
    double compressionStrength2 = 0.0;
    OrthoMat2DFailureDegradation degradation = OrthoMat2DFailureDegradation::Steps;
    double degradationParameter = 100.0;
    OrthoMat2DFailureDegradationStart degradationStart = OrthoMat2DFailureDegradationStart::Individual;
    std::array<OrthoMat2DFailureTheory, MODES> modes{};
    std::array<std::string, MODES> propertyDegradation = {"1111", "1010", "0110", "0110", "0001"};
};

} // namespace oor::model
