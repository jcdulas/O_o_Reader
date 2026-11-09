#include "oor/query/named/common/CardFacts.hpp"

#include "oor/query/providers/StoredColumnCatalogue.hpp"

#include <limits>

namespace oor::query
{

namespace
{

class Collector final : public model::ReferenceVisitor
{
public:
    explicit Collector(model::ReferenceTarget target) : m_target(target) {}
    base::EntityId visit(std::string_view, model::ReferenceTarget target, base::EntityId id) override
    {
        if (target == m_target && id != 0)
            m_ids.push_back(id);
        return id;
    }
    [[nodiscard]] std::vector<base::EntityId> take() { return std::move(m_ids); }

private:
    model::ReferenceTarget m_target;
    std::vector<base::EntityId> m_ids;
};

} // namespace

CardFacts::CardFacts(const model::Model& model) : m_scratch(model.copy()) {}

std::vector<base::EntityId> CardFacts::references(model::CardLocation card, model::ReferenceTarget target)
{
    model::CardTable* table = m_scratch.findTable(card.table);
    if (table == nullptr || card.row >= table->rows().rowCount() || !table->rows().isAlive(card.row))
        return {};
    Collector collector(target);
    table->visitReferences(card.row, collector);
    return collector.take();
}

base::EntityId CardFacts::firstReference(model::CardLocation card, model::ReferenceTarget target)
{
    const std::vector<base::EntityId> ids = references(card, target);
    return ids.empty() ? 0 : ids.front();
}

bool CardFacts::hasField(std::uint16_t type, std::string_view name) const
{
    return StoredColumnCatalogue::instance().contains(type, name);
}

double CardFacts::realField(model::Model& model, model::CardLocation card, std::string_view name) const
{
    constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();
    if (!hasField(card.table, name) || model.findTable(card.table) == nullptr)
        return NOT_A_NUMBER;
    const QueryColumn column = StoredColumnCatalogue::instance().fetch(model, card.table, name);
    if (card.row >= column.size() || column.isNull(card.row))
        return NOT_A_NUMBER;
    switch (column.type())
    {
    case ColumnType::Float64:
        return column.as<double>()[card.row];
    case ColumnType::Int32:
        return column.as<std::int32_t>()[card.row];
    case ColumnType::Int64:
        return static_cast<double>(column.as<std::int64_t>()[card.row]);
    case ColumnType::UInt32:
        return column.as<std::uint32_t>()[card.row];
    default:
        return NOT_A_NUMBER;
    }
}

} // namespace oor::query
