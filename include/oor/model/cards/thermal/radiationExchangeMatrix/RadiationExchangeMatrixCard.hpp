#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One RADMTX card: one column of the radiation exchange matrix of a cavity.
//
// Card layout: RADMTX  ICAVITY  INDEX  F(i,j)  F(i+1,j)  F(i+2,j)  F(i+3,j) -etc.-
// INDEX is the number of the column. A symmetric matrix gives the lower triangle (the values start on the diagonal), an
// unsymmetric one the whole column (they start in row 1). The cavity has one card for each column, so the ID is not
// unique.
struct RadiationExchangeMatrixCard
{
    base::EntityId cavityId = 0;
    int columnIndex = 0;
    std::vector<double> factors;
};

} // namespace oor::model
