#include "oor/model/control/case_control/CaseLineages.hpp"

namespace oor::model
{

namespace
{

std::size_t sectionsBelow(const std::vector<Subcase>& sections) noexcept
{
    std::size_t count = sections.size();
    for (const Subcase& section : sections)
        count += sectionsBelow(section.children);
    return count;
}

} // namespace

void CaseLineages::collect(const Subcase& section, std::vector<const Subcase*>& path, const Subcase* tail)
{
    path.push_back(&section);
    const auto begin = static_cast<std::uint32_t>(m_sections.size());
    m_sections.insert(m_sections.end(), path.rbegin(), path.rend());
    if (tail != nullptr)
        m_sections.push_back(tail);
    m_spans.emplace(section.key, Span{begin, static_cast<std::uint32_t>(m_sections.size()) - begin});
    for (const Subcase& child : section.children)
        collect(child, path, tail);
    path.pop_back();
}

CaseLineages::CaseLineages(const CaseControl& caseControl)
{
    const std::vector<Subcase>& segments = caseControl.segments();
    const std::size_t count = sectionsBelow(segments);
    m_spans.reserve(count);
    m_sections.reserve(2 * count); // a top-level subcase and its tail; a step adds one more
    // format reference AUXCASE: a section of an auxiliary model inherits from its AUXCASE section, the nearest one
    // before it, never from the primary sections; a section of the primary Case Control from the global section.
    const Subcase* auxcase = nullptr;
    std::vector<const Subcase*> path;
    for (const Subcase& segment : segments)
    {
        const bool isTop = segment.kind == SubcaseKind::Global || segment.kind == SubcaseKind::AuxiliaryCase;
        const Subcase* tail = isTop ? nullptr : (auxcase != nullptr ? auxcase : &caseControl.global());
        collect(segment, path, tail);
        if (segment.kind == SubcaseKind::AuxiliaryCase)
            auxcase = &segment;
    }
}

std::span<const Subcase* const> CaseLineages::of(std::uint32_t key) const noexcept
{
    const auto found = m_spans.find(key);
    if (found == m_spans.end())
        return {};
    return std::span<const Subcase* const>(m_sections.data() + found->second.begin, found->second.size);
}

const CaseEntry* CaseLineages::command(std::uint32_t key, std::string_view name, std::string_view variant) const
{
    return CaseControl::commandIn(of(key), name, variant);
}

std::vector<const CaseEntry*> CaseLineages::outputs(std::uint32_t key, std::string_view name) const
{
    return CaseControl::outputsIn(of(key), name);
}

const CaseEntry* CaseLineages::set(std::uint32_t key, std::int64_t id) const
{
    return CaseControl::setIn(of(key), id);
}

const Subcase* CaseLineages::auxiliaryCase(std::uint32_t key) const noexcept
{
    const std::span<const Subcase* const> lineage = of(key);
    if (lineage.empty() || lineage.back()->kind != SubcaseKind::AuxiliaryCase)
        return nullptr;
    return lineage.back();
}

} // namespace oor::model
