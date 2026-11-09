#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <limits>
#include <vector>

namespace oor::analysis
{

// One grid point of a rigid element and what the element does there.
struct RigidTerm
{
    bool isIndependent = true;
    base::EntityId node = 0;
    std::int32_t components = 0; // the digits of the components, as one number; 0: none given
    double weight = std::numeric_limits<double>::quiet_NaN(); // RBE3 only
};

// The thermal expansion of a rigid element: ALPHA and TREF (NaN for RJOINT and RSPLINE, which have none). format
// reference RBE2 with the Lagrange method the element expands by ALPHA times the difference between the mean
// temperature of its grid points and TREF, when the Case Control asks for a thermal load.
struct RigidThermal
{
    double alpha = std::numeric_limits<double>::quiet_NaN();
    double tref = std::numeric_limits<double>::quiet_NaN();
};

// The grid points of the rigid elements (RBE1, RBE2, RBE3, RBAR, RBAR1, RROD, RSPLINE, RJOINT, RTRPLT, RTRPLT1) with
// their roles and components, the independent ones first, then the dependent ones, in the order of the card. format
// reference: an RBE2 has one independent grid point GN (all its components) and dependent GMi with the components CM;
// an RBE3 has a dependent reference grid point (REFC) and independent points with weights and components, and the UM
// list of its dependent degrees of freedom; an RBAR has independent components CNA and CNB and dependent ones CMA and
// CMB.
class RigidElementTerms
{
public:
    static constexpr std::int32_t ALL_COMPONENTS = 123456;

    // The terms of the rigid element at `location` (nothing for a card that is not a rigid element); `thermal`
    // (optional) gets its ALPHA and TREF.
    [[nodiscard]] static std::vector<RigidTerm> of(const model::Model& model, model::CardLocation location,
                                                   RigidThermal* thermal = nullptr);
};

} // namespace oor::analysis
