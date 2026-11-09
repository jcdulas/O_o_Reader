#include "oor/analysis/connections/ContactOptions.hpp"

namespace oor::analysis
{

std::string_view contactModeName(ContactMode mode) noexcept
{
    switch (mode)
    {
    case ContactMode::Merge:
        return "merge";
    case ContactMode::Connection:
        return "connection";
    case ContactMode::Ignore:
        return "ignore";
    }
    return "";
}

std::optional<ContactMode> contactModeOf(std::string_view name) noexcept
{
    for (const ContactMode mode : {ContactMode::Merge, ContactMode::Connection, ContactMode::Ignore})
    {
        if (contactModeName(mode) == name)
            return mode;
    }
    return std::nullopt;
}

std::string_view contactRuleName(ContactRule rule) noexcept
{
    return rule == ContactRule::Solver ? "solver" : "components";
}

std::optional<ContactRule> contactRuleOf(std::string_view name) noexcept
{
    for (const ContactRule rule : {ContactRule::Components, ContactRule::Solver})
    {
        if (contactRuleName(rule) == name)
            return rule;
    }
    return std::nullopt;
}

} // namespace oor::analysis
