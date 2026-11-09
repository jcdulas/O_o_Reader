#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstddef>
#include <cstdint>
#include <string>

namespace oor::model
{

// Conventional excitation-label sample length; the model format imposes no maximum label length.
inline constexpr std::size_t FRF_EXCITATION_LABEL_LENGTH = 48;

// A degree of freedom where a unit load generates a frequency-response function.
// pointId identifies a node or scalar point. components is 1 to 6 for a node, 0 or blank for a scalar point.
// entryLabel is a required literal text value, preserved whole regardless of its length.
// The card has no identifier of its own. Repeated unit loads are retained; the solver may ignore duplicates.
//
//
//
//
//
//
//
struct FrfExcitationDofCard
{
    base::EntityId pointId = 0;
    std::int32_t components = 0;
    std::string entryLabel;

    [[nodiscard]] bool operator==(const FrfExcitationDofCard&) const noexcept = default;
};

} // namespace oor::model
