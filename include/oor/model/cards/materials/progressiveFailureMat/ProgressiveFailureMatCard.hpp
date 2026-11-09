#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

// One keyword line of a MATCB card: the keyword and its values, a blank value being absent. The values are, by keyword:
//   TEMP     AF AC AT TREF                     the thermal expansion coefficients and the reference temperature
//   UNIDIR   M MU-FC MU-TF MU-CT               the exponent of the fracture mode and the friction coefficients
//   WOVEN    M MU-WF MU-WT MU-FT               (the same for a woven material)
//   F-DIR, C-DIR, T-DIR, W-DIR                 a material direction: nine values, the five of its tension line
//                                              (E, R, strain at break, reduction ratio, Poisson's ratio) and the four
//                                              of its compression line (E, R, strain at break, reduction ratio)
//   FC-DIR, CT-DIR, TF-DIR, WF-DIR, FT-DIR, TW-DIR   a shear plane: six values (G, R, shear strain, residual G,
//   residual shear
//                                              strain, reduction ratio)
struct ProgressiveFailureMatEntry
{
    std::string keyword;
    std::vector<std::optional<double>> values;
};

// One MATCB card: the progressive failure properties of an orthotropic material by the Cuntze-Bold material model (2D
// and 3D).
//
// Card layout (the fields are numbered across the lines, eight to a line):
//   MATCB  MID  FAIL  RHO                                                          fields 1 to 3
// then lines that each start with a keyword in the first field of the line:
//   "TEMP"   AF  AC  AT  TREF
//   "UNIDIR" M   MU-FC  MU-TF  MU-CT          then the directions of a unidirectional material and its shear planes
//   "WOVEN"  M   MU-WF  MU-WT  MU-FT          then the directions of a woven material and its shear planes
//   F-DIR    E  R  STRAIN  RATIO  NU          (a direction: the tension line, then a line with a blank first field
//            E  R  STRAIN  RATIO               for the compression)
//   FC-DIR   G  R  GAMMA  GRES  GAMMARES  RATIO    (a shear plane: one line)
// The keywords F-DIR and T-DIR name the fibre and thickness directions of a unidirectional material and the fill and
// thickness directions of a woven one, so the entries are kept in the order of the card; the card is written in that
// order. The format is not consistent about where the exponent M sits on the UNIDIR and WOVEN lines (first or last),
// so the card checks none of their values. The compression
// line of a direction is written when it has a value. FAIL is CUBO, the default and the only theory the format
// reference names; RHO is 0.0 when blank. The material ID is in the space every material type shares.
struct ProgressiveFailureMatCard
{
    static constexpr const char* DEFAULT_FAILURE = "CuntzeBold";

    base::EntityId id = 0;
    std::string failure = DEFAULT_FAILURE;
    double density = 0.0;
    std::vector<ProgressiveFailureMatEntry> entries;
};

} // namespace oor::model
