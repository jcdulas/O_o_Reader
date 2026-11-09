#pragma once

#include "oor/base/errors/OorError.hpp"

namespace oor::model
{

// A design point that cannot be applied to the model: a design variable that does not exist, a relation that names a
// card or a field the library cannot write, an equation that gives no number, a value outside the limits of a relation
// when the caller asked for that to be an error. The message names the relation or the variable; nothing was written to
// the model.
class DesignError : public base::OorError
{
public:
    using OorError::OorError;
};

} // namespace oor::model
