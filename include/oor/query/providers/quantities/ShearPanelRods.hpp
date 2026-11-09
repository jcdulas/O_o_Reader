#pragma once

#include "oor/base/math/Vec3.hpp"
#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/results/ResultSets.hpp"

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace oor::query
{

// The effective extensional rods of a shear panel. A CSHEAR whose PSHEAR has an effectiveness F1
// (F2) above zero carries its extension along edges 1-2 and 3-4 (2-3 and 1-4) in rods of area F T PA / (L12 + L34)
// for F up to 1.01, else 0.5 F T^2 each (PA the area of the panel, half the cross product of its
// diagonals). A rod is a two-node rod of the Young's modulus of the MAT1: its axial force, the same at both ends, is
// E A / L times the elongation of its edge. The force table of the panel gives its shear flows, not the forces of these
// rods: they are recovered here from the translations of its grid points. The edges are taken in the mean plane of the
// panel (the plane through the centroid normal to the cross product of the diagonals), as the element frame is (x from
// G1 to G2, format reference Figure 9-65).
struct PanelRods
{
    bool ok = false;                     // the model describes the panel and its rods
    std::string why;                     // why not
    bool any = false;                    // some edge has a rod
    std::array<std::int32_t, 4> nodes{}; // the grid points G1 to G4 (IDs of the model)
    // per edge 1-2, 2-3, 3-4, 4-1: the unit vector of the edge (basic), its length, the axial stiffness E A / L of its
    // rod (0 without one), and the weights that turn the rod's force into the panel's in-plane resultants in its
    // element frame (the mean over the panel of the stress the rod carries: N = sum of P L d d^T / PA): nx, ny, nxy
    std::array<base::Vec3, 4> direction{};
    std::array<double, 4> length{};
    std::array<double, 4> stiffness{};
    std::array<double, 4> rodArea{};
    std::array<std::array<double, 3>, 4> weights{};
    double area = 0.0; // PA
};

// The rods of the panels of one result set, looked up by the element ID the file uses (through the attachment).
class ShearPanelRodFacts
{
public:
    ShearPanelRodFacts(const ResultQueryContext& context, std::size_t set);
    [[nodiscard]] const PanelRods& panel(std::int32_t resultEid);

private:
    const ResultQueryContext& m_context;
    std::size_t m_set;
    const model::Model* m_model;
    std::unique_ptr<NodePositions> m_positions;
    std::string m_positionsWhy;
    bool m_positionsTried = false;
    std::map<std::int32_t, PanelRods> m_panels;
};

// The translations (basic) of the grid points of the panels for the steps of one force table: those of the
// displacement table of the same result set, subcase and design cycle (the eigenvectors for a table of modes), each
// step of the force table matched to the step of the same value.
class PanelTranslations
{
public:
    // `tables`: the displacement and eigenvector tables of the call turned to the basic system (tablesIn).
    // `nodes`: the grid points wanted (IDs of the model).
    PanelTranslations(const ResultQueryContext& context, const std::vector<TableRef>& tables, const TableRef& forces,
                      const results::ResultData& forceRows, const std::vector<std::int32_t>& nodes);
    [[nodiscard]] bool found() const noexcept { return m_data != nullptr; }
    [[nodiscard]] bool isComplex() const noexcept { return m_complex; }
    // The translation of a grid point at a step of the force table (real or imaginary part); nothing when the table
    // has no row of it or no step of that value.
    [[nodiscard]] std::optional<base::Vec3> real(std::size_t forceStep, std::int32_t node) const;
    [[nodiscard]] std::optional<base::Vec3> imag(std::size_t forceStep, std::int32_t node) const;

private:
    [[nodiscard]] std::optional<base::Vec3> at(std::size_t forceStep, std::int32_t node, bool imaginary) const;

    std::shared_ptr<const results::ResultData> m_data;
    bool m_complex = false;
    int m_t1 = -1;
    std::unordered_map<std::int32_t, std::size_t> m_rowOf; // per model grid ID
    std::vector<std::size_t> m_stepOf;                     // per step of the force table, SIZE_MAX for none
};

// The axial forces of the rods of a panel at one step from the translations of its grid points (tension positive),
// NaN for an edge with a rod whose grid points have no translation, 0 for an edge without one.
[[nodiscard]] std::array<double, 4> rodForces(const PanelRods& rods, const PanelTranslations& translations,
                                              std::size_t forceStep, bool imaginary);

} // namespace oor::query
