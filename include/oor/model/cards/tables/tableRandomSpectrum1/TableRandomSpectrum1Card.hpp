#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/tables/TableSupport.hpp"

#include <vector>

namespace oor::model
{

// One TABRND1 card: a power spectral density as a function of frequency, G(F), for random analysis (referenced by the
// RANDPS cards).
//
// Card layout:
//   TABRND1  TID  XAXIS  YAXIS
//            f1   g1     f2     g2     f3     g3     -etc.-   ENDT
// The points follow each other from field 9, two fields each, over as many continuation lines as needed, and the word
// ENDT ends the table. f is ascending or descending; a repeated f makes a jump. Between the points the function is
// linear, in the logarithm of f when XAXIS is LOG and in the logarithm of g when YAXIS is LOG. A pair that has SKIP in
// a field is not kept.
struct TableRandomSpectrum1Card
{
    base::EntityId id = 0;
    TableAxis xAxisKind = TableAxis::Linear;
    TableAxis yAxisKind = TableAxis::Linear;
    std::vector<double> x; // the frequencies
    std::vector<double> y; // the power spectral densities
};

} // namespace oor::model
