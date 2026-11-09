#include "oor/model/design/DesignUpdate.hpp"

#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/optimisation/designVar/DesignVarTable.hpp"
#include "oor/model/design/DesignErrors.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <tuple>

namespace oor::model
{

namespace
{

constexpr std::size_t PROBLEMS_SHOWN = 8;

// A value the relation reproduces to rounding is the same value.
bool sameToRounding(double left, double right)
{
    const double scale = std::max(std::fabs(left), std::fabs(right));
    return std::fabs(left - right) <= 4.0 * std::numeric_limits<double>::epsilon() * scale;
}

std::string label(const DesignRelation& relation)
{
    return relation.card + " " + std::to_string(relation.id);
}

[[noreturn]] void refuse(const std::string& summary, const std::vector<std::string>& problems)
{
    std::string message = summary;
    for (std::size_t index = 0; index < problems.size() && index < PROBLEMS_SHOWN; ++index)
        message += "\n  " + problems[index];
    if (problems.size() > PROBLEMS_SHOWN)
        message += "\n  ... and " + std::to_string(problems.size() - PROBLEMS_SHOWN) + " more";
    throw DesignError(message);
}

// The field of a card as a user names it, for a message: "PSHELL 10 T".
std::string describeTarget(const DesignRelation& relation)
{
    return relation.targetLabel + " " + std::to_string(relation.targetId) + " " +
           (relation.facts.name.empty() ? describeDesignField(relation.field) : relation.facts.name);
}

// One field to write.
struct PlannedWrite
{
    std::uint16_t type = 0;
    std::uint32_t row = 0;
    std::size_t relation = 0;
    DesignFieldValue value;
};

} // namespace

DesignUpdate applyDesign(Model& model, const DesignModel& design, const DesignPoint& point,
                         const DesignApplyOptions& options, const DesignShapeUpdate& shape)
{
    const DesignEvaluation evaluation = design.evaluate(model, point, options.limits == DesignLimitMode::Clip);
    const std::vector<DesignRelation>& relations = design.relations();

    // Everything that can go wrong is found before the first write.
    std::vector<std::string> problems;
    DesignUpdate update;
    update.variables = evaluation.variables;
    update.variableBounds = evaluation.variableBounds;
    std::vector<std::size_t> applied;
    for (std::size_t index = 0; index < relations.size(); ++index)
    {
        if (!evaluation.relations[index].problem.empty())
        {
            if (options.skipUnapplicable)
                update.skipped.push_back(index);
            else
                problems.push_back(evaluation.relations[index].problem);
            continue;
        }
        applied.push_back(index);
    }
    if (!problems.empty())
        refuse("apply: " + std::to_string(problems.size()) + " relation(s) cannot be applied; nothing was written",
               problems);

    if (options.limits == DesignLimitMode::Error)
    {
        for (const std::size_t index : applied)
        {
            const DesignRelationValue& value = evaluation.relations[index];
            if (value.limit == DesignLimitHit::None)
                continue;
            const bool below = value.limit == DesignLimitHit::Minimum;
            problems.push_back(label(relations[index]) + " gives " + describeTarget(relations[index]) + " the value " +
                               std::to_string(value.requested) + ", " + (below ? "under" : "over") + " its " +
                               (below ? "lower" : "upper") + " limit " +
                               std::to_string(below ? relations[index].lowest : relations[index].highest));
        }
        if (!problems.empty())
            refuse("apply: " + std::to_string(problems.size()) +
                       " relation(s) give a value outside their limits; "
                       "nothing was written",
                   problems);
    }

    std::map<std::tuple<std::uint16_t, std::uint32_t, std::int32_t, std::int32_t, std::int32_t>, std::size_t> drivers;
    for (const std::size_t index : applied)
    {
        const DesignRelation& relation = relations[index];
        const auto key = std::make_tuple(relation.target->table, relation.target->row, relation.facts.kind,
                                         relation.facts.index, relation.facts.station);
        const auto [position, inserted] = drivers.emplace(key, index);
        if (!inserted)
            problems.push_back(label(relations[position->second]) + " and " + label(relation) + " both drive " +
                               describeTarget(relation));
    }
    if (!problems.empty())
        refuse("apply: a field is driven by two relations; nothing was written", problems);

    // What will be written: the fields that change, grouped by card.
    std::vector<PlannedWrite> writes;
    for (const std::size_t index : applied)
    {
        const DesignRelation& relation = relations[index];
        const DesignRelationValue& value = evaluation.relations[index];
        DesignFieldChange change;
        change.relation = index;
        change.before = value.current;
        change.requested = value.requested;
        change.limit = value.limit;
        change.changed = !sameToRounding(value.current, value.value);
        change.after = change.changed ? value.value : value.current;
        update.fields.push_back(change);
        if (!change.changed)
            continue;
        ++update.changedFields;
        PlannedWrite write;
        write.type = relation.target->table;
        write.row = relation.target->row;
        write.relation = index;
        write.value = {&relation.facts, value.value};
        writes.push_back(write);
    }
    std::stable_sort(writes.begin(), writes.end(), [](const PlannedWrite& left, const PlannedWrite& right)
                     { return std::tie(left.type, left.row) < std::tie(right.type, right.row); });

    update.grids = shape.moves;
    update.skippedShapes = shape.skipped;
    update.shapeProblems = shape.problems;
    // The XINIT of every variable with `setInitialValues`, and of the variables that move grid points always.
    std::vector<std::size_t> initialValues;
    for (std::size_t index = 0; index < design.variables().size(); ++index)
    {
        const bool movesGrids =
            std::find(shape.variables.begin(), shape.variables.end(), index) != shape.variables.end();
        if ((options.setInitialValues || movesGrids) && update.variables[index] != design.variables()[index].initial)
            initialValues.push_back(index);
    }
    if (writes.empty() && initialValues.empty() && shape.moves.empty())
        return update;

    Model backup(model);
    try
    {
        std::vector<DesignFieldValue> values;
        for (std::size_t first = 0; first < writes.size();)
        {
            std::size_t last = first;
            values.clear();
            while (last < writes.size() && writes[last].type == writes[first].type &&
                   writes[last].row == writes[first].row)
                values.push_back(writes[last++].value);
            relations[writes[first].relation].fields->write(model, writes[first].row, values);
            first = last;
        }
        if (!shape.moves.empty())
        {
            NodeTable& grids = model.table<NodeTable>();
            for (const DesignGridMove& move : shape.moves)
                grids.setXyz(move.row, move.after);
        }
        if (!initialValues.empty())
        {
            DesignVarTable& desvars = model.table<DesignVarTable>();
            for (const std::size_t index : initialValues)
            {
                DesignVarCard card = desvars.card(design.variables()[index].row);
                card.initialValue = update.variables[index];
                desvars.store(design.variables()[index].row, card);
            }
        }
        model.noteContentChange();
    }
    catch (...)
    {
        model.replaceContents(std::move(backup));
        throw;
    }
    return update;
}

} // namespace oor::model
