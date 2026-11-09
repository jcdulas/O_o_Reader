#pragma once

#include "oor/base/errors/OorError.hpp"

namespace oor::analysis
{

// A coordinate system that cannot be resolved: it does not exist, it refers to itself through a chain of definitions,
// or its three points do not define axes. The message names the cards involved (for a loop, the whole cycle).
class CoordinateSystemError : public base::OorError
{
public:
    using OorError::OorError;
};

// The geometry of an element cannot be computed: one of its grid points does not exist, or is not a geometric point.
class GeometryError : public base::OorError
{
public:
    using OorError::OorError;
};

} // namespace oor::analysis
