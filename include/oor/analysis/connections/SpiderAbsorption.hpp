#pragma once

#include "oor/analysis/components/ComponentBoundary.hpp"
#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/connections/ConnectionSharing.hpp"
#include "oor/analysis/connections/Spiders.hpp"

#include <cstdint>
#include <functional>
#include <vector>

namespace oor::analysis
{

// The boundaries with the eligible spiders absorbed (the `spiders=True` form of
// `component_boundaries`): a spider whose legs are all on a unit and touched by no other connection becomes part of the
// unit, so its legs leave the boundary (unless something else acts on them or they are retained) and its reference
// point stands for them, with the spider's element ID and the legs it replaces. The reference point keeps the other
// connections that act on it; two spiders on one reference point give it the smaller element ID and all their legs. A
// spider the unit already holds (a group absorbs a spider whose reference point only it uses) is left alone:
// `isHeld(unit, spider)` says so (empty for the components, which hold no connection).
class SpiderAbsorption
{
public:
    using HeldTest = std::function<bool(std::int32_t unit, const Spider& spider)>;

    static void apply(std::vector<Boundary>& boundaries, const std::vector<Spider>& spiders,
                      const ConnectionSharing& sharing, const ComponentModel& components, const HeldTest& isHeld = {});
};

} // namespace oor::analysis
