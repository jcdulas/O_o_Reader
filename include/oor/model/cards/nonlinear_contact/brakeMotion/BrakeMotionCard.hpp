#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// FORM of a MOTION: the kind of motion of a body of a brake system. The enumerators are numbered in the order of
// MOTION_FORM_WORDS.
enum class BrakeMotionForm : std::uint8_t
{
    Rotat, // ROTAT: a rotation about an axis through a center
    Trans  // TRANS: a translation along a direction
};

inline constexpr std::array<std::string_view, 2> MOTION_FORM_WORDS{"RotationMotion", "TranslationMotion"};

// One MOTION card (SOL 400): the motion direction of a body of a brake system, which a BRKSYS names (MT1_ID, MT2_ID).
//
// Card layout: MOTION  ID  (blank)  VELOCITY                                          fields 1 to 3 of the first line
//                     FORM  COSX  COSY  COSZ  X  Y  Z                                fields 9 to 15 of the second line
// ID is the number of the motion. VELOCITY is the angular velocity of a rotation or the velocity of a translation; the
// format says it is not used now (it is designed for velocity-related brake analysis), so it is a real that defaults to
// 0.0. FORM is ROTAT or TRANS (required: the two forms cannot be given together on one card). COSX, COSY and
// COSZ are the direction cosines in the basic system of the axis of rotation (ROTAT) or of the direction of the
// translation (TRANS); X, Y and Z are the coordinates in the basic system of the center of rotation, so they belong to
// ROTAT only (a translation has no center: they are refused for a TRANS unless they are zero).
// A blank real is 0.0 in the format, so a blank cosine or coordinate is stored as 0.0; the direction must not be the
// zero vector.
struct BrakeMotionCard
{
    base::EntityId id = 0;
    double velocity = 0.0;
    BrakeMotionForm form = BrakeMotionForm::Rotat;
    double cosineX = 0.0; // direction cosines
    double cosineY = 0.0;
    double cosineZ = 0.0;
    double xCoord = 0.0; // center of rotation (ROTAT only)
    double yCoord = 0.0;
    double zCoord = 0.0;

    [[nodiscard]] bool operator==(const BrakeMotionCard&) const noexcept = default;
};

} // namespace oor::model
