#pragma once

#include "oor/analysis/validation/engine/CheckRegistry.hpp"

// The aero family (aeroelasticity: CAERO1 to CAERO5, PAERO1 to PAERO5, the splines, AESURF, the lists and factors,
// TRIM, FLUTTER, GUST, ...). What the cards say that the reference checks do not see: a card that names something that
// does not exist is `ref.unresolved` (the cards say what they refer to, and a Case Control TRIM, FMETHOD, GUST, DIVERG,
// CSSCHD or AEUXREF that names no card is `case.unresolved-reference`), and a repeated ID of one card type is
// `ref.duplicate-id`. The checks here are about what the numbers of the boxes, the labels of the trim variables and the
// values of the cards mean: panels whose boxes share IDs, a spline that names boxes its panel does not have, a label no
// card defines. One class per check, as in the other families; the order below is the order they run in.

namespace oor::analysis
{

// Two CAERO panels whose boxes take the same IDs.
void addPanelOverlapCheck(CheckRegistry& registry);
// A CAERO that names a PAERO of another kind.
void addPanelPropertyCheck(CheckRegistry& registry);
// A division list (AEFACT) of a CAERO with fewer than two points or not increasing.
void addDivisionListCheck(CheckRegistry& registry);
// An AELIST item that no panel owns and no aerodynamic mesh has.
void addListBoxCheck(CheckRegistry& registry);
// A spline that names boxes its CAERO does not own.
void addSplineBoxCheck(CheckRegistry& registry);
// A SPLINE of one kind with the ID of a SPLINE of another kind.
void addSplineIdRepeatedCheck(CheckRegistry& registry);
// A TRIM and a TRIM2 with the same SID.
void addTrimIdRepeatedCheck(CheckRegistry& registry);
// An AESURF, AESTAT or AEPARM with the label of another one.
void addLabelRepeatedCheck(CheckRegistry& registry);
// A label of a TRIM, TRIM2, AELINK or UXVEC that no AESURF, AESTAT or AEPARM defines.
void addLabelUndefinedCheck(CheckRegistry& registry);
// A variable that two AELINK cards of the same subcases both make dependent.
void addLinkDependentRepeatedCheck(CheckRegistry& registry);
// An AESURFS whose label no AESURF has.
void addSurfaceLabelCheck(CheckRegistry& registry);
// An AECOMP or AECOMPL with the name of another component.
void addComponentNameRepeatedCheck(CheckRegistry& registry);
// An AECOMPL that joins a component no card defines.
void addComponentLabelUndefinedCheck(CheckRegistry& registry);
// A MONPNT1 or MONDSP1 whose COMP names no component.
void addMonitorComponentUndefinedCheck(CheckRegistry& registry);
// A PKNL or PKNLS FLUTTER whose three FLFACT lists differ in length.
void addFlutterFactorCountCheck(CheckRegistry& registry);
// A GUST whose velocity is not the one of the AERO card.
void addGustVelocityCheck(CheckRegistry& registry);
// A subcase that runs an aeroelastic analysis in a deck with no AERO or AEROS card.
void addAeroCardMissingCheck(CheckRegistry& registry);
// A body of a PAERO1 that is not a CAERO2, and one in another interference group than the panel.
void addAeroLiftingPanelPropBodyKindCheck(CheckRegistry& registry);
void addAeroLiftingPanelPropBodyGroupCheck(CheckRegistry& registry);
// A CAERO2 with fewer than two slender body elements or no interference element, and one whose group no panel has.
void addBodyElementsCheck(CheckRegistry& registry);
void addBodyGroupCheck(CheckRegistry& registry);
// An AEFACT list of a CAERO3 with an odd number of values (the (x, y) pairs).
void addAeroMachBoxPanel3ListsCheck(CheckRegistry& registry);
// A strip of a CAERO4 or CAERO5 that has the number of a point of the model.
void addStripIdsCheck(CheckRegistry& registry);
// A SPLINE3 component the kind of its CAERO does not allow.
void addAeroSplineEquation3ComponentCheck(CheckRegistry& registry);
// A SPLINE6 or SPLINE7 that names a CAERO2 body.
void addSplineBodyCheck(CheckRegistry& registry);
// A spline whose set of grid points is smaller than the spline needs.
void addSplineSetSizeCheck(CheckRegistry& registry);
// A control surface whose lower limit is above its upper limit.
void addSurfaceLimitsCheck(CheckRegistry& registry);
// An AELINK whose positive ID is the SID of no TRIM or TRIM2.
void addLinkTrimCheck(CheckRegistry& registry);
// The Mach number 1.0 on an MKAERO1 or MKAERO2 in a model with no CAERO4.
void addMachOneCheck(CheckRegistry& registry);
// An AERO or AEROS coordinate system that is not rectangular.
void addSystemRectangularCheck(CheckRegistry& registry);

// Adds the checks of the family, in the order they run.
void registerAeroChecks(CheckRegistry& registry);

} // namespace oor::analysis
