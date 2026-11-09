#pragma once

#include "oor/analysis/components/EntityDofs.hpp"
#include "oor/analysis/components/GroupSet.hpp"
#include "oor/analysis/components/ModelEntities.hpp"
#include "oor/analysis/components/NodeEntityIndex.hpp"
#include "oor/analysis/connections/FastenerPiercings.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <string_view>
#include <vector>

namespace oor::analysis
{

// What acts on a boundary point of a zone from outside.
enum class CauseKind : std::uint8_t
{
    Element,   // a shell, a solid, a 1D element or a mass next to the zone
    Rigid,     // a rigid element
    Mpc,       // an MPC equation
    Connection // a connector: spring, damper, bush, gap, fastener, weld
};

[[nodiscard]] std::string_view causeKindName(CauseKind kind) noexcept;

struct ZoneCause
{
    CauseKind kind = CauseKind::Element;
    std::uint32_t entity = 0; // its index in ModelEntities
};

// A boundary point of a zone: the degrees of freedom through which the rest of the model acts on it and the entities
// that do.
struct ZoneBoundaryPoint
{
    base::EntityId node = 0;
    ComponentSet components = 0;
    bool isPiercedPoint = false;   // a grid point of an element that a CFAST or CWELD among the causes pierces
    std::vector<ZoneCause> causes; // by entity index
};

// An element a zone encloses: outside its list, all its grid points in it.
struct EnclosedElement
{
    std::uint32_t entity = 0;
    bool isPointElement = false; // all its grid points are one point (a mass, a grounded spring)
};

// The zones: a group given as an element list, with what acts on it from outside and what it
// encloses. Its boundary points are its nodes that an outside entity acts on (an element next to it, a rigid element,
// an MPC equation or a connector that it does not hold; for a CFAST or a CWELD whose other side is outside, the grid
// points of the element it pierces inside the zone), each with the union of the components the outside entities act on
// there (EntityDofs) and the entities themselves.
class Zones
{
public:
    [[nodiscard]] static std::vector<ZoneBoundaryPoint> boundaryOf(const model::Model& model, const GroupSet& groups,
                                                                   std::int32_t unit, const ModelEntities& entities,
                                                                   const NodeEntityIndex& index,
                                                                   const FastenerPiercings* piercings);
    // The elements of the group that are not in its list (absorbed: all their grid points in it), by entity index.
    [[nodiscard]] static std::vector<EnclosedElement> enclosedBy(const GroupSet& groups, std::int32_t unit,
                                                                 const ModelEntities& entities);
};

} // namespace oor::analysis
