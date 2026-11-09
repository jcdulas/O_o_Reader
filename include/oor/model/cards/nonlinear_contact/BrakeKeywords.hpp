#pragma once

#include <array>
#include <cstdint>
#include <string_view>

// The one word that two cards of the brake squeal analysis (SOL 400) share: the BSONLY flag of a BSQUEAL and of a
// BRKSYS. One kind of value used inside one family; each card still lays out, reads, writes and checks its own fields
// by hand.
namespace oor::model
{

// BSONLY: whether the nonlinear iterations stop once the brake squeal analysis is done. The enumerators are numbered in
// the order of BRAKE_SQUEAL_ONLY_WORDS (the helpers of ExplicitSupport.hpp read and write the word that way).
enum class BrakeSquealOnly : std::uint8_t
{
    Yes, // YES (the default): perform the brake squeal analysis only and leave the nonlinear iteration at once
    No   // NO: go on with the nonlinear analysis after the brake squeal analysis
};

inline constexpr std::array<std::string_view, 2> BRAKE_SQUEAL_ONLY_WORDS{"Yes", "No"};

} // namespace oor::model
