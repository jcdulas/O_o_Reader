#include "oor/base/text/MessageNumber.hpp"
#include "oor/model/core/ModelFileWriter.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t13_comparison/ComparisonQueries.hpp"
#include "oor/query/providers/StoredColumnCatalogue.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <map>
#include <set>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// One difference while the table is built.
struct Difference
{
    const char* change = "modified";
    std::string card;
    std::int32_t id = 0;
    std::optional<std::string> field;
    std::optional<std::string> before;
    std::optional<std::string> after;
    double beforeValue = NOT_A_NUMBER;
    double afterValue = NOT_A_NUMBER;
    std::size_t order = 0; // the order they were found in, to keep the sort stable
};

bool equalReals(double left, double right, double tolerance)
{
    if (std::isnan(left) && std::isnan(right))
        return true;
    if (left == right)
        return true;
    if (tolerance <= 0.0)
        return false;
    return std::fabs(left - right) <= tolerance * std::max(std::fabs(left), std::fabs(right));
}

// The value of a numeric scalar cell as a double, or false.
bool numberAt(const QueryColumn& column, std::size_t row, double& value)
{
    if (column.isNull(row))
        return false;
    switch (column.type())
    {
    case ColumnType::Float64:
        value = column.as<double>()[row];
        return true;
    case ColumnType::Int32:
        value = column.as<std::int32_t>()[row];
        return true;
    case ColumnType::UInt32:
        value = column.as<std::uint32_t>()[row];
        return true;
    case ColumnType::Int64:
        value = static_cast<double>(column.as<std::int64_t>()[row]);
        return true;
    default:
        return false;
    }
}

// The text of an element of a list column (child column position `at`).
std::string elementText(const QueryColumn& child, std::size_t at)
{
    double value = 0.0;
    if (child.type() == ColumnType::Float64 && numberAt(child, at, value))
        return base::MessageNumber::shortest(value);
    if (numberAt(child, at, value))
        return std::to_string(static_cast<std::int64_t>(value));
    if (child.type() == ColumnType::Utf8 || child.type() == ColumnType::Dictionary)
        return std::string(child.textAt(at));
    return "?";
}

std::string cellText(const QueryColumn& column, std::size_t row)
{
    if (column.isNull(row))
        return "None";
    double value = 0.0;
    switch (column.type())
    {
    case ColumnType::Bool:
        return column.boolAt(row) ? "true" : "false";
    case ColumnType::Utf8:
    case ColumnType::Dictionary:
        return std::string(column.textAt(row));
    case ColumnType::Float64:
        return base::MessageNumber::shortest(column.as<double>()[row]);
    case ColumnType::FixedList:
    case ColumnType::List:
    {
        std::string text = "[";
        const auto begin = static_cast<std::size_t>(column.listBegin(row));
        const auto count = static_cast<std::size_t>(column.listLength(row));
        for (std::size_t item = 0; item < count; ++item)
            text += (item == 0 ? "" : ", ") + elementText(*column.child(), begin + item);
        return text + "]";
    }
    default:
        if (numberAt(column, row, value))
            return std::to_string(static_cast<std::int64_t>(value));
        return "?";
    }
}

bool equalCells(const QueryColumn& left, std::size_t leftRow, const QueryColumn& right, std::size_t rightRow,
                double tolerance)
{
    if (left.isNull(leftRow) != right.isNull(rightRow))
        return false;
    if (left.isNull(leftRow))
        return true;
    if (left.type() != right.type())
        return cellText(left, leftRow) == cellText(right, rightRow);
    switch (left.type())
    {
    case ColumnType::Float64:
        return equalReals(left.as<double>()[leftRow], right.as<double>()[rightRow], tolerance);
    case ColumnType::FixedList:
    case ColumnType::List:
    {
        if (left.listLength(leftRow) != right.listLength(rightRow))
            return false;
        const auto count = static_cast<std::size_t>(left.listLength(leftRow));
        const auto leftBegin = static_cast<std::size_t>(left.listBegin(leftRow));
        const auto rightBegin = static_cast<std::size_t>(right.listBegin(rightRow));
        for (std::size_t item = 0; item < count; ++item)
        {
            if (!equalCells(*left.child(), leftBegin + item, *right.child(), rightBegin + item, tolerance))
                return false;
        }
        return true;
    }
    default:
        return cellText(left, leftRow) == cellText(right, rightRow);
    }
}

// The card types a `categories` argument names: a word that stands for a group of card types, or a card name.
std::vector<std::uint16_t> typesFor(const std::vector<std::string>& names)
{
    const model::CardRegistry& registry = model::CardRegistry::instance();
    std::set<std::uint16_t> types;
    for (const std::string& name : names)
    {
        std::string word = name;
        for (char& character : word)
            character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        std::vector<model::CardCategory> categories;
        if (word == "nodes")
            categories = {model::CardCategory::Node, model::CardCategory::Point};
        else if (word == "points")
            categories = {model::CardCategory::Point};
        else if (word == "coords" || word == "coordinates")
            categories = {model::CardCategory::Coordinate};
        else if (word == "elements")
            categories = {model::CardCategory::Element};
        else if (word == "properties")
            categories = {model::CardCategory::Property};
        else if (word == "materials")
            categories = {model::CardCategory::Material};
        else if (word == "loads")
            categories = {model::CardCategory::Load};
        else if (word == "constraints")
            categories = {model::CardCategory::Constraint};
        else if (word == "sets")
            categories = {model::CardCategory::Set};
        else if (word == "tables")
            categories = {model::CardCategory::Table};
        else if (word == "params" || word == "parameters")
            categories = {model::CardCategory::Parameter};
        else if (word == "matrices")
            categories = {model::CardCategory::Matrix};
        else if (word == "dynamics")
            categories = {model::CardCategory::DynamicLoad,     model::CardCategory::Method,
                          model::CardCategory::Frequency,       model::CardCategory::ComplexMethod,
                          model::CardCategory::Delay,           model::CardCategory::PhaseLead,
                          model::CardCategory::TimeStep,        model::CardCategory::NonlinearStatic,
                          model::CardCategory::Random,          model::CardCategory::TransferFunction,
                          model::CardCategory::InitialCondition};
        else if (word == "other")
            categories = {model::CardCategory::Other};
        if (!categories.empty())
        {
            for (std::uint16_t type = 0; type < registry.size(); ++type)
            {
                if (std::find(categories.begin(), categories.end(), registry.entry(type).info.category) !=
                    categories.end())
                    types.insert(type);
            }
            continue;
        }
        const auto found = registry.typeIndexOf(name);
        if (!found || *found == model::CardRegistry::UNKNOWN_TYPE)
            throw QueryError(
                "modelDiff: '" + name +
                "' is neither a card type nor a group of cards (nodes, points, coords, elements, properties, "
                "materials, loads, constraints, sets, tables, params, matrices, dynamics, other)");
        types.insert(*found);
    }
    return std::vector<std::uint16_t>(types.begin(), types.end());
}

// The live rows of a table, with their card keys.
struct LiveRows
{
    std::vector<std::uint32_t> rows;
    std::vector<std::uint32_t> keys;
    std::vector<std::int32_t> ids;
};

LiveRows liveRowsOf(const model::Model& model, std::uint16_t type)
{
    LiveRows live;
    const model::CardTable* table = model.findTable(type);
    if (table == nullptr)
        return live;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (!table->rows().isAlive(row))
            continue;
        live.rows.push_back(row);
        live.keys.push_back(table->rows().keyAt(row).value);
        live.ids.push_back(table->idAt(row));
    }
    return live;
}

// modelDiff: the differences between two models.
class ModelDiffQuery final : public QueryBase
{
public:
    ModelDiffQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::shared_ptr<model::Model> other = run.arguments.model("other");
        if (!other)
            throw QueryError("modelDiff: the model to compare with is required: model.queries.modelDiff(other)");
        model::Model& self = *run.model;
        const std::string match = run.arguments.textOr("match", "auto");
        if (match != "auto" && match != "key" && match != "id")
            throw QueryError("modelDiff: match is \"auto\", \"key\" or \"id\", not \"" + match + "\"");
        const double tolerance = run.arguments.realOr("tolerance", 0.0);
        const bool byKey = match == "key" || (match == "auto" && self.lineage() == other->lineage());
        const std::vector<std::string> categories = run.arguments.texts("categories");

        std::vector<std::uint16_t> types;
        if (categories.empty())
        {
            for (std::uint16_t type = 1; type < model::CardRegistry::instance().size(); ++type)
            {
                if (self.findTable(type) != nullptr || other->findTable(type) != nullptr)
                    types.push_back(type);
            }
        }
        else
        {
            types = typesFor(categories);
            types.erase(std::remove(types.begin(), types.end(), model::CardRegistry::UNKNOWN_TYPE), types.end());
        }

        std::vector<Difference> found;
        for (const std::uint16_t type : types)
        {
            run.context.checkCancelled();
            const std::string card(typeName(type));
            const LiveRows left = liveRowsOf(self, type);
            const LiveRows right = liveRowsOf(*other, type);
            // The pairs: (row in self, row in other) as positions in the live lists; -1 for no partner.
            std::vector<std::pair<std::int64_t, std::int64_t>> pairs;
            if (byKey)
            {
                std::size_t leftAt = 0;
                std::size_t rightAt = 0;
                // Both lists are in order of card key: merge them.
                while (leftAt < left.keys.size() || rightAt < right.keys.size())
                {
                    if (rightAt == right.keys.size() ||
                        (leftAt < left.keys.size() && left.keys[leftAt] < right.keys[rightAt]))
                        pairs.emplace_back(static_cast<std::int64_t>(leftAt++), -1);
                    else if (leftAt == left.keys.size() || right.keys[rightAt] < left.keys[leftAt])
                        pairs.emplace_back(-1, static_cast<std::int64_t>(rightAt++));
                    else
                        pairs.emplace_back(static_cast<std::int64_t>(leftAt++), static_cast<std::int64_t>(rightAt++));
                }
            }
            else
            {
                // By ID: the k-th card with an ID in one model goes with the k-th in the other.
                std::map<std::int32_t, std::vector<std::size_t>> leftById;
                std::map<std::int32_t, std::vector<std::size_t>> rightById;
                const bool numbered = model::CardRegistry::instance().entry(type).info.idSpace != base::IdSpace::None;
                for (std::size_t index = 0; index < left.rows.size(); ++index)
                    leftById[numbered ? left.ids[index] : 0].push_back(index);
                for (std::size_t index = 0; index < right.rows.size(); ++index)
                    rightById[numbered ? right.ids[index] : 0].push_back(index);
                std::set<std::int32_t> ids;
                for (const auto& entry : leftById)
                    ids.insert(entry.first);
                for (const auto& entry : rightById)
                    ids.insert(entry.first);
                for (const std::int32_t id : ids)
                {
                    const std::vector<std::size_t> none;
                    const auto& a = leftById.count(id) != 0 ? leftById.at(id) : none;
                    const auto& b = rightById.count(id) != 0 ? rightById.at(id) : none;
                    for (std::size_t item = 0; item < std::max(a.size(), b.size()); ++item)
                        pairs.emplace_back(item < a.size() ? static_cast<std::int64_t>(a[item]) : -1,
                                           item < b.size() ? static_cast<std::int64_t>(b[item]) : -1);
                }
            }
            if (pairs.empty())
                continue;

            // The fields: the columns the card type registers, read once from each model.
            const std::vector<std::string> fields = StoredColumnCatalogue::instance().names(type);
            std::vector<QueryColumn> leftColumns;
            std::vector<QueryColumn> rightColumns;
            for (const std::string& field : fields)
            {
                leftColumns.push_back(self.findTable(type) != nullptr
                                          ? StoredColumnCatalogue::instance().fetch(self, type, field)
                                          : StoredColumnCatalogue::instance().emptyColumn(type, field));
                rightColumns.push_back(other->findTable(type) != nullptr
                                           ? StoredColumnCatalogue::instance().fetch(*other, type, field)
                                           : StoredColumnCatalogue::instance().emptyColumn(type, field));
            }
            for (const auto& [a, b] : pairs)
            {
                if (a < 0 || b < 0)
                {
                    Difference difference;
                    difference.card = card;
                    if (a >= 0)
                    {
                        difference.change = "removed";
                        difference.id = left.ids[static_cast<std::size_t>(a)];
                        difference.before = model::ModelFileWriter::cardToRecord(
                            self, model::CardLocation{type, left.rows[static_cast<std::size_t>(a)]});
                    }
                    else
                    {
                        difference.change = "added";
                        difference.id = right.ids[static_cast<std::size_t>(b)];
                        difference.after = model::ModelFileWriter::cardToRecord(
                            *other, model::CardLocation{type, right.rows[static_cast<std::size_t>(b)]});
                    }
                    found.push_back(std::move(difference));
                    continue;
                }
                const auto leftRow = left.rows[static_cast<std::size_t>(a)];
                const auto rightRow = right.rows[static_cast<std::size_t>(b)];
                bool any = false;
                for (std::size_t item = 0; item < fields.size(); ++item)
                {
                    if (equalCells(leftColumns[item], leftRow, rightColumns[item], rightRow, tolerance))
                        continue;
                    any = true;
                    Difference difference;
                    difference.card = card;
                    difference.id = left.ids[static_cast<std::size_t>(a)];
                    difference.field = fields[item];
                    difference.before = cellText(leftColumns[item], leftRow);
                    difference.after = cellText(rightColumns[item], rightRow);
                    double before = 0.0;
                    double after = 0.0;
                    if (numberAt(leftColumns[item], leftRow, before) && numberAt(rightColumns[item], rightRow, after))
                    {
                        difference.beforeValue = before;
                        difference.afterValue = after;
                    }
                    found.push_back(std::move(difference));
                }
                if (!any && fields.empty())
                {
                    // A card type with no registered fields: compared by its text.
                    const std::string before =
                        model::ModelFileWriter::cardToRecord(self, model::CardLocation{type, leftRow});
                    const std::string after =
                        model::ModelFileWriter::cardToRecord(*other, model::CardLocation{type, rightRow});
                    if (before != after)
                    {
                        Difference difference;
                        difference.card = card;
                        difference.id = left.ids[static_cast<std::size_t>(a)];
                        difference.field = "text";
                        difference.before = before;
                        difference.after = after;
                        found.push_back(std::move(difference));
                    }
                }
            }
        }
        for (std::size_t index = 0; index < found.size(); ++index)
            found[index].order = index;
        std::sort(found.begin(), found.end(),
                  [](const Difference& left, const Difference& right)
                  {
                      if (left.card != right.card)
                          return left.card < right.card;
                      if (left.id != right.id)
                          return left.id < right.id;
                      const std::string leftField = left.field.value_or("");
                      const std::string rightField = right.field.value_or("");
                      if (leftField != rightField)
                          return leftField < rightField;
                      return left.order < right.order;
                  });

        const std::size_t count = found.size();
        std::vector<std::string> change(count);
        std::vector<std::string> card(count);
        std::vector<std::int32_t> id(count);
        std::vector<std::string> field(count);
        std::vector<std::uint8_t> fieldValid(count);
        std::vector<std::string> before(count);
        std::vector<std::uint8_t> beforeValid(count);
        std::vector<std::string> after(count);
        std::vector<std::uint8_t> afterValid(count);
        std::vector<double> beforeValue(count);
        std::vector<double> afterValue(count);
        std::vector<double> delta(count);
        std::vector<double> relative(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            const Difference& difference = found[index];
            change[index] = difference.change;
            card[index] = difference.card;
            id[index] = difference.id;
            field[index] = difference.field.value_or("");
            fieldValid[index] = difference.field ? 1 : 0;
            before[index] = difference.before.value_or("");
            beforeValid[index] = difference.before ? 1 : 0;
            after[index] = difference.after.value_or("");
            afterValid[index] = difference.after ? 1 : 0;
            beforeValue[index] = difference.beforeValue;
            afterValue[index] = difference.afterValue;
            delta[index] = difference.afterValue - difference.beforeValue;
            relative[index] =
                difference.beforeValue != 0.0 ? delta[index] / std::fabs(difference.beforeValue) : NOT_A_NUMBER;
        }
        TableBuilder builder;
        builder.dictionary("change", change);
        builder.dictionary("card", card);
        builder.int32("id", std::move(id));
        builder.dictionary("field", field, std::move(fieldValid));
        builder.text("before", before, std::move(beforeValid));
        builder.text("after", after, std::move(afterValid));
        builder.real("beforeValue", std::move(beforeValue));
        builder.real("afterValue", std::move(afterValue));
        builder.real("delta", std::move(delta));
        builder.real("relativeDelta", std::move(relative));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "modelDiff";
        info.family = "T13";
        info.description =
            "The differences between this model and `other`, card by card and field by field: a card that only this "
            "model has is "
            "`removed` (its text in `before`), one that only `other` has is `added` (its text in `after`), a card in "
            "both "
            "that differs has a row `modified` for each field that differs, with the values as text and, for numeric "
            "fields, as "
            "numbers with their delta and relative delta (delta over |before|). match=\"key\" pairs the cards by card "
            "key "
            "(what a model and its copy() share: a renumbering shows as a change of the ID field), match=\"id\" by "
            "card type "
            "and ID (the k-th card of an ID with the k-th: loads share IDs), match=\"auto\" by key when the "
            "two models "
            "are copies of one another and by ID otherwise. `categories` limits the comparison to groups of cards "
            "(nodes, "
            "points, coords, elements, properties, materials, loads, constraints, sets, tables, params, dynamics, "
            "other) or "
            "card names; tolerance is the relative difference below which two reals are equal. The fields are the "
            "columns the "
            "card type registers; cards the library keeps as text are not compared. id is the ID in this model (in "
            "`other` for "
            "an added card).";
        info.parameters = {
            parameter("other", ParameterKind::Model, "(required)", "the model to compare with"),
            parameter("categories", ParameterKind::TextList, "None", "groups of cards or card names to compare"),
            parameter("match", ParameterKind::Text, "\"auto\"", "\"key\", \"id\" or \"auto\""),
            parameter("tolerance", ParameterKind::Real, "0.0", "relative difference below which reals are equal")};
        info.defaultSort = "card, id, field";
        ColumnListBuilder columns;
        columns.dictionary("change").dictionary("card").int32("id").dictionary("field").text("before").text("after");
        columns.reals("beforeValue afterValue delta relativeDelta");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addModelDiffQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ModelDiffQuery>());
}

} // namespace oor::query
