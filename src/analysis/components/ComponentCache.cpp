#include "oor/analysis/components/ComponentCache.hpp"

#include "oor/analysis/components/RevisionStamp.hpp"

#include <algorithm>

namespace oor::analysis
{

namespace
{

// Making the cache of a model is the one step that two threads reading the same model could do at once.
std::mutex& creationMutex()
{
    static std::mutex mutex;
    return mutex;
}

// The most contact options and distances kept at once: a caller that tries many distances does not make the cache grow
// without end (the oldest goes first).
constexpr std::size_t MAX_CONTACT_ENTRIES = 8;

// True when one of the elements `declared` (sorted IDs) is an element that welds grid points.
bool declaresWeld(const ModelEntities& entities, const WeldedNodes& welds, const std::vector<base::EntityId>& declared)
{
    if (welds.empty())
        return false;
    for (const ModelEntity& entity : entities.entities())
    {
        if (!entity.isEquation && entity.role == ElementRole::Rigid &&
            std::binary_search(declared.begin(), declared.end(), entity.id) && welds.isWeld(entity.card))
            return true;
    }
    return false;
}

} // namespace

ComponentCache::ComponentCache(const ComponentCache& other) : TableAttachment(other)
{
    const std::scoped_lock lock(other.m_mutex);
    m_referencesVersion = other.m_referencesVersion;
    m_references = other.m_references;
    m_piercingsVersion = other.m_piercingsVersion;
    m_piercings = other.m_piercings;
    m_entitiesVersion = other.m_entitiesVersion;
    m_entities = other.m_entities;
    m_weldsVersion = other.m_weldsVersion;
    m_welds = other.m_welds;
    m_contacts = other.m_contacts;
    m_variants = other.m_variants;
    m_memo = other.m_memo;
    m_statistics = other.m_statistics;
    if (!other.m_sections)
        other.m_sections = std::make_shared<SectionCache>();
    m_sections = other.m_sections;
}

SectionCache& ComponentCache::sections()
{
    const std::scoped_lock lock(m_mutex);
    if (!m_sections)
        m_sections = std::make_shared<SectionCache>();
    return *m_sections;
}

std::shared_ptr<const void> ComponentCache::memo(const model::Model& model, const std::string& key,
                                                 const std::function<std::shared_ptr<const void>()>& make,
                                                 bool dependsOnControl)
{
    const std::uint64_t version = model.contentVersion();
    const std::uint64_t control = dependsOnControl ? model.controlVersion() : 0;
    {
        const std::scoped_lock lock(m_mutex);
        for (const MemoEntry& entry : m_memo)
        {
            if (entry.key == key && entry.version == version && entry.controlVersion == control)
                return entry.value;
        }
    }
    std::shared_ptr<const void> value = make();
    const std::scoped_lock lock(m_mutex);
    // what was made for another content goes, and so does an older result of the same key
    std::erase_if(m_memo, [&](const MemoEntry& entry) { return entry.version != version || entry.key == key; });
    if (m_memo.size() >= MEMO_ENTRIES)
        m_memo.erase(m_memo.begin());
    m_memo.push_back(MemoEntry{key, version, control, value});
    return value;
}

std::unique_ptr<model::TableAttachment> ComponentCache::clone() const
{
    return std::make_unique<ComponentCache>(*this);
}

ComponentCache& ComponentCache::of(const model::Model& model)
{
    const std::scoped_lock lock(creationMutex());
    model::AttachmentSlot& slot = model.contentCacheSlot();
    auto* cache = dynamic_cast<ComponentCache*>(slot.get());
    if (cache == nullptr)
    {
        slot.reset(std::make_unique<ComponentCache>());
        cache = static_cast<ComponentCache*>(slot.get());
    }
    return *cache;
}

const ComponentCache* ComponentCache::peek(const model::Model& model) noexcept
{
    return dynamic_cast<const ComponentCache*>(model.contentCacheSlot().get());
}

std::shared_ptr<const ReferenceIndex> ComponentCache::references(const model::Model& model)
{
    const std::scoped_lock lock(m_mutex);
    if (!m_references || m_referencesVersion != model.contentVersion())
    {
        m_references = std::make_shared<const ReferenceIndex>(ReferenceIndex::build(model));
        m_referencesVersion = model.contentVersion();
        ++m_statistics.referenceBuilds;
    }
    return m_references;
}

std::shared_ptr<const FastenerPiercings> ComponentCache::fastenerPiercings(const model::Model& model,
                                                                           base::ThreadPool* pool)
{
    const std::scoped_lock lock(m_mutex);
    if (!m_piercings || m_piercingsVersion != model.contentVersion())
    {
        m_piercings = std::make_shared<const FastenerPiercings>(FastenerPiercings::compute(model, pool));
        m_piercingsVersion = model.contentVersion();
        ++m_statistics.piercingBuilds;
    }
    return m_piercings;
}

std::shared_ptr<const ModelEntities> ComponentCache::entities(const model::Model& model, base::ThreadPool* pool)
{
    const std::scoped_lock lock(m_mutex);
    if (!m_entities || m_entitiesVersion != model.contentVersion())
    {
        const std::shared_ptr<const FastenerPiercings> piercings = fastenerPiercings(model, pool);
        m_entities = std::make_shared<const ModelEntities>(ModelEntities::build(model, piercings.get(), pool));
        m_entitiesVersion = model.contentVersion();
        ++m_statistics.entityBuilds;
    }
    return m_entities;
}

std::shared_ptr<const WeldedNodes> ComponentCache::welds(const model::Model& model, base::ThreadPool* pool)
{
    const std::scoped_lock lock(m_mutex);
    if (!m_welds || m_weldsVersion != model.contentVersion())
    {
        const std::shared_ptr<const ModelEntities> found = entities(model, pool);
        m_welds = std::make_shared<const WeldedNodes>(WeldedNodes::build(model, *found));
        m_weldsVersion = model.contentVersion();
    }
    return m_welds;
}

std::shared_ptr<const ContactSearch> ComponentCache::contacts(const model::Model& model,
                                                              std::optional<double> tolerance, ContactRule rule,
                                                              base::ThreadPool* pool)
{
    const std::scoped_lock lock(m_mutex);
    auto entry = std::find_if(m_contacts.begin(), m_contacts.end(), [&](const ContactEntry& item)
                              { return item.tolerance == tolerance && item.rule == rule; });
    if (entry == m_contacts.end())
    {
        if (m_contacts.size() >= MAX_CONTACT_ENTRIES)
            m_contacts.erase(m_contacts.begin());
        m_contacts.push_back(ContactEntry{tolerance, rule, 0, nullptr});
        entry = m_contacts.end() - 1;
    }
    if (!entry->search || entry->version != model.contentVersion())
    {
        entry->search = ContactSearch::compute(model, tolerance, rule, pool);
        entry->version = model.contentVersion();
        ++m_statistics.contactBuilds;
    }
    return entry->search;
}

ComponentCache::ContactVariant& ComponentCache::variant(const ContactOptions& contact)
{
    auto found = std::find_if(m_variants.begin(), m_variants.end(),
                              [&](const ContactVariant& item) { return item.options == contact; });
    if (found != m_variants.end())
        return *found;
    if (m_variants.size() >= MAX_CONTACT_ENTRIES)
        m_variants.erase(m_variants.begin());
    m_variants.push_back(ContactVariant{contact, 0, nullptr, 0, nullptr});
    return m_variants.back();
}

std::shared_ptr<const ComponentModel> ComponentCache::components(const model::Model& model, base::ThreadPool* pool)
{
    return components(model, ContactOptions{}, pool);
}

std::shared_ptr<const ComponentModel> ComponentCache::components(const model::Model& model,
                                                                 const ContactOptions& contact, base::ThreadPool* pool)
{
    const std::scoped_lock lock(m_mutex);
    ContactVariant& entry = variant(contact);
    if (!entry.components || entry.componentsVersion != model.contentVersion())
    {
        const std::shared_ptr<const ModelEntities> found = entities(model, pool);
        std::shared_ptr<const ContactSearch> search;
        if (contact.mode != ContactMode::Ignore)
            search = contacts(model, contact.tolerance, contact.rule, pool);
        std::shared_ptr<const WeldedNodes> welded = welds(model, pool);
        // A rigid element the caller declares a connection welds nothing: the welds are made again without it (the
        // declared elements are seldom welds; otherwise the welds of the model are those of every call).
        if (!contact.connections.empty() && declaresWeld(*found, *welded, contact.connections))
            welded = std::make_shared<const WeldedNodes>(WeldedNodes::build(model, *found, contact.connections));
        entry.components = std::make_shared<const ComponentModel>(
            ComponentModel::build(model, *found, search, contact.mode, welded.get(), contact.connections));
        entry.componentsVersion = model.contentVersion();
        ++m_statistics.componentBuilds;
    }
    return entry.components;
}

std::shared_ptr<const std::vector<std::uint64_t>> ComponentCache::stamps(const model::Model& model,
                                                                         base::ThreadPool* pool)
{
    return stamps(model, ContactOptions{}, pool);
}

std::shared_ptr<const std::vector<std::uint64_t>> ComponentCache::stamps(const model::Model& model,
                                                                         const ContactOptions& contact,
                                                                         base::ThreadPool* pool)
{
    const std::scoped_lock lock(m_mutex);
    const std::shared_ptr<const ComponentModel> found = components(model, contact, pool);
    ContactVariant& entry = variant(contact); // after components(): the vector of variants does not move from here
    if (!entry.stamps || entry.stampsVersion != model.contentVersion())
    {
        entry.stamps = std::make_shared<const std::vector<std::uint64_t>>(RevisionStamp::compute(model, *found, pool));
        entry.stampsVersion = model.contentVersion();
        ++m_statistics.stampBuilds;
    }
    return entry.stamps;
}

ComponentCache::Statistics ComponentCache::statistics() const
{
    const std::scoped_lock lock(m_mutex);
    return m_statistics;
}

} // namespace oor::analysis
