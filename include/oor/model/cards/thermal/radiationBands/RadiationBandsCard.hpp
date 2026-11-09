#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One RADBND card: Planck's second radiation constant and the wavelength breakpoints of the radiation exchange
// problems, for the RADM entries that give an emissivity for each wave band.
//
// Card layout: RADBND  NUMBER  PLANCK2  LAMBDA1  LAMBDA2  LAMBDA3  LAMBDA4  LAMBDA5  LAMBDA6        fields 1 to 8
//                     LAMBDA7  -etc.-                                                                fields 9 on
// NUMBER is the number of wave bands (more than 1). The first band runs from 0 to LAMBDA1, the last from LAMBDAn to
// infinity with n = NUMBER - 1, so a card has NUMBER - 1 wavelengths and the fields of the wavelengths from
// number NUMBER on are blank. The wavelengths are in ascending order, and a repeated one is a
// band of zero width, which the format reference uses for a discontinuous emissivity curve, so the order is
// not strict. PLANCK2 has the units of a wavelength times a temperature (25898 micrometre degree Rankine or 14388
// micrometre kelvin), in the units of the LAMBDAi and of the temperatures of the radiating surfaces. Only one RADBND
// may be in the Bulk Data; a RADM names no band, but the number of its emissivities is the NUMBER of the
// card.
struct RadiationBandsCard
{
    base::EntityId bandCount = 0;
    double planckSecondConstant = 0.0;
    std::vector<double> wavelengths; // NUMBER - 1 values

    [[nodiscard]] bool operator==(const RadiationBandsCard&) const noexcept = default;
};

} // namespace oor::model
