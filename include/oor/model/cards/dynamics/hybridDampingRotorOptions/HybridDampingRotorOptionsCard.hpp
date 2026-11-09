#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// The ROTSEP field of a DAMPING: whether the rotor stiffness, mass and structural damping are taken out of the hybrid
// damping calculation. The enumerators are numbered in the order of ROTOR_SEPARATION_WORDS.
enum class RotorSeparation : std::uint8_t
{
    No,
    Yes // the default
};

inline constexpr std::array<std::string_view, 2> ROTOR_SEPARATION_WORDS{"No", "Yes"};

// One DAMPING card: the parameter damping (structural, material and Rayleigh) of a Case Control RSDAMP or SEDAMP, and
// the HYBDAMP entry of the hybrid modal damping.
//
// Card layout: DAMPING  ID  G  ALPHA1  ALPHA2  HYBRID  GEFACT  ROTSEP          fields 1 to 7
//                      W3  W4  WH                                             fields 9 to 11
// The Case Control RSDAMP (residual structure) and SEDAMP (a superelement) name the card by its ID; once selected, its
// values, defaults included, replace the damping the PARAM entries give. A blank real is the format
// reference default: G, ALPHA1, ALPHA2, W3, W4 and WH are 0.0 and GEFACT is 1.0. HYBRID is the number of a HYBDAMP (0
// when blank: no hybrid damping). W3, W4 and WH are the average frequencies, in radians per unit time, with which the
// structural, material and hybrid damping of a transient response are turned into equivalent viscous damping: 0.0 means
// no such damping, and they must not be negative.
struct HybridDampingRotorOptionsCard
{
    base::EntityId id = 0;                // unique among the DAMPING cards
    double structuralDamping = 0.0;       // structural damping coefficient
    double alphaCoefficient1 = 0.0;       // mass part of the Rayleigh damping
    double alphaCoefficient2 = 0.0;       // stiffness part of the Rayleigh damping
    base::EntityId hybridDampingId = 0;   // a HYBDAMP, 0 for none
    double structuralDampingFactor = 1.0; // scale factor of the material damping
    RotorSeparation rotorSeparation = RotorSeparation::Yes;
    double structuralDampingFrequency = 0.0; // transient frequency of the structural damping
    double materialDampingFrequency = 0.0;   // transient frequency of the material damping
    double hybridDampingFrequency = 0.0;     // transient frequency of the hybrid damping
};

} // namespace oor::model
