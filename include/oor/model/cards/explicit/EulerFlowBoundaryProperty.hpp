#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

// The two kinds of value that the flow cards of the explicit solver (FLOW, FLOWDEF, FLOWC and FLOWT) share: a property
// of a flow boundary, which is a TYPEi field and the VALUEi field that goes with it, and the type of boundary (IN, OUT
// or BOTH). Where the pairs are on a card, and every other field of the card, belong to the card.
namespace oor::model
{

// The properties a flow boundary of FLOW or FLOWDEF can define. The enumerators are numbered in the order of
// FLOW_PROPERTY_WORDS. The value that goes with a type is an ID for MATERIAL and HYDSTAT, a word for FLOW and a real
// for the others.
enum class FlowPropertyType : std::uint8_t
{
    Material,       // MATERIAL: the MATDEUL of the material that flows in
    XVelocity,      // XVEL
    YVelocity,      // YVEL
    ZVelocity,      // ZVEL
    Pressure,       // PRESSURE
    Density,        // DENSITY at inflow
    SpecificEnergy, // SIE: the specific internal energy at inflow
    Flow,           // FLOW: which way the material may cross the boundary
    Hydrostatic     // HYDSTAT: the hydrostatic pressure profile of a HYDSTAT card
};
inline constexpr std::array<std::string_view, 9> FLOW_PROPERTY_WORDS{
    "Material", "VelocityX",  "VelocityY", "VelocityZ", "Pressure", "Density", "SpecificInternalEnergy",
    "Flow",     "Hydrostatic"};

// The value of a FLOW property: the way material may cross the boundary. BOTH is the default of a blank value; SYM (a
// symmetry boundary) is listed by FLOW and not by FLOWDEF.
enum class FlowMode : std::uint8_t
{
    Both,
    In,
    Out,
    Symmetry
};
inline constexpr std::array<std::string_view, 4> FLOW_MODE_WORDS{"Both", "In", "Out", "Symmetry"};

// The TYPE of a FLOWC or FLOWT: inflow only, outflow only, or either.
enum class FlowBoundaryType : std::uint8_t
{
    In,
    Out,
    Both
};
inline constexpr std::array<std::string_view, 3> FLOW_BOUNDARY_TYPE_WORDS{"In", "Out", "Both"};

// One TYPEi and VALUEi. Only the member that the type uses has a meaning: `id` for MATERIAL and HYDSTAT, `mode` for
// FLOW and `number` for the other types.
struct EulerFlowBoundaryProperty
{
    FlowPropertyType type = FlowPropertyType::XVelocity;
    double number = 0.0;
    base::EntityId id = 0;
    FlowMode mode = FlowMode::Both;

    [[nodiscard]] bool operator==(const EulerFlowBoundaryProperty&) const noexcept = default;
};

// What a card allows in its properties.
struct EulerFlowBoundaryPropertyRules
{
    bool allowSymmetry = true; // FLOW takes the value SYM; FLOWDEF does not
};

// Reads the pair whose TYPEi is in `typeField` (its VALUEi is the next field). `index` counts the pairs of the card
// from 0 and names the fields in the messages (TYPE1, VALUE1, ...). A pair whose fields are both blank gives nothing.
[[nodiscard]] std::optional<EulerFlowBoundaryProperty> readFlowProperty(FieldReader& reader, int typeField,
                                                                        std::size_t index,
                                                                        const EulerFlowBoundaryPropertyRules& rules);

// Writes the pair from `typeField` on. A FLOW property that allows both ways leaves its value blank when the writer
// omits defaults.
void writeFlowProperty(FieldWriter& writer, int typeField, const EulerFlowBoundaryProperty& property);

// The problem with the pair number `index`, if there is one: a number that is not finite, an ID that is not positive,
// or a value the rules refuse.
[[nodiscard]] std::optional<CardProblem> checkFlowProperty(const EulerFlowBoundaryProperty& property, std::size_t index,
                                                           const EulerFlowBoundaryPropertyRules& rules);

// Visits the ID of the pair, if its type has one: a MATDEUL for MATERIAL, a HYDSTAT for HYDSTAT.
void visitFlowProperty(ReferenceVisitor& visitor, EulerFlowBoundaryProperty& property, std::size_t index);

} // namespace oor::model
