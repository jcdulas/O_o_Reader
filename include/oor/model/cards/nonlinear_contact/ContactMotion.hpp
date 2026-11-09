#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactParameters.hpp"
#include "oor/model/cards/nonlinear_contact/ContactSections.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <array>
#include <optional>

namespace oor::model
{

// The motion of a rigid contact body, as BCBODY (RIGID) and BCRIGID give it: the line of the loads and the velocity,
// the approaching velocity and the growth. One kind of value used inside one family.

// The line that follows the first line: NLOAD  ANGVEL  DCOS1  DCOS2  DCOS3  VELRB1  VELRB2  VELRB3. ANGVEL, DCOSi and
// VELRBi are a real or, as an integer, the ID of a TABLED1, TABLED2 or TABL3D (a function of time); a blank field is
// the default (0.0, and 1.0 for DCOS3 of a 2D body), kept as blank.
struct ContactMotion
{
    base::EntityId loadNode = 0;  // the grid point of the moments or rotations of a load controlled body; 0 when blank
    ContactValue angularVelocity; // the angular velocity or position about the local axis
    std::array<ContactValue, 3> axisCosines;     // the direction cosines of the local axis
    std::array<ContactValue, 3> rigidVelocities; // the translation velocity, or the final position with CONTROL -1

    [[nodiscard]] bool isDefault() const noexcept;
    [[nodiscard]] bool operator==(const ContactMotion&) const = default;
};

// The line APPROV  A  N1  N2  N3  V1  V2  V3: the approaching velocity that establishes the initial contact.
struct ContactApproach
{
    double angularVelocity = 0.0;     // the angular velocity about the local axis
    std::array<double, 3> axis{};     // the direction cosines of that axis
    std::array<double, 3> velocity{}; // the components of the approaching velocity

    [[nodiscard]] bool operator==(const ContactApproach&) const noexcept = default;
};

// The line GROW  GF1  GF2  GF3  TAB-GF1  TAB-GF2  TAB-GF3: the growth of the rigid body in the system of its CGID.
struct ContactGrowth
{
    std::array<double, 3> factors{1.0, 1.0, 1.0}; // GF1 to GF3
    std::array<base::EntityId, 3> tables{};       // TAB-GF1 to TAB-GF3: a TABLED1, TABLED2 or TABL3D; 0 when blank

    [[nodiscard]] bool operator==(const ContactGrowth&) const noexcept = default;
};

// The keywords of the lines, for the section splitter of the card.
inline constexpr std::string_view APPROV = "ApproachVelocity";
inline constexpr std::string_view GROW = "Growth";

// Reads the motion line that starts at field `lineStart` (the first field of the line is NLOAD).
[[nodiscard]] ContactMotion readContactMotion(FieldReader& reader, int lineStart);
// Writes it, if it is not the default one, on the line that starts at `lineStart`; returns the first field of the next
// free line.
int writeContactMotion(FieldWriter& writer, int lineStart, const ContactMotion& motion);
[[nodiscard]] std::optional<CardProblem> checkContactMotion(const ContactMotion& motion);

// The APPROV line, whose keyword is at the first field of `section`, and the GROW line.
[[nodiscard]] ContactApproach readContactApproach(FieldReader& reader, const ContactSection& section);
int writeContactApproach(FieldWriter& writer, int lineStart, const ContactApproach& approach);
[[nodiscard]] std::optional<CardProblem> checkContactApproach(const ContactApproach& approach);
[[nodiscard]] ContactGrowth readContactGrowth(FieldReader& reader, const ContactSection& section);
int writeContactGrowth(FieldWriter& writer, int lineStart, const ContactGrowth& growth);
[[nodiscard]] std::optional<CardProblem> checkContactGrowth(const ContactGrowth& growth);

} // namespace oor::model
