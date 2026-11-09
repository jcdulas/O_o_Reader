#pragma once

#include "oor/model/cards/matrices/MatrixCards.hpp"
#include "oor/results/core/NamedResult.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

namespace oor::results
{

// Matrices as named results (kind "matrix"): the terms in coordinate form, as the matrices read from a result file are.

// The named result of a matrix assembled from DMIG, DMIJ, DMIJI, DMIK, DMIAX or DMI cards: `shape`, `row`, `column`,
// `value` (float64, or complex128 for a complex matrix) and, for a matrix of points, the labels of the rows and columns
// (`rowIds`, `rowComponents`, `columnIds`, `columnComponents`, and for DMIAX `rowHarmonics` and
// `columnHarmonics`: the harmonic number of the degree of freedom, -2147483648 where the field is blank). Its
// attributes say the card (`card`), the form (`form` and `formCode`), TIN (`typeCode`), TOUT, and `origin` is
// "model". It is `notValidated`.
[[nodiscard]] std::shared_ptr<NamedResult> matrixResultOf(const model::CooMatrix& matrix, std::int32_t subcase = 0);

// How a matrix result is stored as a DMIG matrix.
struct NodalMatrixStorage
{
    // The IFO of the header: 1 square, 2 or 9 rectangular, 6 symmetric. Nothing: the form attribute of the result (a
    // matrix read from a DMIG or a result file says it), else square when the matrix is square and rectangular (9)
    // otherwise.
    std::optional<std::int32_t> form;
    // TIN: nothing means double precision (2, or 4 for a complex matrix).
    std::optional<std::int32_t> inputType;
    std::int32_t outputType = 0;
};

// The labelled terms of a matrix result, ready for `model::addDirectMatrixNodal`. The result must carry the labels of
// its rows and columns (`rowIds`, `rowComponents`, `columnIds`, `columnComponents`): a DMIG matrix is indexed by grid,
// scalar or extra point and component, which an index does not say. For a rectangular matrix stored with IFO 9 the
// column labels are the column numbers (components 0). Throws UsageError with the reason when a matrix cannot be
// stored.
[[nodiscard]] model::LabelledTerms matrixTermsOf(const NamedResult& matrix, const NodalMatrixStorage& storage = {});

} // namespace oor::results
