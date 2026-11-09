// Checks of the remaining table cards: the multi-dimensional tables of SOL 400 (TABL3D0, TABL3D1, TABL3D2 and
// TABLE3D), the fatigue loading table TABLFTG, the user defined function TABLUDS, and the references that the SOL 400
// materials (MATS3, MATS8, MATSORT, MATVP and MATTUSR) and the CORD3G make to them. Per card: the fields in their
// format reference positions, the defaults, the lists over the continuation lines, the references and the ID spaces,
// the refused values and the round trip through a model file.

#include "oor/model/cards/coordinate_systems/generalFrame3/GeneralFrame3Table.hpp"
#include "oor/model/cards/materials/axisymOrthoMatStressDependent/AxisymOrthoMatStressDependentTable.hpp"
#include "oor/model/cards/materials/orthoMat2DStressDependent/OrthoMat2DStressDependentTable.hpp"
#include "oor/model/cards/materials/orthoMat3DNonlinearStressDependent/OrthoMat3DNonlinearStressDependentTable.hpp"
#include "oor/model/cards/materials/userMatTable/UserMatTableTable.hpp"
#include "oor/model/cards/materials/viscoPlasticMat/ViscoPlasticMatTable.hpp"
#include "oor/model/cards/tables/tableFatigueHistory/TableFatigueHistoryTable.hpp"
#include "oor/model/cards/tables/tableMultiVariable0/TableMultiVariable0Table.hpp"
#include "oor/model/cards/tables/tableMultiVariable1/TableMultiVariable1Table.hpp"
#include "oor/model/cards/tables/tableMultiVariable2/TableMultiVariable2Table.hpp"
#include "oor/model/cards/tables/tableScattered3D/TableScattered3DTable.hpp"
#include "oor/model/cards/tables/tableUserRoutine/TableUserRoutineTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The references that name nothing in a model that was just read (the support function takes a model that stays).
std::size_t unresolvedIn(Model&& model)
{
    return testing::unresolvedIn(model);
}

using Values = std::vector<double>;

// After the NW1 of a TABL3D1 or TABL3D2 of one variable (kind 12) with two values of X: the blank fields of the other
// variables, the two values of X and the two values of Y.
const std::string ONE_VARIABLE_DATA = ";;;;;;;;;;;;;;;;;;;;1.;2.;5.;6.";

// A TABL3D1 of two variables: three values of the first (kind 12, EXTRP 1, SM 1) and two of the second (kind 1), so six
// values of Y: the lists of X, then the values of Y.
const std::string GRID_TABLE =
    "TableMultiVariable1;86011;2;12;1;;;3;2;;;1;;;;;;;;;;;;1;;;;0.;0.5;1.;10.;20.;1.;2.;3.;4.;5.;6.";

} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// TABL3D0
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TABL3D0 has one variable, its flags have the format reference defaults and its pairs run over the lines",
          "[model][cards][tables3d]")
{
    Model model = readBulk(
        "TableMultiVariable0;86001;12;1;;;1;0.;120.;0.005;165.;0.015;190.;0.025;205.;0.035;212.5\n"
        "TableMultiVariable0;86002;5;;;;;0.;1.;1.;2.\n"
        "TableMultiVariable0;86003;5;2;17;18;;0.;1.;;;2.;4.");
    const TableMultiVariable0Card full = cardAt<TableMultiVariable0Table>(model, 0);
    REQUIRE((full.id == 86001 && full.kind == 12 && full.extrapolation == 1 && full.smoothing == 1));
    REQUIRE((full.abscissa == Values{0.0, 0.005, 0.015, 0.025, 0.035}));
    REQUIRE((full.ordinate == Values{120.0, 165.0, 190.0, 205.0, 212.5}));
    const TableMultiVariable0Card bare = cardAt<TableMultiVariable0Table>(model, 1);
    REQUIRE((bare.kind == 5 && bare.extrapolation == 2 && bare.smoothing == 0 && bare.belowRangeTableId == 0 &&
             bare.aboveRangeTableId == 0));
    // ITIDS and ITIDB are kept as written, and a pair of blank fields in the middle is padding.
    const TableMultiVariable0Card padded = cardAt<TableMultiVariable0Table>(model, 2);
    REQUIRE((padded.belowRangeTableId == 17 && padded.aboveRangeTableId == 18));
    REQUIRE((padded.abscissa == Values{0.0, 2.0} && padded.ordinate == Values{1.0, 4.0}));
    REQUIRE((TableMultiVariable0Table::INFO.category == CardCategory::MultiDimensionalTable &&
             TableMultiVariable0Table::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    // The defaults are blanks, field 2 stays empty, and a written ITIDS comes back.

    // The tables the program may fall back on are not used and are not references.
    RecordingVisitor visitor;
    model.table<TableMultiVariable0Table>().visitReferences(2, visitor);
    REQUIRE(visitor.fields.empty());

    expectInvalid("TableMultiVariable0;0;5;;;;;0.;1.;1.;2.", "id");
    expectInvalid("TableMultiVariable0;-3;5;;;;;0.;1.;1.;2.", "id");
    expectInvalid("TableMultiVariable0;1;;;;;;0.;1.;1.;2.", "kind"); // required
    expectInvalid("TableMultiVariable0;1;0;;;;;0.;1.;1.;2.", "kind");
    expectInvalid("TableMultiVariable0;1;-4;;;;;0.;1.;1.;2.", "kind");
    expectInvalid("TableMultiVariable0;1;5;3;;;;0.;1.;1.;2.", "extrapolation");
    expectInvalid("TableMultiVariable0;1;5;;;;2;0.;1.;1.;2.", "smoothing");
    expectInvalid("TableMultiVariable0;1;5", "abscissa");              // no pair
    expectInvalid("TableMultiVariable0;1;5;;;;;0.;1.;2.", "ordinate"); // a pair needs both
    expectInvalid("TableMultiVariable0;1;5;;;;;0.;A", "ordinate");
    expectInvalid("TableMultiVariable0;1;5;;;;;A;1.", "abscissa");
}

// ---------------------------------------------------------------------------------------------------------------------
// TABL3D1 and TABL3D2
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TABL3D1 has its lists of X on lines of their own, then one Y for each point of the grid",
          "[model][cards][tables3d]")
{
    Model model = readBulk(
        GRID_TABLE + "\n" +
        // four variables of one value each: every field of the header is used
        "TableMultiVariable1;86012;4;5;6;7;12;1;1;1;1;2;1;2;1;3;4;5;6;7;8;9;10;0;1;0;1;100.;200.;300.;400.;7.\n"
        "TableMultiVariable1;86013;1;12;;;;2;;;;;2;;;;;;;;;;;0;0;;;1.;2.;5.;6.");
    const TableMultiVariable1Card grid = cardAt<TableMultiVariable1Table>(model, 0);
    REQUIRE(grid.id == 86011);
    REQUIRE(grid.variables.size() == 2);
    REQUIRE((grid.variables[0].kind == 12 && grid.variables[0].extrapolation == 1 && grid.variables[0].smoothing == 1));
    REQUIRE((grid.variables[1].kind == 1 && grid.variables[1].extrapolation == 2 && grid.variables[1].smoothing == 0));
    REQUIRE((grid.variables[0].x == Values{0.0, 0.5, 1.0}));
    REQUIRE((grid.variables[1].x == Values{10.0, 20.0}));
    REQUIRE((grid.ordinate == Values{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}));

    const TableMultiVariable1Card four = cardAt<TableMultiVariable1Table>(model, 1);
    REQUIRE(four.variables.size() == 4);
    const std::vector<int> kinds{5, 6, 7, 12};
    const std::vector<int> extrapolations{2, 1, 2, 1};
    const std::vector<int> smoothings{0, 1, 0, 1};
    for (std::size_t index = 0; index < 4; ++index)
    {
        REQUIRE(four.variables[index].x.size() == 1);
        REQUIRE(four.variables[index].kind == kinds[index]);
        REQUIRE(four.variables[index].extrapolation == extrapolations[index]);
        REQUIRE(four.variables[index].smoothing == smoothings[index]);
        // ITIDSi and ITIDBi are kept as written, and are not references.
        REQUIRE(four.variables[index].belowRangeTableId == static_cast<base::EntityId>(3 + 2 * index));
        REQUIRE(four.variables[index].aboveRangeTableId == static_cast<base::EntityId>(4 + 2 * index));
    }
    REQUIRE((four.variables[3].x == Values{400.0} && four.ordinate == Values{7.0}));
    REQUIRE(cardAt<TableMultiVariable1Table>(model, 2).variables.size() ==
            1); // the defaults of the missing variables are nothing
    REQUIRE((TableMultiVariable1Table::INFO.category == CardCategory::MultiDimensionalTable &&
             TableMultiVariable1Table::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    // The header has four lines; the defaults are blanks, and the lists start at field 33 and at the next lines.

    RecordingVisitor visitor;
    model.table<TableMultiVariable1Table>().visitReferences(1, visitor);
    REQUIRE(visitor.fields.empty());

    // The grid has the six points, so five or seven values of Y are not enough or too many.
    REQUIRE_NOTHROW(
        readBulk("TableMultiVariable1;1;2;12;1;;;3;2;;;;;;;;;;;;;;;;;;;0.;0.5;1.;10.;20.;1.;2.;3.;4.;5.;6."));
    expectInvalid("TableMultiVariable1;1;2;12;1;;;3;2;;;;;;;;;;;;;;;;;;;0.;0.5;1.;10.;20.;1.;2.;3.;4.;5.", "ordinate");
    expectInvalid("TableMultiVariable1;1;2;12;1;;;3;2;;;;;;;;;;;;;;;;;;;0.;0.5;1.;10.;20.;1.;2.;3.;4.;5.;6.;7.",
                  "ordinate");
    expectInvalid("TableMultiVariable1;1;2;12;1;;;3;2;;;;;;;;;;;;;;;;;;;0.;0.5;1.;10.;20.", "ordinate");
}

TEST_CASE("a TABL3D1 refuses the cards whose counts do not match their data", "[model][cards][tables3d]")
{
    REQUIRE_NOTHROW(readBulk("TableMultiVariable1;1;1;12;;;;2;;;;;;;;;;;;;;;;;;;;1.;2.;5.;6."));
    expectInvalid("TableMultiVariable1;0;1;12;;;;2;;;;;;;;;;;;;;;;;;;;1.;2.;5.;6.", "id");
    expectInvalid("TableMultiVariable1;1;0;12;;;;2;;;;;;;;;;;;;;;;;;;;1.;2.;5.;6.", "variableCount");
    expectInvalid("TableMultiVariable1;1;5;12;12;12;12;2;;;;;;;;;;;;;;;;;;;;1.;2.;5.;6.", "variableCount");
    expectInvalid("TableMultiVariable1;1;A;12;;;;2;;;;;;;;;;;;;;;;;;;;1.;2.;5.;6.", "variableCount");
    expectInvalid("TableMultiVariable1;1;1;;;;;2;;;;;;;;;;;;;;;;;;;;1.;2.;5.;6.", "kind1");
    expectInvalid("TableMultiVariable1;1;1;0;;;;2;;;;;;;;;;;;;;;;;;;;1.;2.;5.;6.", "kind1");
    expectInvalid("TableMultiVariable1;1;1;12;;;;;;;;;;;;;;;;;;;;;;;;1.;2.;;;;;;;5.;6.",
                  "valueCount1"); // NWi is above zero for each variable
    expectInvalid("TableMultiVariable1;1;2;12;1;;;2;;;;;;;;;;;;;;;;;;;;1.;2.;3.;4.", "valueCount2");
    // The fields of a variable that NV does not count are blank (or hold the defaults).
    expectInvalid("TableMultiVariable1;1;1;12;7;;;2;;;;;;;;;;;;;;;;;;;;1.;2.;5.;6.", "kind2");
    expectInvalid("TableMultiVariable1;1;1;12;;;;2;;5;;;;;;;;;;;;;;;;;;1.;2.;5.;6.", "kind3"); // NW3
    expectInvalid("TableMultiVariable1;1;1;12;;;;2;;;;;3;;;;;;;;;;;;;;;1.;2.;5.;6.", "kind2"); // EXTRP2 is not 2
    expectInvalid("TableMultiVariable1;1;1;12;;;;2;;;;;;;;;;3;;;;;;;;;;1.;2.;5.;6.", "kind2"); // ITIDS2
    expectInvalid("TableMultiVariable1;1;1;12;;;;2;;;;;;;;;;;;;;;;;1;;;1.;2.;5.;6.", "kind2"); // SM2
    expectInvalid("TableMultiVariable1;1;1;12;;;;2;;;;3;;;;;;;;;;;;;;;;1.;2.;5.;6.", "extrapolation1");
    expectInvalid("TableMultiVariable1;1;1;12;;;;2;;;;0;;;;;;;;;;;;;;;;1.;2.;5.;6.", "extrapolation1");
    expectInvalid("TableMultiVariable1;1;1;12;;;;2;;;;;;;;;;;;;;;;2;;;;1.;2.;5.;6.", "smoothing1");
    // The lists of X: fewer values than NW says, a stray value where the next list starts, a value that is no number.
    expectInvalid("TableMultiVariable1;1;1;12;;;;3;;;;;;;;;;;;;;;;;;;;1.;2.", "variableValues1");
    expectInvalid("TableMultiVariable1;1;1;12;;;;2;;;;;;;;;;;;;;;;;;;;1.;A;5.;6.", "variableValues1");
    expectInvalid("TableMultiVariable1;1;1;12;;;;2;;;;;;;;;;;;;;;;;;;;1.;;5.;6.", "variableValues1");
    // The values of Y: a count that is not the product of the NWi, a missing line and a value that is no number.
    expectInvalid("TableMultiVariable1;1;1;12;;;;2;;;;;;;;;;;;;;;;;;;;1.;2.;5.;6.;7.", "ordinate");
    expectInvalid("TableMultiVariable1;1;1;12;;;;2;;;;;;;;;;;;;;;;;;;;1.;2.;5.", "ordinate");
    expectInvalid("TableMultiVariable1;1;1;12;;;;2;;;;;;;;;;;;;;;;;;;;1.;2.", "ordinate");
    expectInvalid("TableMultiVariable1;1;1;12;;;;2;;;;;;;;;;;;;;;;;;;;1.;2.;5.;A", "ordinate");
    expectInvalid("TableMultiVariable1;1;1;12;;;;2;;;;;;;;;;;;;;;;;;;;1.;2.;5.", "ordinate");
}

TEST_CASE("a TABL3D2 reads the layout of the TABL3D1 and has the same rules", "[model][cards][tables3d]")
{
    Model model = readBulk("TableMultiVariable2" + GRID_TABLE.substr(std::string("TableMultiVariable1").size()));
    const TableMultiVariable2Card card = cardAt<TableMultiVariable2Table>(model);
    REQUIRE(card.id == 86011);
    REQUIRE(card.variables.size() == 2);
    REQUIRE((card.variables[0].kind == 12 && card.variables[0].extrapolation == 1 && card.variables[0].smoothing == 1));
    REQUIRE((card.variables[1].x == Values{10.0, 20.0}));
    REQUIRE((card.ordinate == Values{1.0, 2.0, 3.0, 4.0, 5.0, 6.0}));
    REQUIRE((TableMultiVariable2Table::INFO.category == CardCategory::MultiDimensionalTable &&
             TableMultiVariable2Table::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    expectInvalid("TableMultiVariable2;1;0;12;;;;2;;;;;;;;;;;;;;;;;;;;1.;2.;5.;6.", "variableCount");
    expectInvalid("TableMultiVariable2;1;1;12;;;;2;;;;3;;;;;;;;;;;;;;;;1.;2.;5.;6.", "extrapolation1");
    expectInvalid("TableMultiVariable2;1;1;12;7;;;2;;;;;;;;;;;;;;;;;;;;1.;2.;5.;6.", "kind2");
    expectInvalid("TableMultiVariable2;1;1;12;;;;2;;;;;;;;;;;;;;;;;;;;1.;2.;5.;6.;7.", "ordinate");
    expectInvalid("TableMultiVariable2;1;1;12;;;;3;;;;;;;;;;;;;;;;;;;;1.;2.", "variableValues1");
}

TEST_CASE("the multi-dimensional tables share a category, each type has its own numbers and a reference finds any",
          "[model][cards][tables3d]")
{
    // The ID is unique per card type: a TABL3D0 5, a TABL3D1 5, a TABL3D2 5 and a TABLE3D 5 coexist.
    Model model = readBulk("TableMultiVariable0;5;12;;;;;0.;1.;1.;2.\nTableMultiVariable1;5;1;12;;;;2" +
                           ONE_VARIABLE_DATA + "\nTableMultiVariable2;5;1;12;;;;2" + ONE_VARIABLE_DATA +
                           "\nTableScattered3D;5;;;;;0.;0.;0.;0.;1.;1.;1.;1.;EndTable");
    REQUIRE((rowCount<TableMultiVariable0Table>(model) == 1 && rowCount<TableMultiVariable1Table>(model) == 1 &&
             rowCount<TableMultiVariable2Table>(model) == 1 && rowCount<TableScattered3DTable>(model) == 1));
    REQUIRE_THROWS_AS(readBulk("TableMultiVariable0;5;12;;;;;0.;1.;1.;2.\nTableMultiVariable0;5;12;;;;;0.;1.;1.;2."),
                      base::DeckError);
    REQUIRE_THROWS_AS(readBulk("TableScattered3D;5;;;;;0.;0.;0.;0.;1.;1.;1.;1.;EndTable\nTableScattered3D;5;;;;;0.;0.;"
                               "0.;0.;1.;1.;1.;1.;EndTable"),
                      base::DeckError);

    // The materials and a CORD3G name them with ReferenceTarget::MultiDimensionalTable, and the space renumbers them.
    Model materials = readBulk(
        "OrthoMat2D;34;1.;1.;0.3\nOrthoMat2DStressDependent;34;12;;;;;;;1\nTableMultiVariable0;12;5;;;;;0.;1.;1.;2."
        "\nTableMultiVariable1;1;1;5;;;;2" +
        ONE_VARIABLE_DATA);
    REQUIRE(unresolvedIn(materials) == 0);
    Renumbering::apply(materials, RenumberSpace::MultiDimensionalTable, Renumbering::offset(100));
    REQUIRE((cardAt<TableMultiVariable0Table>(materials).id == 112 &&
             cardAt<TableMultiVariable1Table>(materials).id == 101));
    const OrthoMat2DStressDependentCard renumbered = cardAt<OrthoMat2DStressDependentTable>(materials);
    REQUIRE(
        (renumbered.youngsModulus1TableId == 112 && renumbered.thermalExpansion1TableId == 101 && renumbered.id == 34));
    REQUIRE(unresolvedIn(materials) == 0);
    requireRoundTrip(materials);
}

// ---------------------------------------------------------------------------------------------------------------------
// TABLE3D
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TABLE3D has offsets, points of four fields and ends with ENDT", "[model][cards][tables3d]")
{
    Model model = readBulk(
        "TableScattered3D;3128;0.;0.;2.5;;1.5;2.5;3.5;40.;4.;6.;5.;85.;10.;12.5;11.;310.;20.;22.;24.;575.;EndTable\n"
        "TableScattered3D;129;1.;2.;3.;4.;1.;2.;3.;4.;5.;6.;7.;8.;9.;10.;11.;12.;EndTable\n"
        "TableScattered3D;130;;;;;;2.;;4.;5.;6.;7.;8.;EndTable");
    const TableScattered3DCard sampleCard = cardAt<TableScattered3DTable>(model, 0);
    REQUIRE((sampleCard.id == 3128 && sampleCard.xOffset == 0.0 && sampleCard.yOffset == 0.0 &&
             sampleCard.zOffset == 2.5 && sampleCard.valueOffset == 0.0));
    REQUIRE(sampleCard.points.size() == 4);
    REQUIRE((sampleCard.points[0] == TableScattered3DPoint{1.5, 2.5, 3.5, 40.0}));
    REQUIRE((sampleCard.points[3] == TableScattered3DPoint{20.0, 22.0, 24.0, 575.0}));
    // ENDT follows the last F, in the middle of a line when the last point does not end one.
    const TableScattered3DCard odd = cardAt<TableScattered3DTable>(model, 1);
    REQUIRE((odd.xOffset == 1.0 && odd.yOffset == 2.0 && odd.zOffset == 3.0 && odd.valueOffset == 4.0 &&
             odd.points.size() == 3));
    REQUIRE((odd.points[2] == TableScattered3DPoint{9.0, 10.0, 11.0, 12.0}));
    // A blank coordinate or value is 0.0.
    const TableScattered3DCard blanks = cardAt<TableScattered3DTable>(model, 2);
    REQUIRE(blanks.points.size() == 2);
    REQUIRE((blanks.points[0] == TableScattered3DPoint{0.0, 2.0, 0.0, 4.0}));
    REQUIRE((blanks.points[1] == TableScattered3DPoint{5.0, 6.0, 7.0, 8.0}));
    REQUIRE((TableScattered3DTable::INFO.category == CardCategory::MultiDimensionalTable &&
             TableScattered3DTable::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    // The offsets that are 0.0 are blank, and the points are always written, then ENDT.

    RecordingVisitor visitor;
    model.table<TableScattered3DTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());

    expectInvalid("TableScattered3D;0;;;;;0.;0.;0.;0.;1.;1.;1.;1.;EndTable", "id");
    expectInvalid("TableScattered3D;1;A;;;;0.;0.;0.;0.;1.;1.;1.;1.;EndTable", "xOffset");
    expectInvalid("TableScattered3D;1;;;;;0.;0.;0.;0.;1.;1.;1.;1.", "tableEnd");             // the end is required
    expectInvalid("TableScattered3D;1;;;;;0.;0.;0.;0.;1.;1.;1.;1.;EndTable;5.", "tableEnd"); // nothing follows it
    expectInvalid("TableScattered3D;1;;;;;0.;0.;0.;0.;1.;1.;1.;EndTable",
                  "tableEnd");                                              // right after the F of the last point
    expectInvalid("TableScattered3D;1;;;;;0.;0.;0.;0.;EndTable", "pointX"); // two points at least
    expectInvalid("TableScattered3D;1;;;;;EndTable", "pointX");
    expectInvalid("TableScattered3D;1;;;;;0.;0.;0.;0.;1.;1.;A;1.;EndTable", "pointZ");
}

// ---------------------------------------------------------------------------------------------------------------------
// TABLFTG
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TABLFTG is a list of reals that ends with ENDT and that an FTGLOAD names", "[model][cards][tables3d]")
{
    Model model = readBulk(
        "TableFatigueHistory;1;0.;-0.75;1.25;0.;EndTable\n"
        "TableFatigueHistory;2;1.;2.;3.;4.;5.;6.;7.;8.;9.;10.;EndTable\n"
        "TableFatigueHistory;3;1.;2.;3.;4.;5.;6.;7.;EndTable");
    REQUIRE((cardAt<TableFatigueHistoryTable>(model, 0).ordinate == Values{0.0, -0.75, 1.25, 0.0}));
    REQUIRE((cardAt<TableFatigueHistoryTable>(model, 1).ordinate ==
             Values{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0}));
    REQUIRE((cardAt<TableFatigueHistoryTable>(model, 2).ordinate ==
             Values{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0})); // ENDT is field 16
    REQUIRE((TableFatigueHistoryTable::INFO.category == CardCategory::FatigueTable &&
             TableFatigueHistoryTable::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    // A TID is unique among the TABLFTG cards, and a table of another kind may have the same number.
    REQUIRE_THROWS_AS(readBulk("TableFatigueHistory;1;1.;EndTable\nTableFatigueHistory;1;2.;EndTable"),
                      base::DeckError);
    REQUIRE_NOTHROW(readBulk("TableFatigueHistory;1;1.;EndTable\nTableDynamic1;1;;;0.;1.;1.;2.;EndTable"));

    // A renumbering of the fatigue tables moves the TID.
    Renumbering::apply(model, RenumberSpace::FatigueTable, Renumbering::offset(10));
    REQUIRE(
        (cardAt<TableFatigueHistoryTable>(model, 0).id == 11 && cardAt<TableFatigueHistoryTable>(model, 2).id == 13));
    REQUIRE(cardAt<TableFatigueHistoryTable>(model, 1).ordinate.size() == 10);

    expectInvalid("TableFatigueHistory;0;1.;EndTable", "id");
    expectInvalid("TableFatigueHistory;1;EndTable", "ordinate");        // a value at least
    expectInvalid("TableFatigueHistory;1;1.;2.", "tableEnd");           // the end is required
    expectInvalid("TableFatigueHistory;1;1.;EndTable;3.", "tableEnd");  // nothing follows it
    expectInvalid("TableFatigueHistory;1;1.;;3.;EndTable", "ordinate"); // no value is blank
    expectInvalid("TableFatigueHistory;1;1.;A;EndTable", "ordinate");
    expectInvalid("TableFatigueHistory;1", "tableEnd");
}

// ---------------------------------------------------------------------------------------------------------------------
// TABLUDS
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TABLUDS has a group and a routine and is numbered with the dynamic tables", "[model][cards][tables3d]")
{
    Model model = readBulk(
        "TableUserRoutine;5120;gustgrp;ExternalFunction\nTableUserRoutine;13;grp\nTableUserRoutine;14;grp;MYFUNC2");
    const TableUserRoutineCard sampleCard = cardAt<TableUserRoutineTable>(model, 0);
    REQUIRE((sampleCard.id == 5120 && sampleCard.groupName == "gustgrp" &&
             sampleCard.routineName == "ExternalFunction")); // the case of the group is kept
    REQUIRE(cardAt<TableUserRoutineTable>(model, 1).routineName == TABLE_USER_ROUTINE_DEFAULT_ROUTINE);
    REQUIRE(cardAt<TableUserRoutineTable>(model, 2).routineName == "MYFUNC2");
    REQUIRE((TableUserRoutineTable::INFO.category == CardCategory::Table &&
             TableUserRoutineTable::INFO.idSpace == base::IdSpace::DynamicTable));
    requireRoundTrip(model);

    // the default routine is a blank

    // Tables and user defined functions are one group: the number is unique among them.
    REQUIRE_THROWS_AS(readBulk("TableDynamic1;5;;;0.;1.;1.;2.;EndTable\nTableUserRoutine;5;grp"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("TableUserRoutine;5;grp\nTableUserRoutine;5;other"), base::DeckError);
    REQUIRE_NOTHROW(readBulk("TableDynamic1;6;;;0.;1.;1.;2.;EndTable\nTableUserRoutine;5;grp"));

    // A field that names a dynamic table finds it: a TLOAD1 whose table is a TABLUDS has as many unresolved
    // references as one whose table is a TABLED1, and one more when no card has the number.
    const std::string load = "TransientTableLoad1;1;2;;;;7\n";
    const std::size_t withUds = unresolvedIn(readBulk(load + "TableUserRoutine;7;grp"));
    REQUIRE(withUds == unresolvedIn(readBulk(load + "TableDynamic1;7;;;0.;1.;1.;2.;EndTable")));
    REQUIRE(unresolvedIn(readBulk(load)) == withUds + 1);

    expectInvalid("TableUserRoutine", "id");
    expectInvalid("TableUserRoutine;0;grp", "id");
    expectInvalid("TableUserRoutine;12", "groupName"); // the group has no default
    oor::testing::requireModelFileRoundTrip(readModelText("TableUserRoutine;12;toolonggroup", true));
    oor::testing::requireModelFileRoundTrip(readModelText("TableUserRoutine;12;my'grp", true));
    oor::testing::requireModelFileRoundTrip(readModelText("TableUserRoutine;12;grp;TOOLONGNAME", true));
}

// ---------------------------------------------------------------------------------------------------------------------
// The references to the multi-dimensional tables
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("MATS3, MATS8, MATSORT and MATVP name their tables as multi-dimensional tables", "[model][cards][tables3d]")
{
    Model model = readBulk(
        "AxisymOrthoMatStressDependent;3301;5601;;;6701;;;;1201\n"
        "OrthoMat2DStressDependent;34;12;;;;;;;1\n"
        "OrthoMat3DNonlinearStressDependent;6890;7701;7701;7701;;;;;;;;8901;8901;8901\n"
        "ViscoPlasticMat;10;0.8;20;1.3;1.2;0.9;0.7\n"
        "ViscoPlasticMat;11;1.\n"
        "ViscoPlasticMat;2;AnandModel;1.;2.;3.;4.;5.;6.;7.;8.;9.");
    const std::vector<std::string> axisymOrthoFields{"id", "youngsModulusXTableId", "poissonRatioXthTableId",
                                                     "shearModulusZxTableId"};
    RecordingVisitor axisymOrtho;
    model.table<AxisymOrthoMatStressDependentTable>().visitReferences(0, axisymOrtho);
    REQUIRE(axisymOrtho.fields == axisymOrthoFields);
    REQUIRE(
        (axisymOrtho.targets ==
         std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::MultiDimensionalTable,
                                      ReferenceTarget::MultiDimensionalTable, ReferenceTarget::MultiDimensionalTable}));
    RecordingVisitor ortho2D;
    model.table<OrthoMat2DStressDependentTable>().visitReferences(0, ortho2D);
    REQUIRE((ortho2D.fields == std::vector<std::string>{"id", "youngsModulus1TableId", "thermalExpansion1TableId"}));
    REQUIRE((ortho2D.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material,
                                                             ReferenceTarget::MultiDimensionalTable,
                                                             ReferenceTarget::MultiDimensionalTable}));
    RecordingVisitor ortho3D;
    model.table<OrthoMat3DNonlinearStressDependentTable>().visitReferences(0, ortho3D);
    REQUIRE((ortho3D.fields == std::vector<std::string>{"id", "youngsModulus1TableId", "youngsModulus2TableId",
                                                        "youngsModulus3TableId", "thermalExpansion1TableId",
                                                        "thermalExpansion2TableId", "thermalExpansion3TableId"}));
    for (std::size_t index = 1; index < ortho3D.targets.size(); ++index)
        REQUIRE(ortho3D.targets[index] == ReferenceTarget::MultiDimensionalTable);

    // MATVP: IT3D of a power-law card (not the blank one, and not an Anand card, which has none).
    RecordingVisitor power;
    model.table<ViscoPlasticMatTable>().visitReferences(0, power);
    REQUIRE((power.fields == std::vector<std::string>{"id", "rateTableId"}));
    REQUIRE((power.targets ==
             std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::MultiDimensionalTable}));
    RecordingVisitor blank;
    model.table<ViscoPlasticMatTable>().visitReferences(1, blank);
    REQUIRE(blank.fields == std::vector<std::string>{"id"});
    RecordingVisitor anand;
    model.table<ViscoPlasticMatTable>().visitReferences(2, anand);
    REQUIRE(anand.fields == std::vector<std::string>{"id"});
}

TEST_CASE("the tables of the SOL 400 materials resolve, are reported when missing and follow a renumbering",
          "[model][cards][tables3d]")
{
    const std::string tables =
        "TableMultiVariable0;12;5;;;;;0.;1.;1.;2.\n"
        "TableMultiVariable1;1;1;5;;;;2;;;;;;;;;;;;;;;;;;;;1.;2.;5.;6.\n"
        "TableMultiVariable2;20;1;5;;;;2;;;;;;;;;;;;;;;;;;;;1.;2.;5.;6.\n";
    Model model = readBulk(
        "IsoMat;10;2.1E+11;;0.3\n"
        "OrthoMat2D;34;1.;1.;0.3\n"
        "OrthoMat2DStressDependent;34;12;;;;;;;1\n"
        "ViscoPlasticMat;10;0.8;20;1.3;1.2;0.9;0.7\n"
        "UserMat;50;0;1;1.\n"
        "UserMatTable;50;;;;;12\n" +
        tables);
    REQUIRE(unresolvedIn(model) == 0); // the TGE of a MATTUSR may name a TABL3Di too
    REQUIRE(unresolvedIn(readBulk("OrthoMat2D;34;1.;1.;0.3\nOrthoMat2DStressDependent;34;12;;;;;;;1")) == 2);
    REQUIRE(unresolvedIn(readBulk("IsoMat;10;2.1E+11;;0.3\nViscoPlasticMat;10;0.8;20;1.3;1.2;0.9;0.7")) == 1);

    Renumbering::apply(model, RenumberSpace::MultiDimensionalTable, Renumbering::offset(1000));
    const OrthoMat2DStressDependentCard mats8 = cardAt<OrthoMat2DStressDependentTable>(model);
    REQUIRE((mats8.id == 34 && mats8.youngsModulus1TableId == 1012 && mats8.thermalExpansion1TableId == 1001));
    REQUIRE((cardAt<ViscoPlasticMatTable>(model).rateTableId == 1020 && cardAt<ViscoPlasticMatTable>(model).id == 10));
    REQUIRE(cardAt<UserMatTableTable>(model).structuralDampingTableId == 1012);
    REQUIRE((cardAt<TableMultiVariable0Table>(model).id == 1012 && cardAt<TableMultiVariable1Table>(model).id == 1001 &&
             cardAt<TableMultiVariable2Table>(model).id == 1020));
    REQUIRE(unresolvedIn(model) == 0);

    // The materials keep their own space: renumbering them leaves the tables alone.
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(100));
    REQUIRE((cardAt<OrthoMat2DStressDependentTable>(model).id == 134 &&
             cardAt<OrthoMat2DStressDependentTable>(model).youngsModulus1TableId == 1012));
    REQUIRE(cardAt<TableMultiVariable0Table>(model).id == 1012);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
}

TEST_CASE("a CORD3G names equations or tables as its FORM says", "[model][cards][tables3d]")
{
    RecordingVisitor equations;
    Model byEquation = readBulk("GeneralFrame3;8;;;40;41;42");
    byEquation.table<GeneralFrame3Table>().visitReferences(0, equations);
    REQUIRE((equations.fields == std::vector<std::string>{"angleFunctionId1", "angleFunctionId2", "angleFunctionId3"}));
    REQUIRE((equations.targets == std::vector<ReferenceTarget>(3, ReferenceTarget::DesignEquation)));

    const std::string table = "TableScattered3D;3;;;;;0.;0.;0.;0.;1.;1.;1.;1.;EndTable\n";
    Model byTable = readBulk(
        "GeneralFrame3;7;EulerAngles313;Table;30;31;32\n"
        "TableScattered3D;30;;;;;0.;0.;0.;0.;1.;1.;1.;1.;EndTable\n"
        "TableScattered3D;31;;;;;0.;0.;0.;0.;1.;1.;1.;1.;EndTable\n"
        "TableScattered3D;32;;;;;0.;0.;0.;0.;1.;1.;1.;1.;EndTable");
    RecordingVisitor tables;
    byTable.table<GeneralFrame3Table>().visitReferences(0, tables);
    REQUIRE((tables.fields == std::vector<std::string>{"angleFunctionId1", "angleFunctionId2", "angleFunctionId3"}));
    REQUIRE((tables.targets == std::vector<ReferenceTarget>(3, ReferenceTarget::MultiDimensionalTable)));
    REQUIRE(unresolvedIn(byTable) == 0);

    // The tables follow their space; the equations of a CORD3G of the other FORM are not touched by it.
    Renumbering::apply(byTable, RenumberSpace::MultiDimensionalTable, Renumbering::offset(1000));
    REQUIRE((cardAt<GeneralFrame3Table>(byTable).angleFunctionIds == std::array<base::EntityId, 3>{1030, 1031, 1032}));
    REQUIRE(unresolvedIn(byTable) == 0);
    Model both = readBulk(table + "GeneralFrame3;7;;Table;3;3;3\nGeneralFrame3;8;;;3;3;3");
    Renumbering::apply(both, RenumberSpace::MultiDimensionalTable, Renumbering::offset(10));
    REQUIRE((cardAt<GeneralFrame3Table>(both, 0).angleFunctionIds == std::array<base::EntityId, 3>{13, 13, 13}));
    REQUIRE((cardAt<GeneralFrame3Table>(both, 1).angleFunctionIds == std::array<base::EntityId, 3>{3, 3, 3}));
    requireRoundTrip(byTable);

    // What names nothing is reported, as before for the equations.
    REQUIRE(unresolvedIn(readBulk("GeneralFrame3;9;;Table;1;2;3")) == 3);
}
