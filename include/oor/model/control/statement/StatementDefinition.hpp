#pragma once

#include "oor/model/control/statement/ControlStatement.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// The two sections whose statements are typed by this code.
enum class SectionKind : std::uint8_t
{
    FileManagement,
    Executive
};

// The exact neutral name, shape and parameter names of a supported executive record.
struct StatementDefinition
{
    std::string name;
    SectionKind section = SectionKind::Executive;
    ControlKind shape = ControlKind::Keyword;
    std::vector<std::string> keywords;

    StatementDefinition& describers(std::vector<std::string> list);
};

// Exact name lookup for the supported statements of one section.
class StatementCatalogue
{
public:
    explicit StatementCatalogue(std::vector<StatementDefinition> definitions);

    // The statement with exactly this neutral name.
    [[nodiscard]] const StatementDefinition* findByName(std::string_view name) const noexcept;
    [[nodiscard]] const std::vector<StatementDefinition>& definitions() const noexcept { return m_definitions; }

private:
    std::vector<StatementDefinition> m_definitions;
};

[[nodiscard]] const StatementCatalogue& executiveStatements();
[[nodiscard]] const StatementCatalogue& fileManagementStatements();

} // namespace oor::model
