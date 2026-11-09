// Checks of what the cards of the explicit solver share, on the three cards that show it: EOSGAM (fixed
// fields with optional reals), PEULER (a property: a keyword with a default and a reference) and COUOPT (a set ID, a
// reference and quantities that are a number or a table). The keywords, the quantity of a number or a table, the new ID
// spaces and the references to them are tested here once; the cards of the groups have their own test files.

#include "oor/model/cards/explicit/couplingOptions/CouplingOptionsTable.hpp"
#include "oor/model/cards/explicit/eulerProp/EulerPropTable.hpp"
#include "oor/model/cards/explicit/explicitTextEnd/ExplicitTextEndTable.hpp"
#include "oor/model/cards/explicit/explicitTextStart/ExplicitTextStartTable.hpp"
#include "oor/model/cards/explicit/gammaLawStateEquation/GammaLawStateEquationTable.hpp"
#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"
#include "oor/model/core/UnknownCardTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

TEST_CASE("an EOSGAM has its fields in the order of the format reference and writes the ones it has",
          "[model][cards][explicit]")
{
    const Model model = readBulk(
        "GammaLawStateEquation;2;1.4;287.;;;1.8E-5\nGammaLawStateEquation;3\nGammaLawStateEquation;4;;;718.;1005.;0.");
    const GammaLawStateEquationCard full = cardAt<GammaLawStateEquationTable>(model, 0);
    REQUIRE((full.id == 2 && full.gammaCoefficient == 1.4 && full.gasConstant == 287.0 && !full.specificHeatVolume &&
             !full.specificHeatPressure && full.viscosity == 0.000018));
    const GammaLawStateEquationCard bare = cardAt<GammaLawStateEquationTable>(model, 1);
    REQUIRE((bare.id == 3 && !bare.gammaCoefficient && !bare.gasConstant && !bare.specificHeatVolume &&
             !bare.specificHeatPressure && !bare.viscosity));
    const GammaLawStateEquationCard constants = cardAt<GammaLawStateEquationTable>(model, 2);
    REQUIRE((constants.specificHeatVolume == 718.0 && constants.specificHeatPressure == 1005.0 &&
             constants.viscosity == 0.0)); // no viscous stresses
    requireRoundTrip(model);

    expectInvalid("GammaLawStateEquation;0;1.4", "id");
    expectInvalid("GammaLawStateEquation;1;-1.4", "gammaCoefficient");
    expectInvalid("GammaLawStateEquation;1;1.4;0.", "gasConstant");
    expectInvalid("GammaLawStateEquation;1;1.4;;;;-1.", "viscosity");
    expectInvalid("GammaLawStateEquation;1;GAS", "gammaCoefficient");
}

TEST_CASE("a PEULER is a property: its ID is shared with the other properties and its MID is a material",
          "[model][cards][explicit]")
{
    Model model = readBulk(
        "EulerProp;100;25\nEulerProp;101;26;MultiMaterialStrength\nEulerProp;102\nEulerMat;25;1.E+3;1\nEulerMat;26;1.E+"
        "3;"
        "1\nGammaLawStateEquation;1");
    REQUIRE(cardAt<EulerPropTable>(model, 0).type == EulerSolverType::Hydro); // the default
    REQUIRE(cardAt<EulerPropTable>(model, 1).type == EulerSolverType::MultiMaterialStrength);
    REQUIRE(cardAt<EulerPropTable>(model, 2).matId == 0); // void elements
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<EulerPropTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material});

    // The property space is shared by every property type; the material space by every material.
    REQUIRE_THROWS_AS(readBulk("EulerProp;100;25\nShellProp;100;1;0.1"), base::DeckError);
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(1000));
    REQUIRE((cardAt<EulerPropTable>(model, 0).matId == 1025 && cardAt<EulerPropTable>(model, 1).matId == 1026));
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(5));
    REQUIRE(cardAt<EulerPropTable>(model, 0).id == 105);

    Model dangling = readBulk("EulerProp;100;25");
    REQUIRE(unresolvedIn(dangling) == 1);
    requireRoundTrip(model);

    expectInvalid("EulerProp;100;25;PLASTIC", "solverType");
    expectInvalid("EulerProp;100;-25", "matId");
    expectInvalid("EulerProp;100;;FirstOrder", "matId"); // the Riemann solvers have no void elements
    expectInvalid("EulerProp;100;;SecondOrder", "matId");
    expectInvalid("EulerProp;0;25", "id");
}

TEST_CASE(
    "a COUOPT is filed under the set ID of its OPTID, and its quantities are a number or a table; PLCOVER is "
    "on the continuation line",
    "[model][cards][explicit]")
{
    Model model = readBulk(
        "CouplingOptions;1;10;0\n"
        "CouplingOptions;2;10;20;Constant;0.5;Table;30\n"
        "CouplingOptions;3;11;21;;;;7.5\n"
        "ContactElemSurface;20;1\n"
        "ContactElemSurface;21;1\n"
        "Node;1\n"
        "Node;2;;1.\n"
        "Node;3;;1.;1.\n"
        "Node;4;;0.;1.\n"
        "ShellQuad4;1;1;1;2;3;4\n"
        "ShellProp;1;1;0.1\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "TableDynamic1;30;;;0.;1.;1.;1.;EndTable");
    const CouplingOptionsCard whole = cardAt<CouplingOptionsTable>(model, 0);
    REQUIRE((whole.id == 1 && whole.setId == 10 && whole.subsurfaceId == 0));
    REQUIRE((whole.factor.source == ValueSource::Constant && whole.factor.constant == 1.0)); // the defaults
    REQUIRE((whole.coverPressure.source == ValueSource::Constant && whole.coverPressure.constant == 0.0));
    const CouplingOptionsCard part = cardAt<CouplingOptionsTable>(model, 1);
    REQUIRE((part.subsurfaceId == 20 && part.factor.constant == 0.5));
    REQUIRE((part.coverPressure.source == ValueSource::Table && part.coverPressure.table == 30));
    REQUIRE(cardAt<CouplingOptionsTable>(model, 2).coverPressure.constant == 7.5); // a blank PLCOVER is CONSTANT

    // Two cards of one set share the ID.
    REQUIRE(rowCount<CouplingOptionsTable>(model) == 3);
    REQUIRE(model.table<CouplingOptionsTable>().idAtRow(0) == model.table<CouplingOptionsTable>().idAtRow(1));
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<CouplingOptionsTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"subsurfaceId", "coverPressure"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::ContactSurface, ReferenceTarget::DynamicTable});
    visitor = RecordingVisitor{};
    model.table<CouplingOptionsTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"subsurfaceId"}); // a constant visits no table

    Renumbering::apply(model, RenumberSpace::CouplingOptions, Renumbering::offset(100));
    REQUIRE(
        (cardAt<CouplingOptionsTable>(model, 0).setId == 110 && cardAt<CouplingOptionsTable>(model, 2).setId == 111));
    Renumbering::apply(model, RenumberSpace::ContactSurface, Renumbering::offset(1000));
    REQUIRE(cardAt<CouplingOptionsTable>(model, 1).subsurfaceId == 1020);
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(500));
    REQUIRE(cardAt<CouplingOptionsTable>(model, 1).coverPressure.table == 530);
    requireRoundTrip(model);

    Model dangling = readBulk("CouplingOptions;1;10;99;;;Table;98");
    REQUIRE(unresolvedIn(dangling) == 2);

    expectInvalid("CouplingOptions;0;10", "id");
    expectInvalid("CouplingOptions;1;0", "setId");
    expectInvalid("CouplingOptions;1;10;-1", "subsurfaceId");
    expectInvalid("CouplingOptions;1;10;;Table;5", "factorSource"); // the factor is a constant only
    expectInvalid("CouplingOptions;1;10;;;-1.", "factor");
    expectInvalid("CouplingOptions;1;10;;;;Table", "coverPressure");
    expectInvalid("CouplingOptions;1;10;;;;Element", "coverPressureSource");
    expectInvalid("CouplingOptions;1;10;;;;Table;0", "coverPressure");
    expectInvalid("CouplingOptions;1;10;;;;PRESSURE;1.", "coverPressureSource");
}

TEST_CASE("the cards between TODYNA and ENDDYNA are for an external explicit solver and are kept as text",
          "[model][cards][explicit]")
{
    // The external material record has its own schema and is preserved as an opaque record.
    const std::string deck =
        "[model]\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "ExplicitTextStart\n"
        "MAT1;1;30.5E6;0.29;0.00072\n"
        "PARAM;NOTEXT;YES\n"
        "ExplicitTextEnd\n"
        "IsoMat;2;3.E+7;;0.3\n";
    const Model model = readModelText(deck, true);
    REQUIRE_FALSE(model.diagnostics().hasErrors());
    REQUIRE(rowCount<ExplicitTextStartTable>(model) == 1);
    REQUIRE(rowCount<ExplicitTextEndTable>(model) == 1);
    // The two opaque records retain their positions inside the explicit solver block.
    REQUIRE(model.cardCount() == 6);
    REQUIRE(rowCount<UnknownCardTable>(model) == 2);
    REQUIRE(rowCount<IsoMatTable>(model) == 2); // the MAT1 before and the one after the block

    requireRoundTrip(model);

    // A block that is not closed keeps the rest of the model as text.
    const Model open = readModelText("[model]\nExplicitTextStart\nMAT1;1;30.5E6;0.29;0.00072\n", true);
    REQUIRE(rowCount<UnknownCardTable>(open) == 1);
}
