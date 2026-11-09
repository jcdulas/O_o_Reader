#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace oor::model
{

// What the stress-strain curve of a YLDVM (a TABLED1) holds, the TYPE field: engineering stress and strain (ENG), true
// stress and true strain (TRUE, the default), true stress and plastic strain (PLAST) or plastic modulus and true stress
// (PMOD). The solver works in true stress and plastic strain and converts the others.
enum class VonMisesYieldCurveType : std::uint8_t
{
    EngineeringStrain,
    TrueStrain,
    PlasticStrain,
    PlasticModulus
};
inline constexpr std::array<std::string_view, 4> VON_MISES_CURVE_TYPE_WORDS{"EngineeringStrain", "True", "Plasticity",
                                                                            "PowerModel"};

// One YLDVM card (SOL 700): the von Mises yield model with isotropic hardening. The hardening is bilinear (a yield
// stress and a hardening modulus) or piecewise linear (a stress-strain curve in a TABLED1), and the yield stress may
// grow with the strain rate, by a TABLED1 of scale factors or by the Cowper-Symonds law. A MATDEUL names the model in
// its YID field.
//
// Card layout: YLDVM  YID    YIELD  EH                                         fields 1 to 3 (fields 4 to 8 are blank)
//                    TABLE  TYPE   TABY   D      P                            fields 9 to 13
// The two ways to give the curve exclude each other: with a TABLE the yield stress comes from the curve and YIELD stays
// blank, without one YIELD is required and the curve is the bilinear one of YIELD and EH (0.0). TYPE is
// TRUE when blank and matters only with a TABLE. For the rate effect, TABY is the number of a TABLED1 of scale factors
// against the strain rate and then D and P must be blank or zero; without TABY the Cowper-Symonds law uses D
// and P (not negative; zero for both is no rate effect). The solid elements use only YIELD and the rule that
// the Young modulus be given for the PLAST and PMOD curves belongs to the material, not to the card.
struct VonMisesYieldCard
{
    base::EntityId id = 0;             // unique among the yield models
    std::optional<double> yieldStress; // yield stress; blank when TABLE is given
    double hardeningModulus = 0.0;     // hardening modulus
    base::EntityId tableId = 0;        // a TABLED1 of stress against strain, 0 for the bilinear curve
    VonMisesYieldCurveType type = VonMisesYieldCurveType::TrueStrain;
    base::EntityId rateScaleTableId = 0; // a TABLED1 of the yield stress scale factor against strain rate, 0 for none
    double rateCoefficientD = 0.0;       // Cowper-Symonds factor D, not negative
    double rateExponentP = 0.0;          // Cowper-Symonds factor P, not negative
};

} // namespace oor::model
