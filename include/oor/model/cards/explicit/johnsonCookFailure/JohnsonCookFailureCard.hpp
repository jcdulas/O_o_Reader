#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// How a FAILJC applies the damage to the element, the MTH field: continuously (CONT, the default: the yield stress is
// reduced by the factor 1 - D and the element fails when D reaches one), discretely (DISC: the element fails when D
// reaches one) or not at all (NOFAIL: the damage is computed but never fails the element, for a yield model that uses
// it, such as a YLDUDS).
enum class JohnsonCookFailureMethod : std::uint8_t
{
    Continuous,
    Discrete,
    NoFailure
};
inline constexpr std::array<std::string_view, 3> JOHNSON_COOK_FAILURE_METHOD_WORDS{"Continue", "Discard", "NoFailure"};

// The defaults of a FAILJC that are not zero: the reference plastic strain rate is 1, and the melt temperature and the
// heat capacity are 1.E20 (the room temperature is zero).
inline constexpr double JOHNSON_COOK_FAILURE_NO_LIMIT = 1.0E20;

// One FAILJC card (SOL 700): the Johnson-Cook failure model. The damage D of an element is the sum, over the time
// steps, of the plastic strain increments divided by the strain at fracture, which depends on the pressure over the
// effective stress, on the plastic strain rate (against the reference rate EPS0) and on the temperature (between TROOM
// and TMELT) through D1 to D5. It is available for Eulerian materials of the multi-material solver with strength, and a
// MATDEUL names it in its FID field.
//
// Card layout: FAILJC  FID    D1     D2     D3     D4     D5     EPS0   TROOM       fields 1 to 8
//                     TMELT  CP     MTH                                           fields 9 to 11
// D1 to D5 (0.0) are the constants of the damage law. The reference plastic strain rate of field 7 is a symbol in the
// grid of the format reference (a dotted epsilon with the suffix pl0), not a name: the card calls it EPS0, as the
// Johnson-Cook yield model YLDJC calls the same quantity. EPS0 (1.0), TROOM (0.0), TMELT (1.E20) and CP (1.E20) have no
// sign rule in the description of the fields, so the card only asks for finite numbers.
struct JohnsonCookFailureCard
{
    base::EntityId id = 0; // unique among the failure models
    double coefficientD1 = 0.0;
    double coefficientD2 = 0.0;
    double coefficientD3 = 0.0;
    double coefficientD4 = 0.0;
    double coefficientD5 = 0.0;
    double referenceStrainRate = 1.0;                       // reference plastic strain rate
    double roomTemperature = 0.0;                           // room temperature
    double meltTemperature = JOHNSON_COOK_FAILURE_NO_LIMIT; // melt temperature
    double specificHeat = JOHNSON_COOK_FAILURE_NO_LIMIT;    // heat capacity
    JohnsonCookFailureMethod method = JohnsonCookFailureMethod::Continuous;
};

} // namespace oor::model
