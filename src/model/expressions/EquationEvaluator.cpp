#include "oor/model/expressions/EquationEvaluator.hpp"

#include "oor/base/errors/Errors.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <numbers>
#include <string>
#include <unordered_map>

namespace oor::model
{

namespace
{

// The constants of the weighting curve of DBA and INVDBA: the frequency response of the human ear, the
// A weighting.
constexpr double WEIGHT_K1 = 2.242882e16;
constexpr double WEIGHT_K3 = 1.562339;
constexpr double WEIGHT_P1 = 20.598997;
constexpr double WEIGHT_P2 = 107.65265;
constexpr double WEIGHT_P3 = 737.86223;
constexpr double WEIGHT_P4 = 12194.22;

// The two factors Ta1 and Ta2 of the weighting at a frequency.
struct Weighting
{
    double ta1;
    double ta2;
};

Weighting weightingAt(double frequency) noexcept
{
    const double f2 = frequency * frequency;
    const double f4 = f2 * f2;
    const double ta1 = WEIGHT_K3 * f4 / ((f2 + WEIGHT_P2 * WEIGHT_P2) * (f2 + WEIGHT_P3 * WEIGHT_P3));
    const double ta2 =
        WEIGHT_K1 * f4 / (std::pow(f2 + WEIGHT_P1 * WEIGHT_P1, 2.0) * std::pow(f2 + WEIGHT_P4 * WEIGHT_P4, 2.0));
    return Weighting{ta1, ta2};
}

// A number, an integer (written without a point) or a real, at one point or at every point. A value with one entry is
// the same at every point. A view reads the numbers of the caller, an owned value has its own.
class Operand
{
public:
    static Operand scalar(double value, bool isInteger)
    {
        Operand operand;
        operand.m_owned.assign(1, value);
        operand.m_ownsData = true;
        operand.m_isInteger = isInteger;
        return operand;
    }

    static Operand view(std::span<const double> values)
    {
        Operand operand;
        operand.m_external = values;
        return operand;
    }

    static Operand zeros(std::size_t count)
    {
        Operand operand;
        operand.m_owned.assign(count, 0.0);
        operand.m_ownsData = true;
        return operand;
    }

    [[nodiscard]] std::size_t size() const noexcept { return m_ownsData ? m_owned.size() : m_external.size(); }
    [[nodiscard]] const double* data() const noexcept { return m_ownsData ? m_owned.data() : m_external.data(); }
    [[nodiscard]] double* writable() noexcept { return m_owned.data(); }
    [[nodiscard]] bool isInteger() const noexcept { return m_isInteger; }
    void setInteger(bool isInteger) noexcept { m_isInteger = isInteger; }
    // The value at a point; a value with one entry is the value at every point.
    [[nodiscard]] double at(std::size_t point) const noexcept { return size() == 1 ? data()[0] : data()[point]; }

    // A copy that owns its numbers, for a value that is kept (the variable an equation assigns).
    [[nodiscard]] Operand owned() const
    {
        Operand copy;
        copy.m_owned.assign(data(), data() + size());
        copy.m_ownsData = true;
        copy.m_isInteger = false; // every variable is real
        return copy;
    }

private:
    std::vector<double> m_owned;
    std::span<const double> m_external;
    bool m_ownsData = false;
    bool m_isInteger = false;
};

// The number of points of an operation on operands of these sizes: one entry stands for every point, otherwise the
// sizes must agree.
std::size_t pointCountOf(std::size_t left, std::size_t right)
{
    if (left == 1)
        return right;
    if (right == 1 || right == left)
        return left;
    throw base::UsageError("the arguments of an equation have " + std::to_string(left) + " and " +
                           std::to_string(right) + " values: give one value or the same number of them");
}

// The integer power of integers: Fortran gives an integer, so 2**-1 is zero and 1**-1 is one.
double integerPower(double base, double exponent)
{
    if (exponent >= 0.0)
        return std::round(std::pow(base, exponent));
    if (base == 0.0)
        throw base::UsageError("an equation divides by zero: 0 to a negative integer power");
    if (base == 1.0)
        return 1.0;
    if (base == -1.0)
        return std::fmod(exponent, 2.0) == 0.0 ? 1.0 : -1.0;
    return 0.0;
}

class Evaluation
{
public:
    Evaluation(const EquationSet& set, std::unordered_map<std::string, Operand> variables)
        : m_set(set), m_variables(std::move(variables))
    {
    }

    // The value of the last equation, which owns its numbers. Each equation gives a variable: its value is copied
    // before it is stored, because the expression may be nothing but a view of the variable it replaces.
    [[nodiscard]] Operand run()
    {
        for (const Equation& equation : m_set.equations())
            m_variables[equation.target] = evaluate(equation.root).owned();
        return m_variables.at(m_set.equations().back().target);
    }

private:
    [[nodiscard]] Operand evaluate(std::int32_t index) const
    {
        const ExpressionNode& node = m_set.node(index);
        switch (node.kind)
        {
        case ExpressionKind::Number:
            return Operand::scalar(node.number, node.isInteger);
        case ExpressionKind::Variable:
            return variable(node.text);
        case ExpressionKind::Negate:
            return negate(evaluate(node.first));
        case ExpressionKind::Binary:
            return binary(node.op, evaluate(node.first), evaluate(node.second));
        case ExpressionKind::Call:
            break;
        }
        return call(node);
    }

    [[nodiscard]] Operand variable(const std::string& name) const
    {
        const auto found = m_variables.find(name);
        if (found != m_variables.end())
            return Operand::view(std::span<const double>(found->second.data(), found->second.size()));
        if (name == "PI")
            return Operand::scalar(std::numbers::pi, false);
        throw base::UsageError("the variable " + name + " has no value");
    }

    [[nodiscard]] static Operand negate(const Operand& operand)
    {
        Operand result = Operand::zeros(operand.size());
        for (std::size_t point = 0; point < operand.size(); ++point)
            result.writable()[point] = -operand.data()[point];
        result.setInteger(operand.isInteger());
        return result;
    }

    [[nodiscard]] static Operand binary(BinaryOperator op, const Operand& left, const Operand& right)
    {
        const std::size_t count = pointCountOf(left.size(), right.size());
        const bool integers = left.isInteger() && right.isInteger();
        Operand result = Operand::zeros(count);
        double* out = result.writable();
        for (std::size_t point = 0; point < count; ++point)
        {
            const double a = left.at(point);
            const double b = right.at(point);
            switch (op)
            {
            case BinaryOperator::Add:
                out[point] = a + b;
                break;
            case BinaryOperator::Subtract:
                out[point] = a - b;
                break;
            case BinaryOperator::Multiply:
                out[point] = a * b;
                break;
            case BinaryOperator::Divide:
                if (integers)
                {
                    if (b == 0.0)
                        throw base::UsageError("an equation divides one integer by zero");
                    out[point] = std::trunc(a / b);
                }
                else
                {
                    out[point] = a / b;
                }
                break;
            case BinaryOperator::Power:
                out[point] = integers ? integerPower(a, b) : std::pow(a, b);
                break;
            }
        }
        result.setInteger(integers);
        return result;
    }

    // A function of one argument, applied at every point.
    template <typename Function>
    [[nodiscard]] static Operand apply1(const Operand& a, Function function)
    {
        Operand result = Operand::zeros(a.size());
        for (std::size_t point = 0; point < a.size(); ++point)
            result.writable()[point] = function(a.data()[point]);
        return result;
    }

    template <typename Function>
    [[nodiscard]] static Operand apply2(const Operand& a, const Operand& b, Function function)
    {
        const std::size_t count = pointCountOf(a.size(), b.size());
        Operand result = Operand::zeros(count);
        for (std::size_t point = 0; point < count; ++point)
            result.writable()[point] = function(a.at(point), b.at(point));
        return result;
    }

    template <typename Function>
    [[nodiscard]] static Operand apply3(const Operand& a, const Operand& b, const Operand& c, Function function)
    {
        const std::size_t count = pointCountOf(pointCountOf(a.size(), b.size()), c.size());
        Operand result = Operand::zeros(count);
        for (std::size_t point = 0; point < count; ++point)
            result.writable()[point] = function(a.at(point), b.at(point), c.at(point));
        return result;
    }

    // The functions that combine all their arguments (SUM, AVG, SSQ, RSS, MAX, MIN) at each point: `step` folds an
    // argument into the accumulator, `finish` turns the accumulator into the value (the number of arguments is given).
    template <typename Step, typename Finish>
    [[nodiscard]] static Operand fold(const std::vector<Operand>& arguments, double initial, Step step, Finish finish)
    {
        std::size_t count = 1;
        for (const Operand& argument : arguments)
            count = pointCountOf(count, argument.size());
        Operand result = Operand::zeros(count);
        for (std::size_t point = 0; point < count; ++point)
        {
            double accumulator = initial;
            for (std::size_t index = 0; index < arguments.size(); ++index)
                accumulator = step(accumulator, arguments[index].at(point), index);
            result.writable()[point] = finish(accumulator, arguments.size());
        }
        return result;
    }

    [[nodiscard]] Operand call(const ExpressionNode& node) const
    {
        std::vector<Operand> arguments;
        arguments.reserve(node.arguments.size());
        for (const std::int32_t argument : node.arguments)
        {
            Operand value = evaluate(argument);
            value.setInteger(false); // a function takes and gives reals
            arguments.push_back(std::move(value));
        }
        return applyFunction(node.function, arguments);
    }

    [[nodiscard]] static Operand applyFunction(EquationFunction function, const std::vector<Operand>& a)
    {
        switch (function)
        {
        case EquationFunction::Abs:
            return apply1(a[0], [](double x) { return std::fabs(x); });
        case EquationFunction::Acos:
            return apply1(a[0], [](double x) { return std::acos(x); });
        case EquationFunction::Acosh:
            return apply1(a[0], [](double x) { return std::acosh(x); });
        case EquationFunction::Asin:
            return apply1(a[0], [](double x) { return std::asin(x); });
        case EquationFunction::Asinh:
            return apply1(a[0], [](double x) { return std::asinh(x); });
        case EquationFunction::Atan:
            return apply1(a[0], [](double x) { return std::atan(x); });
        case EquationFunction::Atan2:
            return apply2(a[0], a[1], [](double x, double y) { return std::atan2(x, y); });
        case EquationFunction::Atanh:
            return apply1(a[0], [](double x) { return std::atanh(x); });
        case EquationFunction::Atanh2:
            return apply2(a[0], a[1], [](double x, double y) { return std::atanh(x / y); });
        case EquationFunction::Avg:
            return fold(
                a, 0.0, [](double sum, double x, std::size_t) { return sum + x; },
                [](double sum, std::size_t count) { return sum / static_cast<double>(count); });
        case EquationFunction::Cos:
            return apply1(a[0], [](double x) { return std::cos(x); });
        case EquationFunction::Cosh:
            return apply1(a[0], [](double x) { return std::cosh(x); });
        case EquationFunction::Db:
            return apply2(a[0], a[1], [](double p, double pref) { return 20.0 * std::log10(p / pref); });
        case EquationFunction::Dba:
            return apply3(a[0], a[1], a[2],
                          [](double p, double pref, double f)
                          {
                              const Weighting weighting = weightingAt(f);
                              return 20.0 * std::log10(p / pref) + 10.0 * std::log10(weighting.ta1) +
                                     10.0 * std::log10(weighting.ta2);
                          });
        case EquationFunction::Dim:
            return apply2(a[0], a[1], [](double x, double y) { return x - std::min(x, y); });
        case EquationFunction::Exp:
            return apply1(a[0], [](double x) { return std::exp(x); });
        case EquationFunction::Invdb:
            return apply2(a[0], a[1],
                          [](double db, double pref) { return std::pow(10.0, db / 20.0 + std::log10(pref)); });
        case EquationFunction::Invdba:
            // The format reference formula has no PREF in it (it is in INVDB): the argument is read, as the format has
            // it, and not used.
            return apply3(a[0], a[1], a[2],
                          [](double dba, double /*pref*/, double f)
                          {
                              const Weighting weighting = weightingAt(f);
                              return std::pow(
                                  10.0,
                                  (dba - 10.0 * std::log10(weighting.ta1) - 10.0 * std::log10(weighting.ta2)) / 20.0);
                          });
        case EquationFunction::Log:
            return apply1(a[0], [](double x) { return std::log(x); });
        case EquationFunction::Log10:
            return apply1(a[0], [](double x) { return std::log10(x); });
        case EquationFunction::Logx:
            return apply2(a[0], a[1], [](double x, double y) { return std::log(y) / std::log(x); });
        case EquationFunction::Max:
            return fold(
                a, 0.0, [](double best, double x, std::size_t index) { return index == 0 ? x : std::max(best, x); },
                [](double best, std::size_t) { return best; });
        case EquationFunction::Min:
            return fold(
                a, 0.0, [](double best, double x, std::size_t index) { return index == 0 ? x : std::min(best, x); },
                [](double best, std::size_t) { return best; });
        case EquationFunction::Mod:
            // x - y * INT(x / y): the remainder has the sign of x.
            return apply2(a[0], a[1], [](double x, double y) { return x - y * std::trunc(x / y); });
        case EquationFunction::Pi:
            return apply1(a[0], [](double x) { return x * std::numbers::pi; });
        case EquationFunction::Rss:
            return fold(
                a, 0.0, [](double sum, double x, std::size_t) { return sum + x * x; },
                [](double sum, std::size_t) { return std::sqrt(sum); });
        case EquationFunction::Sin:
            return apply1(a[0], [](double x) { return std::sin(x); });
        case EquationFunction::Sinh:
            return apply1(a[0], [](double x) { return std::sinh(x); });
        case EquationFunction::Sqrt:
            return apply1(a[0], [](double x) { return std::sqrt(x); });
        case EquationFunction::Ssq:
            return fold(
                a, 0.0, [](double sum, double x, std::size_t) { return sum + x * x; },
                [](double sum, std::size_t) { return sum; });
        case EquationFunction::Sum:
            return fold(
                a, 0.0, [](double sum, double x, std::size_t) { return sum + x; },
                [](double sum, std::size_t) { return sum; });
        case EquationFunction::Tan:
            return apply1(a[0], [](double x) { return std::tan(x); });
        case EquationFunction::Tanh:
            return apply1(a[0], [](double x) { return std::tanh(x); });
        }
        throw base::UsageError("an equation uses a function the evaluator does not know");
    }

    const EquationSet& m_set;
    std::unordered_map<std::string, Operand> m_variables;
};

std::string upperCase(std::string_view text)
{
    std::string result(text);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char character) { return static_cast<char>(std::toupper(character)); });
    return result;
}

} // namespace

std::vector<double> evaluateEquations(const EquationSet& equations, std::span<const EquationVariable> variables)
{
    std::unordered_map<std::string, Operand> bound;
    std::size_t count = 1;
    for (const EquationVariable& variable : variables)
    {
        std::string name = upperCase(variable.name);
        if (std::find(equations.parameters().begin(), equations.parameters().end(), name) ==
            equations.parameters().end())
            throw base::UsageError("the equation " + equations.functionName() + " has no argument " + name);
        if (bound.contains(name))
            throw base::UsageError("the argument " + name + " is given twice");
        count = pointCountOf(count, variable.values.size());
        bound.emplace(std::move(name), Operand::view(variable.values));
    }
    for (const std::string& parameter : equations.parameters())
    {
        if (!bound.contains(parameter))
            throw base::UsageError("the argument " + parameter + " of the equation " + equations.functionName() +
                                   " has no value");
    }
    const Operand value = Evaluation(equations, std::move(bound)).run();
    std::vector<double> result(count);
    for (std::size_t point = 0; point < count; ++point)
        result[point] = value.at(point);
    return result;
}

double evaluateEquations(const EquationSet& equations, std::span<const double> arguments)
{
    const std::vector<std::string>& parameters = equations.parameters();
    if (arguments.size() != parameters.size())
        throw base::UsageError("the equation " + equations.functionName() + " takes " +
                               std::to_string(parameters.size()) + " arguments, not " +
                               std::to_string(arguments.size()));
    std::vector<EquationVariable> variables;
    variables.reserve(parameters.size());
    for (std::size_t index = 0; index < parameters.size(); ++index)
        variables.push_back(EquationVariable{parameters[index], arguments.subspan(index, 1)});
    return evaluateEquations(equations, std::span<const EquationVariable>(variables)).front();
}

} // namespace oor::model
