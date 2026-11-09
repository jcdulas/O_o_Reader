#pragma once

#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/components/UnitMembership.hpp"
#include "oor/analysis/connections/ConnectionSharing.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

namespace oor::analysis
{

// Why an RBE2 or an RBE3 is not a spider.
enum class SpiderReason : std::uint8_t
{
    None,
    Um,                          // an RBE3 with UM: its dependent degrees of freedom are not those of the reference
    ReferenceIsLeg,              // the reference point is one of its legs
    ReferenceDependentElsewhere, // the reference point is a dependent point of another rigid element or MPC equation
    LegsOnSeveralUnits           // the legs are not all on one unit (on several, or a leg on none)
};

// The word of a reason in the tables ("um", "referenceIsLeg", ...; empty for None).
[[nodiscard]] std::string_view spiderReasonName(SpiderReason reason) noexcept;

// A leg of a spider: a grid point Gi of an RBE3 with its components Ci and weight WTi, or a dependent grid point of an
// RBE2 with its components CM (no weight).
struct SpiderLeg
{
    base::EntityId node = 0;
    std::int32_t components = 0;
    double weight = std::numeric_limits<double>::quiet_NaN();
};

// A RigidBodyStar (RBE2) or a RigidInterpolation (RBE3) as a candidate spider of one unit.
struct Spider
{
    model::CardLocation card;
    base::EntityId eid = 0;
    bool isInterpolation = false;         // a RigidInterpolation; else a RigidBodyStar
    std::uint32_t connection = 0;         // its index in ComponentModel::connections()
    base::EntityId reference = 0;         // GN of an RBE2, REFGRID of an RBE3
    std::int32_t referenceComponents = 0; // 123456 for an RBE2, REFC for an RBE3
    std::vector<SpiderLeg> legs;          // in the order of the card
    std::int32_t unit = 0;                // the unit that holds every leg; 0 when none does
    bool isSpider = false;
    SpiderReason reason = SpiderReason::None;
    // A spider can be absorbed into its unit when no other connection touches a leg.
    bool isEligible = false;
    std::vector<base::EntityId> blockingNodes;      // its legs that another connection touches, sorted
    std::vector<std::uint32_t> blockingConnections; // those connections, increasing
    std::uint64_t revision = 0;                     // the revision stamp of the card (RevisionStamp::computeFor)

    // The grid points of the legs, sorted, each once.
    [[nodiscard]] std::vector<base::EntityId> legNodes() const;
};

// The spiders of a model: the RBE2 and RBE3 whose legs are all on one unit (a component, or a
// group of the caller), with what keeps the others from being spiders and what keeps a spider from being absorbed.
class Spiders
{
public:
    // One entry per RBE2 and RBE3 and per unit that holds all its legs (groups may overlap); a card whose legs no unit
    // holds has one entry with unit 0. By element ID, then unit.
    [[nodiscard]] static std::vector<Spider> find(const model::Model& model, const ComponentModel& components,
                                                  const ConnectionSharing& sharing, const UnitMembership& units,
                                                  base::ThreadPool* pool = nullptr);
};

} // namespace oor::analysis
