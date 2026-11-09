#include "oor/query/providers/ModelSource.hpp"

#include "oor/model/DataContract.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/operators/ColumnOps.hpp"

#include <algorithm>
#include <numeric>

namespace oor::query
{

std::vector<std::uint16_t> typesOfCategory(model::CardCategory category)
{
    std::vector<std::uint16_t> types;
    const model::CardRegistry& registry = model::CardRegistry::instance();
    for (std::uint16_t type = 0; type < registry.size(); ++type)
    {
        if (registry.entry(type).info.category == category)
            types.push_back(type);
    }
    return types;
}

namespace
{

struct Entry
{
    std::uint16_t type = 0;
    std::uint32_t row = 0;
    std::uint32_t key = 0;
};

// The cards a join to a kind of card follows to.
struct TargetKind
{
    base::IdSpace space = base::IdSpace::Node;
    std::vector<model::CardCategory> categories;
};

std::optional<TargetKind> targetKindOf(std::string_view target)
{
    if (target == "property" || target == "properties")
        return TargetKind{base::IdSpace::Property, {model::CardCategory::Property}};
    if (target == "material" || target == "materials")
        return TargetKind{base::IdSpace::Material, {model::CardCategory::Material}};
    if (target == "node" || target == "nodes" || target == "grid")
        return TargetKind{base::IdSpace::Node, {model::CardCategory::Node, model::CardCategory::Point}};
    if (target == "element" || target == "elements")
        return TargetKind{base::IdSpace::Element, {model::CardCategory::Element}};
    if (target == "coordinate" || target == "coordinates" || target == "coord")
        return TargetKind{base::IdSpace::Coordinate, {model::CardCategory::Coordinate}};
    return std::nullopt;
}

// The cards a join found, by ID, in the ID space of the target.
class ModelJoinTarget final : public JoinTarget
{
public:
    ModelJoinTarget(std::shared_ptr<model::Model> model, const TargetKind& kind)
        : m_model(std::move(model)), m_space(kind.space), m_columns(m_model, typesOf(kind))
    {
    }

    [[nodiscard]] std::vector<ColumnDescription> columns() const override { return m_columns.columns(); }

    [[nodiscard]] TargetRows locate(std::span<const std::int64_t> keys, ExecutionContext& context) const override
    {
        TargetRows matches;
        matches.types.assign(keys.size(), 0);
        matches.rows.assign(keys.size(), NO_ROW);
        const std::vector<std::uint16_t>& allowed = m_columns.types();
        context.parallel(
            keys.size(), LIGHT_CHUNK,
            [&](base::IndexRange range)
            {
                for (std::size_t index = range.begin; index < range.end; ++index)
                {
                    const auto id = base::toEntityId(keys[index]);
                    if (!id)
                        continue;
                    const auto location = m_model->locate(m_space, *id);
                    if (!location || std::find(allowed.begin(), allowed.end(), location->table) == allowed.end())
                        continue;
                    matches.types[index] = location->table;
                    matches.rows[index] = location->row;
                }
            });
        return matches;
    }

    [[nodiscard]] std::vector<NamedColumn> fetch(std::string_view name, const TargetRows& matches,
                                                 ExecutionContext& context) const override
    {
        CardRows cards;
        cards.types = matches.types;
        cards.rows = matches.rows;
        if (m_columns.types().size() == 1)
            cards.single = m_columns.types()[0];
        return m_columns.fetch(name, cards, context);
    }

private:
    static std::vector<std::uint16_t> typesOf(const TargetKind& kind)
    {
        std::vector<std::uint16_t> types;
        for (const model::CardCategory category : kind.categories)
        {
            const std::vector<std::uint16_t> some = typesOfCategory(category);
            types.insert(types.end(), some.begin(), some.end());
        }
        return types;
    }

    std::shared_ptr<model::Model> m_model;
    base::IdSpace m_space;
    CardColumns m_columns;
};

} // namespace

ModelSource::ModelSource(std::shared_ptr<model::Model> model, std::string description, std::vector<std::uint16_t> types,
                         std::optional<std::vector<SourceCard>> selection, std::shared_ptr<const ModelLink> link)
    : m_model(model), m_description(std::move(description)), m_columns(std::move(model), types), m_link(std::move(link))
{
    std::vector<Entry> entries;
    if (selection)
    {
        for (const SourceCard& card : *selection)
        {
            const auto location = m_model->locateKey(card.type, base::CardKey{card.key});
            if (location)
                entries.push_back(Entry{card.type, location->row, card.key});
        }
        std::stable_sort(entries.begin(), entries.end(),
                         [](const Entry& left, const Entry& right) { return left.key < right.key; });
    }
    else
    {
        // Each table is in the order of its keys, so the tables are merged, not sorted.
        for (const std::uint16_t type : m_columns.types())
        {
            const model::CardTable* table = m_model->findTable(type);
            if (table == nullptr)
                continue;
            const model::RowBook& rows = table->rows();
            const std::size_t before = entries.size();
            for (std::uint32_t row = 0; row < rows.rowCount(); ++row)
            {
                if (rows.isAlive(row))
                    entries.push_back(Entry{type, row, rows.keyAt(row).value});
            }
            if (before > 0)
                std::inplace_merge(entries.begin(), entries.begin() + static_cast<std::ptrdiff_t>(before),
                                   entries.end(),
                                   [](const Entry& left, const Entry& right) { return left.key < right.key; });
        }
    }

    // The order of the result is the order of the IDs; cards that share an ID (the cards of one load set) stay in file
    // order.
    std::vector<base::EntityId> ids(entries.size());
    for (std::size_t index = 0; index < entries.size(); ++index)
        ids[index] = m_model->findTable(entries[index].type)->idAt(entries[index].row);
    if (!std::is_sorted(ids.begin(), ids.end()))
    {
        std::vector<std::uint32_t> order(entries.size());
        std::iota(order.begin(), order.end(), 0U);
        std::stable_sort(order.begin(), order.end(),
                         [&](std::uint32_t left, std::uint32_t right) { return ids[left] < ids[right]; });
        std::vector<Entry> sorted(entries.size());
        for (std::size_t index = 0; index < order.size(); ++index)
            sorted[index] = entries[order[index]];
        entries = std::move(sorted);
    }

    m_entryTypes.resize(entries.size());
    m_entryRows.resize(entries.size());
    m_entryKeys.resize(entries.size());
    for (std::size_t index = 0; index < entries.size(); ++index)
    {
        m_entryTypes[index] = entries[index].type;
        m_entryRows[index] = entries[index].row;
        m_entryKeys[index] = entries[index].key;
    }
    if (m_columns.types().size() == 1)
    {
        const model::CardTable* table = m_model->findTable(m_columns.types()[0]);
        m_wholeTable = table != nullptr && entries.size() == table->rows().rowCount();
        for (std::size_t index = 0; m_wholeTable && index < entries.size(); ++index)
            m_wholeTable = m_entryRows[index] == index;
    }
}

std::vector<NamedColumn> ModelSource::fetch(std::string_view name, const RowSelection& rows,
                                            ExecutionContext& context) const
{
    CardRows cards;
    std::vector<std::uint16_t> types;
    std::vector<std::uint32_t> tableRows;
    const bool single = m_columns.types().size() == 1;
    if (single)
        cards.single = m_columns.types()[0];
    if (rows.isAll())
    {
        if (!single)
            cards.types = m_entryTypes;
        cards.rows = m_entryRows;
        cards.wholeTable = m_wholeTable;
    }
    else
    {
        tableRows.resize(rows.count);
        if (!single)
            types.resize(rows.count);
        for (std::size_t index = 0; index < rows.count; ++index)
        {
            tableRows[index] = m_entryRows[rows.rows[index]];
            if (!single)
                types[index] = m_entryTypes[rows.rows[index]];
        }
        cards.types = types;
        cards.rows = tableRows;
    }
    return m_columns.fetch(name, cards, context);
}

RowOrigin ModelSource::originOf(const RowSelection& rows) const
{
    auto types = std::make_shared<std::vector<std::uint16_t>>(rows.count);
    auto keys = std::make_shared<std::vector<std::uint32_t>>(rows.count);
    for (std::size_t index = 0; index < rows.count; ++index)
    {
        const std::uint32_t row = rows.at(index);
        (*types)[index] = m_entryTypes[row];
        (*keys)[index] = m_entryKeys[row];
    }
    RowOrigin origin;
    origin.types = std::move(types);
    origin.keys = std::move(keys);
    origin.link = m_link;
    return origin;
}

std::unique_ptr<JoinTarget> ModelSource::joinTarget(std::string_view target) const
{
    const std::optional<TargetKind> kind = targetKindOf(target);
    if (!kind)
        return nullptr;
    return std::make_unique<ModelJoinTarget>(m_model, *kind);
}

// The key columns of a join, by target: the neutral names of the fields that name one, in the order they are tried
// (a card has some of them).
std::vector<std::string> ModelSource::defaultJoinKeys(std::string_view target) const
{
    if (target == "property" || target == "properties")
        return {"propId"};
    if (target == "material" || target == "materials")
        return {"matId", "matId1", "membraneMatId"}; // a property has one of them
    if (target == "node" || target == "nodes" || target == "grid")
        return {"nodes", "nodeA", "nodeB", "node1", "node2", "node"};
    if (target == "element" || target == "elements")
        return {"elemId"};
    if (target == "coordinate" || target == "coordinates" || target == "coord")
        return {"positionFrameId"};
    return {};
}

int ModelSource::contractVersion() const
{
    return model::DATA_CONTRACT_VERSION;
}

} // namespace oor::query
