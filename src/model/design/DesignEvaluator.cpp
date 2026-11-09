#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/design/DesignErrors.hpp"
#include "oor/model/design/DesignModel.hpp"
#include "oor/model/expressions/EquationEvaluator.hpp"

#include <cmath>
#include <limits>
#include <map>

namespace oor::model
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// The values of the variables a design point gives: the independent ones as the point has them, the dependent ones from
// their DLINK (DDVID = C0 + CMULT * the sum of Ci * IDVi). A value given for a dependent variable is ignored.
std::vector<double> variableValues(const DesignModel& design, const DesignPoint& point)
{
    std::vector<double> values = point.values;
    for (const DesignLink& link : design.links())
    {
        const std::optional<std::size_t> dependent = design.findVariable(link.dependent);
        double sum = 0.0;
        for (const DesignTerm& term : link.terms)
            sum += term.coefficient * point.values[*design.findVariable(term.designVarId)];
        values[*dependent] = link.constant + link.multiplier * sum;
    }
    return values;
}

std::int8_t boundsOf(const DesignVariable& variable, double value)
{
    if (value < variable.lower)
        return -1;
    return value > variable.upper ? 1 : 0;
}

void limit(const DesignRelation& relation, DesignRelationValue& result, bool applyLimits)
{
    result.limit = DesignLimitHit::None;
    if (result.requested < relation.lowest)
        result.limit = DesignLimitHit::Minimum;
    else if (result.requested > relation.highest)
        result.limit = DesignLimitHit::Maximum;
    result.value = result.requested;
    if (!applyLimits)
        return;
    if (result.limit == DesignLimitHit::Minimum)
        result.value = relation.lowest;
    else if (result.limit == DesignLimitHit::Maximum)
        result.value = relation.highest;
}

} // namespace

DesignEvaluation DesignModel::evaluate(const Model& model, const DesignPoint& point, bool applyLimits) const
{
    if (model.contentVersion() != m_contentVersion)
        throw DesignError("the model changed since its design model was read: read it again");
    if (point.values.size() != m_variables.size())
        throw DesignError("a design point has " + std::to_string(point.values.size()) + " values and the model has " +
                          std::to_string(m_variables.size()) + " design variables");
    if (!m_problems.empty())
        throw DesignError("the design variables are not defined: " + m_problems.front());

    DesignEvaluation result;
    result.variables = variableValues(*this, point);
    result.variableBounds.reserve(m_variables.size());
    for (std::size_t index = 0; index < m_variables.size(); ++index)
        result.variableBounds.push_back(boundsOf(m_variables[index], result.variables[index]));

    result.relations.resize(m_relations.size());
    for (std::size_t index = 0; index < m_relations.size(); ++index)
    {
        DesignRelationValue& value = result.relations[index];
        const DesignRelation& relation = m_relations[index];
        value.current = value.requested = value.value = NOT_A_NUMBER;
        if (!relation.applicable)
        {
            value.problem = relation.problem;
            continue;
        }
        value.current = relation.fields->read(model, relation.target->row, relation.facts);
    }

    // The constants an equation receives, looked up once each.
    std::map<std::string, std::optional<double>> constants;
    std::map<std::string, std::string> constantProblems;
    const auto constantOf = [&](const std::string& label) -> std::optional<double>
    {
        const std::string key = std::string(label);
        const auto known = constants.find(key);
        if (known != constants.end())
            return known->second;
        std::string reason;
        const std::optional<double> found = constant(model, label, &reason);
        constants[key] = found;
        if (!found)
            constantProblems[key] = reason;
        return found;
    };

    // The linear relations, in one pass: C0 + the sum of the coefficient times the variable, the first coefficient
    // being the value the field holds when the card says PVAL.
    std::map<base::EntityId, std::vector<std::size_t>> byEquation;
    for (std::size_t index = 0; index < m_relations.size(); ++index)
    {
        const DesignRelation& relation = m_relations[index];
        DesignRelationValue& value = result.relations[index];
        if (!relation.applicable)
            continue;
        if (relation.shape == DesignRelationShape::Equation)
        {
            byEquation[relation.equationId].push_back(index);
            continue;
        }
        double sum = relation.constant;
        for (std::size_t term = 0; term < relation.terms.size(); ++term)
        {
            const double coefficient =
                term == 0 && relation.fromProperty ? value.current : relation.terms[term].coefficient;
            sum += coefficient * result.variables[*findVariable(relation.terms[term].designVarId)];
        }
        value.requested = sum;
    }

    // The equations: the relations that share a DEQATN are evaluated together, each argument an array with one value
    // for each relation. A relation gives the DEQATN its variables and then its constants, in this order.
    for (const auto& [equationId, members] : byEquation)
    {
        const EquationSet& equations = m_equations.at(equationId);
        const std::vector<std::string>& parameters = equations.parameters();
        std::vector<std::vector<double>> arguments(parameters.size(), std::vector<double>(members.size()));
        std::vector<std::size_t> evaluable;
        for (std::size_t member = 0; member < members.size(); ++member)
        {
            const DesignRelation& relation = m_relations[members[member]];
            bool complete = true;
            for (std::size_t position = 0; position < parameters.size(); ++position)
            {
                double argument = NOT_A_NUMBER;
                if (position < relation.variables.size())
                {
                    argument = result.variables[*findVariable(relation.variables[position])];
                }
                else if (const std::optional<double> given =
                             constantOf(relation.labels[position - relation.variables.size()]))
                {
                    argument = *given;
                }
                else
                {
                    const std::string key = std::string(relation.labels[position - relation.variables.size()]);
                    result.relations[members[member]].problem =
                        relation.card + " " + std::to_string(relation.id) + ": " + constantProblems[key];
                    complete = false;
                    break;
                }
                arguments[position][member] = argument;
            }
            if (complete)
                evaluable.push_back(member);
        }
        if (evaluable.empty())
            continue;
        // Only the relations that have all their arguments go to the evaluator, as the columns of the arrays.
        std::vector<std::vector<double>> columns(parameters.size());
        for (std::size_t position = 0; position < parameters.size(); ++position)
        {
            columns[position].reserve(evaluable.size());
            for (const std::size_t member : evaluable)
                columns[position].push_back(arguments[position][member]);
        }
        std::vector<EquationVariable> variables;
        for (std::size_t position = 0; position < parameters.size(); ++position)
            variables.push_back({parameters[position], columns[position]});
        std::vector<double> values;
        try
        {
            values = evaluateEquations(equations, variables);
        }
        catch (const base::OorError&)
        {
            // One of the points cannot be computed (a division of two integers by zero): find which, one at a time.
            values.assign(evaluable.size(), NOT_A_NUMBER);
            for (std::size_t slot = 0; slot < evaluable.size(); ++slot)
            {
                std::vector<double> one(parameters.size());
                for (std::size_t position = 0; position < parameters.size(); ++position)
                    one[position] = columns[position][slot];
                try
                {
                    values[slot] = evaluateEquations(equations, one);
                }
                catch (const base::OorError& error)
                {
                    const DesignRelation& relation = m_relations[members[evaluable[slot]]];
                    result.relations[members[evaluable[slot]]].problem =
                        relation.card + " " + std::to_string(relation.id) + ": " + error.what();
                }
            }
        }
        for (std::size_t slot = 0; slot < evaluable.size(); ++slot)
        {
            const std::size_t index = members[evaluable[slot]];
            const double requested = values.size() == 1 ? values[0] : values[slot];
            if (std::isnan(requested) && result.relations[index].problem.empty())
                result.relations[index].problem = m_relations[index].card + " " +
                                                  std::to_string(m_relations[index].id) +
                                                  ": the equation gives no number for these values";
            result.relations[index].requested = requested;
        }
    }

    // The limits of every relation that has a value.
    for (std::size_t index = 0; index < m_relations.size(); ++index)
    {
        DesignRelationValue& value = result.relations[index];
        if (std::isnan(value.requested) || !value.problem.empty())
        {
            value.requested = value.value = NOT_A_NUMBER;
            continue;
        }
        limit(m_relations[index], value, applyLimits);
    }
    return result;
}

} // namespace oor::model
