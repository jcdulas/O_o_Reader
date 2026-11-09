#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/ExplicitKeywords.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace oor::model
{

// The TYPEi of a PORFLOW: the property of the flow boundary that the VALUEi next to it gives. The words are those of
// the format reference, in its order.
enum class PorousFlowProperty : std::uint8_t
{
    Material,       // MATERIAL: the MATDEUL of the material that flows in (an integer)
    XVelocity,      // XVEL
    YVelocity,      // YVEL
    ZVelocity,      // ZVEL
    Pressure,       // PRESSURE
    Density,        // DENSITY
    SpecificEnergy, // SIE: the specific internal energy
    Flow,           // FLOW: IN, OUT or BOTH (a word)
    Method          // METHOD: how the material is transported, VELOCITY or PRESSURE (a word)
};
inline constexpr std::array<std::string_view, 9> POROUS_FLOW_PROPERTY_WORDS{
    "Material",      "VelocityX", "VelocityY", "VelocityZ", "PressureQuantity", "Density", "SpecificInternalEnergy",
    "FlowDirection", "Method"};

// The VALUEi of a TYPEi of METHOD: the method of material transport through the porous (sub)surface.
enum class PorousFlowMethod : std::uint8_t
{
    Velocity,
    Pressure
};
inline constexpr std::array<std::string_view, 2> POROUS_FLOW_METHOD_WORDS{"VelocityQuantity", "PressureQuantity"};

// A property is given once, so a card has at most one pair for each of the nine words; the names of the pairs are fixed
// here so that a message and a reference can name the pair without building a string.
inline constexpr std::size_t POROUS_FLOW_MAX_ENTRIES = POROUS_FLOW_PROPERTY_WORDS.size();
inline constexpr std::array<const char*, POROUS_FLOW_MAX_ENTRIES> POROUS_FLOW_KIND_NAMES{
    "propertyKind1", "propertyKind2", "propertyKind3", "propertyKind4", "propertyKind5",
    "propertyKind6", "propertyKind7", "propertyKind8", "propertyKind9"};
inline constexpr std::array<const char*, POROUS_FLOW_MAX_ENTRIES> POROUS_FLOW_VALUE_NAMES{
    "propertyValue1", "propertyValue2", "propertyValue3", "propertyValue4", "propertyValue5",
    "propertyValue6", "propertyValue7", "propertyValue8", "propertyValue9"};
inline constexpr std::array<const char*, POROUS_FLOW_MAX_ENTRIES> POROUS_FLOW_VALUE_REFERENCES{
    "propertyValue1", "propertyValue2", "propertyValue3", "propertyValue4", "propertyValue5",
    "propertyValue6", "propertyValue7", "propertyValue8", "propertyValue9"};

// One TYPEi and VALUEi of a PORFLOW. Only the member that the property uses means anything; the others keep their
// defaults, so that two entries with the same meaning compare the same.
struct PorousFlowBoundaryEntry
{
    PorousFlowProperty property = PorousFlowProperty::XVelocity;
    double number = 0.0;                                  // XVEL, YVEL, ZVEL, PRESSURE, DENSITY and SIE
    base::EntityId material = 0;                          // MATERIAL
    FlowDirection flow = FlowDirection::Both;             // FLOW
    PorousFlowMethod method = PorousFlowMethod::Velocity; // METHOD

    [[nodiscard]] bool operator==(const PorousFlowBoundaryEntry&) const noexcept = default;
};

// One PORFLOW card (SOL 700): the properties of a flow of material through a porous part of a coupling surface (a
// constant flow boundary): the velocity, pressure, density and energy of what flows, the material and the kind of
// boundary. A LEAKAGE of PORTYPE PORFLOW names it, and the open area is the area of the surface times the LEAKAGE
// coefficient. Properties that the card does not give are those of the Euler element that the surface cuts.
//
// Card layout: PORFLOW  FID  (blank)  TYPE1  VALUE1  TYPE2  VALUE2  TYPE3  VALUE3        fields 1 and 3 to 8
//                      TYPE4  VALUE4  -etc.-                                            fields 9 on
// The pairs go on without a gap over the continuation lines, TYPEi in field 2i+1 and VALUEi in field 2i+2; field 2 is
// an empty cell. The VALUE is read by the TYPE: an integer for MATERIAL, a real for the six physical quantities, and a
// word for FLOW (IN, OUT or BOTH, BOTH when blank) and for METHOD (VELOCITY or PRESSURE, VELOCITY when blank). The
// format marks the value required but also gives a default for the two words, and the card follows the defaults. At
// least one pair is needed, and a property comes once (the format does not say it, but a repeat would contradict
// itself). A pair that is blank in both fields is skipped. The cards are written back with the pairs in the order they
// were given and without gaps.
struct PorousFlowBoundaryCard
{
    base::EntityId id = 0; // unique among the PORFLOW cards
    std::vector<PorousFlowBoundaryEntry> entries;
};

} // namespace oor::model
