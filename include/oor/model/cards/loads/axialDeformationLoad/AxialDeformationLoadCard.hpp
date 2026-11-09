#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// The enforced deformation of one element of a DEFORM card.
struct ElementDeformation
{
    base::EntityId element = 0; // EIDi: a CROD, CONROD, CTUBE, CBAR or CBEAM
    double deformation = 0.0;   // Di: a positive value is an elongation

    [[nodiscard]] bool operator==(const ElementDeformation&) const noexcept = default;
};

// One DEFORM card: enforced axial deformations of one-dimensional elements, for static analysis.
//
// Card layout: DEFORM  SID  EID1  D1  EID2  D2  EID3  D3
// One to three deformations per card; a card with two or three of them is still one card here, written
// back as it was read. The set is selected by the Case Control command DEFORM = SID.
struct AxialDeformationLoadCard
{
    static constexpr std::size_t MAX_DEFORMATIONS = 3;

    base::EntityId id = 0;
    std::vector<ElementDeformation> deformations;
};

} // namespace oor::model
