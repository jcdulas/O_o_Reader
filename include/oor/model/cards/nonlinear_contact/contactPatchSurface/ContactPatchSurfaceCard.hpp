#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <vector>

namespace oor::model
{

// One quadrilateral patch of a rigid contact body: its number and its four grid points. The right-hand rule on the
// order of the points gives the interior side of the rigid surface.
struct ContactPatch
{
    base::EntityId patchId = 0;            // the patch number, from 1 up
    std::array<base::EntityId, 4> grids{}; // G1 to G4

    [[nodiscard]] bool operator==(const ContactPatch&) const noexcept = default;
};

// One BCPATCH card: a rigid contact body made of quadrilateral patches (SOLs 101 and 400). A BCRGSRF or a BCBODY1 names
// it by its RBID.
//
// Card layout: BCPATCH  RBID      then one patch to a continuation line:  IDP  G1  G2  G3  G4
struct ContactPatchSurfaceCard
{
    base::EntityId id = 0; // RBID: unique among the contact surfaces
    std::vector<ContactPatch> patches;
};

} // namespace oor::model
