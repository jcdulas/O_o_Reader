#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <optional>

namespace oor::model
{

// One MATTEP card: the temperature dependence of the properties of a MATEP (the thermo-elasto-plastic material of SOL
// 400). Each field holds the ID of a table (0 when blank: the property does not depend on the temperature); the value
// of the MATEP with the same MID is replaced or modified by the table.
//
// Card layout (the fields are numbered across the lines, eight to a line):
//   MATTEP  MID  (blank)  T(Y0)  T(FID)  (blank)  (blank)  (blank)  T(H)           fields 1 to 8
// then the CHABOCHE group, on the line that starts at field 9 or at field 17 (the format reference draws a line of
// unused cells before it):
//   "CHABOCHE"  R0  RINF  B  C  GAM  KAP  N                                       the table of each constant
//               (blank)  QM  MU  ETA                                              (second line, when it has values)
// T(Y0), T(H) and the constants of the CHABOCHE group are TABLEMi IDs and T(FID) is a TABLEST ID. T(FID) replaces T(Y0)
// and T(H) (SOL 400 blanks them when T(FID) is given). The CHABOCHE group is written on the line that
// starts at field 17, as the format reference draws it.
struct ElastoPlasticMatTemperatureCard
{
    static constexpr std::size_t CHABOCHE_TABLES = 10;

    base::EntityId id = 0; // the MATEP that the card extends
    base::EntityId yieldStressTableId = 0;
    base::EntityId stressStrainCurvesTableId = 0;
    base::EntityId hardeningSlopeTableId = 0;
    std::optional<std::array<base::EntityId, CHABOCHE_TABLES>>
        chabocheHardening; // R0, RINF, B, C, GAM, KAP, N, QM, MU, ETA
};

} // namespace oor::model
