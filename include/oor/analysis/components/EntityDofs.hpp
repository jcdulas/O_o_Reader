#pragma once

#include "oor/analysis/components/ModelEntities.hpp"
#include "oor/analysis/connections/FastenerPiercings.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <string>

namespace oor::analysis
{

// The degrees of freedom an entity acts on at one of its grid points, as a set of components: bit k for component k
// (bit 0 for a scalar point, bits 1 to 6 for the components of a grid point).
using ComponentSet = std::uint8_t;

// The components through which an element or an MPC equation acts on a grid point (for the zones):
//   solids 123; shells 123456, the axisymmetric and plane elements (CQUADX, CTRIAX, CPLSTN, CPLSTS families) 123;
//   rods (CROD, CONROD) 123456 with a torsion constant J and 123 without one (J blank or 0: no torque), tubes, bars,
//   beams and bends 123456; CONM1 and CONM2 123456;
//   the scalar elements of grid points (CELAS1, CELAS2, CDAMP1, CDAMP2, CMASS1, CMASS2) the component of the card at
//   the point, the scalar elements of scalar points 0; a CBUSH the components its PBUSH gives a stiffness K for (all
//   six when the PBUSH is missing); CGAP and CBUSH1D 123; a CFAST or a CWELD 123 at the grid points of the elements
//   it pierces (its piercing points follow the translations of those grid points through the shape functions) and
//   123456 at its own grid points; any other connector 123456;
//   a rigid element the components its card names at the point (an RROD end without any 123);
//   an MPC equation the components of its terms at the point.
class EntityDofs
{
public:
    [[nodiscard]] static ComponentSet at(const model::Model& model, const ModelEntity& entity, base::EntityId node,
                                         const FastenerPiercings* piercings);
    // The components as text, in increasing order ("123456"; "0" for a scalar point).
    [[nodiscard]] static std::string text(ComponentSet components);
    // The set of the digits of a number written as the components of a deck field (123 -> components 1, 2, 3; 0 ->
    // scalar).
    [[nodiscard]] static ComponentSet fromDigits(std::int32_t digits) noexcept;
};

} // namespace oor::analysis
