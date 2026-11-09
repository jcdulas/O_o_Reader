#pragma once

#include "oor/analysis/components/LoadSets.hpp"
#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/query/named/common/NodePositions.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::query
{

// A force and a moment applied at a node, in the basic system, already multiplied by the factors of the LOAD cards.
struct NodalLoad
{
    std::int32_t node = 0;
    base::Vec3 force;
    base::Vec3 moment;
};

// What a load card puts on the structure as vectors, in the basic system. FORCE and MOMENT give their direction in the
// coordinate system CID (the components along the axes at the node: the R, theta and Z directions of a cylindrical
// system at the position of the node); FORCE1/FORCE2 and MOMENT1/MOMENT2 are along the line between two grid points or
// the cross product of two lines; an SLOAD is a scalar: its value is put in the first component of the force. The
// objects must not outlive the model's cards or an edit of them.
class LoadVectors
{
public:
    LoadVectors(const model::Model& model, base::ThreadPool* pool);

    // The concentrated forces and moments of an elementary load (none for a load of another kind).
    [[nodiscard]] std::vector<NodalLoad> nodal(const analysis::ElementaryLoad& load);
    // The acceleration a GRAV card gives, in the basic system and scaled (nothing for another card).
    [[nodiscard]] std::optional<base::Vec3> acceleration(const analysis::ElementaryLoad& load);
    // The coordinate system a force, moment or gravity card is given in (nothing for another card).
    [[nodiscard]] std::optional<std::int32_t> systemOf(const analysis::ElementaryLoad& load) const;

    // The components (a, b, c) given in the system `frameId` at the basic point `point`, as a basic vector (the
    // components themselves for the basic system and for a system the model does not define).
    [[nodiscard]] base::Vec3 inBasic(std::int32_t cid, const base::Vec3& point, const base::Vec3& components);
    // The position of a grid point in the basic system.
    [[nodiscard]] base::Vec3 positionOf(std::int32_t node) const { return positions().position(node); }

private:
    // The positions of the grid points, the first time a load needs one (a FORCE in the basic system needs none: the
    // loads of a model made only of them no longer pay the positions of every grid point).
    [[nodiscard]] const NodePositions& positions() const;

    const model::Model& m_model;
    base::ThreadPool* m_pool;
    mutable std::optional<NodePositions> m_positions;
    analysis::CoordinateTransform m_transform;
};

} // namespace oor::query
