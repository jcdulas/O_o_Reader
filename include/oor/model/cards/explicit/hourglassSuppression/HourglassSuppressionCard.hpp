#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// The class of elements a HGSUPPR applies to (the PROP field). The property numbers of one class of elements are
// unique, so the type and the number together name the property. The enumerators are numbered in the order of
// HOURGLASS_PROPERTY_WORDS.
enum class HourglassSuppressionProperty : std::uint8_t
{
    Bar,
    Beam,
    Belt,
    Composite, // COMP: composite shell elements
    Damper,    // DAMP
    Spring,    // ELAS
    Euler,
    Rod,
    Shell,
    Solid // Lagrangian solid elements
};
inline constexpr std::array<std::string_view, 10> HOURGLASS_PROPERTY_WORDS{
    "Bar", "Beam", "Belt", "Composite", "Damper", "Elastic", "Euler", "Rod", "Shell", "Solid"};

// The hourglass suppression method (the HGTYPE field): Flanagan-Belytschko viscous (shells), Flanagan-Belytschko
// stiffness (solids) or the standard viscous damping. The enumerators are numbered in the order of
// HOURGLASS_METHOD_WORDS.
enum class HourglassMethod : std::uint8_t
{
    FlanaganBelytschkoViscous,   // FBV
    FlanaganBelytschkoStiffness, // FBS
    StandardViscous              // the standard viscous damping, the word of the external explicit program
};
inline constexpr std::array<std::string_view, 3> HOURGLASS_METHOD_WORDS{
    "FlanaganBelytschkoViscous", "FlanaganBelytschkoStiffness", "ExternalExplicitProgram"};

// The RBRCOR field: whether the hourglass forces are corrected for a large rigid body rotation. The enumerators are
// numbered in the order of ROTATION_CORRECTION_WORDS.
enum class RotationCorrection : std::uint8_t
{
    Off,
    On
};
inline constexpr std::array<std::string_view, 2> ROTATION_CORRECTION_WORDS{"No", "Yes"};

// The method a blank HGTYPE means for a class of elements: FBV for the shells (SHELL and COMP) and FBS for the solids.
// The other classes have no default, because the hourglass modes do not concern them.
[[nodiscard]] constexpr std::optional<HourglassMethod> defaultHourglassMethod(
    HourglassSuppressionProperty property) noexcept
{
    switch (property)
    {
    case HourglassSuppressionProperty::Shell:
    case HourglassSuppressionProperty::Composite:
        return HourglassMethod::FlanaganBelytschkoViscous;
    case HourglassSuppressionProperty::Solid:
        return HourglassMethod::FlanaganBelytschkoStiffness;
    case HourglassSuppressionProperty::Bar:
    case HourglassSuppressionProperty::Beam:
    case HourglassSuppressionProperty::Belt:
    case HourglassSuppressionProperty::Damper:
    case HourglassSuppressionProperty::Spring:
    case HourglassSuppressionProperty::Euler:
    case HourglassSuppressionProperty::Rod:
        break;
    }
    return std::nullopt;
}

// One HGSUPPR card (SOL 700): the hourglass suppression method and the damping coefficients of the elements of one
// property. No other entry of the format reference has this name.
//
// Card layout: HGSUPPR  HID  PROP  PID  HGTYPE  HGCMEM  HGCWRP  HGCTWS  HGCSOL     fields 1 to 8
//             +        RBRCOR  VALUE                                              fields 9 and 10
// PROP and PID are required. HGTYPE has a default that depends on PROP, so the card stores it resolved: a
// blank HGTYPE of a SHELL, COMP or SOLID becomes FBV or FBS when the card is read or built, and is always written. It
// stays blank for the classes that have no default (the solver ignores the card for them). The field table gives FBS
// to shells and solids and the standard viscous damping to solids only, where the format gives the solids that one
// and FBS and the shells that one and FBV: the format disagrees with itself, and the card checks no combination of PROP
// and HGTYPE. The four coefficients are 0.1 when blank and lie between 0 and 0.15 (HGCMEM, HGCWRP and HGCTWS are for
// shells, HGCSOL for solids). RBRCOR is NO when blank. VALUE is a number of steps: blank (0 here) means that the
// correction is applied at every step when RBRCOR is YES; the card keeps a value with RBRCOR = NO, which the solver
// ignores.
struct HourglassSuppressionCard
{
    static constexpr double DEFAULT_COEFFICIENT = 0.1;
    static constexpr double MAX_COEFFICIENT = 0.15;

    base::EntityId id = 0; // unique among the HGSUPPR cards
    HourglassSuppressionProperty propKind = HourglassSuppressionProperty::Shell;
    base::EntityId propId = 0; // a property of the class PROP
    std::optional<HourglassMethod> method;
    double membraneCoefficient = DEFAULT_COEFFICIENT;
    double warpCoefficient = DEFAULT_COEFFICIENT;
    double twistCoefficient = DEFAULT_COEFFICIENT;
    double solidCoefficient = DEFAULT_COEFFICIENT;
    RotationCorrection rotationCorrection = RotationCorrection::Off;
    std::int32_t stepCount = 0; // number of steps, 0 when blank
};

} // namespace oor::model
