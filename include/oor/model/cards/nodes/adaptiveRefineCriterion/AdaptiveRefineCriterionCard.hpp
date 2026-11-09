#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace oor::model
{

// The number of parameters of a HADACRI. The layout has F1 to F6 (fields 3 to 8) while the field list
// goes to F8, but no criterion reads more than six and there is no place for a seventh on the first line of
// the card: the card has six.
inline constexpr std::size_t ADAPTIVE_CRITERION_PARAMETER_COUNT = 6;

// One HADACRI card: a criterion that picks the elements to refine in the adaptive mesh refinement of SOL 101 and 400. A
// HADAPTL names it by CRITID.
//
// Card layout: HADACRI  CRITID  TYPE  F1  F2  F3  F4  F5  F6            fields 1 to 8
// TYPE is the kind of criterion, 1 to 4, and is 1 when blank. The parameters are reals with no default; what each one
// means depends on the TYPE (the format reference lists the others as ignored):
//   TYPE 1  error indicator: an element is refined when the square of its error indicator (computed from the grid point
//           stresses) is at least F1 times the mean square of the indicator over the elements of its set. F1 is a
//           weight from 0 to 1; F2 to F6 are ignored.
//   TYPE 2  spherical region: every element with a node inside the sphere of centre (F1, F2, F3), in basic coordinates,
//           and radius F4 is refined; F5 and F6 are ignored.
//   TYPE 3  box aligned with the basic axes: the elements with a node inside the box whose corners are (F1, F2, F3) and
//           (F4, F5, F6) are refined (all six parameters are used).
//   TYPE 4  contact: the elements with a node in deformable contact, touching or touched, are refined; F1 to F6 are
//           ignored.
// A blank parameter is kept blank: a blank real is usually zero for the solver, and a zero is a legitimate centre or
// corner, so the card does not require the parameters its TYPE uses (a check of the model may ask for them). The values
// of the ignored parameters are kept as written.
//
// The text of the format gives TYPE as strictly between 1 and 4, its drawing as 1 to 4 (a lost sign); the drawing and
// the list of four criteria decide.
//
// The CRITID is unique per card type (IdSpace::PerCardType, category AdaptivityCriterion); a HADAPTL names it.
struct AdaptiveRefineCriterionCard
{
    base::EntityId id = 0;
    std::int32_t kind = 1;                                                            // 1 to 4
    std::array<std::optional<double>, ADAPTIVE_CRITERION_PARAMETER_COUNT> parameters; // F1 to F6, blank where nullopt
};

} // namespace oor::model
