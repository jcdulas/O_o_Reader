#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace oor::model
{

// One PSEAM card: the property of a CSEAM seam connection.
//
// Card layout:
//   PSEAM  PID  MID  TYPE  W  T  IN
//
// MID names a material (the seam is a block of solid elements of that material). TYPE is a keyword for the kind of seam
// weld that is generated: the format reference gives the default LINE and no other word, so the card keeps the word as
// written (in upper case, at most eight characters) and does not limit it to a list. W is the width of the seam
// (positive, no default). T is the thickness and may stay blank: the thickness is then the mean of the thicknesses of
// the two patches, which only the model can tell, so a blank T is nothing here (the format reference gives
// it no number). IN is the integration scheme, 0 (2x2x2 reduced shear integration with bubble functions) or 2 (the
// default: the same without the bubble functions).
struct SeamPropCard
{
    static constexpr std::string_view DEFAULT_TYPE = "Line";
    static constexpr std::int32_t DEFAULT_INTEGRATION = 2;

    base::EntityId id = 0;
    base::EntityId matId = 0;
    std::string kind = std::string(DEFAULT_TYPE);
    double width = 0.0;
    std::optional<double> thickness; // blank: the mean thickness of the two patches
    std::int32_t integration = DEFAULT_INTEGRATION;
};

} // namespace oor::model
