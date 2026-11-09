#include "oor/base/errors/Errors.hpp"
#include "oor/model/cards/matrices/MatrixCards.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using oor::testing::readModelText;
using oor::testing::writeModelText;

namespace
{

const char* const DECK =
    "Node;1;;0.;0.;0.\n"
    "Node;2;;1.;0.;0.\n"
    "Node;3;;2.;0.;0.\n"
    "ScalarPoint;10\n"
    "DirectMatrixNodal;KAA;0;6;2;0\n"
    "DirectMatrixNodal;KAA;1;1;1;1;10.;;1;2;-3.\n"
    "DirectMatrixNodal;KAA;2;1;2;1;40.\n"
    "DirectMatrixNodal;KAA;2;1;3;1;-1.5\n"
    "DirectMatrixNodal;CPX;0;1;3;3;1;2\n"
    "DirectMatrixNodal;CPX;1;1;2;2;2.;90.;1;1;3.;0.\n"
    "DirectMatrixNodal;LOAD;0;9;1;0;;3\n"
    "DirectMatrixNodal;LOAD;3;;1;1;4.;;3;1;5.\n"
    "DirectMatrixNodal;LOAD;1;;2;1;6.\n"
    "DirectMatrix;PNL;0;2;1;1;5;2\n"
    "DirectMatrix;PNL;1;2;7.;2.;9.\n"
    "DirectMatrix;PNL;2;1;4.;5;11.\n"
    "DirectMatrix;THR;0;2;1;1;7;1\n"
    "DirectMatrix;THR;1;1;3.;Through;3;5;2.5\n"
    "DirectMatrix;CPL;0;2;3;3;4;2\n"
    "DirectMatrix;CPL;1;2;4.;1.;2.;0.;4;3.;9.\n"
    "DirectMatrix;CPL;2;1;8.;5.\n";

} // namespace

TEST_CASE("a DMIG matrix is one object: its header and its columns", "[model][matrices]")
{
    const Model model = readModelText(DECK, true);
    CHECK(directMatrixNodalNames(model) == std::vector<std::string>{"KAA", "CPX", "LOAD"});
    const auto terms = directMatrixNodalTerms(model, "KAA");
    REQUIRE(terms.has_value());
    CHECK(terms->form == 6);
    CHECK(terms->inputType == 2);
    CHECK_FALSE(terms->complex);
    CHECK(terms->termCount() == 4);
    CHECK(terms->columnId == std::vector<base::EntityId>{1, 1, 2, 2});
    CHECK(terms->columnComponent == std::vector<std::int32_t>{1, 1, 1, 1});
    CHECK(terms->rowId == std::vector<base::EntityId>{1, 1, 2, 3});
    CHECK(terms->rowComponent == std::vector<std::int32_t>{1, 2, 1, 1});
    CHECK(terms->real == std::vector<double>{10.0, -3.0, 40.0, -1.5});
    CHECK_FALSE(directMatrixNodalTerms(model, "NOPE").has_value());
    CHECK_FALSE(directMatrixNodalTerms(model, "kaa").has_value());
}

TEST_CASE("fixed matrix names keep their neutral spelling through model files and matrix selection",
          "[model][matrices]")
{
    const Model model = readModelText(
        "DirectMatrix;DownwashMatrix;0;2;1;0;2;1\n"
        "DirectMatrix;DownwashMatrix;1;1;0.1;Through;2\n"
        "DirectMatrix;custom;0;2;1;0;1;1\n"
        "DirectMatrix;custom;1;1;0.2\n",
        true);
    CHECK(directMatrixNames(model) == std::vector<std::string>{"DownwashMatrix", "custom"});
    CHECK_FALSE(directMatrixCoo(model, "CUSTOM").has_value());
    const auto matrix = directMatrixCoo(model, "DownwashMatrix");
    REQUIRE(matrix.has_value());
    CHECK(matrix->name == "DownwashMatrix");
    CHECK(matrix->real == std::vector<double>{0.1, 0.1});
    const Model again = readModelText(writeModelText(model), true);
    CHECK(directMatrixNames(again) == directMatrixNames(model));
    CHECK(directMatrixCoo(again, "DownwashMatrix")->real == matrix->real);
    oor::testing::requireModelFileRoundTrip(model);
}

TEST_CASE("the degrees of freedom of a DMIG matrix are numbered in the order of their sort", "[model][matrices]")
{
    const Model model = readModelText(DECK, true);
    const CooMatrix coo = directMatrixNodalCoo("KAA", *directMatrixNodalTerms(model, "KAA"));
    CHECK(coo.rowCount == 4);
    CHECK(coo.columnCount == 4);
    CHECK(coo.rowId == std::vector<base::EntityId>{1, 1, 2, 3});
    CHECK(coo.rowComponent == std::vector<std::int32_t>{1, 2, 1, 1});
    CHECK(coo.row == std::vector<std::int32_t>{0, 1, 2, 3});
    CHECK(coo.column == std::vector<std::int32_t>{0, 0, 2, 2});
    // a rectangular matrix with NCOL: the columns are 1 to NCOL, the rows the degrees of freedom of the terms
    const CooMatrix load = directMatrixNodalCoo("LOAD", *directMatrixNodalTerms(model, "LOAD"));
    CHECK(load.rowCount == 3);
    CHECK(load.columnCount == 3);
    CHECK(load.columnId == std::vector<base::EntityId>{1, 2, 3});
    CHECK(load.column == std::vector<std::int32_t>{2, 2, 0});
    CHECK(load.row == std::vector<std::int32_t>{0, 2, 1});
}

TEST_CASE("an amplitude and phase matrix is converted to real and imaginary parts", "[model][matrices]")
{
    const Model model = readModelText(DECK, true);
    const auto terms = directMatrixNodalTerms(model, "CPX");
    REQUIRE(terms.has_value());
    REQUIRE(terms->complex);
    REQUIRE(terms->imag.size() == 2);
    CHECK(terms->real[0] == Catch::Approx(0.0).margin(1e-12)); // 2 at 90 degrees
    CHECK(terms->imag[0] == Catch::Approx(2.0));
    CHECK(terms->real[1] == Catch::Approx(3.0));
    CHECK(terms->imag[1] == Catch::Approx(0.0).margin(1e-12));
}

TEST_CASE("the columns of a DMI matrix are expanded to elements", "[model][matrices]")
{
    const Model model = readModelText(DECK, true);
    CHECK(directMatrixNames(model) == std::vector<std::string>{"PNL", "THR", "CPL"});
    const CooMatrix real = *directMatrixCoo(model, "PNL");
    CHECK(real.rowCount == 5);
    CHECK(real.columnCount == 2);
    CHECK(real.row == std::vector<std::int32_t>{1, 2, 3, 0, 4});
    CHECK(real.column == std::vector<std::int32_t>{0, 0, 0, 1, 1});
    CHECK(real.real == std::vector<double>{7.0, 2.0, 9.0, 4.0, 11.0});
    // THRU repeats an element down to a row: 3.0 in rows 1 to 3, then 2.5 in row 5
    const CooMatrix thru = *directMatrixCoo(model, "THR");
    CHECK(thru.row == std::vector<std::int32_t>{0, 1, 2, 4});
    CHECK(thru.real == std::vector<double>{3.0, 3.0, 3.0, 2.5});
    // a complex column takes its elements two by two
    const CooMatrix complex = *directMatrixCoo(model, "CPL");
    REQUIRE(complex.complex);
    CHECK(complex.row == std::vector<std::int32_t>{1, 2, 3, 0});
    CHECK(complex.real == std::vector<double>{4.0, 2.0, 3.0, 8.0});
    CHECK(complex.imag == std::vector<double>{1.0, 0.0, 9.0, 5.0});
}

TEST_CASE("a DMI element beyond the matrix is an error", "[model][matrices]")
{
    const Model model = readModelText("DirectMatrix;SMALL;0;2;1;1;2;1\nDirectMatrix;SMALL;1;1;1.;2.;3.\n");
    CHECK_THROWS_AS(directMatrixCoo(model, "SMALL"), base::UsageError);
    const Model wide = readModelText("DirectMatrix;SMALL;0;2;1;1;2;1\nDirectMatrix;SMALL;2;1;1.\n");
    CHECK_THROWS_AS(directMatrixCoo(wide, "SMALL"), base::UsageError);
}

TEST_CASE("a DMI column is one card and its rows increase", "[model][matrices]")
{
    const char* const header = "DirectMatrix;M;0;2;1;1;6;1\n";
    // a column is one logical entry, its terms in increasing row order
    CHECK_THROWS_AS(
        directMatrixCoo(readModelText(std::string(header) + "DirectMatrix;M;1;1;1.\nDirectMatrix;M;1;3;2.\n"), "M"),
        base::UsageError);
    CHECK_THROWS_AS(directMatrixCoo(readModelText(std::string(header) + "DirectMatrix;M;1;3;1.;2.;2;5.\n"), "M"),
                    base::UsageError);
    CHECK_THROWS_AS(directMatrixCoo(readModelText(std::string(header) + "DirectMatrix;M;1;5;1.;Through;3\n"), "M"),
                    base::UsageError);
    // a group that starts at the next row, and a THRU that ends at the row of its element, add nothing
    const CooMatrix same =
        *directMatrixCoo(readModelText(std::string(header) + "DirectMatrix;M;1;1;1.;2;2.;Through;2\n"), "M");
    CHECK(same.row == std::vector<std::int32_t>{0, 1});
    CHECK(same.real == std::vector<double>{1.0, 2.0});
}

TEST_CASE("every matrix of a card is read in one pass, as the matrix of its name", "[model][matrices]")
{
    const Model model = readModelText(DECK, true);
    const std::vector<CooMatrix> directs = directMatrices(model);
    REQUIRE(directs.size() == 3);
    for (const CooMatrix& matrix : directs)
    {
        const CooMatrix one = *directMatrixCoo(model, matrix.name);
        CHECK(matrix.row == one.row);
        CHECK(matrix.column == one.column);
        CHECK(matrix.real == one.real);
        CHECK(matrix.imag == one.imag);
    }
    CHECK(directMatrices(model, {"THR"}).size() == 1);
    const std::vector<CooMatrix> nodalMatrices = pointMatrices(model, PointMatrixCard::Nodal);
    REQUIRE(nodalMatrices.size() == 3);
    CHECK(nodalMatrices[1].name == "CPX");
    const CooMatrix cpx = directMatrixNodalCoo("CPX", *directMatrixNodalTerms(model, "CPX"));
    CHECK(nodalMatrices[1].real == cpx.real);
    CHECK(nodalMatrices[1].imag == cpx.imag);
    CHECK(nodalMatrices[1].rowId == cpx.rowId);
    CHECK(pointMatrices(model, PointMatrixCard::AeroJ).empty());
}

TEST_CASE("the matrix the format reference makes: a diagonal, an identity, both triangles", "[model][matrices]")
{
    const Model model = readModelText(
        "DirectMatrix;D;0;3;1;0;3\nDirectMatrix;D;1;1;2.;3.;4.\nDirectMatrix;I;0;8;1;0;2\n" + std::string(DECK), true);
    const CooMatrix diagonal = madeMatrix(*directMatrixCoo(model, "D"));
    CHECK(diagonal.columnCount == 3);
    CHECK(diagonal.row == std::vector<std::int32_t>{0, 1, 2});
    CHECK(diagonal.column == std::vector<std::int32_t>{0, 1, 2});
    const CooMatrix identity = madeMatrix(*directMatrixCoo(model, "I"));
    CHECK(identity.real == std::vector<double>{1.0, 1.0});
    CHECK(identity.column == std::vector<std::int32_t>{0, 1});
    // KAA (IFO 6): the terms given, then the mirror of those off the diagonal
    const CooMatrix symmetric = madeMatrix(directMatrixNodalCoo("KAA", *directMatrixNodalTerms(model, "KAA")));
    CHECK(symmetric.row == std::vector<std::int32_t>{0, 1, 2, 3, 0, 2});
    CHECK(symmetric.column == std::vector<std::int32_t>{0, 0, 2, 2, 1, 3});
    CHECK(symmetric.real == std::vector<double>{10.0, -3.0, 40.0, -1.5, -3.0, -1.5});
    // a rectangular matrix stays as it is
    const CooMatrix load = directMatrixNodalCoo("LOAD", *directMatrixNodalTerms(model, "LOAD"));
    CHECK(madeMatrix(load).row == load.row);
}

namespace
{

LabelledTerms randomTerms(std::mt19937& generator, std::int32_t size, bool symmetric, bool complex)
{
    LabelledTerms terms;
    terms.form = symmetric ? 6 : 1;
    terms.complex = complex;
    terms.inputType = complex ? 4 : 2;
    std::vector<std::pair<std::int32_t, std::int32_t>> dofs;
    for (std::int32_t id = 1; id <= size; ++id)
        for (std::int32_t component = 1; component <= 3; ++component)
            dofs.emplace_back(id, component);
    std::normal_distribution<double> normal(0.0, 100.0);
    for (std::size_t column = 0; column < dofs.size(); ++column)
        for (std::size_t row = symmetric ? column : 0; row < dofs.size(); ++row)
        {
            if (generator() % 3 != 0 && row != column)
                continue;
            terms.columnId.push_back(dofs[column].first);
            terms.columnComponent.push_back(dofs[column].second);
            terms.rowId.push_back(dofs[row].first);
            terms.rowComponent.push_back(dofs[row].second);
            terms.real.push_back(normal(generator));
            if (complex)
                terms.imag.push_back(normal(generator));
        }
    return terms;
}

} // namespace

TEST_CASE("a matrix stored as DMIG cards reads back to the same terms from a model file", "[model][matrices]")
{
    std::mt19937 generator(7);
    for (const bool symmetric : {false, true})
        for (const bool complex : {false, true})
        {
            const LabelledTerms terms = randomTerms(generator, 4, symmetric, complex);
            Model model;
            (void)addDirectMatrixNodal(model, "krnd", terms);
            const std::size_t cards = model.cardCount();
            const Model again = readModelText(writeModelText(model), true);
            CHECK(again.cardCount() == cards);
            const auto back = directMatrixNodalTerms(again, "krnd");
            REQUIRE(back.has_value());
            CHECK(back->form == terms.form);
            CHECK(back->inputType == terms.inputType);
            CHECK(back->columnId == terms.columnId);
            CHECK(back->columnComponent == terms.columnComponent);
            CHECK(back->rowId == terms.rowId);
            CHECK(back->rowComponent == terms.rowComponent);
            CHECK(back->real == terms.real);
            CHECK(back->imag == terms.imag);
        }
}

TEST_CASE("a refused matrix leaves nothing in the model", "[model][matrices]")
{
    Model model;
    LabelledTerms good;
    good.form = 1;
    good.columnId = {1, 2};
    good.columnComponent = {0, 0};
    good.rowId = {1, 2};
    good.rowComponent = {0, 0};
    good.real = {1.0, 2.0};
    (void)addDirectMatrixNodal(model, "A", good);
    CHECK(model.cardCount() == 3);
    CHECK_THROWS_AS(addDirectMatrixNodal(model, "A", good), base::UsageError); // the name is taken
    LabelledTerms twice = good;
    twice.columnId.push_back(1);
    twice.columnComponent.push_back(0);
    twice.rowId.push_back(1);
    twice.rowComponent.push_back(0);
    twice.real.push_back(3.0);
    CHECK_THROWS_AS(addDirectMatrixNodal(model, "B", twice), base::UsageError); // an element given twice
    LabelledTerms mirrored;
    mirrored.form = 6;
    mirrored.columnId = {1, 2};
    mirrored.columnComponent = {0, 0};
    mirrored.rowId = {2, 1};
    mirrored.rowComponent = {0, 0};
    mirrored.real = {1.0, 1.0};
    CHECK_THROWS_AS(addDirectMatrixNodal(model, "C", mirrored), base::UsageError); // below and above the diagonal
    LabelledTerms badPoint = good;
    badPoint.rowId[0] = -4;
    CHECK_THROWS_AS(addDirectMatrixNodal(model, "D", badPoint), base::UsageError);
    LabelledTerms badType = good;
    badType.inputType = 3; // complex input for real values
    CHECK_THROWS_AS(addDirectMatrixNodal(model, "E", badType), base::UsageError);
    CHECK(model.cardCount() == 3);
    CHECK(removeDirectMatrixNodal(model, "A") == 3);
    CHECK(model.cardCount() == 0);
}
