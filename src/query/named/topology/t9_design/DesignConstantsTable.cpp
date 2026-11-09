#include "oor/model/cards/optimisation/designConstants/DesignConstantsTable.hpp"

#include "oor/model/cards/optimisation/DesignTargets.hpp"
#include "oor/model/cards/optimisation/designFieldConstants/DesignFieldConstantsTable.hpp"
#include "oor/model/design/DesignModel.hpp"
#include "oor/model/design/DesignableCard.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"

#include <algorithm>
#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

struct ConstantRow
{
    std::string label;
    double value = NOT_A_NUMBER;
    std::string card;
    std::int32_t cardId = 0; // 0: null
    std::string field;       // empty: null
};

template <typename TableT, typename Body>
void forEachCard(const model::Model& model, Body&& body)
{
    const TableT* table = model.findTable<TableT>();
    if (table == nullptr)
        return;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (table->rows().isAlive(row))
            body(table->card(row));
    }
}

// The name of the card that has the ID `id` in the space of the dynamic tables (a TABLED1 to TABLED4), or "TABLED".
std::string tableCardName(const model::Model& model, base::EntityId id)
{
    const std::optional<model::CardLocation> location = model.locate(base::IdSpace::DynamicTable, id);
    if (!location)
        return "TABLED";
    return std::string(model::CardRegistry::instance().entry(location->table).info.name);
}

// The contract column of the field a DTABLE2 entry reads, or the field as the card names it when the library cannot
// resolve it.
std::string columnOfEntry(const model::Model& model, const model::DesignFieldConstantsEntry& entry)
{
    const model::DesignableCard* fields = model::findDesignableCard(entry.cardName);
    const std::optional<model::ReferenceTarget> target = model::targetOfDesignedCard(entry.cardName);
    if (fields == nullptr || !target)
        return entry.field;
    base::IdSpace space = base::IdSpace::Element;
    if (*target == model::ReferenceTarget::Property)
        space = base::IdSpace::Property;
    else if (*target == model::ReferenceTarget::Material)
        space = base::IdSpace::Material;
    const std::optional<model::CardLocation> location = model.locate(space, entry.targetId);
    // the card of a DTABLE2 entry is a keyword of the deck; the registry knows the card type
    if (!location || model::CardRegistry::instance().entry(location->table).info.name != entry.cardName)
        return entry.field;
    const model::DesignFieldLookup lookup = fields->find(model, location->row, model::DesignField{entry.field});
    if (!lookup.facts)
        return entry.field;
    return fields->column(model, location->row, *lookup.facts).name;
}

// designConstantsTable: the constants of the DTABLE and DTABLE2 cards.
class DesignConstantsTableQuery final : public QueryBase
{
public:
    DesignConstantsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        std::vector<ConstantRow> rows;
        forEachCard<model::DesignConstantsTable>(model,
                                                 [&](const model::DesignConstantsCard& card)
                                                 {
                                                     for (const model::DesignConstantsEntry& entry : card.entries)
                                                     {
                                                         ConstantRow row;
                                                         row.label = entry.label;
                                                         if (entry.table != 0)
                                                         {
                                                             row.card = tableCardName(model, entry.table);
                                                             row.cardId = entry.table;
                                                         }
                                                         else
                                                         {
                                                             row.card = "DesignConstants";
                                                             row.value = entry.value;
                                                         }
                                                         rows.push_back(std::move(row));
                                                     }
                                                 });
        const model::DesignModel design = model::DesignModel::read(model);
        forEachCard<model::DesignFieldConstantsTable>(
            model,
            [&](const model::DesignFieldConstantsCard& card)
            {
                for (const model::DesignFieldConstantsEntry& entry : card.entries)
                {
                    ConstantRow row;
                    row.label = entry.label;
                    row.value = design.constant(model, entry.label).value_or(NOT_A_NUMBER);
                    row.card = std::string(entry.cardName);
                    row.cardId = entry.targetId;
                    row.field = columnOfEntry(model, entry);
                    rows.push_back(std::move(row));
                }
            });
        if (rows.empty())
            return emptyTable(info().defaultColumns());
        std::stable_sort(rows.begin(), rows.end(),
                         [](const ConstantRow& left, const ConstantRow& right) { return left.label < right.label; });
        const std::size_t count = rows.size();
        std::vector<std::string> label(count), card(count), field(count);
        std::vector<double> value(count);
        std::vector<std::int32_t> cardId(count);
        std::vector<std::uint8_t> idValid(count), fieldValid(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            label[index] = rows[index].label;
            value[index] = rows[index].value;
            card[index] = rows[index].card;
            cardId[index] = rows[index].cardId;
            idValid[index] = rows[index].cardId != 0 ? 1 : 0;
            field[index] = rows[index].field;
            fieldValid[index] = rows[index].field.empty() ? 0 : 1;
        }
        TableBuilder builder;
        builder.text("label", label);
        builder.real("value", std::move(value));
        builder.dictionary("card", card);
        builder.int32("cardId", std::move(cardId), std::move(idValid));
        builder.dictionary("field", field, std::move(fieldValid));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designConstantsTable";
        info.family = "T9";
        info.description =
            "The constants the equations of the design relations and responses receive by label (DTABLE and DTABLE2 "
            "cards); `card` is a card type. A DTABLE value has `card` DesignConstants; a DTABLE entry written as an "
            "integer names a TABLEDi, a "
            "function "
            "of frequency or time (`card` is that table and `cardId` its ID, the value is null). A DTABLE2 constant "
            "is "
            "the value of a field of a card of the model before the design changes it: "
            "`card`, "
            "`cardId` and `field` (the column of the data contract, as in `designRelationsTable`; the name the card "
            "gives when the library cannot read that field, and the value is then null).";
        info.defaultSort = "label";
        ColumnListBuilder columns;
        columns.text("label").real("value").dictionary("card").int32("cardId").dictionary("field");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addDesignConstantsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignConstantsTableQuery>());
}

} // namespace oor::query
