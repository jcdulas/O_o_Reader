#include "oor/query/results/ResultModelIndex.hpp"

namespace oor::query
{

ResultModelIndex::PerModel& ResultModelIndex::of(std::size_t set)
{
    const model::Model* model = m_context.modelOf(set);
    PerModel& entry = m_models[model];
    return entry;
}

std::optional<model::CardLocation> ResultModelIndex::element(std::size_t set, std::int32_t resultEid) const
{
    const model::Model* model = m_context.modelOf(set);
    const std::shared_ptr<results::ResultAttachment>& attachment = m_context.sets()[set].attachment;
    if (model == nullptr || !attachment || resultEid <= 0)
        return std::nullopt;
    return attachment->elementLocation(*model, resultEid);
}

std::int32_t ResultModelIndex::property(std::size_t set, std::int32_t resultEid)
{
    return m_context.elementCard(set, resultEid).pid;
}

std::optional<model::CardLocation> ResultModelIndex::propertyLocation(std::size_t set, std::int32_t resultEid)
{
    const model::Model* model = m_context.modelOf(set);
    const std::int32_t pid = property(set, resultEid);
    if (model == nullptr || pid <= 0)
        return std::nullopt;
    return model->locate(base::IdSpace::Property, pid);
}

std::int32_t ResultModelIndex::material(std::size_t set, std::int32_t resultEid)
{
    const std::uint64_t key = (static_cast<std::uint64_t>(set) << 32U) | static_cast<std::uint32_t>(resultEid);
    if (const auto found = m_materials.find(key); found != m_materials.end())
        return found->second;
    std::int32_t material = 0;
    const model::Model* model = m_context.modelOf(set);
    if (model != nullptr)
    {
        PerModel& entry = of(set);
        if (!entry.facts)
            entry.facts = std::make_unique<CardFacts>(*model);
        if (const auto location = propertyLocation(set, resultEid))
            material = entry.facts->firstReference(*location, model::ReferenceTarget::Material);
        if (material == 0)
            if (const auto location = element(set, resultEid))
                material = entry.facts->firstReference(*location, model::ReferenceTarget::Material);
    }
    m_materials.emplace(key, material);
    return material;
}

std::int32_t ResultModelIndex::component(std::size_t set, std::int32_t resultEid)
{
    const model::Model* model = m_context.modelOf(set);
    if (model == nullptr)
        return 0;
    PerModel& entry = of(set);
    if (!entry.components)
        entry.components = analysis::ComponentModel::of(*model, m_context.exec().pool());
    if (const auto location = element(set, resultEid))
        return entry.components->componentOfElement(*location);
    return 0;
}

} // namespace oor::query
