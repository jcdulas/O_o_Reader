#pragma once

#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

// What the eight table cards (TABLED1 to TABLED4, TABLEM1 to TABLEM4) have in common: the list of points or of
// coefficients that runs over the continuation lines up to the word ENDT, and the rules of the look-up. Each card still
// lays out its own fields by hand and says which of these it uses.
namespace oor::model
{

// How a table interpolates along an axis (XAXIS and YAXIS of TABLED1 and TABLEM1; the other forms are linear).
enum class TableAxis : std::uint8_t
{
    Linear, // LINEAR (the default)
    Log     // LOG
};

// LINEAR or LOG, in any case; nothing for another text. Blank is not handled here: the caller picks the default.
[[nodiscard]] std::optional<TableAxis> parseTableAxis(std::string_view text);
[[nodiscard]] const char* tableAxisWord(TableAxis axis) noexcept;

// The first field of the data of a table: fields 2 to 8 hold the parameters, the points follow from field 9.
inline constexpr int FIRST_TABLE_DATA_FIELD = 9;

// Reads the points (x1, y1), (x2, y2), ... from `firstField` on, two fields each, up to the field that has ENDT in
// either of its two fields. A pair with SKIP in either field is ignored, and so is a pair with both
// fields blank (a line that stops short). Reports an error when ENDT is
// missing, when something follows it, or when a pair is not two reals.
void readTablePoints(FieldReader& reader, int firstField, std::vector<double>& x, std::vector<double>& y);
// Writes the points two fields each from `firstField` on, then ENDT in the next field.
void writeTablePoints(FieldWriter& writer, int firstField, std::span<const double> x, std::span<const double> y);

// Reads the coefficients A0, A1, ... from `firstField` on, one field each, up to the field that has ENDT. A blank
// coefficient is 0.0. Reports an error when ENDT is missing or when something follows it.
void readTableCoefficients(FieldReader& reader, int firstField, std::vector<double>& a);
// Writes the coefficients one field each from `firstField` on, then ENDT in the next field.
void writeTableCoefficients(FieldWriter& writer, int firstField, std::span<const double> a);

// The problem with the points of a table, if there is one: no point, x and y of different lengths, a value that is not
// finite, x not ascending or descending (it may repeat a value to make a jump, but not at the two first
// or the two last points), a logarithmic axis with a value that is not positive.
[[nodiscard]] std::optional<CardProblem> checkTablePoints(std::span<const double> x, std::span<const double> y,
                                                          TableAxis xAxisKind, TableAxis yAxisKind);
// The problem with the parameters and the coefficients of a TABLED4 or TABLEM4: X2 not zero, X3 below X4, at least one
// coefficient, all finite.
[[nodiscard]] std::optional<CardProblem> checkTablePolynomial(double abscissaShift, double abscissaScale,
                                                              double abscissaMinimum, double abscissaMaximum,
                                                              std::span<const double> a);

// The value of the piecewise linear function through the points (x, y) at `at`: interpolation between the two points
// that enclose it and extrapolation, with the same formula, from the two first or the two last points outside.
// With a LOG axis the interpolation is linear in the logarithm of that variable. At a value of x
// that is repeated (a jump) the result is the mean of the two y, the geometric mean when YAXIS is LOG. A table with one
// point is that point's y everywhere. x may be ascending or descending. NaN where the result is not defined: `at`
// is NaN, or is not positive on a LOG axis.
[[nodiscard]] double interpolateTable(std::span<const double> x, std::span<const double> y, TableAxis xAxisKind,
                                      TableAxis yAxisKind, double at) noexcept;

// The value of the polynomial of TABLED4 and TABLEM4: y = sum of Ai * ((x - X1) / X2)^i, where x is first brought into
// [X3, X4] (below X3 it is X3, above X4 it is X4). NaN when `at` is NaN.
[[nodiscard]] double evaluateTablePolynomial(std::span<const double> a, double abscissaShift, double abscissaScale,
                                             double abscissaMinimum, double abscissaMaximum, double at) noexcept;

} // namespace oor::model
