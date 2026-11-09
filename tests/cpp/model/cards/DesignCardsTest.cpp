// Checks of the cards of the design optimisation family: DEQATN with its equations, the design variables,
// the relations (DVPREL1/2, DVMREL1/2, DVCREL1/2), DLINK, DVGRID, DVSHAP, DVBSHAP, the responses (DRESP1/2/3), the
// constraints (DCONSTR, DCONADD), DSCREEN, DTABLE, DTABLE2, DOPTPRM and TOPVAR: field positions, defaults, the
// continuation groups, references, renumbering, the Case Control commands that name them, and refused values.

#include "oor/deck/records/RecordText.hpp"
#include "oor/model/cards/optimisation/designBoundaryShape/DesignBoundaryShapeTable.hpp"
#include "oor/model/cards/optimisation/designConstants/DesignConstantsTable.hpp"
#include "oor/model/cards/optimisation/designConstr/DesignConstrTable.hpp"
#include "oor/model/cards/optimisation/designConstrScreen/DesignConstrScreenTable.hpp"
#include "oor/model/cards/optimisation/designConstrUnion/DesignConstrUnionTable.hpp"
#include "oor/model/cards/optimisation/designElemRelation1/DesignElemRelation1Table.hpp"
#include "oor/model/cards/optimisation/designElemRelation2/DesignElemRelation2Table.hpp"
#include "oor/model/cards/optimisation/designEquation/DesignEquationTable.hpp"
#include "oor/model/cards/optimisation/designFieldConstants/DesignFieldConstantsTable.hpp"
#include "oor/model/cards/optimisation/designMatRelation1/DesignMatRelation1Table.hpp"
#include "oor/model/cards/optimisation/designMatRelation2/DesignMatRelation2Table.hpp"
#include "oor/model/cards/optimisation/designNodeMove/DesignNodeMoveTable.hpp"
#include "oor/model/cards/optimisation/designOptimParams/DesignOptimParamsCatalogue.hpp"
#include "oor/model/cards/optimisation/designOptimParams/DesignOptimParamsTable.hpp"
#include "oor/model/cards/optimisation/designOptimParams/DesignOptimParamsValues.hpp"
#include "oor/model/cards/optimisation/designPropRelation1/DesignPropRelation1Table.hpp"
#include "oor/model/cards/optimisation/designPropRelation2/DesignPropRelation2Table.hpp"
#include "oor/model/cards/optimisation/designResp1/DesignResp1Table.hpp"
#include "oor/model/cards/optimisation/designResp2/DesignResp2Table.hpp"
#include "oor/model/cards/optimisation/designResp3/DesignResp3Table.hpp"
#include "oor/model/cards/optimisation/designShapeBasis/DesignShapeBasisTable.hpp"
#include "oor/model/cards/optimisation/designTopologyRegion/DesignTopologyRegionTable.hpp"
#include "oor/model/cards/optimisation/designVar/DesignVarTable.hpp"
#include "oor/model/cards/optimisation/designVarLink/DesignVarLinkTable.hpp"
#include "oor/model/cards/properties/bushProp1D/BushProp1DTable.hpp"
#include "oor/model/expressions/EquationSet.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;
using Catch::Approx;

namespace
{

// A DEQATN card in the fixed columns the deck lexer reads it by: the ID in columns 9 to 16 and the text from column 17.
std::string designEquationCard(int id, const std::string& text)
{
    return deck::RecordText::format({"DesignEquation", std::to_string(id), text}) + "\n";
}

// The references a card of a table lists, with the target of each.
template <typename TableT>
RecordingVisitor referencesOf(Model& model, std::uint32_t row = 0)
{
    RecordingVisitor visitor;
    model.table<TableT>().visitReferences(row, visitor);
    return visitor;
}

bool lists(const RecordingVisitor& visitor, const std::string& field, ReferenceTarget target)
{
    for (std::size_t index = 0; index < visitor.fields.size(); ++index)
    {
        if (visitor.fields[index] == field && visitor.targets[index] == target)
            return true;
    }
    return false;
}

// The deck of a small design model: the analysis cards the relations name, and the design cards.
const std::string DESIGN_DECK =
    "Node;1;;0.;0.;0.\n"
    "Node;2;;1.;0.;0.\n"
    "Node;3;;1.;1.;0.\n"
    "Node;4;;0.;1.;0.\n"
    "RectFrame2;5;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
    "IsoMat;3;2.1E+11;;0.3;7850.\n"
    "ShellProp;10;3;0.01\n"
    "BarProp;20;3;1.E-4;1.E-8;1.E-8;1.E-8\n"
    "ShellQuad4;100;10;1;2;3;4\n"
    "DesignVar;1;T1;0.01;0.005;0.02\n"
    "DesignVar;2;A1;1.E-4\n"
    "DesignVar;3;RHO;7850.\n"
    "DesignPropRelation1;11;ShellProp;10;thickness;;;;1;1.\n"
    "DesignPropRelation1;12;BarProp;20;area;1.E-5;;;2;1.\n"
    "DesignMatRelation1;21;IsoMat;3;density;;;;3;1.\n"
    "DesignElemRelation1;31;ShellQuad4;100;offset;;;;1;0.5\n"
    "DesignEquation;1;F(A,B)=A*B\n"
    "DesignPropRelation2;41;ShellProp;10;bendingStiffnessRatio;;;1;DesignVar;2;1;2;DesignConstants;1;SCALE\n"
    "DesignConstants;SCALE;2.\n"
    "DesignResp1;51;Stress;Stress;ShellProp;;9;;10\n"
    "DesignResp1;52;U1;Displacement;;;1;;1\n"
    "DesignResp1;53;WT;Weight;;;;;All\n"
    "DesignResp2;61;RATIO;1;;;;;;;DesignVar;2;1;2;DesignResp1;2;51;52\n"
    "DesignConstr;70;51;;;1.E+8\n"
    "DesignConstr;70;52;-0.1;;0.1\n"
    "DesignConstrUnion;71;70\n";

} // namespace

// ------------------------------------------------------------------------------------------------------------ DEQATN

TEST_CASE("a DesignEquation preserves its exact text in one quoted value", "[model][cards][design]")
{
    const std::string equation =
        "G2(P, Q, S, U, W) = P*Q + S - (U**2 - 3.0) + cos(PI(1)*W)"
        "+ Q**3/(P + S) ; G = P - Q + G2*U";
    const Model model = readBulk(designEquationCard(23, equation));
    REQUIRE(rowCount<DesignEquationTable>(model) == 1);
    const DesignEquationCard card = cardAt<DesignEquationTable>(model);
    REQUIRE(card.id == 23);
    REQUIRE(card.text == equation);
    REQUIRE(writeModelText(model).find(deck::RecordText::format({"DesignEquation", "23", equation})) !=
            std::string::npos);
    requireRoundTrip(model);
}

TEST_CASE("an edited or long DesignEquation retains its expression text", "[model][cards][design]")
{
    Model model = readBulk(designEquationCard(1, "f(a,b) = a + b"));
    DesignEquationCard card = cardAt<DesignEquationTable>(model);
    card.text = "f(a, b) = a*b + (a - b)/2.0; g = f*f";
    model.table<DesignEquationTable>().store(0, card);
    REQUIRE(cardAt<DesignEquationTable>(readModelText(writeModelText(model), true)).text == card.text);
    requireRoundTrip(model);

    std::string longText = "F(X)=X";
    for (int term = 1; term <= 30; ++term)
        longText += "+" + std::to_string(term) + ".5*X**" + std::to_string(term % 4 + 2);
    DesignEquationCard longCard;
    longCard.id = 2;
    longCard.text = longText;
    REQUIRE_FALSE(DesignEquationFormat::check(longCard).has_value());
    Model longModel;
    longModel.add<DesignEquationTable>(longCard);
    const Model again = readModelText(writeModelText(longModel), true);
    REQUIRE(cardAt<DesignEquationTable>(again).text == longText);
    REQUIRE(parseEquations(cardAt<DesignEquationTable>(again).text).equations->canonicalText() ==
            parseEquations(longText).equations->canonicalText());
}

TEST_CASE("a DEQATN that does not parse is an error, and in tolerant mode stays as text with a diagnostic",
          "[model][cards][design]")
{
    for (const std::string& text : {std::string("F(A,B)=A+"), std::string("F(A)=B"), std::string("F(A)=FOO(A)"),
                                    std::string("F(A)=SIN(A,A)"), std::string("A+B"), std::string("F(A)=A**")})
    {
        INFO(text);
        const std::string deck = designEquationCard(5, text);
        REQUIRE_THROWS_AS(readModelText(deck, true), base::DeckError);
        const Model tolerant = readModelText(deck, false);
        REQUIRE(tolerant.diagnostics().hasErrors());
        const auto error = tolerant.diagnostics().firstError();
        REQUIRE(error.has_value());
        REQUIRE(error->code == "card.bad-equation");
        REQUIRE(error->location.fieldName == "equation");
        // The card is kept as text and written back as it was.
        REQUIRE(rowCount<DesignEquationTable>(tolerant) == 0);
        REQUIRE(writeModelText(tolerant).find(text) != std::string::npos);
    }
    expectInvalid("DesignEquation;0;F(A)=A", "id");
    expectInvalid("DesignEquation;7", "equation");
}

// ------------------------------------------------------------------------------------------------------- DESVAR, DLINK

TEST_CASE("a DESVAR has its bounds as defaults and does not refuse a start outside them", "[model][cards][design]")
{
    const Model model = readBulk(
        "DesignVar;6;SKIN1;24.;8.;80.;0.3;9\nDesignVar;3;T;0.5\nDesignVar;4;OUT;5.;6.;7.\nDesignVar;5;UP;1.\n");
    const DesignVarCard full = cardAt<DesignVarTable>(model, 0);
    REQUIRE((full.id == 6 && full.entryLabel == "SKIN1" && full.initialValue == 24.0 && full.lowerBound == 8.0 &&
             full.upperBound == 80.0));
    REQUIRE(full.designVarMoveLimit == Approx(0.3));
    REQUIRE(full.discreteValuesId == 9);
    const DesignVarCard minimal = cardAt<DesignVarTable>(model, 1);
    REQUIRE((minimal.lowerBound == -1.0e20 && minimal.upperBound == 1.0e20));
    REQUIRE_FALSE(minimal.designVarMoveLimit.has_value());
    REQUIRE(minimal.discreteValuesId == 0);
    // A start outside the bounds is read: the check design.variable-bounds reports it.
    REQUIRE(cardAt<DesignVarTable>(model, 2).initialValue == 5.0);
    requireRoundTrip(model);
    // The bounds written when they are not the default.
    const std::string text = writeModelText(model);
    REQUIRE(text.find("DesignVar;3;T;0.5\n") != std::string::npos);
    REQUIRE(text.find("DesignVar;6;SKIN1;24.;8.;80.;0.3;9") != std::string::npos);

    expectInvalid("DesignVar;0;A;1.", "id");
    expectInvalid("DesignVar;1;;1.", "entryLabel");
    oor::testing::requireModelFileRoundTrip(readBulk("DesignVar;1;LONGLABEL1;1."));
    expectInvalid("DesignVar;1;A", "initialValue");
    expectInvalid("DesignVar;1;A;1.;;;-0.2", "designVarMoveLimit");
    expectInvalid("DesignVar;1;A;1.;;;;-1", "discreteValuesId");
    // The same ID twice: an error in strict mode, a warning in tolerant mode (the last card wins).
    REQUIRE_THROWS_AS(readModelText("[model]\nDesignVar;1;A;1.\nDesignVar;1;B;2.\n", true), base::DeckError);
}

TEST_CASE("a DLINK is a design variable that is a linear function of others", "[model][cards][design]")
{
    Model model = readBulk(
        "DesignVar;3;A;1.\n"
        "DesignVar;5;B;1.\n"
        "DesignVar;7;C;1.\n"
        "DesignVar;9;D;1.\n"
        "DesignVar;11;E;1.\n"
        "DesignVarLink;12;3;0.2;0.45;5;1.5;7;-2.;9;4.\n"
        "DesignVarLink;13;11;;;5;1.\n");
    const DesignVarLinkCard full = cardAt<DesignVarLinkTable>(model, 0);
    REQUIRE((full.id == 12 && full.dependent == 3 && full.constant0 == 0.2 && full.multiplier == 0.45));
    REQUIRE(full.terms == std::vector<DesignTerm>{{5, 1.5}, {7, -2.0}, {9, 4.0}});
    const DesignVarLinkCard plain = cardAt<DesignVarLinkTable>(model, 1);
    REQUIRE((plain.constant0 == 0.0 && plain.multiplier == 1.0));
    REQUIRE(unresolvedIn(model) == 0);
    const RecordingVisitor visitor = referencesOf<DesignVarLinkTable>(model);
    REQUIRE(lists(visitor, "dependentVarId", ReferenceTarget::DesignVariable));
    REQUIRE(lists(visitor, "independentIds", ReferenceTarget::DesignVariable));
    requireRoundTrip(model);

    expectInvalid("DesignVarLink;10;2;;;4", "coefficients");           // a coefficient is missing
    expectInvalid("DesignVarLink;10;2;;;;1.", "independentIds");       // a design variable is missing
    expectInvalid("DesignVarLink;10;2", "independentIds");             // no independent variable
    expectInvalid("DesignVarLink;10;2;;;4;1.;4;2.", "independentIds"); // the same variable twice
    expectInvalid("DesignVarLink;10;0;;;4;1.", "dependentVarId");
    // A design variable that does not exist is a reference that names nothing.
    Model dangling = readBulk("DesignVar;2;A;1.\nDesignVarLink;10;2;;;4;1.\n");
    REQUIRE(unresolvedIn(dangling) == 1);
}

// ---------------------------------------------------------------------------------------------------- the constants

TEST_CASE("a DTABLE has its constants as pairs, and an integer value is a table", "[model][cards][design]")
{
    Model model = readBulk(
        "DesignConstants;CK;2.718;W;6.4;M;7.2E+6;;;N;2.7E+6;L;250.;TABL;12\nTableDynamic1;12;;;0.;1.;1.;2.;EndTable\n");
    const DesignConstantsCard card = cardAt<DesignConstantsTable>(model);
    REQUIRE(card.entries.size() == 6);
    REQUIRE((card.entries[0].label == "CK" && card.entries[0].value == Approx(2.718)));
    REQUIRE((card.entries[4].label == "L" && card.entries[4].value == Approx(250.0)));
    REQUIRE((card.entries[5].label == "TABL" && card.entries[5].table == 12));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
    Model missing = readBulk("DesignConstants;TABL;12\n");
    REQUIRE(unresolvedIn(missing) == 1);

    expectInvalid("DesignConstants;A", "value");
    expectInvalid("DesignConstants;;1.", "label");
    REQUIRE_NOTHROW(readBulk("DesignConstants;TOOLONGLABEL;1."));
    expectInvalid("DesignConstants;A;xx", "value");
    expectInvalid("DesignConstants", "label");
}

TEST_CASE("a DTABLE2 names the field of a property, a material or a connection a constant is taken from",
          "[model][cards][design]")
{
    Model model = readBulk(
        "IsoMat;7;2.1E+11;;0.3\n"
        "ShellProp;10;7;0.01\n"
        "Node;1;;0.;0.;0.\n"
        "Node;2;;1.;0.;0.\n"
        "Bar2;4210;20;1;2;0.;;0.;1.\n"
        "BarProp;20;7;1.E-4;1.E-8;1.E-8;1.E-8\n"
        "DesignFieldConstants;SKINT;ShellProp;10;thickness;MATE7;IsoMat;7;youngsModulus;BARX2;Bar2;4210;"
        "orientation2\n");
    const DesignFieldConstantsCard card = cardAt<DesignFieldConstantsTable>(model);
    REQUIRE(card.entries.size() == 3);
    REQUIRE((card.entries[0].label == "SKINT" && card.entries[0].cardName == "ShellProp" &&
             card.entries[0].targetId == 10 && card.entries[0].field == "thickness"));
    REQUIRE((card.entries[2].cardName == "Bar2" && card.entries[2].targetId == 4210));
    REQUIRE(unresolvedIn(model) == 0);
    // Each ID is looked for in the kind the card names: a property, a material, an element.
    const RecordingVisitor visitor = referencesOf<DesignFieldConstantsTable>(model);
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Material,
                                                            ReferenceTarget::Element});
    // A card the library does not type names nothing it could look for.
    Model foreign = readBulk("DesignFieldConstants;P1;PWELDX;5;t\n");
    REQUIRE(unresolvedIn(foreign) == 0);
    Model dangling = readBulk("DesignFieldConstants;P1;ShellProp;5;thickness\n");
    REQUIRE(unresolvedIn(dangling) == 1);
    requireRoundTrip(model);

    expectInvalid("DesignFieldConstants;P1;ShellProp;10", "label");
    expectInvalid("DesignFieldConstants;P1;ShellProp;0;thickness", "targetId");
    expectInvalid("DesignFieldConstants", "label");
}

// ------------------------------------------------------------------------------------------------------------ DOPTPRM

TEST_CASE("a DOPTPRM keeps its pairs as the deck wrote them and the catalogue gives the defaults",
          "[model][cards][design]")
{
    Model model = readBulk(
        "DesignOptimParams;OptimizerPrintLevel;3;MaxDesignCycles;14;PropertyMoveLimit;0.5;PrintInterval;1;PrintItems;"
        "15;OptimizerName;ads;ADSCOD;4;DesignVarMoveLimit;1\n");
    const DesignOptimParamsCard card = cardAt<DesignOptimParamsTable>(model);
    REQUIRE(card.entries.size() == 8);
    REQUIRE((card.entries[0].name == "OptimizerPrintLevel" && card.entries[0].value.isInteger() &&
             card.entries[0].value.integer == 3));
    REQUIRE((card.entries[2].name == "PropertyMoveLimit" && card.entries[2].value.isReal() &&
             card.entries[2].value.real == 0.5));
    REQUIRE((card.entries[6].name == "ADSCOD"));  // a name the table does not list is kept
    REQUIRE(card.entries[5].value.isWord("ads")); // a word is kept as written
    requireRoundTrip(model);

    // The value of a parameter: the card, else the default of the format reference.
    REQUIRE(designOptimParamsNumber(model, "MaxDesignCycles").value() == 14.0);
    REQUIRE(designOptimParamsNumber(model, "DesignVarMoveLimit").value() == 1.0); // an integer written for a real
    REQUIRE(designOptimParamsNumber(model, "RelativeObjectiveConvergence").value() == 0.001);
    REQUIRE(designOptimParamsNumber(model, "ApproximationMethod").value() == 2.0);
    REQUIRE_FALSE(designOptimParamsNumber(model, "AbsoluteObjectiveChange").has_value()); // no static default
    REQUIRE_FALSE(designOptimParamsNumber(model, "NOSUCH").has_value());
    REQUIRE(designOptimParamsValue(model, "PropertyMoveLimit")->fromDeck);
    REQUIRE_FALSE(designOptimParamsValue(model, "MaxConstraintViolation")->fromDeck);
    REQUIRE_FALSE(designOptimParamsValue(Model{}, "MaxDesignCycles")->fromDeck);

    // The catalogue is consistent with itself: a default fits its kind, and every name is found.
    for (const DesignOptimParamsInfo& info : designOptimParamsCatalogue())
    {
        INFO(info.name);
        REQUIRE(findDesignOptimParam(info.name) == &info);
        if (info.hasDefault)
            REQUIRE(designOptimParamsValueFits(info, info.kind == DesignOptimParamsKind::Integer
                                                         ? DesignValue::ofInteger(1)
                                                         : DesignValue::ofReal(1.0)));
    }
    REQUIRE(designOptimParamsCatalogue().size() == 57);
    REQUIRE(designOptimParamsValueFits(*findDesignOptimParam("DesignVarMoveLimit"), DesignValue::ofInteger(1)));
    REQUIRE_FALSE(designOptimParamsValueFits(*findDesignOptimParam("MaxDesignCycles"), DesignValue::ofReal(5.0)));
    REQUIRE_FALSE(designOptimParamsValueFits(*findDesignOptimParam("OptimizerName"), DesignValue::ofInteger(1)));

    expectInvalid("DesignOptimParams;MaxDesignCycles", "value");
    expectInvalid("DesignOptimParams;;5", "parameterName");
    expectInvalid("DesignOptimParams", "parameterName");
}

TEST_CASE("a DOPTPRM reads the print flag of the analysis results under both of its spellings",
          "[model][cards][design]")
{
    // NASPRO is the keyword; NASPR0, with a zero, is the alias some decks write. Both are the same parameter.
    Model model = readBulk(
        "DesignOptimParams;PrintAnalysisEveryCycle;1;TrustRegionGrowthFactor;3.\nDesignOptimParams;"
        "PrintAnalysisEveryCycle;0\n");
    REQUIRE(cardAt<DesignOptimParamsTable>(model, 0).entries[0].name == "PrintAnalysisEveryCycle");
    REQUIRE(cardAt<DesignOptimParamsTable>(model, 0).entries[1].name == "TrustRegionGrowthFactor");
    REQUIRE(cardAt<DesignOptimParamsTable>(model, 1).entries[0].name == "PrintAnalysisEveryCycle");
    requireRoundTrip(model);
}

// ------------------------------------------------------------------------------------- DSCREEN, DCONSTR and DCONADD

TEST_CASE("a DSCREEN has its threshold and count as defaults", "[model][cards][design]")
{
    const Model model =
        readBulk("DesignConstrScreen;Stress;-0.6;3\nDesignConstrScreen;Displacement\nDesignConstrScreen;All;;5\n");
    REQUIRE(cardAt<DesignConstrScreenTable>(model, 0).responseKind == "Stress");
    REQUIRE((cardAt<DesignConstrScreenTable>(model, 0).threshold == -0.6 &&
             cardAt<DesignConstrScreenTable>(model, 0).maxRetained == 3));
    REQUIRE((cardAt<DesignConstrScreenTable>(model, 1).responseKind == "Displacement" &&
             cardAt<DesignConstrScreenTable>(model, 1).threshold == -0.5 &&
             cardAt<DesignConstrScreenTable>(model, 1).maxRetained == 20));
    REQUIRE(cardAt<DesignConstrScreenTable>(model, 2).maxRetained == 5);
    requireRoundTrip(model);
    expectInvalid("DesignConstrScreen", "responseKind");
    expectInvalid("DesignConstrScreen;Stress;-0.5;0", "maxRetained");
}

TEST_CASE("a DCONSTR bounds a response, by numbers or by tables, and joins the set DCID", "[model][cards][design]")
{
    Model model = readBulk(
        "DesignResp1;4;S;Stress;ShellProp;;9;;10\n"
        "DesignResp1;5;F;FreqDisplacement;;;1;;1\n"
        "ShellProp;10;1;0.01\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "Node;1;;0.;0.;0.\n"
        "TableDynamic1;21;;;0.;1.;1.;2.;EndTable\n"
        "TableDynamic1;22;;;0.;1.;1.;2.;EndTable\n"
        "DesignConstr;10;4;;;1.6\n"
        "DesignConstr;10;5;;21;;22;10.;200.\n"
        "DesignConstr;11;4;-1.E+3\n");
    const DesignConstrCard first = cardAt<DesignConstrTable>(model, 0);
    REQUIRE((first.constraintSetId == 10 && first.responseId == 4 && first.lowerAllowable == -1.0e20 &&
             first.upperAllowable == 1.6));
    REQUIRE((first.lowerAllowableTableId == 0 && first.upperAllowableTableId == 0 && first.lowFrequency == 0.0 &&
             first.highFrequency == 1.0e20));
    const DesignConstrCard table = cardAt<DesignConstrTable>(model, 1);
    REQUIRE((table.lowerAllowableTableId == 21 && table.upperAllowableTableId == 22 && table.lowFrequency == 10.0 &&
             table.highFrequency == 200.0));
    REQUIRE(unresolvedIn(model) == 0);
    const RecordingVisitor visitor = referencesOf<DesignConstrTable>(model, 1);
    REQUIRE(lists(visitor, "responseId", ReferenceTarget::DesignResponse));
    REQUIRE(lists(visitor, "lowerAllowableTableId", ReferenceTarget::DynamicTable));
    REQUIRE(lists(visitor, "upperAllowableTableId", ReferenceTarget::DynamicTable));
    requireRoundTrip(model);
    // Two cards of one set are not a duplicate: the DCID is a set number.
    REQUIRE(rowCount<DesignConstrTable>(model) == 3);

    expectInvalid("DesignConstr;0;4", "constraintSetId");
    expectInvalid("DesignConstr;10", "responseId");
    expectInvalid("DesignConstr;10;4;;;;;50.;10.", "highFrequency");
    expectInvalid("DesignConstr;10;4;;;;;-1.", "lowFrequency");
    Model dangling = readBulk("DesignConstr;10;4;;;1.\n");
    REQUIRE(unresolvedIn(dangling) == 1);
}

TEST_CASE("a DCONADD joins DCONSTR sets, and a DESSUB or DESGLB selects a set", "[model][cards][design][control]")
{
    const std::string cards =
        "DesignResp1;4;S;Stress;ShellProp;;9;;10\n"
        "ShellProp;10;1;0.01\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "DesignConstr;10;4;;;1.\n"
        "DesignConstr;12;4;;;2.\n"
        "DesignConstrUnion;20;10;12\n";
    Model model = readModelText(
        "[executive]\nSolutionSequence;200\n[case]\nGlobalDesignConstrSelection;20\nSubcaseStart;"
        "1\nSubcaseDesignConstrSelection;10\nDesignObjectiveSelection;4\n[model]\n" +
        cards);
    REQUIRE(cardAt<DesignConstrUnionTable>(model).sets == std::vector<base::EntityId>{10, 12});
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(resolveReference(model, ReferenceTarget::DesignConstraintSet, 20).has_value()); // a DCONADD
    REQUIRE(resolveReference(model, ReferenceTarget::DesignConstraintSet, 12).has_value()); // a DCONSTR
    REQUIRE_FALSE(resolveReference(model, ReferenceTarget::DesignConstraintSet, 13).has_value());
    // The commands name sets that do not exist.
    Model missing = readModelText(
        "[executive]\nSolutionSequence;200\n[case]\nGlobalDesignConstrSelection;21\nSubcaseStart;"
        "1\nSubcaseDesignConstrSelection;11\nDesignObjectiveSelection;5\n[model]\n" +
        cards);
    REQUIRE(unresolvedIn(missing) == 3);

    // The numbers follow a renumbering of the spaces they belong to, in the cards and in the commands.
    Renumbering::apply(model, RenumberSpace::DesignConstraintSet, Renumbering::offset(100));
    REQUIRE(cardAt<DesignConstrUnionTable>(model).constraintSetId == 120);
    REQUIRE(cardAt<DesignConstrUnionTable>(model).sets == std::vector<base::EntityId>{110, 112});
    REQUIRE(cardAt<DesignConstrTable>(model, 1).constraintSetId == 112);
    const std::string text = writeModelText(model);
    REQUIRE(text.find("GlobalDesignConstrSelection;120") != std::string::npos);
    REQUIRE(text.find("SubcaseDesignConstrSelection;110") != std::string::npos);
    Renumbering::apply(model, RenumberSpace::DesignResponse, Renumbering::offset(1000));
    REQUIRE(cardAt<DesignConstrTable>(model, 0).responseId == 1004);
    REQUIRE(writeModelText(model).find("DesignObjectiveSelection;1004") != std::string::npos);
    REQUIRE(unresolvedIn(model) == 0);

    expectInvalid("DesignConstrUnion;20", "memberSetId");
    expectInvalid("DesignConstrUnion;0;10", "constraintSetId");
}

// -------------------------------------------------------------------------------------- DVGRID, DVSHAP and DVBSHAP

TEST_CASE("a DVGRID moves a grid point by a vector, and several cards add", "[model][cards][design]")
{
    Model model = readBulk(
        "Node;117;;0.;0.;0.\n"
        "RectFrame2;6;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "DesignVar;4;X;1.\n"
        "DesignNodeMove;4;117;6;0.3;0.4;0.6;0.8\n"
        "DesignNodeMove;4;117;;;0.;0.;1.\n");
    const DesignNodeMoveCard first = cardAt<DesignNodeMoveTable>(model, 0);
    REQUIRE((first.designVarId == 4 && first.nodeId == 117 && first.frameId == 6 && first.coefficient == 0.3));
    REQUIRE((first.direction[0] == 0.4 && first.direction[1] == 0.6 && first.direction[2] == 0.8));
    const DesignNodeMoveCard second = cardAt<DesignNodeMoveTable>(model, 1);
    REQUIRE((second.frameId == 0 && second.coefficient == 0.0)); // the format reference default of COEFF is 0.0
    REQUIRE(unresolvedIn(model) == 0);
    const RecordingVisitor visitor = referencesOf<DesignNodeMoveTable>(model);
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::DesignVariable, ReferenceTarget::Node,
                                                            ReferenceTarget::Coordinate});
    requireRoundTrip(model);
    expectInvalid("DesignNodeMove;4;117;6;0.3", "directionX"); // at least one component is not zero
    expectInvalid("DesignNodeMove;4;0;6;0.3;1.", "nodeId");
    expectInvalid("DesignNodeMove;4;117;-1;0.3;1.", "frameId");
    Model dangling = readBulk("DesignNodeMove;4;117;6;0.3;1.\n");
    REQUIRE(unresolvedIn(dangling) == 3);
}

TEST_CASE("a DVSHAP and a DVBSHAP combine columns with scale factors", "[model][cards][design]")
{
    Model model = readBulk(
        "DesignVar;2;S;1.\nDesignVar;4;B;1.\nDesignShapeBasis;2;2;1.5;5;0.5\nDesignShapeBasis;2;3\nDesignBoundaryShape;"
        "4;1;1;1.4;2;;3;-1.\n");
    REQUIRE(cardAt<DesignShapeBasisTable>(model, 0).terms == std::vector<ShapeTerm>{{2, 1.5}, {5, 0.5}});
    REQUIRE(cardAt<DesignShapeBasisTable>(model, 1).terms == std::vector<ShapeTerm>{{3, 1.0}});
    const DesignBoundaryShapeCard boundary = cardAt<DesignBoundaryShapeTable>(model);
    REQUIRE((boundary.designVarId == 4 && boundary.auxiliaryModified == 1));
    REQUIRE(boundary.terms == std::vector<ShapeTerm>{{1, 1.4}, {2, 1.0}, {3, -1.0}});
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
    expectInvalid("DesignShapeBasis;2", "columns");
    expectInvalid("DesignShapeBasis;2;0;1.", "columns");
    expectInvalid("DesignShapeBasis;2;;1.", "columns");
    expectInvalid("DesignBoundaryShape;4;0;1;1.", "auxiliaryModified");
    expectInvalid("DesignBoundaryShape;4", "auxiliaryModified");
    Model dangling = readBulk("DesignShapeBasis;2;1;1.\n");
    REQUIRE(unresolvedIn(dangling) == 1);
}

// --------------------------------------------------------------------------------------------- the linear relations

TEST_CASE("a DVPREL1 drives a field of a property by name or by position", "[model][cards][design]")
{
    Model model = readBulk(
        "DesignVar;6;A;0.3\n"
        "DesignVar;21;B;1.\n"
        "DesignVar;8;C;1.\n"
        "BarProp;534;1;1.;1.;1.;1.\n"
        "IsoMat;1;2.1E+11;;0.15\n"
        "DesignPropRelation1;17;BarProp;534;bendingInertia1;0.3;2.5;0.4;6;0.35;21;12.;8;0.15\n"
        "DesignPropRelation1;13;BarProp;534;area(EndB);;;;6;1.\n"
        "DesignPropRelation1;14;BarProp;534;area;;;;6;PropertyValue\n"
        "DesignPropRelation1;15;GlobalPly;3;t;;;;6;1.\n");
    const DesignPropRelation1Card first = cardAt<DesignPropRelation1Table>(model, 0);
    REQUIRE((first.id == 17 && first.kind == "BarProp" && first.propId == 534));
    REQUIRE((first.field.name == "bendingInertia1"));
    REQUIRE((first.minValue == 0.3 && first.maxValue == 2.5 && first.constant0 == 0.4));
    REQUIRE(first.terms == std::vector<DesignTerm>{{6, 0.35}, {21, 12.0}, {8, 0.15}});
    REQUIRE_FALSE(first.fromProperty);
    const DesignPropRelation1Card byName = cardAt<DesignPropRelation1Table>(model, 1);
    REQUIRE((byName.field.name == "area(EndB)"));
    REQUIRE((byName.maxValue == 1.0e20 && byName.constant0 == 0.0));
    REQUIRE_FALSE(byName.minValue.has_value()); // its default depends on the field: not resolved
    const DesignPropRelation1Card property = cardAt<DesignPropRelation1Table>(model, 2);
    REQUIRE((property.field.name == "area" && property.fromProperty));
    REQUIRE(property.terms.size() == 1);
    requireRoundTrip(model);
    const std::string text = writeModelText(model);
    REQUIRE(text.find("DesignPropRelation1;14;BarProp;534;area;;;;6;PropertyValue\n") != std::string::npos);

    // The PID names a property of the type the card says; a ply (GPLY) and a type the library does not type name
    // nothing.
    const RecordingVisitor visitor = referencesOf<DesignPropRelation1Table>(model, 0);
    REQUIRE(lists(visitor, "propId", ReferenceTarget::Property));
    REQUIRE(lists(visitor, "designVarIds", ReferenceTarget::DesignVariable));
    REQUIRE(referencesOf<DesignPropRelation1Table>(model, 3).fields == std::vector<std::string>{"designVarIds"});
    REQUIRE(unresolvedIn(model) == 0);

    expectInvalid("DesignPropRelation1;17;BarProp;534;bendingInertia1", "designVarIds");
    expectInvalid("DesignPropRelation1;17;BarProp;534;0;;;;6;1.", "fieldName");
    expectInvalid("DesignPropRelation1;17;BarProp;534;1.5;;;;6;1.", "fieldName");
    expectInvalid("DesignPropRelation1;17;BarProp;534;;;;;6;1.", "fieldName");
    expectInvalid("DesignPropRelation1;17;;534;t;;;;6;1.", "kind");
    expectInvalid("DesignPropRelation1;17;BarProp;0;t;;;;6;1.", "propId");
    expectInvalid("DesignPropRelation1;17;BarProp;534;t;;;;6", "coefficients");
    expectInvalid("DesignPropRelation1;17;BarProp;534;t;;;;6;1.;8", "coefficients");
    expectInvalid("DesignPropRelation1;17;BarProp;534;t;;;;6;1.;8;PropertyValue", "coefficients");
    expectInvalid("DesignPropRelation1;17;BarProp;534;t;;;;6;1.;8;PropertyValue", "coefficients");
}

TEST_CASE("the material and connectivity relations are read like the property relation", "[model][cards][design]")
{
    Model model = readBulk(
        "DesignVar;1;A;1.\n"
        "IsoMat;5;2.1E+11;;0.3\n"
        "Node;1;;0.;0.;0.\n"
        "Node;2;;1.;0.;0.\n"
        "Bar2;100;20;1;2;0.;;0.;1.\n"
        "BarProp;20;5;1.;1.;1.;1.\n"
        "DesignMatRelation1;6;IsoMat;5;density;0.02;0.9;;1;1.\n"
        "DesignElemRelation1;5;Bar2;100;orientation1;;;0.5;1;PropertyValue\n");
    const DesignMatRelation1Card material = cardAt<DesignMatRelation1Table>(model);
    REQUIRE((material.id == 6 && material.kind == "IsoMat" && material.matId == 5 && material.name == "density"));
    REQUIRE((material.minValue == 0.02 && material.maxValue == 0.9 && material.constant0 == 0.0));
    REQUIRE(material.terms == std::vector<DesignTerm>{{1, 1.0}});
    const DesignElemRelation1Card element = cardAt<DesignElemRelation1Table>(model);
    REQUIRE((element.kind == "Bar2" && element.elemId == 100 && element.name == "orientation1" &&
             element.constant0 == 0.5));
    REQUIRE(element.fromProperty);
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(lists(referencesOf<DesignMatRelation1Table>(model), "matId", ReferenceTarget::Material));
    REQUIRE(lists(referencesOf<DesignElemRelation1Table>(model), "elemId", ReferenceTarget::Element));
    requireRoundTrip(model);
    // A material card that no card types (and those of other implementations) is not looked up.
    Model foreign = readBulk("DesignVar;1;A;1.\nDesignMatRelation1;5;MATXYZ;5;rHO;;;;1;1.\n");
    REQUIRE(unresolvedIn(foreign) == 0);
    Model dangling = readBulk(
        "DesignVar;1;A;1.\nDesignMatRelation1;5;IsoMat;5;density;;;;1;1.\nDesignElemRelation1;5;Bar2;100;"
        "orientation1;;;;1;1.\n");
    REQUIRE(unresolvedIn(dangling) == 2);
    expectInvalid("DesignMatRelation1;5;IsoMat;5;;;;;1;1.", "fieldName");
    expectInvalid("DesignMatRelation1;5;IsoMat;5;0;;;;1;1.", "fieldName");
    expectInvalid("DesignElemRelation1;5;Bar2;100;;;;;1;1.", "fieldName");
    expectInvalid("DesignElemRelation1;5;Bar2;0;orientation1;;;;1;1.", "elemId");
}

// ------------------------------------------------------------------------------------------ the equation relations

TEST_CASE("a DVPREL2 gives a field by an equation of design variables and constants", "[model][cards][design]")
{
    Model model =
        readBulk("DesignVar;1;A;1.\nDesignVar;2;B;1.\nDesignVar;3;C;1.\nShellProp;10;1;0.01\nIsoMat;1;2.1E+11;;0.3\n" +
                 designEquationCard(1, "F(A,B,C,D)=A*B+C*D") +
                 "DesignPropRelation2;13;ShellProp;10;thickness;0.15;0.35;1;DesignVar;3;1;2;3;DesignConstants;1;KT\n"
                 "DesignConstants;KT;2.5\n"
                 "DesignPropRelation2;14;ShellProp;10;thickness;;;1;DesignVar;8;1;2;;;;;;3\n");
    const DesignPropRelation2Card card = cardAt<DesignPropRelation2Table>(model, 0);
    REQUIRE((card.id == 13 && card.kind == "ShellProp" && card.propId == 10 && card.field.name == "thickness"));
    REQUIRE((card.minValue == 0.15 && card.maxValue == 0.35 && card.equationId == 1));
    REQUIRE(card.designVarIds == std::vector<base::EntityId>{1, 2, 3});
    REQUIRE(card.labels == std::vector<std::string>{"KT"});
    // The DESVAR line goes on over the lines after it.
    REQUIRE(cardAt<DesignPropRelation2Table>(model, 1).designVarIds == std::vector<base::EntityId>{1, 2, 3});
    REQUIRE(unresolvedIn(model) == 0);
    const RecordingVisitor visitor = referencesOf<DesignPropRelation2Table>(model, 0);
    REQUIRE(lists(visitor, "equationId", ReferenceTarget::DesignEquation));
    REQUIRE(lists(visitor, "propId", ReferenceTarget::Property));
    requireRoundTrip(model);
    // The DTABLE line before the DESVAR line is out of order.
    expectInvalid("DesignPropRelation2;13;ShellProp;10;thickness;;;1;DesignConstants;1;PI;DesignVar;1;1",
                  "groupKeyword");
    expectInvalid("DesignPropRelation2;13;ShellProp;10;thickness;;;1", "designVarIds");
    expectInvalid("DesignPropRelation2;13;ShellProp;10;thickness;;;;DesignVar;1;1", "equationId");
    expectInvalid("DesignPropRelation2;13;ShellProp;10;thickness;;;1;DesignVar;2;1;0", "designVarIds");
    Model dangling = readBulk("DesignPropRelation2;13;ShellProp;10;thickness;;;1;DesignVar;1;1\n");
    REQUIRE(unresolvedIn(dangling) == 3); // the property, the equation and the design variable
}

TEST_CASE("the material and connectivity relations by equation have the same groups", "[model][cards][design]")
{
    Model model = readBulk(
        "DesignVar;1;A;1.\n"
        "IsoMat;5;2.1E+11;;0.3\n"
        "Node;1;;0.;0.;0.\n"
        "Node;2;;1.;0.;0.\n"
        "Bar2;100;20;1;2;0.;;0.;1.\n"
        "BarProp;20;5;1.;1.;1.;1.\n" +
        designEquationCard(100, "G(A,B)=A+B") +
        "DesignMatRelation2;5;IsoMat;5;youngsModulus;0.03;1.5;100;DesignVar;1;1;DesignConstants;1;E0\n"
        "DesignElemRelation2;1;Bar2;100;orientation1;0.03;1.5;100;DesignVar;1;1;DesignConstants;1;X10\n"
        "DesignConstants;E0;2.1E+11;X10;1.5\n");
    const DesignMatRelation2Card material = cardAt<DesignMatRelation2Table>(model);
    REQUIRE((material.matId == 5 && material.name == "youngsModulus" && material.equationId == 100 &&
             material.minValue == 0.03));
    REQUIRE(material.designVarIds == std::vector<base::EntityId>{1});
    REQUIRE(material.labels == std::vector<std::string>{"E0"});
    const DesignElemRelation2Card element = cardAt<DesignElemRelation2Table>(model);
    REQUIRE(
        (element.elemId == 100 && element.name == "orientation1" && element.labels == std::vector<std::string>{"X10"}));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
    expectInvalid("DesignMatRelation2;5;IsoMat;5;youngsModulus;;;100", "designVarIds");
    expectInvalid("DesignElemRelation2;5;Bar2;100;orientation1;;;100", "designVarIds");
}

// ------------------------------------------------------------------------------------------------------- responses

TEST_CASE("a DRESP1 keeps its attributes as written and names what the response type says", "[model][cards][design]")
{
    Model model = readBulk(
        "Node;100;;0.;0.;0.\n"
        "Node;101;;1.;0.;0.\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "ShellProp;10;1;0.01\n"
        "ShellProp;11;1;0.01\n"
        "BarProp;20;1;1.;1.;1.;1.\n"
        "Node;5;;2.;0.;0.\n"
        "Node;6;;2.;1.;0.\n"
        "ShellQuad4;200;10;100;101;5;6\n"
        "ShellQuad4;201;11;100;101;5;6\n"
        "DesignResp1;1;SPX;Stress;PXYZ;2;3;;106\n"
        "DesignResp1;2;S1;Stress;ShellProp;;9;;10;11\n"
        "DesignResp1;3;S2;Stress;Elements;;9;;200;Through;201\n"
        "DesignResp1;4;U1;Displacement;;;1;;100;101\n"
        "DesignResp1;5;W;Weight;;;3;3;All\n"
        "DesignResp1;6;F;Frequency;;;1\n"
        "DesignResp1;7;CS;CompositeStress;CompProp;;1;2;10\n"
        "DesignResp1;8;G;NodeForce;;;1;;200\n"
        "DesignResp1;9;P;SpectralDisplacement;7;;1;;100\n"
        "DesignResp1;10;R;RootMeanSquareDisplacement;;;1;7;100\n"
        "DesignResp1;11;FR;FreqDisplacement;;;1;1.5;100\n"
        "DesignResp1;12;FRA;FreqDisplacement;;;1;Average;100\n"
        "DesignResp1;13;FL;Flutter;PkMethodNoLoop;;;;30;31;32;33\n"
        "DesignResp1;14;WM;WeightPerPropertyId;ShellProp;;1;0;10\n"
        "DesignResp1;15;GP;NodeForcePanel;;;100;;101\n"
        "IdListSet1;30;1;2;3\n"
        "RandomSpectralDensity;7;1;1;1.;0.\n");
    const DesignResp1Card stress = cardAt<DesignResp1Table>(model, 1);
    REQUIRE((stress.id == 2 && stress.entryLabel == "S1" && stress.responseKind == "Stress" &&
             stress.ownerKind.isWord("ShellProp")));
    REQUIRE(stress.attributeA.isInteger());
    REQUIRE(stress.attributeA.integer == 9);
    REQUIRE(stress.attributeB.isBlank());
    REQUIRE(
        (stress.attributes.size() == 2 && stress.attributes[0].integer == 10 && stress.attributes[1].integer == 11));
    REQUIRE(cardAt<DesignResp1Table>(model, 2).attributes[1].isWord("Through"));
    REQUIRE(cardAt<DesignResp1Table>(model, 4).attributes[0].isWord("All"));
    REQUIRE(cardAt<DesignResp1Table>(model, 10).attributeB.isReal());
    REQUIRE(cardAt<DesignResp1Table>(model, 11).attributeB.isWord("Average"));
    REQUIRE(cardAt<DesignResp1Table>(model, 8).ownerKind.isInteger());

    // What each integer names: the targets of every card of the model.
    const auto targetsOf = [&](std::uint32_t row) { return referencesOf<DesignResp1Table>(model, row).targets; };
    REQUIRE(targetsOf(0).empty()); // PXYZ is no card the library types
    REQUIRE(targetsOf(1) == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Property});
    REQUIRE(targetsOf(2) ==
            std::vector<ReferenceTarget>{ReferenceTarget::ElementInRange, ReferenceTarget::ElementInRange});
    REQUIRE(targetsOf(3) == std::vector<ReferenceTarget>{ReferenceTarget::Node, ReferenceTarget::Node});
    REQUIRE(targetsOf(4).empty());
    REQUIRE(targetsOf(5).empty());
    REQUIRE(targetsOf(6) == std::vector<ReferenceTarget>{ReferenceTarget::Property});
    REQUIRE(targetsOf(7) == std::vector<ReferenceTarget>{ReferenceTarget::Element});
    REQUIRE(targetsOf(8) == std::vector<ReferenceTarget>{ReferenceTarget::RandomSet, ReferenceTarget::Node});
    REQUIRE(targetsOf(9) == std::vector<ReferenceTarget>{ReferenceTarget::RandomSet, ReferenceTarget::Node});
    REQUIRE(targetsOf(10) == std::vector<ReferenceTarget>{ReferenceTarget::Node});
    REQUIRE(targetsOf(12) == std::vector<ReferenceTarget>{ReferenceTarget::Set});
    REQUIRE(targetsOf(13) == std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::Property});
    REQUIRE(targetsOf(14) == std::vector<ReferenceTarget>{ReferenceTarget::Node, ReferenceTarget::Node});
    requireRoundTrip(model);
    const std::string text = writeModelText(model);
    REQUIRE(text.find("DesignResp1;5;W;Weight;;;3;3;All") != std::string::npos);

    // An element that the response names and the model lacks is reported; so is the end of a range that names no
    // element.
    Model dangling = readBulk(
        "ShellProp;10;1;0.01\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "DesignResp1;2;S1;Stress;ShellProp;;9;;10;11\n"
        "DesignResp1;3;S2;Stress;Elements;;9;;5;Through;9\n"
        "DesignResp1;4;U1;Displacement;;;1;;100;101\n");
    REQUIRE(unresolvedIn(dangling) == 3);

    expectInvalid("DesignResp1;1;;Stress;ShellProp;;9;;10", "entryLabel");
    expectInvalid("DesignResp1;1;S1;;ShellProp;;9;;10", "responseKind");
    expectInvalid("DesignResp1;0;S1;Stress", "id");
    expectInvalid("DesignResp1;1;S1;Stress;1.5;;9;;10", "ownerKind");
    expectInvalid("DesignResp1;1;S1;Stress;ShellProp;-1;9;;10", "regionId");
    // a word of 65 characters: longer than any name of the format
    oor::testing::requireModelFileRoundTrip(
        readBulk("DesignResp1;1;S1;Stress;ShellProp;;9;;10;" + std::string(650, 'A')));
}

TEST_CASE("a DRESP2 gives a response by an equation or a function over groups of arguments", "[model][cards][design]")
{
    Model model = readBulk(
        "DesignVar;104;A;1.\n"
        "DesignVar;7;B;1.\n"
        "Node;26;;0.;0.;0.\n"
        "ShellProp;10;1;0.01\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "DesignResp1;24;S;Stress;ShellProp;;9;;10\n"
        "DesignResp1;25;T;Stress;ShellProp;;9;;10\n" +
        designEquationCard(8, "G2(P,Q,S,U,W)=P*Q+S-(U**2-3.0)+COS(PI(1)*W)") +
        "DesignPropRelation1;111;ShellProp;10;thickness;;;;104;1.\n"
        "DesignPropRelation2;112;ShellProp;10;thickness;;;8;DesignVar;1;104\n"
        "DesignMatRelation1;211;IsoMat;1;youngsModulus;;;;7;1.\n"
        "DesignResp2;1;PNLBKL;8;;6;;;;;DesignVar;2;104;7;DesignConstants;2;CK;KS;DesignResp1;2;24;25;DesignNodes;1;26;"
        "2;DesignPropRelation1;1;111;DesignMatRelation1;"
        "1;211;DesignPropRelation2;1;112\n"
        "DesignResp2;2;AVGFD;;Average;;;;;;DesignResp1;2;24;25\n"
        "DesignResp2;3;MATCHER;;MatchedResponses;;LeastSquares;2.;0.1;5.;DesignResp1;1;24\n"
        "DesignResp2;4;OTHER;8;;;;;;;DesignResp2;1;1\n"
        "DesignConstants;CK;2.5;KS;7.2E+10\n");
    const DesignResp2Card card = cardAt<DesignResp2Table>(model, 0);
    REQUIRE((card.id == 1 && card.entryLabel == "PNLBKL" && card.equationId == 8 && card.function.empty() &&
             card.regionId == 6));
    REQUIRE((card.constant1 == 1.0 && card.constant2 == 0.005 && card.constant3 == 10.0 && card.methodName.empty()));
    REQUIRE(findDesignGroup(card.groups, "DesignVar")->ids == std::vector<base::EntityId>{104, 7});
    REQUIRE(findDesignGroup(card.groups, "DesignConstants")->words == std::vector<std::string>{"CK", "KS"});
    REQUIRE(findDesignGroup(card.groups, "DesignNodes")->ids == std::vector<base::EntityId>{26, 2});
    REQUIRE(findDesignGroup(card.groups, "DesignMatRelation1")->ids == std::vector<base::EntityId>{211});
    // The groups come in the order of the format reference, which is the order of the arguments.
    std::vector<std::string> order;
    for (const DesignGroup& group : card.groups)
        order.push_back(group.keyword);
    REQUIRE(order == std::vector<std::string>{"DesignVar", "DesignConstants", "DesignResp1", "DesignNodes",
                                              "DesignPropRelation1", "DesignMatRelation1", "DesignPropRelation2"});
    const DesignResp2Card function = cardAt<DesignResp2Table>(model, 1);
    REQUIRE((function.function == "Average" && function.equationId == 0));
    const DesignResp2Card match = cardAt<DesignResp2Table>(model, 2);
    REQUIRE((match.function == "MatchedResponses" && match.methodName == "LeastSquares" && match.constant1 == 2.0 &&
             match.constant2 == 0.1 && match.constant3 == 5.0));
    REQUIRE(unresolvedIn(model) == 0);
    const RecordingVisitor visitor = referencesOf<DesignResp2Table>(model, 0);
    REQUIRE(lists(visitor, "equationId", ReferenceTarget::DesignEquation));
    REQUIRE(lists(visitor, "designVarIds", ReferenceTarget::DesignVariable));
    REQUIRE(lists(visitor, "responseIds", ReferenceTarget::DesignResponse));
    REQUIRE(lists(visitor, "nodeIds", ReferenceTarget::Node));
    REQUIRE(lists(visitor, "propRelationIds", ReferenceTarget::PropertyRelation));
    REQUIRE(lists(visitor, "matRelationIds", ReferenceTarget::MaterialRelation));
    requireRoundTrip(model);
    const std::string text = writeModelText(model);
    REQUIRE(text.find(";DesignNodes;1;26;2;") != std::string::npos);

    // A name that is no function and no number, a missing group, a group out of order, a component other than 1 to 3.
    expectInvalid("DesignResp2;1;X;;FOO;;;;;;DesignVar;1;1", "function");
    expectInvalid("DesignResp2;1;X;5", "groupKeyword");
    expectInvalid("DesignResp2;1;X;5;;;;;;;DesignResp1;1;1;DesignVar;1;2", "groupKeyword");
    expectInvalid("DesignResp2;1;X;5;;;;;;;DesignNodes;1;1;4", "nodeComponents");
    expectInvalid("DesignResp2;1;X;5;;;;;;;DesignNodes;2;1;1;2", "groupKeyword");
    expectInvalid("DesignResp2;1;X;5;;;BIG;;;;DesignVar;1;1", "methodName");
    expectInvalid("DesignResp2;1;;5;;;;;;;DesignVar;1;1", "entryLabel");
    expectInvalid("DesignResp2;1;X;;;;;;;;DesignVar;1;1", "equationId");
}

TEST_CASE("a DRESP3 names a group and a type of external response and keeps its user data", "[model][cards][design]")
{
    Model model = readBulk(
        "DesignVar;101;A;1.\n"
        "DesignVar;3;B;1.\n"
        "Node;14;;0.;0.;0.\n"
        "DesignResp3;1;PNLRSP;FINGRP;CRIP;;DesignVar;2;101;3;DesignConstants;1;KT;DesignNodes;1;14;1;UserData;1;"
        "Factors: 2468.5 31.25 7.\n"
        "DesignConstants;KT;2.5\n");
    const DesignResp3Card card = cardAt<DesignResp3Table>(model);
    REQUIRE((card.id == 1 && card.groupName == "FINGRP" && card.type == "CRIP" && card.regionId == 0));
    REQUIRE(findDesignGroup(card.groups, "DesignVar")->ids == std::vector<base::EntityId>{101, 3});
    REQUIRE(findDesignGroup(card.groups, "UserData")->words == std::vector<std::string>{"Factors: 2468.5 31.25 7."});
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
    expectInvalid("DesignResp3;1;X;GRP;TYP", "groupKeyword");
    expectInvalid("DesignResp3;1;X;;TYP;;DesignVar;1;1", "groupName");
    expectInvalid("DesignResp3;1;X;GRP;;;DesignVar;1;1", "kind");
}

TEST_CASE("the responses of the three cards share one number space", "[model][cards][design]")
{
    // Each type is unique on its own for the reader; the check design.response-id-repeated reports the ID two of them
    // share.
    Model model =
        readBulk("DesignVar;1;A;1.\nDesignResp1;5;W;Weight;;;1;;All\nDesignResp2;5;X;;Sum;;;;;;DesignVar;1;1\n");
    REQUIRE(rowCount<DesignResp1Table>(model) == 1);
    REQUIRE(rowCount<DesignResp2Table>(model) == 1);
    REQUIRE_THROWS_AS(readModelText("[model]\nDesignResp1;5;W;Weight;;;1;;All\nDesignResp1;5;V;Volume\n", true),
                      base::DeckError);
}

// --------------------------------------------------------------------------------------------------------- TOPVAR

TEST_CASE("a TOPVAR has its constraint lines each once and its density limits as written", "[model][cards][design]")
{
    Model model = readBulk(
        "ShellProp;1;1;0.01\n"
        "SolidProp;2;1;0\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "RectFrame2;3;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "DesignTopologyRegion;1;SHTOP;ShellProp;0.35;0.001;0.2;3.;1\n"
        "DesignTopologyRegion;2;SL2;SolidProp;0.4;0.001;;;2;Symmetry;3;PlaneXy;PlaneYz;;ZDirection;4;Casting;3;"
        "NegativeXDirection;2;Yes;Overhang;3;ZDirection;MemberSize;0.1;0.5;"
        "Stress;5.\n"
        "DesignTopologyRegion;3;EX;SolidProp;;;;;2;Extrusion;3;YDirection\n");
    const DesignTopologyRegionCard plain = cardAt<DesignTopologyRegionTable>(model, 0);
    REQUIRE(
        (plain.id == 1 && plain.entryLabel == "SHTOP" && plain.propType == "ShellProp" && plain.initialValue == 0.35));
    REQUIRE((plain.lowerBound == 0.001 && plain.designVarMoveLimit == 0.2 && plain.penaltyPower == 3.0 &&
             plain.propId == 1));
    REQUIRE_FALSE(plain.symmetry.has_value());
    const DesignTopologyRegionCard lines = cardAt<DesignTopologyRegionTable>(model, 1);
    REQUIRE_FALSE(lines.designVarMoveLimit.has_value());
    REQUIRE_FALSE(lines.penaltyPower.has_value());
    REQUIRE((lines.symmetry->frameId == 3 && lines.symmetry->mirrorPlanes[0] == "PlaneXy" &&
             lines.symmetry->mirrorPlanes[1] == "PlaneYz" && lines.symmetry->mirrorPlanes[2].empty() &&
             lines.symmetry->cyclicAxis == "ZDirection" && lines.symmetry->cyclicSegments == 4));
    REQUIRE((lines.casting->direction == "NegativeXDirection" && lines.casting->dies == 2 &&
             lines.casting->alignment == "Yes"));
    REQUIRE((lines.overhang->direction == "ZDirection"));
    REQUIRE((lines.memberSize->minimum == 0.1 && lines.memberSize->maximum == 0.5));
    REQUIRE(lines.stressLimit == 5.0);
    const DesignTopologyRegionCard extruded = cardAt<DesignTopologyRegionTable>(model, 2);
    REQUIRE((extruded.extrusion->direction == "YDirection" && extruded.extrusion->alignment == "No"));
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(lists(referencesOf<DesignTopologyRegionTable>(model, 1), "propId", ReferenceTarget::Property));
    REQUIRE(lists(referencesOf<DesignTopologyRegionTable>(model, 1), "frameId", ReferenceTarget::Coordinate));
    requireRoundTrip(model);

    expectInvalid("DesignTopologyRegion;1;A;ShellProp;;;;;1;Symmetry;;PlaneXy;;;;;Symmetry;;PlaneYz", "lineKeyword");
    expectInvalid("DesignTopologyRegion;1;A;ShellProp;;;;;1;Symmetry;;AB", "mirrorPlanes");
    expectInvalid("DesignTopologyRegion;1;A;ShellProp;;;;;1;Symmetry;;;;;ZDirection", "cyclicSegments");
    expectInvalid("DesignTopologyRegion;1;A;ShellProp;;;;;1;Casting;;Q", "castDirection");
    expectInvalid("DesignTopologyRegion;1;A;ShellProp;;;;;1;Casting;;XDirection;3", "dieCount");
    expectInvalid("DesignTopologyRegion;1;A;ShellProp;;;;;1;MemberSize;0.5;0.1", "maxMemberSize");
    expectInvalid("DesignTopologyRegion;1;A;ShellProp;;;;3.", "propId");
    expectInvalid("DesignTopologyRegion;1;A;ShellProp;;;;0.5;1", "penaltyPower");
    expectInvalid("DesignTopologyRegion;1;A;ShellProp;;;-0.1;;1", "designVarMoveLimit");
}

// -------------------------------------------------------------------------------- references and renumbering

TEST_CASE("the design cards of a small model resolve and follow every renumbering", "[model][cards][design]")
{
    Model model = readModelText(
        "[executive]\nSolutionSequence;200\n[case]\nDesignObjectiveSelection;51;"
        "MinimumObjective\nSubcaseDesignConstrSelection;70\n[model]\n" +
        DESIGN_DECK);
    REQUIRE(unresolvedIn(model) == 0);

    Renumbering::apply(model, RenumberSpace::DesignVariable, Renumbering::offset(100));
    REQUIRE(cardAt<DesignVarTable>(model, 2).id == 103);
    REQUIRE(cardAt<DesignPropRelation1Table>(model, 0).terms[0].designVarId == 101);
    REQUIRE(cardAt<DesignMatRelation1Table>(model).terms[0].designVarId == 103);
    REQUIRE(cardAt<DesignElemRelation1Table>(model).terms[0].designVarId == 101);
    REQUIRE(cardAt<DesignPropRelation2Table>(model).designVarIds == std::vector<base::EntityId>{101, 102});
    REQUIRE(findDesignGroup(cardAt<DesignResp2Table>(model).groups, "DesignVar")->ids ==
            std::vector<base::EntityId>{101, 102});

    Renumbering::apply(model, RenumberSpace::DesignEquation, Renumbering::offset(10));
    REQUIRE(cardAt<DesignEquationTable>(model).id == 11);
    REQUIRE(cardAt<DesignPropRelation2Table>(model).equationId == 11);

    Renumbering::apply(model, RenumberSpace::DesignResponse, Renumbering::offset(1000));
    REQUIRE(cardAt<DesignResp1Table>(model, 0).id == 1051);
    REQUIRE(cardAt<DesignResp2Table>(model).id == 1061);
    REQUIRE(findDesignGroup(cardAt<DesignResp2Table>(model).groups, "DesignResp1")->ids ==
            std::vector<base::EntityId>{1051, 1052});
    REQUIRE(cardAt<DesignConstrTable>(model, 0).responseId == 1051);
    oor::testing::requireModelFileRoundTrip(model);

    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(5));
    REQUIRE(cardAt<DesignPropRelation1Table>(model, 0).propId == 15);
    REQUIRE(cardAt<DesignPropRelation1Table>(model, 1).propId == 25);
    REQUIRE(lists(referencesOf<DesignResp1Table>(model, 0), "attributes", ReferenceTarget::Property));
    REQUIRE(cardAt<DesignResp1Table>(model, 0).attributes[0].integer == 15);
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(1));
    REQUIRE(cardAt<DesignMatRelation1Table>(model).matId == 4);
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(1));
    REQUIRE(cardAt<DesignElemRelation1Table>(model).elemId == 101);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(10));
    REQUIRE(cardAt<DesignResp1Table>(model, 1).attributes[0].integer == 11);
    REQUIRE(unresolvedIn(model) == 0);

    // The relations as the lists of a DRESP2 name them.
    Model relations = readBulk(
        "DesignVar;1;A;1.\n"
        "ShellProp;10;1;0.01\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "Node;1;;0.;0.;0.\n"
        "Node;2;;1.;0.;0.\n"
        "Bar2;100;20;1;2;0.;;0.;1.\n"
        "BarProp;20;1;1.;1.;1.;1.\n"
        "DesignPropRelation1;7;ShellProp;10;thickness;;;;1;1.\n"
        "DesignMatRelation1;8;IsoMat;1;youngsModulus;;;;1;1.\n"
        "DesignElemRelation1;9;Bar2;100;orientation1;;;;1;1.\n"
        "DesignResp2;1;R;;Sum;;;;;;DesignPropRelation1;1;7;DesignElemRelation1;1;9;DesignMatRelation1;1;8\n");
    REQUIRE(unresolvedIn(relations) == 0);
    Renumbering::apply(relations, RenumberSpace::PropertyRelation, Renumbering::offset(100));
    Renumbering::apply(relations, RenumberSpace::MaterialRelation, Renumbering::offset(200));
    Renumbering::apply(relations, RenumberSpace::ConnectivityRelation, Renumbering::offset(300));
    const DesignResp2Card response = cardAt<DesignResp2Table>(relations);
    REQUIRE(findDesignGroup(response.groups, "DesignPropRelation1")->ids == std::vector<base::EntityId>{107});
    REQUIRE(findDesignGroup(response.groups, "DesignMatRelation1")->ids == std::vector<base::EntityId>{208});
    REQUIRE(findDesignGroup(response.groups, "DesignElemRelation1")->ids == std::vector<base::EntityId>{309});
    REQUIRE(cardAt<DesignPropRelation1Table>(relations).id == 107);
    REQUIRE(unresolvedIn(relations) == 0);
    // A relation the model lacks is reported.
    Model lacking = readBulk("DesignVar;1;A;1.\nDesignResp2;1;R;;Sum;;;;;;DesignPropRelation1;1;7\n");
    REQUIRE(unresolvedIn(lacking) == 1);
}

TEST_CASE("a PBUSH1D of type EQUAT and a CORD3G of form EQN name their equations", "[model][cards][design]")
{
    const std::string equations = designEquationCard(30, "F(X)=2.*X") + designEquationCard(31, "G(X)=3.*X") +
                                  designEquationCard(32, "H(X)=2.") + designEquationCard(33, "K(X)=3.");
    const std::string cards =
        "BushProp1D;1;;;;;;Spring;Equation;30;31;32;33\nGeneralFrame3;7;EulerAngles313;EquationForm;30;31;30\n";
    Model model = readBulk(equations + cards);
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::DesignEquation, Renumbering::offset(5));
    const BushProp1DCard bush = cardAt<BushProp1DTable>(model);
    REQUIRE((bush.spring.tensionCurveId == 35 && bush.spring.compressionCurveId == 36 &&
             bush.spring.tensionDerivativeId == 37 && bush.spring.compressionDerivativeId == 38));
    REQUIRE(unresolvedIn(model) == 0);
    // Without the equations the six numbers of the two cards name nothing.
    Model missing = readBulk(cards);
    REQUIRE(unresolvedIn(missing) == 7);
    // A table is a table: the same numbers of a curve of type TABLE are looked up among the TABLEDi.
    Model table = readBulk("BushProp1D;1;;;;;;Spring;Table;30\n");
    REQUIRE(unresolvedIn(table) == 2);
}
