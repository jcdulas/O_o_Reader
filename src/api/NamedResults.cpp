#include "oor/api/Results.hpp"
#include "oor/base/errors/Errors.hpp"

namespace oor::api
{
std::vector<results::NamedResultMeta> Results::named() const
{
    std::vector<results::NamedResultMeta> metas;
    for (const auto& result : m_tables->namedResults())
        metas.push_back(result->meta());
    return metas;
}

std::shared_ptr<const results::NamedResultData> Results::readNamed(std::size_t index) const
{
    if (index >= m_tables->namedCount())
        throw base::UsageError("named result index is out of range");
    return m_tables->namedResults()[index]->data();
}

void Results::addNamed(const std::string& kind, const results::NamedResultOrigin& origin,
                       const std::vector<results::NamedArrayInput>& arrays,
                       const std::vector<results::NamedTextInput>& texts)
{
    auto result = results::makeNamedResult(kind, origin, arrays, texts);
    m_tables->addNamed(std::move(result));
    m_metadata.namedCount = static_cast<std::uint32_t>(m_tables->namedCount());
}
} // namespace oor::api
