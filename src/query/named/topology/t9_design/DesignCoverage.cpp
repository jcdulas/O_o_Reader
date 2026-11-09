#include "DesignQuerySupport.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/model/design/DesignableCard.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"

#include <algorithm>
#include <set>
#include <tuple>

namespace oor::query
{

namespace
{

enum Kind : int
{
    UndrivenProperty,
    UndrivenElement,
    UnusedVariable,
    MissingTarget,
    FieldDrivenTwice,
};

constexpr const char* KIND_NAMES[] = {"undrivenProperty", "undrivenElement", "unusedVariable", "missingTarget",
                                      "fieldDrivenTwice"};

struct Finding
{
    Kind kind = UndrivenProperty;
    std::string card;
    std::int32_t id = 0;
    std::string field;         // empty: null
    std::int32_t variable = 0; // 0: null
};

// designCoverage: what the design model leaves out or gets wrong.
class DesignCoverageQuery final : public QueryBase
{
public:
    DesignCoverageQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const model::DesignModel design = model::DesignModel::read(model);
        // A model with no design variable and no relation has no design model to be incomplete.
        if (design.variables().empty() && design.relations().empty())
            return emptyTable(info().defaultColumns());
        DrivenCards driven(run.model, design, run.context);
        std::vector<Finding> findings;

        // Properties and elements nothing drives.
        const CardList& elements = driven.elements();
        const std::vector<std::int32_t>& properties = driven.data().properties();
        std::set<std::int32_t> propertiesWithElements(properties.begin(), properties.end());
        propertiesWithElements.erase(0);
        const std::vector<std::uint16_t> propertyTypes =
            typesInSpace(model::CardCategory::Property, base::IdSpace::Property);
        const CardList propertyCards = listCards(model, propertyTypes);
        for (std::size_t index = 0; index < propertyCards.size(); ++index)
        {
            const std::int32_t pid = propertyCards.ids[index];
            if (propertiesWithElements.contains(pid) && !driven.isPropertyDriven(pid))
                findings.push_back({UndrivenProperty, std::string(typeName(propertyCards.types[index])), pid, "", 0});
        }
        for (std::size_t position = 0; position < elements.size(); ++position)
        {
            const std::string name(typeName(elements.types[position]));
            // An element with a property is driven through it; one without (a mass, a spring) only if the library can
            // design a field of its card.
            const bool candidate = properties[position] > 0 || model::findDesignableCard(name) != nullptr;
            if (candidate && !driven.isElementDriven(position))
                findings.push_back({UndrivenElement, name, elements.ids[position], "", 0});
        }

        // Variables nothing uses.
        for (const model::DesignVariable& variable : design.variables())
        {
            if (!design.isUsed(variable.id))
                findings.push_back({UnusedVariable, "DesignVar", variable.id, "", 0});
        }

        // Relations that name nothing, and fields two relations drive.
        std::map<std::tuple<std::uint16_t, std::uint32_t, std::int32_t, std::int32_t, std::int32_t>,
                 std::vector<std::size_t>>
            drivers;
        for (std::size_t index = 0; index < design.relations().size(); ++index)
        {
            const model::DesignRelation& relation = design.relations()[index];
            if (!relation.applicable)
            {
                const std::string field =
                    relation.facts.name.empty() ? model::describeDesignField(relation.field) : relation.facts.name;
                const base::EntityId variable = !relation.terms.empty()       ? relation.terms.front().designVarId
                                                : !relation.variables.empty() ? relation.variables.front()
                                                                              : 0;
                const std::string relationType(model::CardRegistry::instance().entry(relation.cardType).info.name);
                findings.push_back({MissingTarget, relationType, relation.id, field, variable});
                continue;
            }
            drivers[std::make_tuple(relation.target->table, relation.target->row, relation.facts.kind,
                                    relation.facts.index, relation.facts.station)]
                .push_back(index);
        }
        for (const auto& entry : drivers)
        {
            if (entry.second.size() < 2)
                continue;
            std::set<base::EntityId> variables;
            for (const std::size_t index : entry.second)
            {
                for (const model::DesignTerm& term : design.relations()[index].terms)
                    variables.insert(term.designVarId);
                variables.insert(design.relations()[index].variables.begin(),
                                 design.relations()[index].variables.end());
            }
            const model::DesignRelation& first = design.relations()[entry.second.front()];
            if (variables.empty())
                variables.insert(0);
            for (const base::EntityId variable : variables)
                findings.push_back(
                    {FieldDrivenTwice, std::string(first.targetCard), first.targetId, first.facts.name, variable});
        }
        if (findings.empty())
            return emptyTable(info().defaultColumns());
        std::stable_sort(
            findings.begin(), findings.end(), [](const Finding& left, const Finding& right)
            { return std::tie(left.kind, left.card, left.id) < std::tie(right.kind, right.card, right.id); });

        const std::size_t count = findings.size();
        std::vector<std::string> kind(count), card(count), field(count);
        std::vector<std::int32_t> id(count), variable(count);
        std::vector<std::uint8_t> fieldValid(count), variableValid(count);
        for (std::size_t row = 0; row < count; ++row)
        {
            const Finding& finding = findings[row];
            kind[row] = KIND_NAMES[finding.kind];
            card[row] = finding.card;
            id[row] = finding.id;
            field[row] = finding.field;
            fieldValid[row] = finding.field.empty() ? 0 : 1;
            variable[row] = finding.variable;
            variableValid[row] = finding.variable == 0 ? 0 : 1;
        }
        TableBuilder builder;
        builder.dictionary("kind", kind);
        builder.dictionary("card", card);
        builder.int32("id", std::move(id));
        builder.dictionary("field", field, fieldValid);
        builder.int32("designVariable", std::move(variable), std::move(variableValid));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designCoverage";
        info.family = "T9";
        info.description =
            "What the design model leaves out or gets wrong, one row for each finding: `undrivenProperty` (a property "
            "of "
            "elements that no DVPREL names and no DVMREL reaches through its material), `undrivenElement` (an element "
            "whose property is not driven, or a mass or spring that no DVCREL names), `unusedVariable` (a DESVAR no "
            "relation, DLINK, DVGRID, DVSHAP, DRESP2 or DRESP3 uses), `missingTarget` (a relation that cannot be "
            "applied: "
            "its property, material or element is not there, or the card type or the field is one the library cannot "
            "write: `card` and `id` are the relation, `field` the field it names) and `fieldDrivenTwice` (`card` and "
            "`id` are the property, material or element, `designVariable` each variable that drives the field). A "
            "model "
            "with no design variable and no relation has no findings.";
        info.defaultSort = "kind, card, id";
        ColumnListBuilder columns;
        columns.dictionary("kind").dictionary("card").int32("id").dictionary("field").int32("designVariable");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addDesignCoverageQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignCoverageQuery>());
}

} // namespace oor::query
