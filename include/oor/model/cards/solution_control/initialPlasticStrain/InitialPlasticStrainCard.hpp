#pragma once

#include "oor/model/cards/solution_control/InitialStateRange.hpp"

namespace oor::model
{

// Initial equivalent plastic strain for a range of elements, integration points and layers.
// It describes prior plastic deformation, typically at the start of a metal-forming analysis, and applies to
// advanced nonlinear elements. The last element defaults to the first; integration point limits default to 1 and 4.
// Layer limits are zero or blank when the model has no beam or shell layers. Strain defaults to 0.0 and accepts
// any finite real. The complete card is one model-file record, with no field-width restriction.
//
//
//
//
//
//
//
struct InitialPlasticStrainCard
{
    InitialStateRange range;
    double strain = 0.0;

    [[nodiscard]] bool operator==(const InitialPlasticStrainCard&) const noexcept = default;
};

} // namespace oor::model
