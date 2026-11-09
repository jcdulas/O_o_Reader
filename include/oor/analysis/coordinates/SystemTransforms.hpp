#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/base/math/Mat3.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/base/threading/ThreadPool.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace oor::model
{
class Model;
}

namespace oor::analysis
{

// Which way a batch transform goes: from the coordinates of a system to the basic system, or back.
enum class TransformDirection : std::uint8_t
{
    ToBasic,
    FromBasic
};

// Points, vectors and local axes turned between the coordinate systems of a model and the basic system, many at a time
// (`model.coords.to_basic`, `from_basic`, `axes_at` in Python). Each system is resolved once a call (the systems of
// its card, chains of RID and CORD1x grid points included), then the items are turned on the threads of `pool`.
//
// `cids` holds one system for every item, or one per item; 0 is the basic system. A system that does not exist, refers
// to itself or is not defined by three distinct points throws CoordinateSystemError.
class SystemTransforms
{
public:
    // Points: ToBasic takes the coordinates in the system (x y z; R theta Z; R theta phi, angles in degrees) and gives
    // the positions in the basic system; FromBasic the inverse (on the axis of a cylindrical or spherical system the
    // angles are 0).
    [[nodiscard]] static std::vector<base::Vec3> points(const model::Model& model,
                                                        std::span<const base::EntityId> cids,
                                                        std::span<const base::Vec3> points,
                                                        TransformDirection direction, base::ThreadPool* pool);

    // Vectors at points: ToBasic takes the components along the directions 1, 2, 3 of the system at the point `at`
    // (in the basic system) and gives the components in the basic system; FromBasic the inverse. `at` holds one point
    // for every vector, one per vector, or none when every system is rectangular (its directions are the same
    // everywhere): a cylindrical or spherical system without `at` throws base::UsageError.
    [[nodiscard]] static std::vector<base::Vec3> vectors(const model::Model& model,
                                                         std::span<const base::EntityId> cids,
                                                         std::span<const base::Vec3> vectors,
                                                         std::span<const base::Vec3> at, TransformDirection direction,
                                                         base::ThreadPool* pool);

    // The directions 1, 2, 3 of the system at each point given in the basic system, as the columns of a matrix in the
    // basic system (CoordinateFrame::directionsAt: x y z of a rectangular system, R theta Z of a cylindrical one,
    // R theta phi of a spherical one), so that `axes * local` is a vector in the basic system.
    [[nodiscard]] static std::vector<base::Mat3> axesAt(const model::Model& model,
                                                        std::span<const base::EntityId> cids,
                                                        std::span<const base::Vec3> at, base::ThreadPool* pool);
};

} // namespace oor::analysis
