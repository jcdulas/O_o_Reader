#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/matrices/MatrixCards.hpp"
#include "oor/model/cards/matrices/MatrixName.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t1_inventory/InventoryQueries.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace oor::query
{

namespace
{

// The card types of the direct matrix input the query reads (DMI, DMIG, DMIJ, DMIJI, DMIK), in the order of its rows.
constexpr std::string_view DIRECT_MATRIX = "DirectMatrix";
constexpr std::string_view DIRECT_MATRIX_NODAL = "DirectMatrixNodal";
constexpr std::string_view DIRECT_MATRIX_AERO_J = "DirectMatrixAeroJ";
constexpr std::string_view DIRECT_MATRIX_AERO_J_INTERFERENCE = "DirectMatrixAeroJInterference";
constexpr std::string_view DIRECT_MATRIX_AERO_K = "DirectMatrixAeroK";
constexpr std::array<std::string_view, 5> CARDS{DIRECT_MATRIX, DIRECT_MATRIX_NODAL, DIRECT_MATRIX_AERO_J,
                                                DIRECT_MATRIX_AERO_J_INTERFERENCE, DIRECT_MATRIX_AERO_K};

// The matrices of one card type of the model (the names asked for, all when empty).
std::vector<model::CooMatrix> matricesOf(const model::Model& model, std::string_view card,
                                         const std::vector<std::string>& names)
{
    if (card == DIRECT_MATRIX)
        return model::directMatrices(model, names);
    if (card == DIRECT_MATRIX_NODAL)
        return model::pointMatrices(model, model::PointMatrixCard::Nodal, names);
    if (card == DIRECT_MATRIX_AERO_J)
        return model::pointMatrices(model, model::PointMatrixCard::AeroJ, names);
    if (card == DIRECT_MATRIX_AERO_J_INTERFERENCE)
        return model::pointMatrices(model, model::PointMatrixCard::AeroJInterference, names);
    return model::pointMatrices(model, model::PointMatrixCard::AeroK, names);
}

// The matrices a solution reads by a fixed name, by their role: the downwash and the forces of the j-set (a DMI, DMIJ
// or DMIJI named W2GJ or FA2J), the weights and factors of the ks-set (a DMI or DMIK named WKK or WTFACT). A matrix
// takes its role by the name the format gives it or by the neutral name of the role, in any case.
struct FixedMatrix
{
    std::string_view role;
    bool downwashCards; // a DMI, DMIJ or DMIJI; else a DMI or DMIK
};

constexpr std::array<FixedMatrix, 4> FIXED_MATRICES{{
    {"DownwashMatrix", true},
    {"AeroPressureMatrix", true},
    {"AeroWeightingMatrix", false},
    {"AeroWeightingFactor", false},
}};

// The role of a matrix of `card` named `name`, if it has one.
std::optional<std::string_view> roleOf(std::string_view card, std::string_view name)
{
    const bool readsDownwash =
        card == DIRECT_MATRIX || card == DIRECT_MATRIX_AERO_J || card == DIRECT_MATRIX_AERO_J_INTERFERENCE;
    const bool readsWeights = card == DIRECT_MATRIX || card == DIRECT_MATRIX_AERO_K;
    for (const FixedMatrix& fixed : FIXED_MATRICES)
    {
        const bool named = name == fixed.role;
        if (named && (fixed.downwashCards ? readsDownwash : readsWeights))
            return fixed.role;
    }
    return std::nullopt;
}

// directMatricesTable: the matrices of the direct matrix input cards, as sparse matrices.
class DirectMatricesTableQuery final : public QueryBase
{
public:
    DirectMatricesTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::string detail = run.arguments.textOr("detail", "terms");
        if (detail != "terms" && detail != "matrices")
            throw QueryError("directMatricesTable: detail is \"terms\" or \"matrices\", not \"" + detail + "\"");
        const bool full = run.arguments.boolOr("full", false);
        std::vector<std::string> names;
        for (const std::string& name : run.arguments.texts("names"))
            names.push_back(model::matrixNameText(name));
        const model::Model& model = *run.model;
        std::set<std::string_view> cards;
        for (const std::string& asked : run.arguments.texts("cards"))
        {
            // a card type, or its keyword in any case
            const std::string_view card = asked;
            const auto found = std::find(CARDS.begin(), CARDS.end(), card);
            if (found == CARDS.end())
                throw QueryError(
                    "directMatricesTable: cards are the card types DirectMatrix, DirectMatrixNodal, "
                    "DirectMatrixAeroJ, DirectMatrixAeroJInterference and DirectMatrixAeroK, not \"" +
                    asked + "\"");
            cards.insert(*found);
        }
        // a DMI of a fixed name is used in place of a DMIJ, DMIJI or DMIK of that name
        const std::vector<std::string> directNames = model::directMatrixNames(model);

        std::vector<model::CooMatrix> matrices;
        for (const std::string_view card : CARDS)
        {
            if (!cards.empty() && !cards.contains(card))
                continue;
            run.context.checkCancelled();
            for (model::CooMatrix& matrix : matricesOf(model, card, names))
                matrices.push_back(full ? model::madeMatrix(matrix) : std::move(matrix));
        }
        if (detail == "matrices")
            return matrixRows(matrices, directNames);
        return termRows(matrices);
    }

private:
    [[nodiscard]] std::shared_ptr<const QueryTable> termRows(const std::vector<model::CooMatrix>& matrices) const
    {
        std::size_t count = 0;
        for (const model::CooMatrix& matrix : matrices)
            count += matrix.termCount();
        if (count == 0)
            return emptyTable(info().defaultColumns());
        std::vector<std::string_view> name;
        std::vector<std::string_view> card;
        std::vector<std::int32_t> row;
        std::vector<std::int32_t> column;
        std::vector<double> real;
        std::vector<double> imag;
        std::vector<std::int32_t> rowId;
        std::vector<std::int32_t> rowComponent;
        std::vector<std::int32_t> columnId;
        std::vector<std::int32_t> columnComponent;
        for (auto* vector : {&row, &column, &rowId, &rowComponent, &columnId, &columnComponent})
            vector->reserve(count);
        name.reserve(count);
        card.reserve(count);
        real.reserve(count);
        imag.reserve(count);
        for (const model::CooMatrix& matrix : matrices)
        {
            const bool labelled = !matrix.rowId.empty() || !matrix.columnId.empty();
            for (std::size_t index = 0; index < matrix.termCount(); ++index)
            {
                const std::int32_t r = matrix.row[index];
                const std::int32_t c = matrix.column[index];
                name.emplace_back(matrix.name);
                card.emplace_back(matrix.card);
                row.push_back(r);
                column.push_back(c);
                real.push_back(matrix.real[index]);
                imag.push_back(matrix.complex ? matrix.imag[index] : 0.0);
                // a DMI is indexed by its row and column numbers, a matrix at points by the labels of its degrees of
                // freedom
                rowId.push_back(labelled ? matrix.rowId[static_cast<std::size_t>(r)] : r + 1);
                rowComponent.push_back(labelled ? matrix.rowComponent[static_cast<std::size_t>(r)] : 0);
                columnId.push_back(labelled ? matrix.columnId[static_cast<std::size_t>(c)] : c + 1);
                columnComponent.push_back(labelled ? matrix.columnComponent[static_cast<std::size_t>(c)] : 0);
            }
        }
        TableBuilder builder;
        builder.dictionary("name", name);
        builder.dictionary("card", card);
        builder.int32("row", std::move(row));
        builder.int32("column", std::move(column));
        builder.real("real", std::move(real));
        builder.real("imag", std::move(imag));
        builder.int32("rowId", std::move(rowId));
        builder.int32("rowComponent", std::move(rowComponent));
        builder.int32("columnId", std::move(columnId));
        builder.int32("columnComponent", std::move(columnComponent));
        return builder.finish();
    }

    [[nodiscard]] std::shared_ptr<const QueryTable> matrixRows(const std::vector<model::CooMatrix>& matrices,
                                                               const std::vector<std::string>& directNames) const
    {
        if (matrices.empty())
            return emptyTable(info().columnSets[1]);
        std::vector<std::string_view> name;
        std::vector<std::string_view> card;
        std::vector<std::string_view> form;
        std::vector<std::int32_t> formCode;
        std::vector<std::int32_t> inputTypes;
        std::vector<std::int32_t> outputTypes;
        std::vector<std::uint8_t> polar;
        std::vector<std::uint8_t> complex;
        std::vector<std::int64_t> rowCount;
        std::vector<std::int64_t> columnCount;
        std::vector<std::int64_t> termCount;
        std::vector<std::string_view> role;
        std::vector<std::uint8_t> roleValid;
        std::vector<std::uint8_t> inEffect;
        for (const model::CooMatrix& matrix : matrices)
        {
            name.emplace_back(matrix.name);
            card.emplace_back(matrix.card);
            form.push_back(model::matrixFormName(matrix.form, matrix.card));
            formCode.push_back(matrix.form);
            inputTypes.push_back(matrix.inputType);
            outputTypes.push_back(matrix.outputType);
            polar.push_back(matrix.polar ? 1 : 0);
            complex.push_back(matrix.complex ? 1 : 0);
            rowCount.push_back(matrix.rowCount);
            columnCount.push_back(matrix.columnCount);
            termCount.push_back(static_cast<std::int64_t>(matrix.termCount()));
            const std::optional<std::string_view> fixed = roleOf(matrix.card, matrix.name);
            role.push_back(fixed.value_or(std::string_view()));
            roleValid.push_back(fixed ? 1 : 0);
            const bool shadowed = fixed && matrix.card != DIRECT_MATRIX &&
                                  std::find(directNames.begin(), directNames.end(), matrix.name) != directNames.end();
            inEffect.push_back(shadowed ? 0 : 1);
        }
        TableBuilder builder;
        builder.dictionary("name", name);
        builder.dictionary("card", card);
        builder.dictionary("form", form);
        builder.int32("formCode", std::move(formCode));
        builder.int32("inputType", std::move(inputTypes));
        builder.int32("outputType", std::move(outputTypes));
        builder.flag("polar", polar);
        builder.flag("isComplex", complex);
        builder.int64("rowCount", std::move(rowCount));
        builder.int64("columnCount", std::move(columnCount));
        builder.int64("termCount", std::move(termCount));
        builder.dictionary("role", role, std::move(roleValid));
        builder.flag("inEffect", inEffect);
        return builder.finish();
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "directMatricesTable";
        info.family = "T1";
        info.description =
            "The matrices of the direct matrix input cards DMI, DMIG, DMIJ, DMIJI and DMIK (the downwash "
            "and pressure matrices of a static aeroelastic trim) as sparse matrices: one row a term, in the order of "
            "the cards "
            "(DMI, DMIG, DMIJ, DMIJI, DMIK), of their headers in the deck, and of the terms of each matrix in the "
            "deck. row and column are from 0 (the coordinate form of directMatrices and pointMatrices: "
            "scipy.sparse.coo_array((real + 1j * imag, (row, column)), shape) with the shape of detail=\"matrices\"); "
            "imag is 0 for a real matrix, and a matrix given as magnitude and phase (POLAR) is given as real and "
            "imaginary parts. A DMI is indexed by numbers: rowId and columnId are its row I and column J (from 1), "
            "the components 0; its columns are expanded as the format reference reads them (the groups of each row "
            "number, Through, "
            "the two fields of a complex element; a blank field skipped, not a zero), and a matrix whose column is "
            "given twice, whose rows go back or whose terms fall outside M or N is refused with the reason. A matrix "
            "at points (the layout of DMIG) is indexed by its degrees of freedom, numbered in the order of their sort "
            "(point, component), one list for the rows and columns of a square or symmetric matrix; rowId, "
            "rowComponent, columnId and columnComponent are the point and component (for IFO 9 with NCOL the "
            "column numbers, component 0; for a DMIJ and DMIJI the boxes of the js-set). The terms are those the "
            "cards store; full=True gives the matrix the format reference makes: a diagonal DMI (FORM 3) on the "
            "diagonal of an M x "
            "M matrix, an identity (FORM 8) with its M ones, a symmetric matrix at points (IFO 6, one triangle given) "
            "with both triangles. names keeps the matrices of these names, cards those of these cards (a matrix not "
            "kept is not read). `card` is the card type (DirectMatrix for a DMI, DirectMatrixNodal for a DMIG, "
            "DirectMatrixAeroJ, DirectMatrixAeroJInterference, DirectMatrixAeroK). detail=\"matrices\" gives a row a "
            "matrix: its form (for a DMI square, rectangular, "
            "diagonal, lower or upper triangular factor, symmetric, identity; for a matrix at points square, "
            "symmetric, rectangular) and formCode (FORM or IFO), TIN, TOUT, polar, isComplex, the shape (rowCount, "
            "columnCount) and termCount of the terms given; role is the role of a matrix a solution reads by "
            "its fixed name (DownwashMatrix and AeroPressureMatrix for a DMI, DMIJ or DMIJI named W2GJ or FA2J; "
            "AeroWeightingMatrix and AeroWeightingFactor for a DMI or DMIK named WKK or WTFACT, or named by the role; "
            "no Case Control command or PARAM selects them) and inEffect is false for a DMIJ, DMIJI or DMIK of such a "
            "name that a DMI also has (the "
            "DMI is used). DMIAX is not here (directMatrixAxisymCoo gives "
            "it).";
        info.detailParameter = "detail";
        info.parameters = {
            parameter("names", ParameterKind::TextList, "None", "keep the matrices of these names"),
            parameter("cards", ParameterKind::TextList, "None",
                      "keep the matrices of these card types or keywords (DirectMatrixNodal or DMIG, ...)"),
            parameter("full", ParameterKind::Bool, "False",
                      "the matrix the format reference makes (a diagonal, an identity, both triangles of a symmetric "
                      "matrix)"),
            parameter("detail", ParameterKind::Text, "\"terms\"",
                      "\"terms\" (a row a term) or \"matrices\" (a row a matrix)")};
        info.defaultSort = "the cards (DMI, DMIG, DMIJ, DMIJI, DMIK), their headers in the deck, the terms in the deck";
        ColumnListBuilder terms;
        terms.dictionary("name").dictionary("card").int32("row").int32("column").real("real").real("imag");
        terms.int32("rowId").int32("rowComponent").int32("columnId").int32("columnComponent");
        info.columnSets.push_back(terms.build());
        ColumnListBuilder matrices;
        matrices.dictionary("name")
            .dictionary("card")
            .dictionary("form")
            .int32("formCode")
            .int32("inputType")
            .int32("outputType");
        matrices.flag("polar").flag("isComplex").int64("rowCount").int64("columnCount").int64("termCount");
        matrices.dictionary("role").flag("inEffect");
        info.columnSets.push_back(matrices.build("matrices"));
        return info;
    }
};

} // namespace

void addDirectMatricesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DirectMatricesTableQuery>());
}

} // namespace oor::query
