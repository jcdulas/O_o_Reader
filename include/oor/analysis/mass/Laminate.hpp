#pragma once

#include "oor/model/core/Model.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oor::analysis
{

// One ply of a laminate as it is stacked: the material, the thickness and fibre angle of the card, and where the ply is
// across the thickness (the z of its bottom and top faces measured from the reference plane of the element: the first
// ply starts at Z0 and each one follows the one before it).
struct LaminatePly
{
    base::EntityId materialId = 0;
    double thickness = 0.0;
    double angleInDegrees = 0.0;
    double zBottom = 0.0;
    double zTop = 0.0;
};

// The stiffness of a laminate by classical lamination theory.
//
//   a   the membrane matrix A: the sum over the plies of the stiffness Q (turned by the ply angle) times the thickness
//   b   the membrane-bending coupling matrix B: the sum of Q (z top squared - z bottom squared) / 2
//   d   the bending matrix D: the sum of Q (z top cubed - z bottom cubed) / 3
//   g   the transverse shear matrix: SHEAR_FACTOR times the sum of the shear moduli of the plies (turned by the ply
//       angle) times the thickness, for the shears (xz, yz); infinite on the diagonal (and 0 off it) when a ply has no
//       transverse shear flexibility (a MAT8 with G1Z or G2Z blank or zero)
//
// A, B and D are 3 x 3 matrices for the resultants (x, y, xy) and the engineering strains, G is 2 x 2; all are row by
// row. The stack of the plies and the density of the plies come with them. All the matrices are NaN for a laminate
// whose option is SMEAR or SMCORE (the format reference gives no ply stacking for them), A only is kept for MEM and D
// only for BEND.
struct LaminateStiffness
{
    std::vector<LaminatePly> plies;
    double totalThickness = 0.0;
    double massPerArea = 0.0; // the sum of density times thickness, without the non-structural mass of the card
    std::array<double, 9> a = {};
    std::array<double, 9> b = {};
    std::array<double, 9> d = {};
    std::array<double, 4> g = {};
    // what the matrices are made of, a layer of the stack each: the stiffness of the layer and its
    // transverse shear moduli (infinite on the diagonal for no flexibility) in the axes of the laminate, and the ply of
    // the card it is (the two layers of a ply of a SYM laminate are the same ply); whether Z0 was given
    std::vector<std::array<double, 9>> layerStiffness;
    std::vector<std::array<double, 4>> layerShear;
    std::vector<std::uint32_t> plyOf;
    std::size_t plyCount = 0;
    bool bottomOffsetGiven = true;
    // with the derivatives: those of a, b, d and g with respect to the thickness of each ply of the card, the
    // reference plane moved with the plies when Z0 is blank (one entry per ply; empty without)
    std::vector<std::array<double, 9>> aDerivatives;
    std::vector<std::array<double, 9>> bDerivatives;
    std::vector<std::array<double, 9>> dDerivatives;
    std::vector<std::array<double, 4>> gDerivatives;
    // with the second derivatives: those of a, b and d with respect to the thicknesses of two plies
    // of the card, entry p * plyCount + q for the plies p and q (symmetric; empty without). G, linear in the
    // thicknesses as A is, has none.
    std::vector<std::array<double, 9>> aHessian;
    std::vector<std::array<double, 9>> bHessian;
    std::vector<std::array<double, 9>> dHessian;
};

// The A, B, D and transverse shear matrices of a stack of layers (bottom first) and their derivatives with respect to
// the thickness of each ply. Layer k is ply plyOf[k] of `plyCount` (the two layers of a ply of a SYM
// laminate are one ply: its thickness moves both); the stack starts at `z0`, or at -h/2 when it is nullopt (a blank Z0,
// which moves with the plies: its derivative, -1/2 per layer, is included). `shear` infinite on a diagonal: a layer
// with no transverse shear flexibility, G infinite (and its derivatives 0).
struct LaminateMatrices
{
    std::array<double, 9> a{};
    std::array<double, 9> b{};
    std::array<double, 9> d{};
    std::array<double, 4> g{};
    std::vector<std::array<double, 9>> aDerivatives;
    std::vector<std::array<double, 9>> bDerivatives;
    std::vector<std::array<double, 9>> dDerivatives;
    std::vector<std::array<double, 4>> gDerivatives;
    // The second derivatives with respect to the thicknesses of two plies, entry p * plyCount + q, in
    // closed form: the faces of the layers are affine in the thicknesses, so A is linear (its Hessian zero), B
    // quadratic and D cubic (the blank Z0 included).
    std::vector<std::array<double, 9>> aHessian;
    std::vector<std::array<double, 9>> bHessian;
    std::vector<std::array<double, 9>> dHessian;
};

class Laminate
{
public:
    // The fraction of the shear modulus times the thickness that carries the transverse shear of a laminate: the
    // classical correction of 5/6 for a rectangular section.
    static constexpr double SHEAR_FACTOR = 5.0 / 6.0;

    // True for PCOMP and PCOMPG.
    [[nodiscard]] static bool supports(std::uint16_t propertyType) noexcept;

    // The plane stress stiffness of a ply (3 x 3, row by row, engineering shear) turned by its fibre angle (degrees,
    // from the x axis of the laminate to the fibres): the stiffness of the ply in the axes of the laminate.
    [[nodiscard]] static std::array<double, 9> turnedStiffness(const std::array<double, 9>& stiffness,
                                                               double angleInDegrees);

    // The stack of the plies of a PCOMP (the mirror image of the plies of a SYM laminate included) or PCOMPG, with the
    // z of each face. Empty for another property.
    [[nodiscard]] static std::vector<LaminatePly> stack(const model::Model& model, model::CardLocation location);

    // The stack and the matrices; nullopt for a property that is not a laminate. Throws MassError when a ply
    // refers to a material that does not exist or has no plane stress law (the MAT1, MAT2, MAT8 and MAT9 give one).
    [[nodiscard]] static std::optional<LaminateStiffness> of(const model::Model& model, model::CardLocation location);
    // The same with the derivatives of the matrices with respect to the thickness of each ply of the card.
    [[nodiscard]] static std::optional<LaminateStiffness> of(const model::Model& model, model::CardLocation location,
                                                             bool gradient);
    // The same with, when `hessian`, the second derivatives too.
    [[nodiscard]] static std::optional<LaminateStiffness> of(const model::Model& model, model::CardLocation location,
                                                             bool gradient, bool hessian);
    [[nodiscard]] static std::optional<LaminateStiffness> of(const model::Model& model, base::EntityId pid);

    // The matrices of a stack of layers (LaminateMatrices) and, with `gradient`, their derivatives; with `hessian`,
    // their second derivatives too.
    [[nodiscard]] static LaminateMatrices matrices(std::span<const std::array<double, 9>> stiffness,
                                                   std::span<const std::array<double, 4>> shear,
                                                   std::span<const double> thickness,
                                                   std::span<const std::uint32_t> plyOf, std::size_t plyCount,
                                                   std::optional<double> z0, bool gradient, bool hessian = false);
};

} // namespace oor::analysis
