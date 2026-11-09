#include "oor/model/cards/tables/TableSupport.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace oor::model
{

namespace
{

bool isWord(std::string_view text, std::string_view word)
{
    return (text == word);
}

double notDefined()
{
    return std::numeric_limits<double>::quiet_NaN();
}

bool allFinite(std::span<const double> values)
{
    for (const double value : values)
    {
        if (!std::isfinite(value))
            return false;
    }
    return true;
}

// The point k of a table seen in ascending order of x: a descending table is read from its end.
class AscendingView
{
public:
    AscendingView(std::span<const double> x, std::span<const double> y)
        : m_x(x), m_y(y), m_descending(x.size() > 1 && x.back() < x.front())
    {
    }

    [[nodiscard]] std::size_t size() const { return m_x.size(); }
    [[nodiscard]] double x(std::size_t k) const { return m_x[index(k)]; }
    [[nodiscard]] double y(std::size_t k) const { return m_y[index(k)]; }

private:
    [[nodiscard]] std::size_t index(std::size_t k) const { return m_descending ? m_x.size() - 1 - k : k; }

    std::span<const double> m_x;
    std::span<const double> m_y;
    bool m_descending;
};

// The weights of the two points of a segment for a value `at`, in the scale of the axis: (xj - at) / (xj - xi) for
// a linear axis and ln(xj / at) / ln(xj / xi) for a logarithmic one; the weight of the point j is one minus it.
double lowerWeight(TableAxis axis, double xi, double xj, double at)
{
    if (axis == TableAxis::Log)
        return std::log(xj / at) / std::log(xj / xi);
    return (xj - at) / (xj - xi);
}

double upperWeight(TableAxis axis, double xi, double xj, double at)
{
    if (axis == TableAxis::Log)
        return std::log(at / xi) / std::log(xj / xi);
    return (at - xi) / (xj - xi);
}

// The value on the segment from point i to point j, which also extrapolates it.
double segmentValue(const AscendingView& points, std::size_t i, std::size_t j, TableAxis xAxisKind, TableAxis yAxisKind,
                    double at)
{
    const double wi = lowerWeight(xAxisKind, points.x(i), points.x(j), at);
    const double wj = upperWeight(xAxisKind, points.x(i), points.x(j), at);
    if (yAxisKind == TableAxis::Log)
        return std::exp(wi * std::log(points.y(i)) + wj * std::log(points.y(j)));
    return wi * points.y(i) + wj * points.y(j);
}

} // namespace

std::optional<TableAxis> parseTableAxis(std::string_view text)
{
    if (isWord(text, "Linear"))
        return TableAxis::Linear;
    if (isWord(text, "Logarithmic"))
        return TableAxis::Log;
    return std::nullopt;
}

const char* tableAxisWord(TableAxis axis) noexcept
{
    return axis == TableAxis::Log ? "Logarithmic" : "Linear";
}

void readTablePoints(FieldReader& reader, int firstField, std::vector<double>& x, std::vector<double>& y)
{
    const int last = reader.lastUsedField();
    bool ended = false;
    for (int field = firstField; field <= last && !reader.failed(); field += 2)
    {
        const std::string_view first = reader.text(field);
        const std::string_view second = reader.text(field + 1);
        if (isWord(first, "EndTable") || isWord(second, "EndTable"))
        {
            ended = true;
            if (last > field + 1)
                reader.error("card.bad-value", "nothing may follow the end of the table, ENDT", "tableEnd");
            break;
        }
        if (isWord(first, "Skip") || isWord(second, "Skip"))
            continue;
        // A pair with both fields blank is padding: a line that stops short, before its continuation.
        if (first.empty() && second.empty())
            continue;
        const double xi = reader.requiredReal(field, "x");
        const double yi = reader.requiredReal(field + 1, "y");
        if (reader.failed())
            return;
        x.push_back(xi);
        y.push_back(yi);
    }
    if (!ended && !reader.failed())
        reader.error("card.missing-field", "the end of the table, ENDT, is missing", "tableEnd");
}

void writeTablePoints(FieldWriter& writer, int firstField, std::span<const double> x, std::span<const double> y)
{
    int field = firstField;
    for (std::size_t index = 0; index < x.size() && index < y.size(); ++index)
    {
        writer.real(field, x[index]);
        writer.real(field + 1, y[index]);
        field += 2;
    }
    writer.text(field, "EndTable");
}

void readTableCoefficients(FieldReader& reader, int firstField, std::vector<double>& a)
{
    const int last = reader.lastUsedField();
    bool ended = false;
    for (int field = firstField; field <= last && !reader.failed(); ++field)
    {
        if (isWord(reader.text(field), "EndTable"))
        {
            ended = true;
            if (last > field)
                reader.error("card.bad-value", "nothing may follow the end of the table, ENDT", "tableEnd");
            break;
        }
        a.push_back(reader.real(field, "coefficients", 0.0));
    }
    if (!ended && !reader.failed())
        reader.error("card.missing-field", "the end of the table, ENDT, is missing", "tableEnd");
}

void writeTableCoefficients(FieldWriter& writer, int firstField, std::span<const double> a)
{
    int field = firstField;
    for (const double coefficient : a)
        writer.real(field++, coefficient);
    writer.text(field, "EndTable");
}

std::optional<CardProblem> checkTablePoints(std::span<const double> x, std::span<const double> y, TableAxis xAxisKind,
                                            TableAxis yAxisKind)
{
    if (x.size() != y.size())
        return CardProblem{"card.bad-value", "x and y must have the same number of values", "y"};
    if (x.empty())
        return CardProblem{"card.missing-field", "a table needs at least one point", "x"};
    if (!allFinite(x) || !allFinite(y))
        return CardProblem{"card.bad-value", "the values of a table must be finite numbers", "x"};
    bool rises = false;
    bool falls = false;
    for (std::size_t index = 1; index < x.size(); ++index)
    {
        rises = rises || x[index] > x[index - 1];
        falls = falls || x[index] < x[index - 1];
    }
    if (rises && falls)
        return CardProblem{"card.bad-value", "x must be ascending or descending, not both", "x"};
    // A repeated x is a jump, but the extrapolation needs two different x at each end.
    if (x.size() > 1 && (x[0] == x[1] || x[x.size() - 2] == x[x.size() - 1]))
        return CardProblem{"card.bad-value", "the first two and the last two x must be different", "x"};
    if (xAxisKind == TableAxis::Log)
    {
        for (const double value : x)
        {
            if (!(value > 0.0))
                return CardProblem{"card.bad-value", "x must be positive on a logarithmic axis", "x"};
        }
    }
    if (yAxisKind == TableAxis::Log)
    {
        for (const double value : y)
        {
            if (!(value > 0.0))
                return CardProblem{"card.bad-value", "y must be positive on a logarithmic axis", "y"};
        }
    }
    return std::nullopt;
}

std::optional<CardProblem> checkTablePolynomial(double abscissaShift, double abscissaScale, double abscissaMinimum,
                                                double abscissaMaximum, std::span<const double> a)
{
    if (!std::isfinite(abscissaShift) || !std::isfinite(abscissaScale) || !std::isfinite(abscissaMinimum) ||
        !std::isfinite(abscissaMaximum) || !allFinite(a))
        return CardProblem{"card.bad-value", "the values of a table must be finite numbers", "abscissaShift"};
    if (abscissaScale == 0.0)
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be zero", "abscissaScale"};
    if (!(abscissaMinimum < abscissaMaximum))
        return CardProblem{
            "card.bad-value",
            std::string(CardProblem::FIELD) + " must be below " + CardProblem::fieldNamed("abscissaMaximum"),
            "abscissaMinimum"};
    if (a.empty())
        return CardProblem{"card.missing-field", "a table needs at least one coefficient", "coefficients"};
    return std::nullopt;
}

double interpolateTable(std::span<const double> x, std::span<const double> y, TableAxis xAxisKind, TableAxis yAxisKind,
                        double at) noexcept
{
    const std::size_t count = std::min(x.size(), y.size());
    if (count == 0 || std::isnan(at))
        return notDefined();
    if (count == 1)
        return y[0];
    if (xAxisKind == TableAxis::Log && !(at > 0.0))
        return notDefined();
    const AscendingView points(x.first(count), y.first(count));

    // The first point whose x is not below `at`.
    std::size_t low = 0;
    std::size_t high = count;
    while (low < high)
    {
        const std::size_t middle = low + (high - low) / 2;
        if (points.x(middle) < at)
            low = middle + 1;
        else
            high = middle;
    }
    if (low == 0 && points.x(0) > at)
        return segmentValue(points, 0, 1, xAxisKind, yAxisKind, at); // below the table: the first segment, extended
    if (low == count)
        return segmentValue(points, count - 2, count - 1, xAxisKind, yAxisKind,
                            at); // above: the last segment, extended
    if (points.x(low) == at)
    {
        // On a point; on a jump (two points with this x) the mean of the two values.
        if (low + 1 < count && points.x(low + 1) == at)
        {
            if (yAxisKind == TableAxis::Log)
                return std::sqrt(points.y(low) * points.y(low + 1));
            return 0.5 * (points.y(low) + points.y(low + 1));
        }
        return points.y(low);
    }
    return segmentValue(points, low - 1, low, xAxisKind, yAxisKind, at);
}

double evaluateTablePolynomial(std::span<const double> a, double abscissaShift, double abscissaScale,
                               double abscissaMinimum, double abscissaMaximum, double at) noexcept
{
    if (std::isnan(at))
        return notDefined();
    const double clamped = at < abscissaMinimum ? abscissaMinimum : (at > abscissaMaximum ? abscissaMaximum : at);
    const double t = (clamped - abscissaShift) / abscissaScale;
    double value = 0.0;
    for (std::size_t index = a.size(); index > 0; --index)
        value = value * t + a[index - 1];
    return value;
}

} // namespace oor::model
