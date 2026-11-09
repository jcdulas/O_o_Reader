#pragma once

#include "oor/analysis/validation/engine/ValidationCheck.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace oor::analysis
{

// A family of checks: the checks of one subject, run together on one thread, in the order the families are listed.
struct CheckFamily
{
    std::string_view name;   // "references"
    std::string_view prefix; // "ref": what the codes of the family start with
    std::string_view title;  // "References and duplicate IDs"
};

// The checks of the library, in the order they run: the families in the order of CheckRegistry::families(), and inside
// a family in the order they were added. Built once, by hand, from one `add...Check` call per check (one file each); a
// check that is not in this list does not exist. Read-only after it is built.
class CheckRegistry
{
public:
    // The registry of all the checks, built on first use.
    [[nodiscard]] static const CheckRegistry& instance();

    [[nodiscard]] static const std::vector<CheckFamily>& families();
    [[nodiscard]] const std::vector<std::unique_ptr<ValidationCheck>>& checks() const noexcept { return m_checks; }
    [[nodiscard]] const ValidationCheck* find(std::string_view code) const noexcept;
    // The position of the family in families(), or families().size() when it is not one.
    [[nodiscard]] static std::size_t familyIndex(std::string_view family) noexcept;

    // Does a name of a `select` or `ignore` list stand for this check: its code, its family, or the prefix of its code.
    [[nodiscard]] static bool matches(std::string_view name, const CheckInfo& info) noexcept;
    // True when a name is one a check can match ("all" included): the lists are refused otherwise, so that a typo does
    // not silently select nothing.
    [[nodiscard]] bool isKnownName(std::string_view name) const noexcept;

    // Used by the registration functions of the families, one call per check.
    void add(std::unique_ptr<ValidationCheck> check);

private:
    CheckRegistry() = default;
    friend struct CheckRegistryBuilder;

    std::vector<std::unique_ptr<ValidationCheck>> m_checks;
};

} // namespace oor::analysis
