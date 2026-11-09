#include "oor/model/cards/parameters/parameter/ParameterTable.hpp"
#include "oor/model/parameters/ParameterCatalogue.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t8_loads_constraints/LoadConstraintQueries.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <set>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// One row of paramsTable: a parameter and its value in one scope.
struct ParameterRow
{
    std::string name;
    std::optional<std::int64_t> subcase; // none: the Bulk Data
    std::string kind;
    std::optional<std::int32_t> integerValue;
    double realValue = NOT_A_NUMBER;
    double imaginaryValue = NOT_A_NUMBER;
    std::optional<std::string> textValue;
    bool isDefault = false;
};

// A PARAM card (or a catalogue default given as a card) as a row.
ParameterRow rowOfCard(const model::ParameterCard& card, std::optional<std::int64_t> subcase, bool isDefault)
{
    ParameterRow row;
    row.name = card.name;
    row.subcase = subcase;
    row.isDefault = isDefault;
    switch (card.kind)
    {
    case model::ParameterKind::Integer:
        row.kind = "integer";
        row.integerValue = card.integerValue;
        break;
    case model::ParameterKind::Real:
        row.kind = "real";
        row.realValue = card.realValue;
        break;
    case model::ParameterKind::Text:
        row.kind = "text";
        row.textValue = card.textValue;
        break;
    case model::ParameterKind::Complex:
        row.kind = "complex";
        row.realValue = card.realValue;
        row.imaginaryValue = card.imaginaryValue;
        break;
    case model::ParameterKind::Blank:
        row.kind = "blank";
        break;
    }
    return row;
}

// A PARAM of the Case Control (PARAM,NAME,value in a subcase) as a row: ControlRecords gives an integer, a real, a
// word, two reals (a complex value) or the rest of the statement as text.
ParameterRow rowOfStatement(const model::CaseEntry& entry, std::int64_t subcase)
{
    ParameterRow row;
    row.name = entry.variant;
    row.subcase = subcase;
    const model::CaseValue& value = entry.value;
    switch (value.kind)
    {
    case model::CaseValueKind::Integer:
        row.kind = "integer";
        row.integerValue = static_cast<std::int32_t>(value.integer);
        break;
    case model::CaseValueKind::Real:
        row.kind = "real";
        row.realValue = value.real;
        break;
    case model::CaseValueKind::RealList:
        row.kind = "complex";
        row.realValue = value.reals.empty() ? NOT_A_NUMBER : value.reals[0];
        row.imaginaryValue = value.reals.size() > 1 ? value.reals[1] : NOT_A_NUMBER;
        break;
    case model::CaseValueKind::Keyword:
    case model::CaseValueKind::Text:
        row.kind = "text";
        row.textValue = value.text;
        break;
    default:
        row.kind = "blank";
        break;
    }
    return row;
}

// The PARAMs of one Case Control section and of its steps, the last of each name winning (as the solver reads them).
void addStatements(const model::Subcase& section, std::int64_t subcase, std::map<std::string, ParameterRow>& rows)
{
    for (const model::CaseEntry& entry : section.entries)
    {
        if (entry.kind == model::EntryKind::Param)
            rows[entry.variant] = rowOfStatement(entry, subcase);
    }
    for (const model::Subcase& child : section.children)
        addStatements(child, subcase, rows);
}

// paramsTable: every PARAM of the Bulk Data and of the Case Control, typed.
class ParamsTableQuery final : public QueryBase
{
public:
    ParamsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        std::set<std::string> onlyNames;
        // A name of the argument is a neutral name, or a PARAM name of the deck (the keyword of the model's
        // dictionary).
        for (const std::string& name : run.arguments.texts("names"))
            onlyNames.insert(std::string(name));
        const bool withDefaults = run.arguments.boolOr("withDefaults", false);

        // The Bulk Data: the last card of each name is the one the solver uses.
        std::map<std::string, ParameterRow> bulk;
        if (const model::ParameterTable* table = model.findTable<model::ParameterTable>())
        {
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (!table->rows().isAlive(row))
                    continue;
                ParameterRow found = rowOfCard(table->card(row), std::nullopt, false);
                bulk[found.name] = std::move(found);
            }
        }
        std::vector<ParameterRow> rows;
        std::set<std::string> named;
        for (auto& [name, row] : bulk)
        {
            named.insert(name);
            rows.push_back(std::move(row));
        }
        for (const model::Subcase& segment : model.control().caseControl.segments())
        {
            std::map<std::string, ParameterRow> section;
            addStatements(segment, segment.id, section);
            for (auto& [name, row] : section)
            {
                named.insert(name);
                rows.push_back(std::move(row));
            }
        }
        if (withDefaults)
        {
            for (const model::ParamInfo& info : model::ParameterCatalogue::instance().entries())
            {
                if (info.numbered || !info.defaultValue.exists() || named.contains(std::string(info.name)))
                    continue;
                named.insert(std::string(info.name));
                rows.push_back(rowOfCard(model::ParameterCatalogue::defaultCard(info), std::nullopt, true));
            }
        }
        if (!onlyNames.empty())
            std::erase_if(rows, [&](const ParameterRow& row) { return !onlyNames.contains(row.name); });
        // The Bulk Data and the defaults first, by name; then the Case Control sections in file order.
        std::stable_sort(rows.begin(), rows.end(),
                         [](const ParameterRow& left, const ParameterRow& right)
                         {
                             if (left.subcase.has_value() != right.subcase.has_value())
                                 return !left.subcase.has_value();
                             if (!left.subcase)
                                 return left.name < right.name;
                             return false;
                         });
        return tableOf(rows);
    }

private:
    static std::shared_ptr<const QueryTable> tableOf(const std::vector<ParameterRow>& rows)
    {
        std::vector<std::string> name;
        std::vector<std::int32_t> subcase;
        std::vector<std::uint8_t> subcaseValid;
        std::vector<std::string> kind;
        std::vector<std::int32_t> integerValue;
        std::vector<std::uint8_t> integerValid;
        std::vector<double> realValue;
        std::vector<double> imaginaryValue;
        std::vector<std::string> textValue;
        std::vector<std::uint8_t> textValid;
        std::vector<std::uint8_t> isDefault;
        for (const ParameterRow& row : rows)
        {
            name.push_back(row.name);
            subcase.push_back(row.subcase ? static_cast<std::int32_t>(*row.subcase) : 0);
            subcaseValid.push_back(row.subcase ? 1 : 0);
            kind.push_back(row.kind);
            integerValue.push_back(row.integerValue.value_or(0));
            integerValid.push_back(row.integerValue ? 1 : 0);
            realValue.push_back(row.realValue);
            imaginaryValue.push_back(row.imaginaryValue);
            textValue.push_back(row.textValue.value_or(std::string()));
            textValid.push_back(row.textValue ? 1 : 0);
            isDefault.push_back(row.isDefault ? 1 : 0);
        }
        TableBuilder builder;
        builder.dictionary("name", name);
        builder.int32("subcase", std::move(subcase), std::move(subcaseValid));
        builder.dictionary("kind", kind);
        builder.int32("intValue", std::move(integerValue), std::move(integerValid));
        builder.real("realValue", std::move(realValue));
        builder.real("imagValue", std::move(imaginaryValue));
        builder.text("textValue", textValue, std::move(textValid));
        builder.flag("isDefault", isDefault);
        return builder.finish();
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "paramsTable";
        info.family = "T8";
        info.description =
            "Every PARAM of the deck, typed, by its neutral name (WeightToMass for WTMASS; a name the dictionary does "
            "not know as the deck writes it): one row for each name of the Bulk Data (subcase null; the last card of a "
            "name, the one the solver uses) and for each name of each Case Control section (subcase 0 for the global "
            "section; a PARAM of a step goes with its subcase; the last of a name in the section). kind is integer, "
            "real, text, complex or blank (a PARAM with no value); intValue, realValue (also the real part of a "
            "complex value), imagValue and textValue hold the value, the others are null. withDefaults=True adds "
            "the format reference default of every parameter of the catalogue that the deck does not set anywhere "
            "(isDefault; "
            "subcase null; a parameter whose default the program computes has none). names keeps these parameters "
            "(neutral names or the PARAM names of the deck). "
            "Sorted: the Bulk Data and the defaults by name, then the Case Control sections in file order.";
        info.parameters = {parameter("names", ParameterKind::TextList, "None", "keep these parameters"),
                           parameter("withDefaults", ParameterKind::Bool, "False", "add the catalogue defaults")};
        info.defaultSort = "subcase (Bulk Data first), name";
        ColumnListBuilder columns;
        columns.dictionary("name").int32("subcase").dictionary("kind").int32("intValue").real("realValue");
        columns.real("imagValue").text("textValue").flag("isDefault");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addParamsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ParamsTableQuery>());
}

} // namespace oor::query
