#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>

namespace oor::model
{

// One AEPARM card: a general controller of a trim, an aerodynamic extra point whose forces come from the AEDW, AEFORCE
// and AEPRESS cards that name it.
//
// Card layout: AEPARM  ID  LABEL  UNITS
// The LABEL names the controller; together with the labels of every AESURF and AESTAT it must be unique, because the
// set of labels is the set of trim variables of the model (a check across the cards, not made here). A
// TRIM, an AELINK or a UXVEC finds the controller by its label, without regard to the case. UNITS is a text that only
// labels the output and may be blank, which the card keeps as an empty string. The ID is ignored by
// the solver: two cards may have the same one. Label and units are kept as written.
struct AeroControllerCard
{
    base::EntityId id = 0;
    std::string label; // up to eight characters
    std::string units; // up to eight characters, empty when blank

    [[nodiscard]] bool operator==(const AeroControllerCard&) const noexcept = default;
};

} // namespace oor::model
