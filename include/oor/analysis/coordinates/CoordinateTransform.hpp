#pragma once

#include "oor/analysis/coordinates/CoordinateFrame.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/base/math/Mat3.hpp"
#include "oor/base/math/Vec3.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace oor::model
{
class Model;
}

namespace oor::analysis
{

// A coordinate system card, as the geometry needs it: CORD1R/1C/1S give three grid points, CORD2R/2C/2S three points in
// the system RID. The cards of the six types are read into this one shape.
struct SystemDefinition
{
    enum class Form : std::uint8_t
    {
        ThreeGrids, // CORD1x: the basic positions of three grid points
        ThreePoints // CORD2x: three points given in the system `referenceFrameId`
    };

    std::string_view cardName; // the card type: "RectFrame1", ...
    CoordinateKind kind = CoordinateKind::Rectangular;
    Form form = Form::ThreePoints;
    base::EntityId id = 0;
    base::EntityId referenceFrameId = 0;      // CORD2x: the reference system (0: the basic system)
    std::array<base::EntityId, 3> nodes = {}; // CORD1x: the grid points (origin, on the z axis, in the x-z plane)
    std::array<std::array<double, 3>, 3> points = {}; // CORD2x: A, B, C

    [[nodiscard]] bool operator==(const SystemDefinition&) const noexcept = default;
};

using SystemDefinitions = std::unordered_map<base::EntityId, SystemDefinition>;

// The coordinate system cards of a model, by ID (the six card types).
[[nodiscard]] SystemDefinitions readSystemDefinitions(const model::Model& model);

// Resolves coordinate systems to the basic system and converts between them: rectangular, cylindrical and spherical
// systems, defined by points in another system (CORD2x) or by grid points that may themselves be in other systems
// (CORD1x), through chains of any length. A system that depends on itself is refused with a message naming the cycle
// (CoordinateSystemError); so is a system that does not exist or whose three points do not define axes.
//
// The resolved systems are kept (a system is computed once). The class does not watch the model: BasicPositionCache
// owns one and tells it which systems to forget when cards change; a transform made from a model for a single use
// (`CoordinateTransform(model)`) sees the model as it was when it was made.
class CoordinateTransform
{
public:
    // Gives the position in the basic system of a grid point (by ID), for the CORD1x systems. It must call visit() for
    // the point it resolves, so that a loop through a grid point is found.
    using NodeResolver = std::function<base::Vec3(base::EntityId)>;

    CoordinateTransform() = default;
    // A transform of the systems and grid points of a model, for a single use.
    explicit CoordinateTransform(const model::Model& model);

    void setDefinitions(SystemDefinitions definitions) { m_definitions = std::move(definitions); }
    [[nodiscard]] const SystemDefinitions& definitions() const noexcept { return m_definitions; }
    void setNodeResolver(NodeResolver resolver) { m_resolver = std::move(resolver); }

    // True for the basic system (0) and for a system that has a card.
    [[nodiscard]] bool hasSystem(base::EntityId cid) const noexcept { return cid == 0 || m_definitions.contains(cid); }

    // The system with this ID resolved to the basic system. 0 is the basic system. Throws CoordinateSystemError.
    [[nodiscard]] const CoordinateFrame& frame(base::EntityId cid);

    // The position in the basic system of the point whose coordinates are given in the system `cp` (0: basic).
    [[nodiscard]] base::Vec3 toBasic(base::EntityId cp, const base::Vec3& coordinates);

    // The coordinates, in the system `cp` (0: basic), of a point given in the basic system: the inverse of toBasic. A
    // query edit that gives a position in the basic system is written to the grid point in its CP system with it.
    [[nodiscard]] base::Vec3 fromBasic(base::EntityId cp, const base::Vec3& basicPoint);

    // The position of a grid point in the basic system (its X1 X2 X3 taken in its CP system).
    [[nodiscard]] base::Vec3 basicPositionOfNode(base::EntityId nid);

    // The rotation that carries the displacement components of a grid point whose CD is `cd` into the basic system, at
    // the point `basicPosition`: the columns are the directions 1, 2, 3 of the CD system there (see
    // CoordinateFrame::directionsAt). The basic system (0) and -1 (a fixed point) give the identity.
    [[nodiscard]] base::Mat3 displacementRotation(base::EntityId cd, const base::Vec3& basicPosition);

    // Forgets the resolved systems (all, or the ones listed): they are resolved again when asked for.
    void forgetAll() { m_frames.clear(); }
    void forget(base::EntityId cid) { m_frames.erase(cid); }
    // Forgets the positions of grid points a standalone transform has computed (not used with a node resolver).
    void forgetPositions() { m_positions.clear(); }

    // How many systems have been resolved since the transform was made (a counter, for tests and statistics).
    [[nodiscard]] std::uint64_t resolvedSystemCount() const noexcept { return m_resolvedCount; }

    // --- loop detection
    // -----------------------------------------------------------------------------------------------

    // Marks that a system or a grid point is being resolved; a second visit of the same thing before the first ends is
    // a loop. The mark ends when the object goes out of scope.
    class Visit
    {
    public:
        Visit(const Visit&) = delete;
        Visit& operator=(const Visit&) = delete;
        Visit(Visit&& other) noexcept : m_transform(other.m_transform) { other.m_transform = nullptr; }
        Visit& operator=(Visit&&) = delete;
        ~Visit()
        {
            if (m_transform != nullptr)
                m_transform->m_stack.pop_back();
        }

    private:
        friend class CoordinateTransform;
        explicit Visit(CoordinateTransform* transform) noexcept : m_transform(transform) {}
        CoordinateTransform* m_transform;
    };

    enum class Thing : std::uint8_t
    {
        System,
        Node
    };
    // Throws CoordinateSystemError naming the cycle when the thing is already being resolved.
    [[nodiscard]] Visit visit(Thing thing, base::EntityId id);

private:
    struct StackEntry
    {
        Thing thing;
        base::EntityId id;
    };

    [[nodiscard]] std::string describe(const StackEntry& entry) const;
    [[nodiscard]] CoordinateFrame resolve(const SystemDefinition& definition);

    SystemDefinitions m_definitions;
    std::unordered_map<base::EntityId, CoordinateFrame> m_frames;
    NodeResolver m_resolver;
    std::vector<StackEntry> m_stack;
    std::uint64_t m_resolvedCount = 0;
    // A standalone transform (made from a model) resolves grid points itself.
    const model::Model* m_model = nullptr;
    std::unordered_map<base::EntityId, base::Vec3> m_positions;
};

} // namespace oor::analysis
