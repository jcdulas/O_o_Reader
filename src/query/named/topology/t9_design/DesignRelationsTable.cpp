#include "DesignQuerySupport.hpp"
#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/model/design/DesignModel.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"

#include <cmath>
#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();
constexpr double BLANK_UPPER_LIMIT = 1.0e20; // PMAX, MPMAX and CPMAX when the card leaves them blank

const char* targetTypeOf(model::DesignTargetKind kind)
{
    switch (kind)
    {
    case model::DesignTargetKind::Property:
        return "property";
    case model::DesignTargetKind::Material:
        return "material";
    default:
        return "element";
    }
}

// The columns of the table, filled one row at a time.
struct RelationRows
{
    std::vector<std::string> relationCard, targetType, targetCard, field, fieldName, problem, label;
    std::vector<std::int32_t> relationId, targetId, variable, equation, item, component, argument;
    std::vector<std::uint8_t> variableValid, equationValid, applicable, problemValid, fieldValid, itemValid,
        componentValid, argumentValid, labelValid;
    std::vector<double> coefficient, constant, current, lowerLimit, upperLimit, fieldMin, fieldMax;
};

// What a relation shares between its rows.
struct RelationFacts
{
    const model::DesignRelation* relation = nullptr;
    double current = NOT_A_NUMBER;
    model::DesignColumn column;
    std::int32_t component = 0; // 0: none
};

// One argument of a relation: a design variable (linear: with its coefficient), or a constant of a DVxREL2.
struct RelationArgument
{
    std::int32_t variable = 0; // 0: a constant
    double coefficient = NOT_A_NUMBER;
    std::int32_t index = -1; // the position in the argument list of the equation; -1 for a linear relation
    std::string label;       // the constant
};

void addRow(RelationRows& rows, const RelationFacts& facts, const RelationArgument& argument)
{
    const model::DesignRelation& relation = *facts.relation;
    // the card type of the relation (relation.card is its keyword, for the messages)
    rows.relationCard.emplace_back(model::CardRegistry::instance().entry(relation.cardType).info.name);
    rows.relationId.push_back(relation.id);
    rows.targetType.push_back(targetTypeOf(relation.targetKind));
    rows.targetCard.push_back(relation.targetCard);
    rows.targetId.push_back(relation.targetId);
    rows.field.push_back(facts.column.name);
    rows.fieldValid.push_back(facts.column.name.empty() ? 0 : 1);
    rows.fieldName.push_back(relation.facts.name.empty() ? model::describeDesignField(relation.field)
                                                         : relation.facts.name);
    rows.item.push_back(facts.column.item);
    rows.itemValid.push_back(facts.column.item >= 0 ? 1 : 0);
    rows.variable.push_back(argument.variable);
    rows.variableValid.push_back(argument.variable != 0 ? 1 : 0);
    rows.coefficient.push_back(argument.coefficient);
    const bool linear = relation.shape == model::DesignRelationShape::Linear;
    rows.constant.push_back(linear ? relation.constant : NOT_A_NUMBER);
    rows.equation.push_back(relation.equationId);
    rows.equationValid.push_back(linear ? 0 : 1);
    rows.current.push_back(facts.current);
    // The limits in force; the card's own PMIN and PMAX when the relation cannot be resolved.
    rows.lowerLimit.push_back(relation.applicable ? relation.lowest : relation.lowerLimit.value_or(NOT_A_NUMBER));
    rows.upperLimit.push_back(relation.applicable ? relation.highest : relation.upperLimit);
    rows.applicable.push_back(relation.applicable ? 1 : 0);
    rows.problem.push_back(relation.problem);
    rows.problemValid.push_back(relation.problem.empty() ? 0 : 1);
    rows.fieldMin.push_back(relation.lowerLimit.value_or(NOT_A_NUMBER));
    rows.fieldMax.push_back(relation.upperLimit == BLANK_UPPER_LIMIT ? NOT_A_NUMBER : relation.upperLimit);
    rows.component.push_back(facts.component);
    rows.componentValid.push_back(facts.component > 0 ? 1 : 0);
    rows.argument.push_back(argument.index);
    rows.argumentValid.push_back(argument.index >= 0 ? 1 : 0);
    rows.label.push_back(argument.label);
    rows.labelValid.push_back(argument.variable == 0 && !argument.label.empty() ? 1 : 0);
}

// The component of the elements a relation drives: the one they are in, the smallest number (the largest component)
// when they are in several; 0 when they are in none (a connection, a mass on no component) or there is no element.
std::int32_t componentOf(DrivenCards& driven, std::size_t relation, const analysis::ComponentModel& components)
{
    std::int32_t found = 0;
    for (const std::size_t element : driven.of(relation).elements)
    {
        const std::int32_t component = components.componentOfElement(driven.elements().location(element));
        if (component > 0 && (found == 0 || component < found))
            found = component;
    }
    return found;
}

// designRelationsTable: one row for each variable of a linear relation, one row for each argument of an equation.
class DesignRelationsTableQuery final : public QueryBase
{
public:
    DesignRelationsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::DesignModel design = model::DesignModel::read(*run.model);
        if (design.relations().empty())
            return emptyTable(info().defaultColumns());
        DrivenCards driven(run.model, design, run.context);
        const std::shared_ptr<const analysis::ComponentModel> components =
            analysis::ComponentModel::of(*run.model, run.context.pool());
        RelationRows rows;
        for (std::size_t index = 0; index < design.relations().size(); ++index)
        {
            const model::DesignRelation& relation = design.relations()[index];
            RelationFacts facts;
            facts.relation = &relation;
            if (relation.fields != nullptr && relation.target && !relation.facts.name.empty())
            {
                facts.current = relation.fields->read(*run.model, relation.target->row, relation.facts);
                facts.column = relation.fields->column(*run.model, relation.target->row, relation.facts);
            }
            facts.component = componentOf(driven, index, *components);
            const std::size_t before = rows.relationId.size();
            if (relation.shape == model::DesignRelationShape::Linear)
            {
                for (std::size_t term = 0; term < relation.terms.size(); ++term)
                {
                    // PVAL: the first coefficient is the value the field has in the model.
                    const double value =
                        term == 0 && relation.fromProperty ? facts.current : relation.terms[term].coefficient;
                    addRow(rows, facts, RelationArgument{relation.terms[term].designVarId, value, -1, {}});
                }
            }
            else
            {
                // The arguments of the equation in the order of its header: the design variables, then the constants.
                std::int32_t position = 0;
                for (const base::EntityId id : relation.variables)
                    addRow(rows, facts, RelationArgument{id, NOT_A_NUMBER, position++, {}});
                for (const std::string& label : relation.labels)
                    addRow(rows, facts, RelationArgument{0, NOT_A_NUMBER, position++, label});
            }
            if (rows.relationId.size() == before)
                addRow(rows, facts, RelationArgument{});
        }
        TableBuilder builder;
        builder.dictionary("relationCard", rows.relationCard);
        builder.int32("relationId", std::move(rows.relationId));
        builder.dictionary("targetType", rows.targetType);
        builder.dictionary("targetCard", rows.targetCard);
        builder.int32("targetId", std::move(rows.targetId));
        builder.dictionary("field", rows.field, std::move(rows.fieldValid));
        builder.int32("designVariable", std::move(rows.variable), std::move(rows.variableValid));
        builder.real("coefficient", std::move(rows.coefficient));
        builder.real("constant", std::move(rows.constant));
        builder.int32("equationId", std::move(rows.equation), std::move(rows.equationValid));
        builder.real("currentFieldValue", std::move(rows.current));
        builder.real("lowerLimit", std::move(rows.lowerLimit));
        builder.real("upperLimit", std::move(rows.upperLimit));
        builder.flag("isApplicable", rows.applicable);
        builder.text("problem", rows.problem, std::move(rows.problemValid));
        builder.dictionary("fieldName", rows.fieldName);
        builder.int32("fieldItem", std::move(rows.item), std::move(rows.itemValid));
        builder.real("fieldMin", std::move(rows.fieldMin));
        builder.real("fieldMax", std::move(rows.fieldMax));
        builder.int32("componentId", std::move(rows.component), std::move(rows.componentValid));
        builder.int32("argumentIndex", std::move(rows.argument), std::move(rows.argumentValid));
        builder.text("constantLabel", rows.label, std::move(rows.labelValid));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designRelationsTable";
        info.family = "T9";
        info.description =
            "What each design variable drives (DVPREL1/2, DVMREL1/2, DVCREL1/2; `relationCard` is the card type, "
            "DesignPropRelation1 for a DVPREL1): one row for each variable of a "
            "linear relation, one row for each argument of a relation by an equation, in the order of the argument "
            "list of its DEQATN (its design variables, then its DTABLE constants, which have a null `designVariable` "
            "and their `constantLabel`; `argumentIndex` from 0). `field` is the column of the data contract that "
            "holds the field (`thickness` of a PSHELL, `area` of a PROD, `youngsModulus` of a MAT1, `stationSection` "
            "of a PBEAM) and "
            "`fieldItem` the position of the field among the values of that column for the card when it holds several "
            "(a vector or a list, flattened: station * width + component), null for a column of one value; "
            "`fieldName` is the name the format reference gives it (`T`, `A(B)`, `DIM2`). Then the `coefficient` and "
            "`constant` of "
            "a linear relation or the `equationId`, the value the field has in the model now (`currentFieldValue`), "
            "the limits in force (`lowerLimit`: PMIN of the card or the default of its field, `upperLimit`), PMIN "
            "and "
            "PMAX as the card gives them (`fieldMin`, `fieldMax`: null when blank), whether `model.design.apply` can "
            "apply the relation (`isApplicable`, and the `problem` when it cannot) and the component of the elements "
            "the target drives (`componentId`: the smallest number when they are in several, null when they are in "
            "none). A relation whose first coefficient is PVAL shows the value of the field as that coefficient.";
        info.defaultSort = "relationCard, relationId, position of the variable in the relation";
        ColumnListBuilder columns;
        columns.dictionary("relationCard").int32("relationId").dictionary("targetType").dictionary("targetCard");
        columns.int32("targetId").dictionary("field").int32("designVariable").real("coefficient").real("constant");
        columns.int32("equationId").real("currentFieldValue").real("lowerLimit").real("upperLimit");
        columns.flag("isApplicable").text("problem").dictionary("fieldName").int32("fieldItem");
        columns.real("fieldMin").real("fieldMax").int32("componentId").int32("argumentIndex");
        columns.text("constantLabel");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addDesignRelationsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignRelationsTableQuery>());
}

} // namespace oor::query
