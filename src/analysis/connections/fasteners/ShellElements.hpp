#pragma once

#include "NodePositions.hpp"
#include "ShellShape.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/model/core/Model.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>

namespace oor::analysis::detail
{

// One element a fastener can pierce, with the positions of its grid points: a shell of the model or the virtual
// element of a CWELD GRIDID (`isVirtual`, no location).
struct ShellElement
{
    model::CardLocation location;
    bool isVirtual = false;
    base::EntityId eid = 0;
    base::EntityId pid = 0;
    ShellShapeKind kind = ShellShapeKind::Quad4;
    std::uint8_t presentNodes = 0; // bit k: slot k holds a grid point
    std::array<base::EntityId, MAX_SHELL_NODES> nodes{};
    std::array<base::Vec3, MAX_SHELL_NODES> points{};

    [[nodiscard]] std::size_t slotCount() const noexcept { return ShellShape::nodeCount(kind); }
    [[nodiscard]] bool hasNode(std::size_t slot) const noexcept { return (presentNodes & (1U << slot)) != 0; }
    [[nodiscard]] base::Vec3 pointAt(const ShapeValues& values) const noexcept;
};

// Reads the elements a fastener can pierce: CQUAD4, CQUADR, CTRIA3, CTRIAR (linear) and CQUAD8, CTRIA6
// (quadratic, mid-side nodes optional). The format reference names the shells of a PSHELL or a PCOMP; the property
// type is not checked here.
class ShellElements
{
public:
    ShellElements(const model::Model& model, const NodePositions& positions);

    [[nodiscard]] static bool isShellType(std::uint16_t type) noexcept;
    // The element at `location`; nothing when it is not a shell type or a grid point of it has no position.
    [[nodiscard]] std::optional<ShellElement> read(model::CardLocation location) const;
    [[nodiscard]] std::optional<ShellElement> readById(base::EntityId eid) const;
    // The virtual element of a CWELD GRIDID on the grid points GA1..GA8 (or GB1..GB8) with the shape letter of SPTYP:
    // 'Q' (corners 1 to 4, mid-sides 5 to 8 optional) or 'T' (corners 1 to 3, mid-sides 4 to 6 optional).
    [[nodiscard]] std::optional<ShellElement> fromGrids(std::span<const base::EntityId> grids, char shape) const;

private:
    [[nodiscard]] bool place(ShellElement& element) const;

    const model::Model& m_model;
    const NodePositions& m_positions;
};

} // namespace oor::analysis::detail
