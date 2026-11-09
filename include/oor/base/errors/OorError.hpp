#pragma once

#include <stdexcept>
#include <string>

namespace oor::base
{

// Root of every exception the library throws. Anything else (std::bad_alloc, ...) is not ours.
class OorError : public std::runtime_error
{
public:
    explicit OorError(const std::string& message) : std::runtime_error(message) {}
};

} // namespace oor::base
