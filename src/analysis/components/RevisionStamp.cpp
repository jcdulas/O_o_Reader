#include "oor/analysis/components/RevisionStamp.hpp"

#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/analysis/components/ElementRoles.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <algorithm>
#include <cstring>

namespace oor::analysis
{

namespace
{

constexpr std::uint64_t FNV_OFFSET = 14695981039346656037ULL;
constexpr std::uint64_t FNV_PRIME = 1099511628211ULL;

// The non-structural mass card types: NonStructMass, NonStructMass1, NonStructMassLumped, NonStructMassLumped1 and
// NonStructMassUnion.
bool isNonStructMassCard(std::string_view name) noexcept
{
    return name.starts_with("NonStructMass");
}

bool isCompanionCategory(model::CardCategory category) noexcept
{
    return category == model::CardCategory::Material || category == model::CardCategory::ThermalMaterial ||
           category == model::CardCategory::Property;
}

// The cards that complete another card, or add mass to it, without being referred to by anything: MATT1 and MATS1 (the
// same ID as their MAT1), PBUSHT and PELAST (the same ID as their property), the NSM cards. Found once per model.
std::vector<model::CardLocation> companionCandidates(const model::Model& model, const ReferenceIndex& references)
{
    std::vector<model::CardLocation> candidates;
    const model::CardRegistry& registry = model::CardRegistry::instance();
    for (std::uint16_t type = 1; type < registry.size(); ++type)
    {
        const model::CardTable* table = model.findTable(type);
        if (table == nullptr)
            continue;
        const model::CardTypeInfo& info = registry.entry(type).info;
        const bool nsm = isNonStructMassCard(info.name);
        if (!nsm && !isCompanionCategory(info.category))
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            const model::CardLocation location{type, row};
            if (references.from(location).empty())
                continue;
            // A card that something refers to is a card of the model, not a companion (a PSHELL, a MAT1).
            if (!nsm && references.isReferenced(location))
                continue;
            candidates.push_back(location);
        }
    }
    return candidates;
}

} // namespace

std::uint64_t RevisionStamp::mix(std::uint64_t value) noexcept
{
    value += 0x9E3779B97F4A7C15ULL;
    value = (value ^ (value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
    value = (value ^ (value >> 27U)) * 0x94D049BB133111EBULL;
    return value ^ (value >> 31U);
}

std::uint64_t RevisionStamp::hashBytes(std::uint64_t seed, const void* data, std::size_t size) noexcept
{
    const auto* bytes = static_cast<const unsigned char*>(data);
    std::uint64_t hash = seed;
    for (std::size_t index = 0; index < size; ++index)
    {
        hash ^= bytes[index];
        hash *= FNV_PRIME;
    }
    return hash;
}

std::uint64_t RevisionStamp::hashReal(std::uint64_t seed, double value) noexcept
{
    if (value == 0.0)
        value = 0.0; // -0.0 and 0.0 are the same content
    std::uint64_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    return hashBytes(seed, &bits, sizeof(bits));
}

std::uint64_t RevisionStamp::cardHash(const model::Model& model, model::CardLocation card)
{
    const model::CardTable* table = model.findTable(card.table);
    if (table == nullptr)
        return 0;
    model::FieldWriter writer(false, 0, false, true);
    table->writeCard(card.row, writer);
    std::uint64_t hash = FNV_OFFSET;
    const std::string_view name = table->info().name;
    hash = hashBytes(hash, name.data(), name.size());
    for (const std::string& field : writer.fields())
    {
        hash = hashBytes(hash, "\x1f", 1);
        hash = hashBytes(hash, field.data(), field.size());
    }
    return hash;
}

std::vector<std::uint64_t> RevisionStamp::compute(const model::Model& model, const ComponentModel& components,
                                                  base::ThreadPool* pool)
{
    const std::size_t count = components.componentCount();
    if (count == 0)
        return {};
    const std::size_t typeCount = model::CardRegistry::instance().size();

    // The member elements of each component.
    std::vector<std::vector<model::CardLocation>> members(count);
    for (std::uint16_t type = 1; type < typeCount; ++type)
    {
        if (roleOfType(type) == ElementRole::None || model.findTable(type) == nullptr)
            continue;
        const std::uint32_t rows = static_cast<std::uint32_t>(model.findTable(type)->rows().rowCount());
        for (std::uint32_t row = 0; row < rows; ++row)
        {
            const std::int32_t component = components.componentOfElement(model::CardLocation{type, row});
            if (component > 0)
                members[static_cast<std::size_t>(component - 1)].push_back(model::CardLocation{type, row});
        }
    }
    // A component that contact merges holds the contact cards that join it: the table, its parameter
    // cards, the bodies and what they are made of.
    if (components.contactMode() == ContactMode::Merge && components.contactSearch() != nullptr)
    {
        for (const ContactJoin& join : components.contactJoins())
        {
            const ContactPair& pair = components.contactSearch()->pairs()[join.pair];
            for (const std::int32_t component : join.components)
            {
                std::vector<model::CardLocation>& cards = members[static_cast<std::size_t>(component - 1)];
                cards.insert(cards.end(), pair.cards.begin(), pair.cards.end());
            }
        }
    }
    return computeFor(model, members, pool);
}

std::vector<std::uint64_t> RevisionStamp::computeFor(const model::Model& model,
                                                     const std::vector<std::vector<model::CardLocation>>& members,
                                                     base::ThreadPool* pool)
{
    const std::size_t count = members.size();
    std::vector<std::uint64_t> stamps(count, 0);
    if (count == 0)
        return stamps;
    const std::shared_ptr<const ReferenceIndex> references = ComponentCache::of(model).references(model);
    const model::CardRegistry& registry = model::CardRegistry::instance();
    const std::size_t typeCount = registry.size();
    const std::vector<model::CardLocation> candidates = companionCandidates(model, *references);

    // The component that last visited each card: a card is in the closure when it holds the number of the component.
    std::vector<std::vector<std::int32_t>> visited(typeCount);
    for (std::size_t type = 1; type < typeCount; ++type)
    {
        const model::CardTable* table = model.findTable(static_cast<std::uint16_t>(type));
        if (table != nullptr)
            visited[type].assign(table->rows().rowCount(), 0);
    }
    base::ThreadPool serialPool(0);
    base::ThreadPool& workers = pool != nullptr ? *pool : serialPool;

    std::vector<model::CardLocation> closure;
    for (std::size_t index = 0; index < count; ++index)
    {
        const auto number = static_cast<std::int32_t>(index + 1); // the mark of this set in `visited`
        closure.clear();
        const auto visit = [&](model::CardLocation location) -> bool
        {
            std::int32_t& mark = visited[location.table][location.row];
            if (mark == number)
                return false;
            mark = number;
            closure.push_back(location);
            return true;
        };
        const auto expand = [&](std::size_t from)
        {
            for (std::size_t position = from; position < closure.size(); ++position)
            {
                for (const ReferenceEdge& edge : references->from(closure[position]))
                {
                    if (edge.resolved && edge.to.table < visited.size() && edge.to.row < visited[edge.to.table].size())
                        visit(edge.to);
                }
            }
        };
        for (const model::CardLocation member : members[index])
        {
            if (member.table < visited.size() && member.row < visited[member.table].size())
                visit(member);
        }
        expand(0);
        // Companions: a card that completes a card of the closure (the same ID in the same category, another table), or
        // adds mass to a property or an element of it.
        const std::size_t before = closure.size();
        for (const model::CardLocation candidate : candidates)
        {
            if (visited[candidate.table][candidate.row] == number)
                continue;
            const model::CardTable* table = model.findTable(candidate.table);
            const bool nsm = isNonStructMassCard(table->info().name);
            for (const ReferenceEdge& edge : references->from(candidate))
            {
                if (!edge.resolved || visited[edge.to.table].size() <= edge.to.row ||
                    visited[edge.to.table][edge.to.row] != number)
                    continue;
                const bool completes = !nsm && edge.to.table != candidate.table &&
                                       registry.entry(edge.to.table).info.category == table->info().category &&
                                       model.findTable(edge.to.table)->idAt(edge.to.row) == table->idAt(candidate.row);
                if (completes || nsm)
                {
                    visit(candidate);
                    break;
                }
            }
        }
        expand(before);

        // The hashes are added: the sum does not depend on the order, so the chunks can be added in any order.
        constexpr std::size_t CHUNK = 4096;
        const std::size_t chunks = (closure.size() + CHUNK - 1) / CHUNK;
        std::vector<std::uint64_t> partial(chunks, 0);
        const auto work = [&](std::size_t chunk)
        {
            const std::size_t begin = chunk * CHUNK;
            const std::size_t end = std::min(begin + CHUNK, closure.size());
            std::uint64_t sum = 0;
            for (std::size_t position = begin; position < end; ++position)
                sum += mix(cardHash(model, closure[position]));
            partial[chunk] = sum;
        };
        if (chunks > 1)
            base::parallelFor(workers, chunks, work);
        else if (chunks == 1)
            work(0);
        std::uint64_t total = 0;
        for (const std::uint64_t part : partial)
            total += part;
        stamps[index] = mix(total ^ mix(static_cast<std::uint64_t>(closure.size())));
    }
    return stamps;
}

std::shared_ptr<const std::vector<std::uint64_t>> RevisionStamp::of(const model::Model& model, base::ThreadPool* pool)
{
    return ComponentCache::of(model).stamps(model, pool);
}

std::shared_ptr<const std::vector<std::uint64_t>> RevisionStamp::of(const model::Model& model,
                                                                    const ContactOptions& contact,
                                                                    base::ThreadPool* pool)
{
    return ComponentCache::of(model).stamps(model, contact, pool);
}

} // namespace oor::analysis
