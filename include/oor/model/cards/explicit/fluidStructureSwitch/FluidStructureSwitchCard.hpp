#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace oor::model
{

// When the Euler elements of a DYFSISW are initialized (the INITV field): at cycle 0, or only once the elements become
// active. The enumerators are numbered in the order of EULER_INITIALIZATION_WORDS.
enum class EulerInitialization : std::uint8_t
{
    AtCycleZero,
    WhenActive
};
inline constexpr std::array<std::string_view, 2> EULER_INITIALIZATION_WORDS{"No", "Yes"};

// One DYFSISW card (SOL 700): the switch that turns the fluid-structure interaction and the Euler solver on and off in
// time. Without the card the coupling, the airbags and the Euler solver are active at all times. No other entry of the
// format reference has this name.
//
// Card layout: DYFSISW  SWID  TID  INITV            fields 1 to 3
// TID is the TABLED1 of the switch: time is its x value, and the solvers are on where its y value is positive and off
// where it is negative. The format does not call the field required, so a blank TID is kept (0). INITV is NO when
// blank: the Euler elements are initialized at cycle 0; YES postpones the initialization to the moment they become
// active (a prestress phase moves the structure before the fluid acts).
struct FluidStructureSwitchCard
{
    base::EntityId id = 0;      // unique among the DYFSISW cards
    base::EntityId tableId = 0; // a TABLED1, 0 when blank
    EulerInitialization eulerInitialization = EulerInitialization::AtCycleZero;
};

} // namespace oor::model
