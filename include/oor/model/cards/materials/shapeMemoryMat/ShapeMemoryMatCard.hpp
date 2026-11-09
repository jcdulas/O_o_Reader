#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <optional>

namespace oor::model
{

// One MATSMA card: a shape memory alloy of SOL 400.
//
// Card layout (the fields are numbered across the lines, eight to a line):
//   MATSMA  MID   MODEL  T0     L                                                 fields 1 to 4
//           EA    NUA    ALPHAA SIGMAA RHOA   SIGMASAS SIGMAFAS CA                9 to 16   the austenite
//           EM    NUM    ALPHAM SIGMAM RHOM   SIGMASS  SIGMAFSA CM                17 to 24  the martensite
//           VT    MFRAC  SIGEFF SIGMAX                                             25 to 28  the thermo-mechanical
//           model G0    GA     GB     GC     GD     GE       GF       GMAX               33 to 40  the g function
// MODEL is 1 (the mechanical, Auricchio, model) or 2 (the thermo-mechanical, Asaro-Sayeedvafa, model). The last two
// lines are for the thermo-mechanical model only (the format reference gets the mechanical model by ignoring them).
// Every blank value is 0.0. The material ID is in the space every material type shares.
struct ShapeMemoryMatCard
{
    static constexpr std::size_t PHASE_VALUES = 8;
    static constexpr std::size_t THERMO_VALUES = 12;

    base::EntityId id = 0;
    int model = 1;
    double referenceTemperature = 0.0; // the reference temperature of the stresses
    // The maximum deformation obtained by detwinning (mechanical), or the one of the equivalent strain.
    double maximumDetwinningStrain = 0.0;
    // The austenite and the martensite as (E, NU, ALPHA, SIGMA, RHO, SIGMA-S start, SIGMA-F finish, C).
    std::array<double, PHASE_VALUES> austenite{};
    std::array<double, PHASE_VALUES> martensite{};
    // (VT, MFRAC, SIGMA-EFF, SIGMA-MAX, G0, GA, GB, GC, GD, GE, GF, GMAX); absent for the mechanical model.
    std::optional<std::array<double, THERMO_VALUES>> thermo;
};

} // namespace oor::model
