#pragma once

#include "oor/base/errors/OorError.hpp"

namespace oor::analysis
{

// The mass of the model cannot be computed: a property refers to a material that does not exist or has no density, a
// ply has no plane stress law, a set of non-structural mass is not defined. The message names the card and the ID.
class MassError : public base::OorError
{
public:
    using OorError::OorError;
};

} // namespace oor::analysis
