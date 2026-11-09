#pragma once

#include <string>

namespace oor::base
{

// Numbers written into messages and keys (a diagnostic, a difference of two models, a test deck), in the two forms
// `std::format` gives for "{}" and "{:g}". The library is built by GCC 12 too, which has no <format>
// (tools/check_portability.py refuses it), so these forms are made with std::to_chars.
struct MessageNumber
{
    // The shortest text that reads back as the same double: "0.5", "1e-07", "250000", "-3.25".
    [[nodiscard]] static std::string shortest(double value);

    // Six significant digits, the trailing zeros removed, an exponent when it is shorter (printf "%g"): "0.333333",
    // "1e+06", "2.5".
    [[nodiscard]] static std::string general(double value);
};

} // namespace oor::base
