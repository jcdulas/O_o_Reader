#pragma once

#include "oor/model/control/case_control/CaseControl.hpp"

#include <cstdint>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace oor::model
{

// The lineage of every section of a Case Control (CaseControl::lineage of each), made in one walk of its sections.
// CaseControl finds a section by walking its sections, for every effective* question: a loop over the subcases that
// asks them is quadratic in the number of subcases (a table of 2 000 subcases asked about 25 commands of each walked
// the sections 50 000 times). Made once before such a loop, this answers each question in the depth of the section.
// It points into the Case Control: valid until the next change of its structure.
class CaseLineages
{
public:
    explicit CaseLineages(const CaseControl& caseControl);

    // The section with this key, then the ones it inherits from (CaseControl::lineage); empty for an unknown key.
    [[nodiscard]] std::span<const Subcase* const> of(std::uint32_t key) const noexcept;
    // As CaseControl::effectiveCommand, effectiveOutputs, effectiveSet and auxiliaryCaseOf.
    [[nodiscard]] const CaseEntry* command(std::uint32_t key, std::string_view name,
                                           std::string_view variant = {}) const;
    [[nodiscard]] std::vector<const CaseEntry*> outputs(std::uint32_t key, std::string_view name) const;
    [[nodiscard]] const CaseEntry* set(std::uint32_t key, std::int64_t id) const;
    [[nodiscard]] const Subcase* auxiliaryCase(std::uint32_t key) const noexcept;

private:
    // Where the lineage of a section is in m_sections.
    struct Span
    {
        std::uint32_t begin = 0;
        std::uint32_t size = 0;
    };

    // Records the lineage of `section` and of every section below it: `path` holds the sections from the top-level one
    // down to the parent of `section`, `tail` what a top-level section of its kind inherits from
    // (CaseControl::lineage).
    void collect(const Subcase& section, std::vector<const Subcase*>& path, const Subcase* tail);

    std::vector<const Subcase*> m_sections; // the lineages one after the other
    std::unordered_map<std::uint32_t, Span> m_spans;
};

} // namespace oor::model
