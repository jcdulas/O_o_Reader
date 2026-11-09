#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One VIEW3D card: the parameters of the Gaussian integration method of view factor calculation for one radiation
// cavity.
//
// Card layout: VIEW3D  ICAVITY  GITB  GIPS  CIER  ETOL  ZTOL  WTOL  RADCHK
// GITB and GIPS (the integration order with third body and with self shadowing: 2, 3, 4, 5, 6 or 10) and CIER (the
// level of the contour integration, 1 to 20) are 4 when blank, ETOL is 0.1, ZTOL 1.0E-10, WTOL 0.01 and RADCHK 3.
struct ViewFactorIntegrationCard
{
    base::EntityId cavityId = 0;
    int blockingIntegrationOrder = 4;
    int shadowingIntegrationOrder = 4;
    int contourIntegrationLevel = 4;
    double errorTolerance = 0.1;
    double zeroTolerance = 1.0e-10;
    double warpTolerance = 0.01;
    int radiationCheck = 3;
};

} // namespace oor::model
