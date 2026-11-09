#pragma once

#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/components/ModelEntities.hpp"
#include "oor/analysis/components/ReferenceIndex.hpp"
#include "oor/analysis/connections/ContactOptions.hpp"
#include "oor/analysis/connections/ContactSearch.hpp"
#include "oor/analysis/connections/FastenerPiercings.hpp"
#include "oor/analysis/connections/WeldedNodes.hpp"
#include "oor/analysis/mass/SectionCache.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/model/storage/TableAttachment.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace oor::analysis
{

// What the component code keeps with a model: the reference index, the fastener piercings, the entities, the welded
// grid points, the contact searches, the components and the revision stamps, each made the first time something asks
// and kept while the content of the model is the one it was made for (Model::contentVersion). The contact searches are
// kept for each distance asked for and each rule, the components and the stamps for each set of contact options. It
// lives in the content slot of the model, so a copy of a model has a cache of its own, in the state of the original:
// the data is immutable and shared until the copy changes. It also keeps the library sections the model asked for
// (sections()), which depend on their dimensions only: they are kept whatever the content and shared with the copies.
class ComponentCache final : public model::TableAttachment
{
public:
    // What the cache has built since it was made, for the tests and the performance notes.
    struct Statistics
    {
        std::uint64_t referenceBuilds = 0;
        std::uint64_t piercingBuilds = 0;
        std::uint64_t entityBuilds = 0;
        std::uint64_t contactBuilds = 0;
        std::uint64_t componentBuilds = 0;
        std::uint64_t stampBuilds = 0;
    };

    ComponentCache() = default;
    ComponentCache(const ComponentCache& other);
    ComponentCache& operator=(const ComponentCache&) = delete;

    [[nodiscard]] std::unique_ptr<model::TableAttachment> clone() const override;

    // The cache of a model (made empty on first use).
    [[nodiscard]] static ComponentCache& of(const model::Model& model);
    // The cache the model has now, untouched, or null when it has none.
    [[nodiscard]] static const ComponentCache* peek(const model::Model& model) noexcept;

    [[nodiscard]] std::shared_ptr<const ReferenceIndex> references(const model::Model& model);
    [[nodiscard]] std::shared_ptr<const FastenerPiercings> fastenerPiercings(const model::Model& model,
                                                                             base::ThreadPool* pool = nullptr);
    [[nodiscard]] std::shared_ptr<const ModelEntities> entities(const model::Model& model,
                                                                base::ThreadPool* pool = nullptr);
    [[nodiscard]] std::shared_ptr<const WeldedNodes> welds(const model::Model& model, base::ThreadPool* pool = nullptr);
    [[nodiscard]] std::shared_ptr<const ContactSearch> contacts(const model::Model& model,
                                                                std::optional<double> tolerance, ContactRule rule,
                                                                base::ThreadPool* pool = nullptr);
    // The components and the stamps with the default contact options, and with others.
    [[nodiscard]] std::shared_ptr<const ComponentModel> components(const model::Model& model,
                                                                   base::ThreadPool* pool = nullptr);
    [[nodiscard]] std::shared_ptr<const ComponentModel> components(const model::Model& model,
                                                                   const ContactOptions& contact,
                                                                   base::ThreadPool* pool = nullptr);
    [[nodiscard]] std::shared_ptr<const std::vector<std::uint64_t>> stamps(const model::Model& model,
                                                                           base::ThreadPool* pool = nullptr);
    [[nodiscard]] std::shared_ptr<const std::vector<std::uint64_t>> stamps(const model::Model& model,
                                                                           const ContactOptions& contact,
                                                                           base::ThreadPool* pool = nullptr);

    [[nodiscard]] Statistics statistics() const;

    // The library sections of the model and its copies (sections_table, Container.sections()), by type, dimensions and
    // rule: valid whatever the content of the cards (a section is a function of its key), so kept across the edits of
    // the model, and the same cache for the model and the copies made of it (each section solved once for all of
    // them). Safe to share between threads.
    [[nodiscard]] SectionCache& sections();

    // A result made from the content of the model by a caller that knows its type (the table of a named query, in the
    // module above this one), kept under `key` while the content is the one it was made for: a call again with the same
    // key returns it as it is. `make` runs when there is none, without the lock of the cache (the threads of a query
    // may use the cache meanwhile). The last MEMO_ENTRIES results are kept. A result that depends on the control
    // sections too (`dependsOnControl`: the NSM set the Case Control selects) is kept while the control version of the
    // model is the one it was made for as well (Model::controlVersion).
    [[nodiscard]] std::shared_ptr<const void> memo(const model::Model& model, const std::string& key,
                                                   const std::function<std::shared_ptr<const void>()>& make,
                                                   bool dependsOnControl = false);

private:
    static constexpr std::size_t MEMO_ENTRIES = 8;

    struct MemoEntry
    {
        std::string key;
        std::uint64_t version = 0;
        std::uint64_t controlVersion = 0; // 0: the result does not depend on the control sections
        std::shared_ptr<const void> value;
    };

    // The components and the stamps of one set of contact options.
    struct ContactVariant
    {
        ContactOptions options;
        std::uint64_t componentsVersion = 0;
        std::shared_ptr<const ComponentModel> components;
        std::uint64_t stampsVersion = 0;
        std::shared_ptr<const std::vector<std::uint64_t>> stamps;
    };
    // The search of one distance and rule.
    struct ContactEntry
    {
        std::optional<double> tolerance;
        ContactRule rule = ContactRule::Components;
        std::uint64_t version = 0;
        std::shared_ptr<const ContactSearch> search;
    };

    [[nodiscard]] ContactVariant& variant(const ContactOptions& contact);

    mutable std::recursive_mutex m_mutex;
    std::uint64_t m_referencesVersion = 0;
    std::shared_ptr<const ReferenceIndex> m_references;
    std::uint64_t m_piercingsVersion = 0;
    std::shared_ptr<const FastenerPiercings> m_piercings;
    std::uint64_t m_entitiesVersion = 0;
    std::shared_ptr<const ModelEntities> m_entities;
    std::uint64_t m_weldsVersion = 0;
    std::shared_ptr<const WeldedNodes> m_welds;
    std::vector<ContactEntry> m_contacts;
    std::vector<ContactVariant> m_variants;
    std::vector<MemoEntry> m_memo;
    Statistics m_statistics;
    // Shared with the copies: made on first use, or when the cache is first copied, so that both sides share it.
    mutable std::shared_ptr<SectionCache> m_sections;
};

} // namespace oor::analysis
