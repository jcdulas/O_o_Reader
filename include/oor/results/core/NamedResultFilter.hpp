#pragma once

#include "oor/results/core/ResultFilter.hpp"

#include <cstdint>
#include <string_view>

namespace oor::results
{

// True when the filter keeps a named result of that kind and table name, subcase and design cycle (the members of the
// filter that name rows do not concern it). The builders of the named results of the result file and HDF5 readers share it.
[[nodiscard]] bool namedKept(const ResultFilter& filter, std::string_view kind, std::string_view tableName,
                             std::int32_t subcase, std::int32_t designCycle);

} // namespace oor::results
