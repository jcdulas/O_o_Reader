#pragma once

#include "oor/model/control/statement/ControlStatementList.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// External file assignments stored in the executive section of a model file.
class FileManagement : public ControlStatementList
{
public:
    ControlStatement& addKeyword(std::string_view name, std::vector<StatementParameter> parameters);
};

} // namespace oor::model
