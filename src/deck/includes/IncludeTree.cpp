#include "oor/deck/includes/IncludeTree.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <string>
#include <system_error>

namespace oor::deck
{

IncludeTree::EnterResult IncludeTree::enter(const std::filesystem::path& path)
{
    if (m_open.size() >= m_maxDepth)
        return EnterResult::TooDeep;
    std::error_code error;
    std::filesystem::path canonical = std::filesystem::weakly_canonical(path, error);
    if (error)
        canonical = path.lexically_normal();
    if (std::find(m_open.begin(), m_open.end(), canonical) != m_open.end())
        return EnterResult::Cycle;
    m_open.push_back(canonical);
    m_names.push_back(path.filename());
    return EnterResult::Entered;
}

std::string IncludeTree::describe() const
{
    std::string text;
    for (const std::filesystem::path& name : m_names)
        text += (text.empty() ? "" : " -> ") + base::pathToUtf8(name);
    return text;
}

} // namespace oor::deck
