#pragma once

#include "oor/results/core/ResultTable.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace oor::analysis
{

// What the values of a result table are, as far as frames go: the families that have a frame change defined, and the
// families that have none (which every frame change refuses, with a reason).
enum class FrameFamily : std::uint8_t
{
    NodalVector, // displacements, velocities, accelerations, eigenvectors, SPC and MPC forces, applied loads,
                 // non-linear loads: a translation vector and a rotation vector (or force and moment) at a grid point
    NodeForceBalance, // the grid point force balance: force and moment of an element at a grid point
    ShellStress,      // oxx oyy txy of a shell (non-composite) at a fibre
    ShellStrain,      // exx eyy exy (engineering shear) of a shell at a fibre
    ShellForce,       // the membrane, bending and transverse shear resultants of a shell, per unit length
    PlyStress,        // o11 o22 t12 t1z t2z of a ply of a laminate
    PlyStrain,
    SolidStress, // the six stress components of a solid
    SolidStrain,
    // families with no frame change
    OneDimensionalStress, // rods, tubes, bars, beams: values along the axis and at recovery points
    OneDimensionalForce,  // rod, bar, beam forces and moments
    ConnectorForce,       // springs, dampers, bushes, gaps, welds, fasteners
    Scalar,               // temperatures, heat loads, scalar values at points
    Nonlinear,            // non-linear element tables
    NodePointStress,
    RandomOrModal, // random analysis results, modal contributions
    Other
};

// What a frame change goes to.
enum class FrameTargetKind : std::uint8_t
{
    Basic,       // the basic system
    NodeOutput,  // the displacement system (CD) of each grid point
    Element,     // the element system of each element
    Material,    // the material system of each shell
    Ply,         // the ply system of each ply
    Coordinate,  // the system of a card, as it defines itself (rectangular, cylindrical or spherical)
    Cylindrical, // the axes of a system read as cylindrical axes about its z axis
    Spherical,   // the axes of a system read as spherical axes about its origin
    Direction,   // `along`: the component along a direction given in the basic system
    Any          // every target (a row of a family that has none)
};

enum class ConventionStatus : std::uint8_t
{
    Validated, // offered: an analytic test and, where possible, a check against stored data
    Refused    // UnsupportedFrameError with the reason
};

[[nodiscard]] std::string_view frameFamilyName(FrameFamily family) noexcept;
[[nodiscard]] std::string_view frameTargetName(FrameTargetKind target) noexcept;
[[nodiscard]] std::string_view conventionStatusName(ConventionStatus status) noexcept;
// A short description of a family, for messages and the documentation.
[[nodiscard]] std::string_view frameFamilyDescription(FrameFamily family) noexcept;

// One row of the convention table: what happens when values of `family` stored in `stored` are asked in `target`.
struct ConventionRow
{
    FrameFamily family;
    results::StoredFrame stored; // Unknown in the rows of the families that have no frame change
    FrameTargetKind target;
    ConventionStatus status;
    std::string reason;   // a refused row: why
    std::string evidence; // a validated row: how it was checked
};

// The single convention table: one row for every (family, stored frame, target) pair, validated or refused with its
// reason. Written once, here: `frame_conventions()` in Python and the refusals of every frame change all come from it.
class FrameConventionTable
{
public:
    [[nodiscard]] static const std::vector<ConventionRow>& rows();
    // The row of a pair; a pair that is not in the table (a stored frame the family cannot have) is refused with a
    // reason made on the spot.
    [[nodiscard]] static ConventionRow find(FrameFamily family, results::StoredFrame stored, FrameTargetKind target);
    // The family of the values of a table, from its kind, element type and variant.
    [[nodiscard]] static FrameFamily familyOf(const results::ResultTableMeta& meta);
    // The stored frames a family can have a row for.
    [[nodiscard]] static std::vector<results::StoredFrame> storedFramesOf(FrameFamily family);
    // True when the family has a frame change defined at all.
    [[nodiscard]] static bool hasFrameChange(FrameFamily family) noexcept;
};

} // namespace oor::analysis
