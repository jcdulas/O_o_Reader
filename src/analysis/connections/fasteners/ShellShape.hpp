#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace oor::analysis::detail
{

// The shapes of the elements a fastener can pierce: the shells (CTRIA3, CTRIAR; CQUAD4, CQUADR; CTRIA6; CQUAD8)
// and the virtual element of a CWELD GRIDID (SPTYP T3 to T6, Q4 to Q8).
enum class ShellShapeKind : std::uint8_t
{
    Tria3,
    Quad4,
    Tria6,
    Quad8
};

constexpr std::size_t MAX_SHELL_NODES = 8;

// The shape functions of a shell element at one point of its natural coordinates, with their first and second
// derivatives (what the closest-point projection needs). Slots that hold no grid point are 0.
struct ShapeValues
{
    std::array<double, MAX_SHELL_NODES> n{};
    std::array<double, MAX_SHELL_NODES> dXi{};
    std::array<double, MAX_SHELL_NODES> dEta{};
    std::array<double, MAX_SHELL_NODES> dXiXi{};
    std::array<double, MAX_SHELL_NODES> dXiEta{};
    std::array<double, MAX_SHELL_NODES> dEtaEta{};
};

// The shape functions of the shell elements. The corners come first
// in the order of the element; the mid-side nodes follow the edges 1-2, 2-3, ... A triangle is written in the area
// coordinates (xi, eta) = (L2, L3), L1 = 1 - xi - eta; a quadrilateral in (xi, eta) from -1 to 1.
//
// A missing mid-side node (a blank field of a CQUAD8 or CTRIA6, the GRIDID shapes Q5 to Q7 and T4 to T5) leaves its
// edge straight: its function is dropped and half of it goes to each corner of the edge.
class ShellShape
{
public:
    [[nodiscard]] static std::size_t nodeCount(ShellShapeKind kind) noexcept;
    [[nodiscard]] static std::size_t cornerCount(ShellShapeKind kind) noexcept;
    [[nodiscard]] static bool isTriangle(ShellShapeKind kind) noexcept
    {
        return kind == ShellShapeKind::Tria3 || kind == ShellShapeKind::Tria6;
    }
    [[nodiscard]] static bool isQuadratic(ShellShapeKind kind) noexcept
    {
        return kind == ShellShapeKind::Tria6 || kind == ShellShapeKind::Quad8;
    }
    // The natural coordinates of the centre of the element: where the projection starts.
    [[nodiscard]] static std::array<double, 2> centre(ShellShapeKind kind) noexcept;

    // The values at (xi, eta). Bit k of `presentNodes` is set when slot k holds a grid point; the corners are always
    // taken as present.
    [[nodiscard]] static ShapeValues evaluate(ShellShapeKind kind, std::uint8_t presentNodes, double xi,
                                              double eta) noexcept;

    // How far (xi, eta) is outside the element in natural coordinates: 0 inside or on the boundary, else the largest
    // excess (|xi| - 1 or |eta| - 1 for a quadrilateral, the most negative area coordinate for a triangle).
    [[nodiscard]] static double outsideDistance(ShellShapeKind kind, double xi, double eta) noexcept;
};

} // namespace oor::analysis::detail
