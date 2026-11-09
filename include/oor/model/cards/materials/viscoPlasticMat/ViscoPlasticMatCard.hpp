#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// The creep model of a MATVP card.
enum class ViscoPlasticMatModel : std::uint8_t
{
    PowerLaw, // the power-law creep model (the default): a rate A * stress^M * strain^N * temperature^P * time^Q
    Anand     // ANAND: the Anand solder creep model, nine constants
};

// One MATVP card: the viscoplastic or creep properties of the material of the MAT1, MAT2, MATORT or MAT9 with the same
// ID (SOL 400).
//
// Card layout, power-law creep (the default):
//   MATVP  MID  A    IT3D  M    N    P    Q    IUSER                            fields 1 to 8
// Anand solder creep model:
//   MATVP  MID  "ANAND"  PREXF  ACTEN  MULST  STNRT  SATCO  STNSA                1 to 8
//          HRCN STNHR    DEFRS                                                    9 to 11
// A is the coefficient of the rate (1.0 when IT3D, the TABL3Di that carries the dependence on stress, strain,
// temperature and time, already holds it; 0.0, or blank as in a common deck, asks for the user subroutine crplaw, and
// then M to Q are ignored). IT3D is 0 when blank and names a TABL3D0, TABL3D1 or TABL3D2 (a reference of the target
// MultiDimensionalTable). M, N, P and Q are 0.0 when blank and IUSER is 0 (1 asks for the user subroutine). The nine
// constants of the Anand model have no default.
struct ViscoPlasticMatCard
{
    static constexpr std::size_t ANAND_CONSTANTS = 9;

    base::EntityId id = 0; // the material that the card extends
    ViscoPlasticMatModel model = ViscoPlasticMatModel::PowerLaw;
    // Power law
    double rateCoefficient = 0.0;
    base::EntityId rateTableId = 0;
    double stressExponent = 0.0;
    double strainExponent = 0.0;
    double temperatureExponent = 0.0;
    double timeExponent = 0.0;
    int userSubroutineFlag = 0;
    // Anand: PREXF, ACTEN, MULST, STNRT, SATCO, STNSA, HRCN, STNHR, DEFRS
    std::array<double, ANAND_CONSTANTS> anand{};
};

} // namespace oor::model
