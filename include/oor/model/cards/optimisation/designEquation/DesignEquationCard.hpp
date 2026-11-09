#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>

namespace oor::model
{

// A numbered set of equations. The expression grammar permits semicolons between equations;
// the whole text is one quoted model-file value when it contains a separator.
struct DesignEquationCard
{
    base::EntityId id = 0;
    std::string text;
};

} // namespace oor::model
