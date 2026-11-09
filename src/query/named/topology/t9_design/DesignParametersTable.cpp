#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/optimisation/designOptimParams/DesignOptimParamsCatalogue.hpp"
#include "oor/model/cards/optimisation/designOptimParams/DesignOptimParamsTable.hpp"
#include "oor/model/cards/optimisation/designOptimParams/DesignOptimParamsValues.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace oor::query
{

namespace
{

// designParametersTable: the parameters of the optimisation (DOPTPRM), the deck's value or the default.
class DesignParametersTableQuery final : public QueryBase
{
public:
    DesignParametersTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        // The catalogue in the order of the format reference, then the names a DOPTPRM gives that it does not list, as
        // written.
        std::vector<std::string> names;
        for (const model::DesignOptimParamsInfo& item : model::designOptimParamsCatalogue())
            names.emplace_back(item.name);
        if (const model::DesignOptimParamsTable* table = model.findTable<model::DesignOptimParamsTable>())
        {
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (!table->rows().isAlive(row))
                    continue;
                for (const model::DesignOptimParamsEntry& entry : table->card(row).entries)
                {
                    const bool known = std::any_of(names.begin(), names.end(),
                                                   [&](const std::string& name) { return ((name) == (entry.name)); });
                    if (!known)
                        names.push_back(entry.name);
                }
            }
        }
        std::vector<std::string> kind;
        std::vector<std::uint8_t> kindValid;
        std::vector<std::int64_t> integer;
        std::vector<std::uint8_t> integerValid;
        std::vector<double> real;
        std::vector<std::uint8_t> realValid;
        std::vector<std::string> text;
        std::vector<std::uint8_t> textValid;
        std::vector<std::uint8_t> isDefault;
        for (const std::string& name : names)
        {
            const model::DesignOptimParamsInfo* info = model::findDesignOptimParam(name);
            kind.emplace_back(info != nullptr ? model::designOptimParamsKindName(info->kind) : std::string_view());
            kindValid.push_back(info != nullptr ? 1 : 0);
            const std::optional<model::DesignOptimParamsValue> value = model::designOptimParamsValue(model, name);
            const model::DesignValue given = value ? value->value : model::DesignValue{};
            integer.push_back(given.isInteger() ? given.integer : 0);
            integerValid.push_back(given.isInteger() ? 1 : 0);
            real.push_back(given.isReal() ? given.real : 0.0);
            realValid.push_back(given.isReal() ? 1 : 0);
            text.push_back(given.isWord() ? given.word : std::string());
            textValid.push_back(given.isWord() ? 1 : 0);
            isDefault.push_back(value && value->fromDeck ? 0 : 1);
        }
        TableBuilder builder;
        builder.text("name", names);
        builder.dictionary("kind", kind, std::move(kindValid));
        builder.int64("integerValue", std::move(integer), std::move(integerValid));
        builder.real("realValue", std::move(real), std::move(realValid));
        builder.text("textValue", text, std::move(textValid));
        builder.flag("isDefault", isDefault);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designParametersTable";
        info.family = "T9";
        info.description =
            "The parameters of the design optimisation (DOPTPRM): one row per "
            "parameter of the catalogue, in the order of the format reference, then each name a DOPTPRM gives that the "
            "catalogue "
            "does not list. kind is integer, real or text as the catalogue says (null for a name it does not list); "
            "the value is in integerValue, realValue or textValue, as the deck wrote it (an integer for a real "
            "parameter stays an integer) or the default of the catalogue (the one of sizing and shape optimisation "
            "where the format reference gives two); all three are null for a parameter the deck does not give whose "
            "default the "
            "program computes (DABOBJ, OPTCOD, ...). isDefault is false for a value a DOPTPRM gives.";
        info.defaultSort = "the order of the format reference catalogue, then the other names in the order of the deck";
        ColumnListBuilder columns;
        columns.text("name").dictionary("kind").int64("integerValue").real("realValue").text("textValue");
        columns.flag("isDefault");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addDesignParametersTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignParametersTableQuery>());
}

} // namespace oor::query
