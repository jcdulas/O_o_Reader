#pragma once

#include "oor/model/parameters/ParameterCatalogue.hpp"

#include <vector>

namespace oor::model
{

// The typed table of the format reference parameters (ParameterCatalogueData.cpp). Read it through ParameterCatalogue.
[[nodiscard]] const std::vector<ParamInfo>& parameterCatalogueEntries();

} // namespace oor::model
