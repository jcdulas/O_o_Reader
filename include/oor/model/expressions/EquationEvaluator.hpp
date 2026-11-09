#pragma once

#include "oor/model/expressions/EquationSet.hpp"

#include <span>
#include <string_view>
#include <vector>

namespace oor::model
{

// The values of one argument of a DEQATN: one number, or one number per point when the equation is evaluated for many
// points at once. The name is matched without regard to the case.
struct EquationVariable
{
    std::string_view name;
    std::span<const double> values;
};

// Evaluates the equations. Every argument of the function must be given, and nothing else; the arguments that have
// several values all have the same number of them, and an argument with one value is the same at every point. The
// result has one value for each point (one when no argument has more than one).
//
// The equations are evaluated point by point with the rules of the format reference: the first term of an expression takes its sign
// as a whole, a quantity written without a point is an integer and two integers give an integer, and every function
// gives a real. A real that goes out of the domain of a function (SQRT of a negative number) is NaN, a division of a
// real by zero is infinity, as in any floating point program. The arguments of AVG, MAX, MIN, RSS, SSQ and SUM are
// combined at each point, not across the points.
//
// Throws base::UsageError for an argument that is missing, unknown or has the wrong number of values, and for a
// division by zero of two integers (a number written without a point has no infinity to give).
[[nodiscard]] std::vector<double> evaluateEquations(const EquationSet& equations,
                                                    std::span<const EquationVariable> variables);

// The value of the equations for one set of numbers (the arguments in the order of EquationSet::parameters).
[[nodiscard]] double evaluateEquations(const EquationSet& equations, std::span<const double> arguments);

} // namespace oor::model
