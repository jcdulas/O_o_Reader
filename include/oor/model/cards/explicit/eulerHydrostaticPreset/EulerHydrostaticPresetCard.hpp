#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// One HYDSTAT card (SOL 700): the hydrostatic preset of the density of a fluid in the Euler elements, which a COUPLE
// names in its HYDSTAT field. The fluid is the MATDEUL of MID (an EOSPOL material); the free surface sits at a point
// that the card gives in one of two ways, and PATM is the pressure there (the pressure of the air above it).
//
// Card layout: HYDSTAT  HID  MID  GID  CID  XCG  YCG  ZCG  PATM            fields 1 to 8
// The format gives the two ways to place the free surface: a grid point GID (CID and the coordinates are then blank),
// or coordinates XCG, YCG, ZCG in the system CID (0 or blank: basic) with GID blank. The card keeps what was written
// and checks that exactly one of the two is used. With gravity along one axis only that coordinate counts, so a
// coordinate may be left blank: the check asks for at least one when there is no grid point, and a blank coordinate
// stays blank (None in Python) instead of becoming 0.0.
struct EulerHydrostaticPresetCard
{
    base::EntityId id = 0;               // unique among the hydrostatic presets
    base::EntityId matId = 0;            // a MATDEUL
    base::EntityId nodeId = 0;           // a grid point on the free surface, 0 when the coordinates are used
    base::EntityId frameId = 0;          // the system of the coordinates, 0 for the basic one
    std::optional<double> surfacePointX; // the point on the free surface
    std::optional<double> surfacePointY;
    std::optional<double> surfacePointZ;
    double surfacePressure = 0.0; // the pressure at the free surface
};

} // namespace oor::model
