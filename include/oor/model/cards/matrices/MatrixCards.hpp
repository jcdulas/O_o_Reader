#pragma once

#include "oor/model/cards/matrices/directMatrix/DirectMatrixCard.hpp"
#include "oor/model/cards/matrices/directMatrixAxisym/DirectMatrixAxisymCard.hpp"
#include "oor/model/cards/matrices/directMatrixNodal/DirectMatrixNodalCard.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// The matrices of the direct input cards (DMIG, DMIJ, DMIJI, DMIK, DMIAX and DMI) as one object each: a header card and
// the column cards that carry its name, in the order of the deck, turned into the coordinate form (COO) a solver or an
// exporter wants.

// The terms of a matrix of points with their labels: the column (a point and its component, and for DMIAX the
// harmonic) and the row of each term. This is what the cards hold, and what is written back; the index form
// (`CooMatrix`) is derived from it.
struct LabelledTerms
{
    std::int32_t form = 6; // IFO
    std::int32_t inputType = 2;
    std::int32_t outputType = 0;
    std::int32_t polar = 0; // the values were given as amplitude and phase (read converts them)
    std::int32_t columnCount = 0;
    bool complex = false;
    std::vector<base::EntityId> columnId;
    std::vector<std::int32_t> columnComponent;
    std::vector<base::EntityId> rowId;
    std::vector<std::int32_t> rowComponent;
    // DMIAX only (empty for the other cards): the harmonic number of the column and of the row of each term, the value
    // DirectMatrixAxisymCard::NO_HARMONIC when the field is blank.
    std::vector<std::int32_t> columnHarmonic;
    std::vector<std::int32_t> rowHarmonic;
    std::vector<double> real;
    std::vector<double> imag; // as many as `real` when the matrix is complex, else empty

    [[nodiscard]] std::size_t termCount() const noexcept { return real.size(); }
};

// A matrix in coordinate form: the stored terms with 0-based row and column indices, and the label of every row and
// column index (for a matrix of points the point and component of the degree of freedom, and for DMIAX the harmonic
// number too; for a DMI matrix none).
struct CooMatrix
{
    std::string name;
    std::string card; // the card type: "DirectMatrixNodal" (a DMIG), "DirectMatrixAeroJ", ..., "DirectMatrix" (a DMI)
    std::int32_t form = 0;
    std::int32_t inputType = 0;
    std::int32_t outputType = 0;
    bool complex = false;
    bool polar = false; // the amplitude and phase of the file were converted to real and imaginary parts
    std::int64_t rowCount = 0;
    std::int64_t columnCount = 0;
    std::vector<std::int32_t> row;
    std::vector<std::int32_t> column;
    std::vector<double> real;
    std::vector<double> imag; // empty for a real matrix
    std::vector<base::EntityId> rowId;
    std::vector<std::int32_t> rowComponent;
    std::vector<base::EntityId> columnId;
    std::vector<std::int32_t> columnComponent;
    std::vector<std::int32_t> rowHarmonic; // DMIAX only
    std::vector<std::int32_t> columnHarmonic;

    [[nodiscard]] std::size_t termCount() const noexcept { return real.size(); }
};

// The names of the matrices that have a header card, in the order of the deck (once each).
[[nodiscard]] std::vector<std::string> directMatrixNodalNames(const Model& model);
[[nodiscard]] std::vector<std::string> directMatrixNames(const Model& model);

// The terms of the DMIG matrix `name`, from its header and its columns (nullopt when it has no header). An amplitude
// and phase matrix is converted to real and imaginary parts. Complex is decided by the header (TIN 3 or 4).
[[nodiscard]] std::optional<LabelledTerms> directMatrixNodalTerms(const Model& model, std::string_view name);

// The degrees of freedom of a DMIG matrix are numbered in the order of their sort (point, then component), rows and
// columns of a square or symmetric matrix on one list (every degree of freedom that appears as a row or a column), the
// rows and the columns of a rectangular matrix on a list each. A rectangular matrix with NCOL has the columns 1 to NCOL
// (the column index is GJ - 1, the label has component 0). The terms keep the order of the deck. Throws UsageError for
// a column index above NCOL.
[[nodiscard]] CooMatrix directMatrixNodalCoo(std::string_view name, const LabelledTerms& terms);

// The coordinate form of the DMI matrix `name` (nullopt when it has no header): the elements of each column after
// expansion of the groups and the THRU runs, rows and columns from 0, in the order of the deck. A matrix of FORM 3 is a
// column of M elements (N = 1) and one of FORM 8 has no element: they are given as stored. Throws UsageError for a row
// beyond M or a column beyond N, a column given on two cards (a column is one logical entry),
// rows that go back (a group starting above the next row, a THRU ending above its element: the rows of a
// column increase).
[[nodiscard]] std::optional<CooMatrix> directMatrixCoo(const Model& model, std::string_view name);

// Every DMI matrix that has a header card, in the order of the headers (the first header of a name; the columns of a
// name without a header are left out), each as directMatrixCoo gives it, in one pass over the cards; with `names`
// (upper case) only the matrices of those names (a matrix not asked for is not assembled, so its problems do not
// throw).
[[nodiscard]] std::vector<CooMatrix> directMatrices(const Model& model, const std::vector<std::string>& names = {});

// The cards of the DMIG matrix `name` for `terms`: the header, then one column card for each column in the order the
// columns first appear, with its terms in the order given. Throws UsageError with every problem found (a repeated
// element, a point or component out of range, arrays of different lengths).
[[nodiscard]] std::vector<DirectMatrixNodalCard> makeDirectMatrixNodalCards(std::string_view name,
                                                                            const LabelledTerms& terms);

// Adds the matrix to the model as DMIG cards, all or none; the location of the header card. Throws UsageError when a
// DMIG matrix of that name exists or the terms are not valid.
CardLocation addDirectMatrixNodal(Model& model, std::string_view name, const LabelledTerms& terms);

// Removes the cards of the DMIG or DMI matrix; the number of cards removed.
std::size_t removeDirectMatrixNodal(Model& model, std::string_view name);
std::size_t removeDirectMatrix(Model& model, std::string_view name);

// DMIJ, DMIJI and DMIK have the layout of DMIG and the same terms; the matrix of one of them is assembled, stored and
// removed as a DMIG one is, the card named by `kind`.
enum class PointMatrixCard : std::uint8_t
{
    Nodal,             // DirectMatrixNodal
    AeroJ,             // DirectMatrixAeroJ
    AeroJInterference, // DirectMatrixAeroJInterference
    AeroK              // DirectMatrixAeroK
};

// The card type of `kind` ("DirectMatrixNodal" for a DMIG).
[[nodiscard]] std::string_view pointMatrixCardName(PointMatrixCard kind) noexcept;
[[nodiscard]] std::vector<std::string> pointMatrixNames(const Model& model, PointMatrixCard kind);
[[nodiscard]] std::optional<LabelledTerms> pointMatrixTerms(const Model& model, PointMatrixCard kind,
                                                            std::string_view name);
[[nodiscard]] CooMatrix pointMatrixCoo(PointMatrixCard kind, std::string_view name, const LabelledTerms& terms);
CardLocation addPointMatrix(Model& model, PointMatrixCard kind, std::string_view name, const LabelledTerms& terms);
std::size_t removePointMatrix(Model& model, PointMatrixCard kind, std::string_view name);
// Every matrix of the card `kind` that has a header card, in the order of the headers, each as pointMatrixCoo gives it,
// in one pass over the cards; with `names` (upper case) only the matrices of those names.
[[nodiscard]] std::vector<CooMatrix> pointMatrices(const Model& model, PointMatrixCard kind,
                                                   const std::vector<std::string>& names = {});

// The name of the form of a matrix of the card type `card` (CooMatrix::card): for a DMI square, rectangular, diagonal,
// lower triangular factor, upper triangular factor, symmetric, identity (FORM 1 to 6 and 8; "unknown" otherwise); for
// the matrices at points square (IFO 1), symmetric (6), rectangular (any other).
[[nodiscard]] std::string_view matrixFormName(std::int32_t form, std::string_view card) noexcept;

// The matrix the format reference makes from the terms a matrix stores: a diagonal DMI (FORM 3, its M x 1 column of
// elements) on the diagonal of an M x M matrix, an identity DMI (FORM 8, no element) with its M ones, a symmetric
// matrix at points (IFO 6: either triangle given) with both triangles, the mirror of
// each term off the diagonal after the terms given. Any other matrix as it is (a FORM 6 DMI gives the whole matrix).
// Throws UsageError for a symmetric matrix at points that gives an element both above
// and below the diagonal.
[[nodiscard]] CooMatrix madeMatrix(const CooMatrix& stored);

// DMIAX: the names with a header card, the terms (columns and rows labelled by point, component and harmonic) and the
// coordinate form of a matrix. The degrees of freedom are sorted by point, component and harmonic (a blank harmonic
// first); a square or symmetric matrix has one list for its rows and columns.
[[nodiscard]] std::vector<std::string> directMatrixAxisymNames(const Model& model);
[[nodiscard]] std::optional<LabelledTerms> directMatrixAxisymTerms(const Model& model, std::string_view name);
[[nodiscard]] CooMatrix directMatrixAxisymCoo(std::string_view name, const LabelledTerms& terms);
std::size_t removeDirectMatrixAxisym(Model& model, std::string_view name);

} // namespace oor::model
