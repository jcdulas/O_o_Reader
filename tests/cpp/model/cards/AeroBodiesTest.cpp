// Checks of the aerodynamic body cards: PAERO4 and PAERO5 (the properties of the strips of a strip theory
// and of a piston theory panel) and CAXIF2, CAXIF3 and CAXIF4 (the axisymmetric fluid elements). Per card: the fields
// in their format reference positions, the defaults, the references the card lists, the renumbering of its ID space,
// the refused values and the round trip through the three formats.

#include "oor/model/cards/aero/aeroFactorList/AeroFactorListTable.hpp"
#include "oor/model/cards/aero/aeroLiftingPanel1/AeroLiftingPanel1Table.hpp"
#include "oor/model/cards/aero/aeroPistonProp5/AeroPistonProp5Table.hpp"
#include "oor/model/cards/aero/aeroStripProp4/AeroStripProp4Table.hpp"
#include "oor/model/cards/elements/acousticAxisymFluid2/AcousticAxisymFluid2Table.hpp"
#include "oor/model/cards/elements/acousticAxisymFluid3/AcousticAxisymFluid3Table.hpp"
#include "oor/model/cards/elements/acousticAxisymFluid4/AcousticAxisymFluid4Table.hpp"
#include "oor/model/cards/elements/rod2/Rod2Table.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;
