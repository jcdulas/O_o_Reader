#include "oor/query/operators/ExecutionContext.hpp"

namespace oor::query
{

void ExecutionContext::parallel(std::size_t itemCount, std::size_t chunkSize,
                                const std::function<void(base::IndexRange)>& body) const
{
    const std::vector<base::IndexRange> chunks = base::splitIntoChunks(itemCount, chunkSize);
    if (chunks.empty())
        return;
    const auto run = [&](std::size_t index)
    {
        body(chunks[index]);
        advance(chunks[index].end - chunks[index].begin);
    };
    if (m_pool == nullptr || m_pool->workerCount() == 0 || chunks.size() == 1)
    {
        for (std::size_t index = 0; index < chunks.size(); ++index)
        {
            checkCancelled();
            run(index);
        }
        return;
    }
    base::parallelFor(*m_pool, chunks.size(), run, m_token);
}

void ExecutionContext::beginStage(const std::string& stage, std::size_t total) const
{
    if (m_progress != nullptr)
        m_progress->beginStage(stage, total);
}

} // namespace oor::query
