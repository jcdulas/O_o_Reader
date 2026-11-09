#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One ACCSSPT card: the grid points inside an external superelement whose motion the residual structure of the
// assembly keeps (the access points). The entry is used in the run that creates the external superelement and is valid
// only there.
//
// Card layout: ACCSSPT  GID1  GID2  GID3  -etc.-        fields 1 to 8, and on the continuation lines
// The format draws the IDs one to a field and says nothing about THRU, so the list is plain IDs; a blank field between
// two of them is padding and is not kept. The points are interior points of the external superelement and have to be
// unique across all the external superelements (rules between cards and between models, not
// checked here). A point is a grid point of the model that holds the card, so `Node` is what the references name.
struct SuperelementAccessPointsCard
{
    std::vector<base::EntityId> nodeIds;

    [[nodiscard]] bool operator==(const SuperelementAccessPointsCard&) const noexcept = default;
};

} // namespace oor::model
