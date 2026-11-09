#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <optional>

namespace oor::model
{

// One MATRIG card: a rigid material of SOL 700 (a rigid body is the elements whose properties name it).
//
// Card layout:
//   MATRIG  MID        RHO        E          NU      MASS    XC      YC     ZC
//           IXX        IXY        IXZ        IYY     IYZ     IZZ     CID
//           VX         VY         VZ         WX      WY      WZ
//           XC-LOCAL   YC-LOCAL   ZC-LOCAL
// The continuation lines may be left out. A blank mass, centre of gravity or inertia is computed from the density and
// the mesh of the body, and a blank velocity is taken from the initial conditions of the grid points of the body (a
// blank field is not 0.0 for these), so they are kept as optional values. The centre of gravity is given in the basic
// system (XC, YC, ZC) or in the system CID (XC-LOCAL, YC-LOCAL, ZC-LOCAL), not both.
struct RigidMatCard
{
    base::EntityId id = 0;
    double density = 1.0;
    double youngsModulus = 1.0;
    double poissonRatio = 0.2;
    std::optional<double> mass;
    std::array<std::optional<double>, 3> center{};      // XC, YC, ZC
    std::array<std::optional<double>, 6> inertia{};     // IXX, IXY, IXZ, IYY, IYZ, IZZ
    base::EntityId frameId = 0;                         // the system of the inertia and of the local centre; 0 is blank
    std::array<std::optional<double>, 3> velocity{};    // VX, VY, VZ: the initial translational velocity
    std::array<std::optional<double>, 3> rotation{};    // WX, WY, WZ: the initial rotational velocity
    std::array<std::optional<double>, 3> localCenter{}; // XC-LOCAL, YC-LOCAL, ZC-LOCAL
};

} // namespace oor::model
