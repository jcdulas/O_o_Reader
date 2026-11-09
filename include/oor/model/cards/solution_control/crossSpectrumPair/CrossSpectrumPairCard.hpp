#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace oor::model
{

// One RCROSS card: a pair of response quantities for the cross-power spectral density and the cross-correlation
// functions of a random analysis (selected by RCROSS in Case Control; several cards may share a SID).
//
// Card layout: RCROSS  SID  RTYPE1  ID1  COMP1  RTYPE2  ID2  COMP2  CURID
// RTYPEi is the kind of response (DISP, VELO, ACCEL, OLOAD, SPCF, MPCF, STRESS, STRAIN, FORCE; a deck also has PRESS),
// IDi the element (STRESS, STRAIN, FORCE) or the grid or scalar point, COMPi the item code, CURID the optional curve
// ID. A blank RTYPE is the other one, stored resolved. The words are kept as written, in upper case.
struct CrossSpectrumPairCard
{
    base::EntityId id = 0;
    std::array<std::string, 2> responseKinds;
    std::array<base::EntityId, 2> targetIds{0, 0};
    std::array<std::int32_t, 2> components{0, 0};
    base::EntityId curveId = 0; // 0 is blank

    // True when the response i is an element response (its ID is an element).
    [[nodiscard]] bool isElementResponse(std::size_t index) const noexcept
    {
        const std::string& type = responseKinds[index];
        return type == "Stress" || type == "Strain" || type == "Force";
    }
};

} // namespace oor::model
