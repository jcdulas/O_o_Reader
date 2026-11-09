#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PBELT card (SOL 700): the property of a belt element, a CROD that names the card in its PID field. A belt is a
// tension only element whose force follows a table of the engineering strain, (length - initial length) / initial
// length, one table when it loads and another when it unloads (the unloading curve is shifted along the strain axis to
// meet the loading curve where the unloading starts). The tables are TABLED1 cards that start at (0.0, 0.0).
//
// Card layout: PBELT  PID  LOAD  UNLOAD  DENSITY  DAMP1  DAMP2  SLACK  PRESTRESS            fields 1 to 8
// PID is a property ID, in the space of all the properties. LOAD and UNLOAD (required) and SLACK and PRESTRESS (blank:
// not used) are the numbers of TABLED1 cards; the two last give a slack that is subtracted from the strain of the
// element and a prestrain that is added to it, both as functions of time. DENSITY is the mass per unit length. DAMP1 is
// the coefficient of the damping force added to the internal force (damping coefficient times the mass of the element
// times the rate of elongation) and DAMP2 limits that force to a fraction of the internal force; both are 0.1 when
// blank. The format wants both positive and the card keeps that rule: zero damping is not a value of the card.
struct BeltPropCard
{
    base::EntityId id = 0;               // unique among the properties
    base::EntityId loadingTableId = 0;   // a TABLED1: force against strain while loading
    base::EntityId unloadingTableId = 0; // a TABLED1: force against strain while unloading
    double density = 0.0;                // mass per unit length, greater than zero
    double damping1 = 0.1;               // damping coefficient, greater than zero
    double damping2 = 0.1;               // largest damping force as a fraction of the internal force, greater than zero
    base::EntityId slackTableId = 0;     // a TABLED1: the slack against time, 0 for none
    base::EntityId prestrainTableId = 0; // a TABLED1: the prestrain against time, 0 for none
};

} // namespace oor::model
