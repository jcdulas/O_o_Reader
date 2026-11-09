#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace oor::model
{

// The TYPE of an RBJOINT: which relative motion of the two rigid bodies the joint allows (format reference, in this
// order).
enum class RigidBodyJointType : std::uint8_t
{
    Spherical,    // SPHER: two coincident nodes stay coincident
    Revolute,     // REVOLUTE: two pairs of coincident nodes: rotation about the line through them
    Cylindrical,  // CYLIND: a revolute joint that also slides along that line
    Planar,       // PLANAR: a revolute joint that also moves normal to the line
    Universal,    // UNIVERS: two orthogonal axes about which the bodies rotate
    Translational // TRANSL: a cylindrical joint with a third pair of nodes that stops the rotation
};
inline constexpr std::array<std::string_view, 6> RIGID_BODY_JOINT_TYPE_WORDS{
    "SphericalJoint", "RevoluteJoint", "CylindricalJoint", "PlanarJoint", "UniversalJoint", "TranslationalJoint"};

// How many of N1 to N6 a joint of this type has: two for a spherical joint, four for the types with two pairs of nodes
// and six for the translational one (format reference: N3 and N4 are defined for every type but SPHER, N5 and N6 for
// TRANSL only).
[[nodiscard]] constexpr std::size_t rigidBodyJointNodeCount(RigidBodyJointType type) noexcept
{
    switch (type)
    {
    case RigidBodyJointType::Spherical:
        return 2;
    case RigidBodyJointType::Translational:
        return 6;
    case RigidBodyJointType::Revolute:
    case RigidBodyJointType::Cylindrical:
    case RigidBodyJointType::Planar:
    case RigidBodyJointType::Universal:
        break;
    }
    return 4;
}

// One RBJOINT card (SOL 700): a joint between two rigid bodies. The nodes alternate between the bodies: N1, N3 and N5
// are grid points of rigid body A and N2, N4 and N6 of rigid body B, and the type says which relative motions the pairs
// allow. The joint is enforced with a penalty whose stiffness is RPS times a stiffness the solver chooses.
//
// Card layout: RBJOINT  ID  TYPE  N1  N2  N3  N4  N5  N6      fields 1 to 8
//             (line 2)  (blank)  RPS                           field 10 (field 9 is blank)
// The RPS of the format reference drawing is in the third cell of the second line, field 10. The format does not say
// that the ID shares a space with any other card, so it is unique among the RBJOINT cards only (RJOINT, the rigid joint
// element of the rigid family, is another card). The nodes are grid points; the format does not require them to be
// distinct.
struct RigidBodyJointCard
{
    base::EntityId id = 0;
    RigidBodyJointType type = RigidBodyJointType::Spherical;
    std::vector<base::EntityId> nodes; // rigidBodyJointNodeCount(type) grid points, in the order N1, N2, ...
    double penaltyScale = 1.0;         // relative penalty stiffness, not negative
};

} // namespace oor::model
