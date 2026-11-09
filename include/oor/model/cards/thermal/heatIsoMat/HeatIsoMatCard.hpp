#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// One MAT4 card: the heat transfer material of a conduction element, and the fluid of a forced convection (CONVM).
//
// Card layout:
//   MAT4  MID  K  CP  RHO  H  MU  HGEN  REFENTH
//         TCH  TDELTA  QLAT
// The format reference draws the symbols RHO and MU where the text of the format loses them; the order is the one of
// the layout above.
//
// A blank CP is 0.0 (the format reference allows a blank), RHO and HGEN are 1.0 when blank (format reference). K,
// H, MU, REFENTH, TCH, TDELTA and QLAT may be absent: they are `std::nullopt` here and NaN in the table.
struct HeatIsoMatCard
{
    base::EntityId id = 0;
    std::optional<double> conductivity;           // thermal conductivity
    double specificHeat = 0.0;                    // specific heat
    double density = 1.0;                         // density
    std::optional<double> convectionCoefficient;  // free convection heat transfer coefficient
    std::optional<double> viscosity;              // dynamic viscosity (a forced convection fluid needs it)
    double heatGenerationFactor = 1.0;            // heat generation capability (the multiplier of QVOL)
    std::optional<double> referenceEnthalpy;      // reference enthalpy
    std::optional<double> phaseChangeTemperature; // lower temperature of the phase change region
    std::optional<double> phaseChangeRange;       // temperature range of the phase change
    std::optional<double> latentHeat;             // latent heat of fusion

    [[nodiscard]] bool hasPhaseChange() const noexcept
    {
        return phaseChangeTemperature || phaseChangeRange || latentHeat;
    }
};

} // namespace oor::model
