// Checks of the remaining materials: MAT3, MAT9, MAT10, MAT11, MATHP, MATHE, the companions of a material (MATT1 to
// MATT11, MATS1, CREEP, MATEP, MATVE) and the tables of stress dependent and heat transfer properties (TABLES1,
// TABLEST, TABLEH1, TABLEHT): field positions, defaults, variants, references, renumbering and rules.

#include "oor/model/cards/materials/anisoMat2DTemperature/AnisoMat2DTemperatureTable.hpp"
#include "oor/model/cards/materials/anisoMat3D/AnisoMat3DTable.hpp"
#include "oor/model/cards/materials/anisoMat3DTemperature/AnisoMat3DTemperatureTable.hpp"
#include "oor/model/cards/materials/axisymOrthoMat/AxisymOrthoMatTable.hpp"
#include "oor/model/cards/materials/axisymOrthoMatTemperature/AxisymOrthoMatTemperatureTable.hpp"
#include "oor/model/cards/materials/creepMat/CreepMatTable.hpp"
#include "oor/model/cards/materials/elastoPlasticMat/ElastoPlasticMatTable.hpp"
#include "oor/model/cards/materials/fluidMat/FluidMatTable.hpp"
#include "oor/model/cards/materials/heatAnisoMatTemperature/HeatAnisoMatTemperatureTable.hpp"
#include "oor/model/cards/materials/heatIsoMatTemperature/HeatIsoMatTemperatureTable.hpp"
#include "oor/model/cards/materials/hyperelasticMat/HyperelasticMatTable.hpp"
#include "oor/model/cards/materials/hyperelasticPolynomialMat/HyperelasticPolynomialMatTable.hpp"
#include "oor/model/cards/materials/isoMat/IsoMatTable.hpp"
#include "oor/model/cards/materials/isoMatStressDependent/IsoMatStressDependentTable.hpp"
#include "oor/model/cards/materials/isoMatTemperature/IsoMatTemperatureTable.hpp"
#include "oor/model/cards/materials/orthoMat2DTemperature/OrthoMat2DTemperatureTable.hpp"
#include "oor/model/cards/materials/orthoMat3D/OrthoMat3DTable.hpp"
#include "oor/model/cards/materials/orthoMat3DTemperature/OrthoMat3DTemperatureTable.hpp"
#include "oor/model/cards/materials/viscoElasticMat/ViscoElasticMatTable.hpp"
#include "oor/model/cards/tables/tableDynamic1/TableDynamic1Table.hpp"
#include "oor/model/cards/tables/tableHeat1/TableHeat1Table.hpp"
#include "oor/model/cards/tables/tableHeatTwoVariables/TableHeatTwoVariablesTable.hpp"
#include "oor/model/cards/tables/tableMaterial1/TableMaterial1Table.hpp"
#include "oor/model/cards/tables/tableStress1/TableStress1Table.hpp"
#include "oor/model/cards/tables/tableStressTemperature/TableStressTemperatureTable.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/model/references/Renumbering.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using Catch::Approx;
using oor::testing::readModelText;
using oor::testing::writeModelText;

namespace
{

Model readBulk(const std::string& cards)
{
    return readModelText(cards, true);
}

template <typename TableT>
typename TableT::CardType cardAt(const Model& model, std::uint32_t row = 0)
{
    return model.findTable<TableT>()->card(row);
}

std::size_t unresolvedIn(Model& model)
{
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    return ReferenceChecker::check(model, sink);
}

// The card record is an error in strict mode and in tolerant mode, where the first error names the field (when given).
void expectInvalid(const std::string& card, const std::string& field = "")
{
    INFO(card);
    REQUIRE_THROWS_AS(readModelText(card, true), base::DeckError);
    const Model tolerant = readModelText(card, false);
    REQUIRE(tolerant.diagnostics().hasErrors());
    if (!field.empty())
    {
        const auto error = tolerant.diagnostics().firstError();
        REQUIRE(error.has_value());
        INFO(error->message);
        REQUIRE(error->location.fieldName == field);
    }
}

// Records the references a card lists: the field names and what each points to.
struct RecordingVisitor final : ReferenceVisitor
{
    std::vector<std::string> fields;
    std::vector<ReferenceTarget> targets;
    base::EntityId visit(std::string_view field, ReferenceTarget target, base::EntityId id) override
    {
        fields.emplace_back(field);
        targets.push_back(target);
        return id;
    }
};

// Writes the model as a model file and reads that again: the same cards.
void requireRoundTrip(const Model& model)
{
    oor::testing::requireModelFileRoundTrip(model);
}

} // namespace

// ------------------------------------------------------------------------------------------------ the tables

TEST_CASE("TABLES1 evaluates like the other material tables and has a TYPE", "[model][cards]")
{
    Model model = readBulk(
        "TableStress1;1;;0.;0.;0.01;1.2E+4;0.02;1.7E+4;EndTable\nTableStress1;2;2;0.;0.;Skip;Skip;1.;2.;EndTable");
    const TableStress1Card first = cardAt<TableStress1Table>(model, 0);
    REQUIRE(first.kind == 1);
    REQUIRE(first.x == std::vector<double>{0.0, 0.01, 0.02});
    REQUIRE(TableStress1Table::evaluate(first, 0.005) == Approx(6000.0));
    REQUIRE(TableStress1Table::evaluate(first, 0.03) == Approx(22000.0)); // the last segment goes on
    REQUIRE(TableStress1Table::evaluate(first, -0.01) == Approx(-12000.0));
    REQUIRE(std::isnan(TableStress1Table::evaluate(first, std::nan(""))));
    REQUIRE(model.table<TableStress1Table>().evaluateRow(0, 0.015) == Approx(14500.0));
    const std::vector<double> at = {0.0, 0.01};
    std::vector<double> out(2);
    model.table<TableStress1Table>().evaluateRow(0, at, out);
    REQUIRE(out[1] == Approx(12000.0));
    const TableStress1Card second = cardAt<TableStress1Table>(model, 1);
    REQUIRE(second.kind == 2);
    REQUIRE(second.x.size() == 2); // the SKIP pair is not kept
    requireRoundTrip(model);
    expectInvalid("TableStress1;0;;0.;0.;1.;1.;EndTable", "id");
    expectInvalid("TableStress1;1;3;0.;0.;1.;1.;EndTable", "kind");
    expectInvalid("TableStress1;1;;0.;0.;1.;1.", "tableEnd");
    expectInvalid("TableStress1;1;;0.;0.;1.;1.;0.;2.;EndTable", "x");
}

TEST_CASE("TABLEH1 is constant outside its points and must ascend", "[model][cards]")
{
    Model model = readBulk("TableHeat1;117;40.;6.12;80.;4.08;120.;1.24;EndTable");
    const TableHeat1Card card = cardAt<TableHeat1Table>(model);
    REQUIRE(TableHeat1Table::evaluate(card, 60.0) == Approx(5.1));
    REQUIRE(TableHeat1Table::evaluate(card, 10.0) == Approx(6.12)); // before the first point: its value
    REQUIRE(TableHeat1Table::evaluate(card, 500.0) == Approx(1.24));
    REQUIRE(model.table<TableHeat1Table>().evaluateRow(0, 80.0) == Approx(4.08));
    requireRoundTrip(model);
    expectInvalid("TableHeat1;1;3.;1.;2.;2.;1.;4.;EndTable", "");
    expectInvalid("TableHeat1;0;1.;1.;EndTable", "id");
    expectInvalid("TableHeat1;1;1.;1.", "tableEnd");
}

TEST_CASE("TABLEST and TABLEHT list the tables they select and refer to them", "[model][cards]")
{
    Model model = readBulk(
        "TableStress1;10;;0.;0.;1.;1.;EndTable\n"
        "TableStress1;20;;0.;0.;1.;2.;EndTable\n"
        "TableStressTemperature;101;120.;10;160.;20;EndTable\n"
        "TableHeat1;30;0.;1.;1.;2.;EndTable\n"
        "TableHeat1;40;0.;3.;1.;4.;EndTable\n"
        "TableHeatTwoVariables;64;12.;30;28.;40;EndTable");
    const TableStressTemperatureCard stress = cardAt<TableStressTemperatureTable>(model);
    REQUIRE(stress.temperatures == std::vector<double>{120.0, 160.0});
    REQUIRE(stress.tables == std::vector<base::EntityId>{10, 20});
    const TableHeatTwoVariablesCard heat = cardAt<TableHeatTwoVariablesTable>(model);
    REQUIRE(heat.x == std::vector<double>{12.0, 28.0});
    REQUIRE(heat.tables == std::vector<base::EntityId>{30, 40});
    RecordingVisitor visitor;
    model.table<TableStressTemperatureTable>().visitReferences(0, visitor);
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::StressTable));
    RecordingVisitor heatVisitor;
    model.table<TableHeatTwoVariablesTable>().visitReferences(0, heatVisitor);
    REQUIRE(heatVisitor.targets == std::vector<ReferenceTarget>(2, ReferenceTarget::HeatTable));
    REQUIRE(unresolvedIn(model) == 0);
    // Renumbering one space follows into the references, and leaves the other alone.
    Renumbering::apply(model, RenumberSpace::StressTable, Renumbering::offset(1000));
    REQUIRE(cardAt<TableStressTemperatureTable>(model).id == 1101);
    REQUIRE(cardAt<TableStressTemperatureTable>(model).tables == std::vector<base::EntityId>{1010, 1020});
    REQUIRE(cardAt<TableHeatTwoVariablesTable>(model).tables == std::vector<base::EntityId>{30, 40});
    Renumbering::apply(model, RenumberSpace::HeatTable, Renumbering::offset(5000));
    REQUIRE(cardAt<TableHeatTwoVariablesTable>(model).id == 5064);
    REQUIRE(cardAt<TableHeatTwoVariablesTable>(model).tables == std::vector<base::EntityId>{5030, 5040});
    REQUIRE(cardAt<TableStressTemperatureTable>(model).tables == std::vector<base::EntityId>{1010, 1020});
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
    // A table that is not there is reported.
    Model lacking = readBulk("TableStressTemperature;101;120.;10;160.;20;EndTable");
    REQUIRE(unresolvedIn(lacking) == 2);
    expectInvalid("TableStressTemperature;101;160.;10;120.;20;EndTable", "temperatures");
    expectInvalid("TableStressTemperature;101;120.;0;EndTable", "tables");
    expectInvalid("TableStressTemperature;101;120.;10", "tableEnd");
    expectInvalid("TableStressTemperature;0;120.;10;EndTable", "id");
    expectInvalid("TableHeatTwoVariables;64;28.;30;12.;40;EndTable", "x");
    expectInvalid("TableHeatTwoVariables;64;28.;-1;EndTable", "tables");
}

TEST_CASE("the table spaces are separate: the same ID in each of them is not a clash", "[model][cards]")
{
    Model model = readBulk(
        "TableDynamic1;5;;;0.;0.;1.;1.;EndTable\n"
        "TableMaterial1;5;;;0.;0.;1.;1.;EndTable\n"
        "TableStress1;5;;0.;0.;1.;1.;EndTable\n"
        "TableHeat1;5;0.;0.;1.;1.;EndTable");
    REQUIRE(model.locate(base::IdSpace::DynamicTable, 5).has_value());
    REQUIRE(model.locate(base::IdSpace::MaterialTable, 5).has_value());
    REQUIRE(model.locate(base::IdSpace::StressTable, 5).has_value());
    REQUIRE(model.locate(base::IdSpace::HeatTable, 5).has_value());
    // TABLES1 and TABLEST share a space, and so do TABLEH1 and TABLEHT.
    REQUIRE_THROWS_AS(readBulk("TableStress1;5;;0.;0.;EndTable\nTableStressTemperature;5;150.;5;EndTable"),
                      base::DeckError);
    REQUIRE_THROWS_AS(readBulk("TableHeat1;5;0.;0.;EndTable\nTableHeatTwoVariables;5;150.;5;EndTable"),
                      base::DeckError);
}

// ------------------------------------------------------------------------------------------------ the materials

TEST_CASE("a MAT3 has GZX in field 11 and blank cells in fields 9 and 10", "[model][cards]")
{
    Model model = readBulk(
        "AxisymOrthoMat;1;1.4E+7;1.3E+7;1.5E+7;0.28;0.22;0.31;2.E-5;2.1E+6;1.2E-4;1.2E-4;1.4E-4;72.5;0."
        "19\nAxisymOrthoMat;2;1.;2.;3.;;;;;4.");
    const AxisymOrthoMatCard card = cardAt<AxisymOrthoMatTable>(model);
    REQUIRE(card.youngsModulusX == 1.4e7);
    REQUIRE(card.youngsModulusZ == 1.5e7);
    REQUIRE(card.poissonRatioXth == 0.28);
    REQUIRE(card.poissonRatioZx == 0.31);
    REQUIRE(card.density == 2.0e-5);
    REQUIRE(card.shearModulusZx == 2.1e6);
    REQUIRE(card.thermalExpansionX == 1.2e-4);
    REQUIRE(card.thermalExpansionZ == 1.4e-4);
    REQUIRE(card.referenceTemperature == 72.5);
    REQUIRE(card.structuralDamping == 0.19);
    const AxisymOrthoMatCard plain = cardAt<AxisymOrthoMatTable>(model, 1);
    REQUIRE(plain.poissonRatioXth == 0.0); // blank ratios are 0.0
    REQUIRE(plain.shearModulusZx == 4.0);
    requireRoundTrip(model);
    // The material is in the material space with the other materials.
    REQUIRE_THROWS_AS(readBulk("IsoMat;1;2.1E+11;;0.3\nAxisymOrthoMat;1;1.;2.;3.;;;;;4."), base::DeckError);
    expectInvalid("AxisymOrthoMat;1;1.;2.;3.", "shearModulusZx");
    expectInvalid("AxisymOrthoMat;1;1.;2.;;;;;;4.", "youngsModulusZ");
    expectInvalid("AxisymOrthoMat;1;0.;2.;3.;;;;;4.", "youngsModulusX");
    expectInvalid("AxisymOrthoMat;1;1.;2.;3.;;;;;-4.", "shearModulusZx");
    expectInvalid("AxisymOrthoMat;0;1.;2.;3.;;;;;4.", "id");
}

TEST_CASE("a MAT9 has the 21 terms of the matrix, the expansions and the optional damping matrix", "[model][cards]")
{
    Model model = readBulk(
        "AnisoMat3D;23;7400.;;;;;;7400.;;;;;7400.;;;;4600.;;;4600.;;4600.;2.8;1.1E-5;1.1E-5;;;;;95.;0.004\n"
        "AnisoMat3D;18;1.\n"
        "AnisoMat3D;19;1.;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;0.01;0.02;;;;;;0.03");
    const AnisoMat3DCard card = cardAt<AnisoMat3DTable>(model);
    REQUIRE(card.stiffness[0] == 7.4e3);  // G11
    REQUIRE(card.stiffness[6] == 7.4e3);  // G22 in field 8
    REQUIRE(card.stiffness[11] == 7.4e3); // G33 in field 13
    REQUIRE(card.stiffness[15] == 4.6e3); // G44 in field 17
    REQUIRE(card.stiffness[18] == 4.6e3); // G55
    REQUIRE(card.stiffness[20] == 4.6e3); // G66
    REQUIRE(card.stiffness[1] == 0.0);
    REQUIRE(card.density == 2.8);
    REQUIRE(card.thermalExpansion[0] == 1.1e-5);
    REQUIRE(card.thermalExpansion[1] == 1.1e-5);
    REQUIRE(card.referenceTemperature == 95.0);
    REQUIRE(card.structuralDamping == 0.004);
    REQUIRE_FALSE(card.structuralDampingTerms.has_value());
    REQUIRE_FALSE(cardAt<AnisoMat3DTable>(model, 1).structuralDampingTerms.has_value());
    const AnisoMat3DCard damped = cardAt<AnisoMat3DTable>(model, 2);
    REQUIRE(damped.structuralDampingTerms.has_value());
    REQUIRE((*damped.structuralDampingTerms)[0] == 0.01);
    REQUIRE((*damped.structuralDampingTerms)[1] == 0.02);
    REQUIRE((*damped.structuralDampingTerms)[7] == 0.03); // GE23 in field 40
    requireRoundTrip(model);
    // A matrix present with zeros stays there when written.
    Model zero = readBulk("AnisoMat3D;1;1.;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;0.");
    REQUIRE(cardAt<AnisoMat3DTable>(zero).structuralDampingTerms.has_value());
    const Model again = readModelText(writeModelText(zero), true);
    REQUIRE(cardAt<AnisoMat3DTable>(again).structuralDampingTerms.has_value());
    expectInvalid("AnisoMat3D;0;1.", "id");
    expectInvalid("AnisoMat3D;1;xx", "stiffness11");
    oor::testing::requireModelFileRoundTrip(zero);
}

TEST_CASE("a MAT10 keeps two of BULK, RHO and C and computes the third", "[model][cards]")
{
    Model model = readBulk(
        "FluidMat;1;;1.18E-7;12500.\nFluidMat;2;1.;0.1\nFluidMat;3;0.712;0.012;;0.02;0.4\nFluidMat;4;1.;0.1;5.");
    const FluidMatCard fromC = cardAt<FluidMatTable>(model, 0);
    REQUIRE(fromC.bulkModulus == 12500.0 * 12500.0 * 1.18e-7);
    REQUIRE(fromC.density == 1.18e-7);
    const FluidMatCard fromBulk = cardAt<FluidMatTable>(model, 1);
    REQUIRE(fromBulk.soundSpeed == std::sqrt(10.0));
    const FluidMatCard full = cardAt<FluidMatTable>(model, 2);
    REQUIRE(full.structuralDamping == 0.02);
    REQUIRE(full.normalizedAdmittance.has_value());
    REQUIRE(*full.normalizedAdmittance == 0.4);
    // All three given and inconsistent: BULK and RHO count, C stays as written.
    const FluidMatCard inconsistent = cardAt<FluidMatTable>(model, 3);
    REQUIRE(inconsistent.bulkModulus == 1.0);
    REQUIRE(inconsistent.soundSpeed == 5.0);
    // The three are written, so that nothing depends on a blank.

    requireRoundTrip(model);
    expectInvalid("FluidMat;1;1.", "bulkModulus");
    expectInvalid("FluidMat;1;;;3.", "bulkModulus");
    expectInvalid("FluidMat;1;-1.;0.1", "soundSpeed");
    expectInvalid("FluidMat;1;1.;-0.1", "soundSpeed");
    expectInvalid("FluidMat;1;-1.;-0.1", "bulkModulus");
    expectInvalid("FluidMat;1;0.;1.", "soundSpeed");
    // With an ALPHA the bulk modulus and the density may be negative.
    Model porous = readBulk("FluidMat;1;-1.;-0.1;;;2.");
    REQUIRE(cardAt<FluidMatTable>(porous).soundSpeed == std::sqrt(10.0));
}

TEST_CASE("a MAT11 has its constants in the order of the card", "[model][cards]")
{
    Model model = readBulk(
        "OrthoMat3D;3;4.E+7;5.E+7;8.E+6;0.15;0.18;0.22;2.5E+6;3.25E+6;4.75E+6;0.12;1.5E-5;2.5E-5;4.E-6;40.;0.02");
    const OrthoMat3DCard card = cardAt<OrthoMat3DTable>(model);
    REQUIRE(card.youngsModulus1 == 4.0e7);
    REQUIRE(card.youngsModulus3 == 8.0e6);
    REQUIRE(card.poissonRatio13 == 0.18);
    REQUIRE(card.shearModulus12 == 2.5e6);
    REQUIRE(card.shearModulus13 == 3.25e6);
    REQUIRE(card.shearModulus23 == 4.75e6);
    REQUIRE(card.density == 0.12);
    REQUIRE(card.thermalExpansion3 == 4.0e-6);
    REQUIRE(card.referenceTemperature == 40.0);
    REQUIRE(card.structuralDamping == 0.02);
    requireRoundTrip(model);
    expectInvalid("OrthoMat3D;1;1.;1.;1.;0.3;0.3;0.3;1.;1.", "shearModulus23");
    expectInvalid("OrthoMat3D;1;1.;1.;0.;0.3;0.3;0.3;1.;1.;1.", "youngsModulus3");
}

TEST_CASE("a MATHP has its constants on the lines of the format reference and a D1 that depends on A10 and A01",
          "[model][cards]")
{
    Model model = readBulk(
        "HyperelasticPolynomialMat;1;70.;30.\n"
        "HyperelasticPolynomialMat;2;0.3;0.1;0.326\n"
        "HyperelasticPolynomialMat;3\n"
        "HyperelasticPolynomialMat;4;70.;30.;0.005;1.E-6;1.E-5;30.;0.01;3;2;0.5;0.1;0.05;0.002;0.3;0.2;0.1;0.05;0.001;"
        "0.1;0.2;0.3;0.4;0.5;0.003;0.6;0.7;0.8;0.9;1.;1.1;0.004;101;102;103;104;105");
    REQUIRE(cardAt<HyperelasticPolynomialMatTable>(model, 0).coefficientD1 == Approx(1.0e5)); // 10^3 (A10 + A01)
    REQUIRE(cardAt<HyperelasticPolynomialMatTable>(model, 1).coefficientD1 == 0.326);
    REQUIRE(cardAt<HyperelasticPolynomialMatTable>(model, 2).coefficientD1 == 0.0);
    REQUIRE(cardAt<HyperelasticPolynomialMatTable>(model, 2).distortionalOrder == 1);
    const HyperelasticPolynomialMatCard full = cardAt<HyperelasticPolynomialMatTable>(model, 3);
    REQUIRE(full.distortionalOrder == 3);
    REQUIRE(full.volumetricOrder == 2);
    REQUIRE(full.coefficientsA[0] == 0.5);  // A20
    REQUIRE(full.coefficientsA[2] == 0.05); // A02
    REQUIRE(full.coefficientsA[3] == 0.3);  // A30
    REQUIRE(full.coefficientsA[5] == 0.1);  // A12
    REQUIRE(full.coefficientsA[6] == 0.05); // A03
    REQUIRE(full.coefficientsA[7] == 0.1);  // A40
    REQUIRE(full.coefficientsA[11] == 0.5); // A04
    REQUIRE(full.coefficientsA[12] == 0.6); // A50
    REQUIRE(full.coefficientsA[17] == 1.1); // A05
    REQUIRE(full.coefficientsD == std::array<double, 4>{0.002, 0.001, 0.003, 0.004});
    REQUIRE(full.tensionTableId == 101);
    REQUIRE(full.pureShearTableId == 104);
    REQUIRE(full.volumetricTableId == 105);
    // D1 is written whatever it is (it depends on A10 and A01).

    requireRoundTrip(model);
    RecordingVisitor visitor;
    model.table<HyperelasticPolynomialMatTable>().visitReferences(3, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"tensionTableId", "equibiaxialTableId", "simpleShearTableId",
                                                       "pureShearTableId", "volumetricTableId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(5, ReferenceTarget::StressTable));
    expectInvalid("HyperelasticPolynomialMat;1;;;;;;;;0", "distortionalOrder");
    expectInvalid("HyperelasticPolynomialMat;1;;;;;;;;;6", "volumetricOrder");
    expectInvalid("HyperelasticPolynomialMat;1;;;-1.", "coefficientD1");
    expectInvalid("HyperelasticPolynomialMat;1;;;;;;;;;;;;;-1.", "coefficientD2");
    expectInvalid("HyperelasticPolynomialMat;1;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;-1", "tensionTableId");
}

TEST_CASE("a MATHE has the four formats of the format reference", "[model][cards]")
{
    Model model = readBulk(
        "HyperelasticMat;1;MooneyRivlin;2.8E+7;4.2E-4;5.E-6;80;0.02;1.6E+4;1.5;1.25;101;102;103;104;105;0.3;0.2;0.1;0."
        "05;4;3;"
        "0.1;0.2;0.3;0.4;0.5;0.1;0.2;0.3;0.4;0.5;0.6;0.1;0.2;0.3;0.4;0.5;0.6;0.7\n"
        "HyperelasticMat;2;Ogden;2;200000;;;;;16.;2.;;-4.;-2.;;;;;;;;;;;1.25;0.1\n"
        "HyperelasticMat;3;Foam;2;2.8E+7;4.2E-4;5.E-6;80;0.02;1.6E+4;1.5;1.25;1.5E+4;1.2;0.5;3.E+4;1.1;0.2\n"
        "HyperelasticMat;4;ArrudaBoyce;2.8E+7;4.2E-4;;;;1.5;3.;;0.1;0.2;0.3;0.4;0.5\n"
        "HyperelasticMat;5;Gent;2.8E+7;;;;;1.6E+4;1250.;50.;0.01\n"
        "HyperelasticMat;6;GeneralHyperelasticModel3;2.8E+7;4.2E-4;2.5E-6;70.5;0.015\n"
        "HyperelasticMat;7;;;;;;;0.3");
    const HyperelasticMatCard mooney = cardAt<HyperelasticMatTable>(model, 0);
    REQUIRE(mooney.model == HyperelasticMatModel::Mooney);
    REQUIRE(mooney.bulkModulus.has_value());
    REQUIRE(*mooney.bulkModulus == 2.8e7);
    REQUIRE(mooney.density == 0.00042);
    REQUIRE(mooney.thermalExpansion == 5.0e-6);
    REQUIRE(mooney.referenceTemperature == 80.0);
    REQUIRE(mooney.structuralDamping == 0.02);
    REQUIRE(mooney.coefficientsC[0] == 1.6e4); // C10 in field 9
    REQUIRE(mooney.coefficientsC[1] == 1.5);   // C01
    REQUIRE(mooney.coefficientsC[2] == 0.3);   // C20 in field 17
    REQUIRE(mooney.coefficientsC[5] == 0.1);   // C30 in field 25
    REQUIRE(mooney.coefficientsC[9] == 0.1);   // C40 in field 33
    REQUIRE(mooney.coefficientsC[14] == 0.1);  // C50 in field 41
    REQUIRE(mooney.coefficientsC[19] == 0.6);  // C05
    REQUIRE(mooney.coefficientsD == std::array<double, 5>{1.25, 0.05, 0.5, 0.6, 0.7});
    REQUIRE(mooney.distortionalOrder == 4);
    REQUIRE(mooney.volumetricOrder == 3);
    REQUIRE(mooney.tensionTableId == 101);
    REQUIRE(mooney.volumetricTableId == 105);
    const HyperelasticMatCard ogden = cardAt<HyperelasticMatTable>(model, 1);
    REQUIRE(ogden.model == HyperelasticMatModel::Ogden);
    REQUIRE(ogden.termCount == 2);
    REQUIRE(ogden.ogdenModuli[0] == 16.0);
    REQUIRE(ogden.alpha[1] == -2.0);
    REQUIRE(ogden.ogdenModuli[1] == -4.0);
    REQUIRE(ogden.coefficientsD[0] == 1.25);
    REQUIRE(ogden.coefficientsD[1] == 0.1);
    const HyperelasticMatCard foam = cardAt<HyperelasticMatTable>(model, 2);
    REQUIRE(foam.ogdenModuli[2] == 3.0e4); // MU3 in field 20
    REQUIRE(foam.alpha[2] == 1.1);
    REQUIRE(foam.beta[2] == 0.2);
    REQUIRE(foam.beta[1] == 0.5);
    const HyperelasticMatCard boyce = cardAt<HyperelasticMatTable>(model, 3);
    REQUIRE(boyce.rubberModulus == 1.5);
    REQUIRE(boyce.exponentOrModulus == 3.0);
    REQUIRE(boyce.coefficientsD[4] == 0.5);
    const HyperelasticMatCard gent = cardAt<HyperelasticMatTable>(model, 4);
    REQUIRE(gent.invariantLimit == 50.0);
    REQUIRE(gent.coefficientsD[0] == 0.01);
    REQUIRE(cardAt<HyperelasticMatTable>(model, 5).model == HyperelasticMatModel::Ghem3);
    const HyperelasticMatCard bare = cardAt<HyperelasticMatTable>(model, 6);
    REQUIRE_FALSE(bare.bulkModulus.has_value());
    REQUIRE(bare.distortionalOrder == 2); // the default of NA
    REQUIRE(bare.coefficientsC[0] == 0.3);
    requireRoundTrip(model);
    RecordingVisitor visitor;
    model.table<HyperelasticMatTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"tensionTableId", "equibiaxialTableId", "simpleShearTableId",
                                                       "pureShearTableId", "volumetricTableId"});
    expectInvalid("HyperelasticMat;1;SUSSBAT", "modelName");
    expectInvalid("HyperelasticMat;1;MooneyRivlin;-2.;;;;;0.3", "bulkModulus");
    expectInvalid("HyperelasticMat;1;MooneyRivlin;;;;;;0.3;0.1;;;;;;;;;;;6", "distortionalOrder");
    expectInvalid("HyperelasticMat;1;Ogden;6", "termCount");
    expectInvalid("HyperelasticMat;1;Ogden;1;;;;;;1.;2.;3.", "betaCoefficient1");
    expectInvalid("HyperelasticMat;1;Foam;1;;;;;;1.;2.;3.;;;;;;;;;;;;;1.", "coefficientD1");
    expectInvalid("HyperelasticMat;1;ArrudaBoyce;;;;;;1.;2.;3.", "");
    expectInvalid("HyperelasticMat;1;ArrudaBoyce;;;;;;0.", "rubberModulus");
    expectInvalid("HyperelasticMat;1;Gent;;;;;;1.;1.;;-1.", "coefficientD1");
}

// ------------------------------------------------------------------------------------------------ the companions

TEST_CASE("the temperature tables of a MATT1 to a MATT11 are in the fields of the material they extend",
          "[model][cards]")
{
    Model model = readBulk(
        "IsoMatTemperature;1;2;3;4;5;-6;8;9;10;11\n"
        "AnisoMat2DTemperature;2;2;3;4;5;6;7;8;-9;10;11;13;14;15;16;19;20;21;22;23;24\n"
        "AxisymOrthoMatTemperature;3;2;3;4;5;6;7;8;11;12;13;14;16\n"
        "HeatIsoMatTemperature;4;2;3;5;6;7\n"
        "HeatAnisoMatTemperature;5;2;3;4;5;6;7;8;10\n"
        "OrthoMat2DTemperature;8;2;3;4;5;6;7;8;-9;10;12;13;14;15;16;17;18\n"
        "OrthoMat3DTemperature;11;2;3;4;5;6;7;8;9;10;11;12;13;14;16");
    const IsoMatTemperatureCard matt1 = cardAt<IsoMatTemperatureTable>(model);
    REQUIRE(matt1.id == 1);
    REQUIRE(matt1.youngsModulusTableId == 2);
    REQUIRE(matt1.thermalExpansionTableId == -6); // a table of thermal strains
    REQUIRE(matt1.structuralDampingTableId == 8); // field 7 is blank
    REQUIRE(matt1.tensionAllowableTableId == 9);
    REQUIRE(matt1.shearAllowableTableId == 11);
    const AnisoMat2DTemperatureCard matt2 = cardAt<AnisoMat2DTemperatureTable>(model);
    REQUIRE(matt2.stiffnessTableIds == std::array<base::EntityId, 6>{2, 3, 4, 5, 6, 7});
    REQUIRE(matt2.densityTableId == 8);
    REQUIRE(matt2.thermalExpansionTableIds == std::array<base::EntityId, 3>{-9, 10, 11});
    REQUIRE(matt2.structuralDampingTableId == 13);
    REQUIRE(matt2.shearAllowableTableId == 16);
    REQUIRE(matt2.structuralDampingTermTableIds == std::array<base::EntityId, 6>{19, 20, 21, 22, 23, 24});
    const AxisymOrthoMatTemperatureCard matt3 = cardAt<AxisymOrthoMatTemperatureTable>(model);
    REQUIRE(matt3.youngsModulusZTableId == 4); // T(EZ) is field 4
    REQUIRE(matt3.poissonRatioXthTableId == 5);
    REQUIRE(matt3.densityTableId == 8);
    REQUIRE(matt3.shearModulusZxTableId == 11);
    REQUIRE(matt3.thermalExpansionZTableId == 14);
    REQUIRE(matt3.structuralDampingTableId == 16);
    const HeatIsoMatTemperatureCard matt4 = cardAt<HeatIsoMatTemperatureTable>(model);
    REQUIRE(matt4.conductivityTableId == 2);
    REQUIRE(matt4.specificHeatTableId == 3);
    REQUIRE(matt4.convectionCoefficientTableId == 5);
    REQUIRE(matt4.viscosityTableId == 6);
    REQUIRE(matt4.heatGenerationFactorTableId == 7);
    const HeatAnisoMatTemperatureCard matt5 = cardAt<HeatAnisoMatTemperatureTable>(model);
    REQUIRE(matt5.conductivityXzTableId == 4);
    REQUIRE(matt5.specificHeatTableId == 8);
    REQUIRE(matt5.heatGenerationFactorTableId == 10);
    const OrthoMat2DTemperatureCard matt8 = cardAt<OrthoMat2DTemperatureTable>(model);
    REQUIRE(matt8.densityTableId == 8);
    REQUIRE(matt8.thermalExpansion1TableId == -9);
    REQUIRE(matt8.tensionStrength1TableId == 12);
    REQUIRE(matt8.shearStrengthTableId == 16);
    REQUIRE(matt8.structuralDampingTableId == 17);
    REQUIRE(matt8.interactionTerm12TableId == 18);
    const OrthoMat3DTemperatureCard matt11 = cardAt<OrthoMat3DTemperatureTable>(model);
    REQUIRE(matt11.youngsModulus1TableId == 2);
    REQUIRE(matt11.shearModulus23TableId == 10);
    REQUIRE(matt11.thermalExpansion3TableId == 14);
    REQUIRE(matt11.structuralDampingTableId == 16);
    requireRoundTrip(model);
}

TEST_CASE("a MATT9 has the 21 + 21 tables of a MAT9 in its fields", "[model][cards]")
{
    Model model = readBulk(
        "AnisoMat3DTemperature;9;2;;4;;;;8;;;;;12;;;;;;;;;;22;23;;;;;;31;"
        "33;34;35;36;37;38;39;40;41;42;43;44;45;46;47;48;49;50;51;52;53");
    const AnisoMat3DTemperatureCard card = cardAt<AnisoMat3DTemperatureTable>(model);
    REQUIRE(card.stiffnessTableIds[0] == 2);   // T(G11)
    REQUIRE(card.stiffnessTableIds[2] == 4);   // T(G13)
    REQUIRE(card.stiffnessTableIds[6] == 8);   // T(G22) in field 8
    REQUIRE(card.stiffnessTableIds[11] == 12); // T(G33) in field 13
    REQUIRE(card.densityTableId == 22);
    REQUIRE(card.thermalExpansionTableIds[0] == 23);
    REQUIRE(card.thermalExpansionTableIds[5] == 0);
    REQUIRE(card.structuralDampingTableId == 31);
    REQUIRE(card.structuralDampingTermTableIds[0] == 33);
    REQUIRE(card.structuralDampingTermTableIds[20] == 53);
    requireRoundTrip(model);
    expectInvalid("AnisoMat3DTemperature;9;-1", "stiffness11TableId");
}

TEST_CASE(
    "the blank cells of a companion must stay blank, a negative table is allowed only where the format reference says",
    "[model][cards]")
{
    expectInvalid("IsoMatTemperature;1;-2", "youngsModulusTableId");
    expectInvalid("IsoMatTemperature;0;2", "id");
    expectInvalid("AnisoMat2DTemperature;1;;;;;;;-3", "densityTableId");
    expectInvalid("AxisymOrthoMatTemperature;1;;;;;;;;;-1", "thermalExpansionXTableId");
    expectInvalid("OrthoMat2DTemperature;1;;;;;;;;;;;;;;;-1", "structuralDampingTableId");
    // A negative T(A) is allowed.
    REQUIRE(cardAt<IsoMatTemperatureTable>(readBulk("IsoMatTemperature;1;;;;;-2")).thermalExpansionTableId == -2);
    REQUIRE(
        cardAt<OrthoMat2DTemperatureTable>(readBulk("OrthoMat2DTemperature;1;;;;;;;;-1;-2")).thermalExpansion2TableId ==
        -2);
}

TEST_CASE("a companion has an ID space of its own and refers to its material and its tables", "[model][cards]")
{
    Model model = readBulk(
        "IsoMat;10;2.1E+11;;0.3\n"
        "IsoMatTemperature;10;201;202\n"
        "IsoMatStressDependent;10;301;NonlinearElastic\n"
        "CreepMat;10;;;Table;401;402;403\n"
        "OrthoMat2D;20;1.;1.;0.3\n"
        "OrthoMat2DTemperature;20;201\n"
        "AnisoMat2D;30;1.\n"
        "AnisoMat2DTemperature;30;201\n"
        "AnisoMat3D;40;1.\n"
        "AnisoMat3DTemperature;40;201\n"
        "AxisymOrthoMat;50;1.;1.;1.;;;;;1.\n"
        "AxisymOrthoMatTemperature;50;201\n"
        "TableMaterial1;201;;;0.;1.;1.;2.;EndTable\n"
        "TableMaterial1;202;;;0.;1.;1.;2.;EndTable\n"
        "TableMaterial1;-203;;;0.;1.;1.;2.;EndTable\n"
        "TableStress1;301;;0.;0.;1.;1.;EndTable\n"
        "TableStress1;401;;0.;0.;1.;1.;EndTable\n"
        "TableStress1;402;;0.;0.;1.;1.;EndTable\n"
        "TableStress1;403;;0.;0.;1.;1.;EndTable");
    // The same MID in the material space and in the space of each companion: no clash.
    REQUIRE(model.locate(base::IdSpace::Material, 10).has_value());
    REQUIRE(model.locateInType(typeIndexOf<IsoMatTemperatureTable>(), 10).has_value());
    REQUIRE(model.locateInType(typeIndexOf<IsoMatStressDependentTable>(), 10).has_value());
    REQUIRE(model.locateInType(typeIndexOf<CreepMatTable>(), 10).has_value());
    RecordingVisitor visitor;
    model.table<IsoMatTemperatureTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"id", "youngsModulusTableId", "shearModulusTableId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::MaterialTable,
                                                            ReferenceTarget::MaterialTable});
    RecordingVisitor stressVisitor;
    model.table<IsoMatStressDependentTable>().visitReferences(0, stressVisitor);
    REQUIRE(stressVisitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::StressTable});
    REQUIRE(unresolvedIn(model) == 0);
    // Renumbering the materials follows into the companions.
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(1000));
    REQUIRE(cardAt<IsoMatTemperatureTable>(model).id == 1010);
    REQUIRE(cardAt<IsoMatStressDependentTable>(model).id == 1010);
    REQUIRE(cardAt<CreepMatTable>(model).id == 1010);
    REQUIRE(cardAt<OrthoMat2DTemperatureTable>(model).id == 1020);
    REQUIRE(cardAt<AnisoMat2DTemperatureTable>(model).id == 1030);
    REQUIRE(cardAt<AnisoMat3DTemperatureTable>(model).id == 1040);
    REQUIRE(cardAt<AxisymOrthoMatTemperatureTable>(model).id == 1050);
    REQUIRE(model.locateInType(typeIndexOf<IsoMatTemperatureTable>(), 1010).has_value());
    REQUIRE(unresolvedIn(model) == 0);
    // The tables of each space follow their own renumbering.
    Renumbering::apply(model, RenumberSpace::MaterialTable, Renumbering::offset(5000));
    REQUIRE(cardAt<IsoMatTemperatureTable>(model).youngsModulusTableId == 5201);
    REQUIRE(cardAt<IsoMatTemperatureTable>(model).shearModulusTableId == 5202);
    REQUIRE(cardAt<IsoMatStressDependentTable>(model).tableId == 301);
    Renumbering::apply(model, RenumberSpace::StressTable, Renumbering::offset(7000));
    REQUIRE(cardAt<IsoMatStressDependentTable>(model).tableId == 7301);
    REQUIRE(cardAt<CreepMatTable>(model).primaryStiffnessTableId == 7401);
    REQUIRE(cardAt<CreepMatTable>(model).secondaryDampingTableId == 7403);
    REQUIRE(cardAt<IsoMatTemperatureTable>(model).youngsModulusTableId == 5201);
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);
    // Two companions of one material are a duplicate ID.
    REQUIRE_THROWS_AS(readBulk("IsoMatTemperature;10;201\nIsoMatTemperature;10;202"), base::DeckError);
    // A material that is not there, and a table that is not there, are reported.
    Model lacking = readBulk("IsoMatTemperature;10;201;202\nIsoMatStressDependent;10;301;NonlinearElastic");
    REQUIRE(unresolvedIn(lacking) == 5);
}

TEST_CASE("a negative T(A) names the table of thermal strains of that number and follows its renumbering",
          "[model][cards]")
{
    Model model = readBulk(
        "IsoMat;1;2.1E+11;;0.3\n"
        "IsoMatTemperature;1;10;;;;-20\n"
        "OrthoMat2DTemperature;1;;;;;;;;-20;30\n"
        "OrthoMat2D;2;1.;1.;0.3\n"
        "TableMaterial1;10;;;0.;1.;1.;2.;EndTable\n"
        "TableMaterial1;-20;;;0.;1.;1.;2.;EndTable\n"
        "TableMaterial1;30;;;0.;1.;1.;2.;EndTable\n"
        "TableMaterial1;20;;;0.;1.;1.;2.;EndTable");
    REQUIRE(unresolvedIn(model) == 0);
    // The table -20 is the thermal strain table of 20: renumbering by an offset keeps the sign of both the table and
    // the fields that name it, and does not mix it up with the table 20.
    Renumbering::apply(model, RenumberSpace::MaterialTable, Renumbering::offset(100));
    REQUIRE(cardAt<IsoMatTemperatureTable>(model).youngsModulusTableId == 110);
    REQUIRE(cardAt<IsoMatTemperatureTable>(model).thermalExpansionTableId == -120);
    REQUIRE(cardAt<OrthoMat2DTemperatureTable>(model).thermalExpansion1TableId == -120);
    REQUIRE(cardAt<OrthoMat2DTemperatureTable>(model).thermalExpansion2TableId == 130);
    REQUIRE(model.locate(base::IdSpace::MaterialTable, -120).has_value());
    REQUIRE(model.locate(base::IdSpace::MaterialTable, 120).has_value());
    REQUIRE(unresolvedIn(model) == 0);
    // Consecutive numbers: the table -20 takes the number of 20's turn with its sign.
    Renumbering::apply(model, RenumberSpace::MaterialTable,
                       Renumbering::consecutive(RenumberSpace::MaterialTable, 1, 1));
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(cardAt<IsoMatTemperatureTable>(model).thermalExpansionTableId < 0);
    REQUIRE(model.locate(base::IdSpace::MaterialTable, cardAt<IsoMatTemperatureTable>(model).thermalExpansionTableId)
                .has_value());
    requireRoundTrip(model);
}

TEST_CASE("a MATS1 is nonlinear elastic or plastic and has the rules of the format reference", "[model][cards]")
{
    Model model = readBulk(
        "IsoMatStressDependent;1;60;NonlinearElastic\n"
        "IsoMatStressDependent;2;;Plastic;0.;1;1;1.E+8\n"
        "IsoMatStressDependent;3;;Plastic;1200.;;;1.2E+6\n"
        "IsoMatStressDependent;4;60;Plastic;;;;1.E+4\n"
        "IsoMatStressDependent;5;;Plastic;;3;2;1.E+8;30.");
    const IsoMatStressDependentCard elastic = cardAt<IsoMatStressDependentTable>(model, 0);
    REQUIRE(elastic.kind == IsoMatStressDependentType::NonlinearElastic);
    REQUIRE(elastic.tableId == 60);
    REQUIRE_FALSE(elastic.hardeningSlope.has_value());
    const IsoMatStressDependentCard perfect = cardAt<IsoMatStressDependentTable>(model, 1);
    REQUIRE(perfect.kind == IsoMatStressDependentType::Plastic);
    REQUIRE(perfect.hardeningSlope.has_value());
    REQUIRE(*perfect.hardeningSlope == 0.0);
    REQUIRE(perfect.yieldFunction == 1);
    REQUIRE(*perfect.initialYieldStress == 1.0e8);
    REQUIRE(*cardAt<IsoMatStressDependentTable>(model, 2).hardeningSlope == 1200.0);
    REQUIRE(cardAt<IsoMatStressDependentTable>(model, 3).tableId == 60);
    const IsoMatStressDependentCard mohr = cardAt<IsoMatStressDependentTable>(model, 4);
    REQUIRE(mohr.yieldFunction == 3);
    REQUIRE(mohr.hardeningRule == 2);
    REQUIRE(*mohr.frictionAngle == 30.0);
    requireRoundTrip(model);
    expectInvalid("IsoMatStressDependent;1", "kind");
    expectInvalid("IsoMatStressDependent;1;;ELASTIC", "kind");
    expectInvalid("IsoMatStressDependent;1;60;Plastic;1200.;;;1.E+4",
                  "hardeningSlope"); // the table or the slope, not both
    expectInvalid("IsoMatStressDependent;1;;Plastic;1200.", "initialYieldStress");
    expectInvalid("IsoMatStressDependent;1;;Plastic;;5;;1.E+4", "yieldFunction");
    expectInvalid("IsoMatStressDependent;1;;Plastic;;;4;1.E+4", "hardeningRule");
    expectInvalid("IsoMatStressDependent;1;;Plastic;;3;;1.E+4", "frictionAngle");
    expectInvalid("IsoMatStressDependent;1;;Plastic;;4;;1.E+4;45.", "frictionAngle");
    expectInvalid("IsoMatStressDependent;1;-1;NonlinearElastic", "tableId");
    // A nonlinear elastic material does not use H, YF, HR and the limits: they are kept as written.
    Model unused = readBulk("IsoMatStressDependent;1;60;NonlinearElastic;5.;2;3;1.;2.");
    REQUIRE(cardAt<IsoMatStressDependentTable>(unused).yieldFunction == 2);
    requireRoundTrip(unused);
}

TEST_CASE("a CREEP is a creep law or three tables", "[model][cards]")
{
    Model model = readBulk(
        "CreepMat;8;950.;;CreepLaw;;;;;121;5.214E-6;2.118;6.415E-4;0.0935;5.86E-9;0.1263;2.5\n"
        "CreepMat;9;;;Table;401;402;403\n"
        "CreepMat;10;;1.E-8;CreepLaw;;;;5.E-5;300;0.001;2.;;0.5\n"
        "CreepMat;11;;;;;;;;222;1.;2.");
    const CreepMatCard law = cardAt<CreepMatTable>(model, 0);
    REQUIRE(law.referenceTemperature == 950.0);
    REQUIRE(law.exponent == CreepMatCard::DEFAULT_EXPONENT);
    REQUIRE(law.threshold == CreepMatCard::DEFAULT_THRESHOLD);
    REQUIRE(law.form == CreepMatForm::CrLaw);
    REQUIRE(law.kind == 121);
    REQUIRE(law.coefficients[0] == 5.214e-6);
    REQUIRE(law.coefficients[6] == 2.5);
    const CreepMatCard tables = cardAt<CreepMatTable>(model, 1);
    REQUIRE(tables.form == CreepMatForm::Table);
    REQUIRE(tables.primaryStiffnessTableId == 401);
    REQUIRE(tables.secondaryDampingTableId == 403);
    REQUIRE(std::isnan(tables.coefficients[0]));
    const CreepMatCard three = cardAt<CreepMatTable>(model, 2);
    REQUIRE(three.kind == 300);
    REQUIRE(std::isnan(three.coefficients[2]));
    REQUIRE(three.coefficients[3] == 0.5);
    REQUIRE(three.threshold == 5.0e-5);
    REQUIRE(cardAt<CreepMatTable>(model, 3).form == CreepMatForm::Blank);
    requireRoundTrip(model);
    expectInvalid("CreepMat;1;;;CreepLaw;;;;;123;1.;2.", "kind");
    expectInvalid("CreepMat;1;;;CreepLaw", "kind");
    expectInvalid("CreepMat;1;;;FOO;;;;;111;1.;2.", "form");
    expectInvalid("CreepMat;1;;;Table;401;402", "secondaryDampingTableId");
    expectInvalid("CreepMat;1;;;Table;401;402;403;;111", "kind");
    expectInvalid("CreepMat;1;;;CreepLaw;401;;;;111;1.;2.", "primaryStiffnessTableId");
    expectInvalid("CreepMat;1;;;CreepLaw;;;;;112;1.;2.;3.;4.;5.;6.;7.", "coefficientG");
    expectInvalid("CreepMat;1;;;CreepLaw;;;;;300;1.;2.;3.", "coefficientC");
    expectInvalid("CreepMat;1;;2.;CreepLaw;;;;;111;1.;2.", "exponent");
    expectInvalid("CreepMat;1;;;CreepLaw;;;;0.1;111;1.;2.", "threshold");
    RecordingVisitor visitor;
    model.table<CreepMatTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"id", "primaryStiffnessTableId", "primaryDampingTableId",
                                                       "secondaryDampingTableId"});
    REQUIRE(visitor.targets[1] == ReferenceTarget::StressTable);
}

TEST_CASE("a MATEP has the groups of the format reference, each starting a line", "[model][cards]")
{
    Model model = readBulk(
        "ElastoPlasticMat;1;Table;;50;Hill;;;RateEffect;CowperSymonds;;40.;5.;AnisotropicYield;1.1;0.9;1.02\n"
        "ElastoPlasticMat;2;;250.;;;ChabocheHardening;;ChabocheHardening;100.;200.;10.;5.E+3;50.;0.001;2.;3.;0.5;0.1\n"
        "ElastoPlasticMat;3;;250.;;;JohnsonCookHardening;;JohnsonCookHardening;300.;100.;0.3;0.02;1.;1500.;20.;0.001\n"
        "ElastoPlasticMat;4;;250.;;ImplicitCreep;;;ImplicitCreep\n"
        "ElastoPlasticMat;5;;250.;;LinearMohrCoulomb;;;PressureDependence;Parabolic;;0.5\n"
        "ElastoPlasticMat;6;Slope;250.;;;;1.E+3");
    const ElastoPlasticMatCard first = cardAt<ElastoPlasticMatTable>(model, 0);
    REQUIRE(first.form == ElastoPlasticMatForm::Table);
    REQUIRE(first.stressStrainTableId == 50);
    REQUIRE(first.yieldCriterion == ElastoPlasticMatYield::Hill);
    REQUIRE(first.rateDependence.has_value());
    REQUIRE(first.rateDependence->option == ElastoPlasticMatRateOption::Cowper);
    REQUIRE(first.rateDependence->cowperCoefficient == 40.0);
    REQUIRE(first.rateDependence->cowperExponent == 5.0);
    REQUIRE(first.anisotropy.has_value());
    REQUIRE(*first.anisotropy == std::array<double, 6>{1.1, 0.9, 1.02, 1.0, 1.0, 1.0});
    const ElastoPlasticMatCard chaboche = cardAt<ElastoPlasticMatTable>(model, 1);
    REQUIRE(chaboche.workHardening == ElastoPlasticMatHardening::Chaboche);
    REQUIRE(chaboche.chabocheHardening.has_value());
    REQUIRE((*chaboche.chabocheHardening)[6] == 2.0);
    REQUIRE((*chaboche.chabocheHardening)[7] == 3.0);
    REQUIRE((*chaboche.chabocheHardening)[9] == 0.1);
    const ElastoPlasticMatCard cook = cardAt<ElastoPlasticMatTable>(model, 2);
    REQUIRE((*cook.johnsonCookHardening)[6] == 20.0);
    REQUIRE((*cook.johnsonCookHardening)[7] == 1.0e-3);
    const ElastoPlasticMatCard creep = cardAt<ElastoPlasticMatTable>(model, 3);
    REQUIRE(creep.implicitCreep);
    REQUIRE_FALSE(creep.creepEquivalentStress.has_value());
    const ElastoPlasticMatCard pressure = cardAt<ElastoPlasticMatTable>(model, 4);
    REQUIRE(pressure.pressureDependence.has_value());
    REQUIRE(pressure.pressureDependence->option == ElastoPlasticMatPressureOption::Parabolic);
    REQUIRE_FALSE(pressure.pressureDependence->mohrCoulombAlpha.has_value());
    REQUIRE(*pressure.pressureDependence->mohrCoulombBeta == 0.5);
    const ElastoPlasticMatCard plain = cardAt<ElastoPlasticMatTable>(model, 5);
    REQUIRE(plain.hardeningSlope == 1000.0);
    REQUIRE(*plain.yieldStress == 250.0);
    REQUIRE_FALSE(plain.rateDependence.has_value());
    requireRoundTrip(model);
    expectInvalid("ElastoPlasticMat;1;FOO", "form");
    expectInvalid("ElastoPlasticMat;1;;;;BAD", "yieldCriterion");
    expectInvalid("ElastoPlasticMat;1;;;;;BAD", "workHardening");
    expectInvalid("ElastoPlasticMat;1;Table", "stressStrainTableId");
    expectInvalid("ElastoPlasticMat;1;Slope;;5", "stressStrainTableId");
    expectInvalid("ElastoPlasticMat;1;PerfectPlasticity;;;;;5.", "hardeningSlope");
    expectInvalid("ElastoPlasticMat;1;;-1.", "yieldStress");
    expectInvalid("ElastoPlasticMat;1;;;;Hill;;;AnisotropicYield;;;;;;;AnisotropicYield", "anisotropy");
    expectInvalid("ElastoPlasticMat;1;;;;;;;AnisotropicYield;1.", "anisotropy");            // needs HILL or BARLAT
    expectInvalid("ElastoPlasticMat;1;;;;;;;PressureDependence", "pressureDependence");     // needs LINMOHR or PBLMOHR
    expectInvalid("ElastoPlasticMat;1;;;;;;;ChabocheHardening", "chabocheHardening");       // needs WKHARD = CHABOCHE
    expectInvalid("ElastoPlasticMat;1;;;;;;;JohnsonCookHardening", "johnsonCookHardening"); // needs WKHARD = JHCOOK
    expectInvalid("ElastoPlasticMat;1;;;;;;;ImplicitCreep", "implicitCreep");               // needs RYIELD = IMPCREEP
    expectInvalid("ElastoPlasticMat;1;;;;;;;RateEffect;Table", "strainRateTableId"); // the TABLE option needs a table
    expectInvalid("ElastoPlasticMat;1;;;;;;;RateEffect;CowperSymonds;5",
                  "strainRateTableId"); // RTID belongs to the TABLE option
    expectInvalid("ElastoPlasticMat;1;;;;;;;RateEffect;Table;5;3.",
                  "hardeningOrRateCoefficient"); // C belongs to COWPER
    expectInvalid("ElastoPlasticMat;1;;;;;;;RateEffect;SOMETHING", "groupOption");
    expectInvalid("ElastoPlasticMat;1;;;;LinearMohrCoulomb;;;PressureDependence;Linear;-1.", "mohrCoulombAlpha");
    // The groups refer to their tables.
    RecordingVisitor visitor;
    model.table<ElastoPlasticMatTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"id", "stressStrainTableId"});
    Model rate = readBulk("ElastoPlasticMat;1;Table;;50;;;;RateEffect;Table;60");
    RecordingVisitor rateVisitor;
    rate.table<ElastoPlasticMatTable>().visitReferences(0, rateVisitor);
    REQUIRE(rateVisitor.fields == std::vector<std::string>{"id", "stressStrainTableId", "strainRateTableId"});
    REQUIRE(rateVisitor.targets[2] == ReferenceTarget::StressTable);
}

TEST_CASE("a MATVE has the fixed layout, the term lines and the orthotropic layout", "[model][cards]")
{
    Model model = readBulk(
        "ViscoElasticMat;1;IsotropicForm;1.E-5;2.E-5;1.E+3;10.;2.E+3;20.;500.;100.;800.;200.\n"
        "ViscoElasticMat;2;;;;1.E+3;10.;2.E+3;20.;500.;100.\n"
        "ViscoElasticMat;3;MooneyRivlin;;;0.5;10.;0.4;12.\n"
        "ViscoElasticMat;4;Linear;;;1.;2.\n"
        "ViscoElasticMat;5;OrthotropicForm;10.;1.E+5;1.E+5;2.E+5;0.3;0.3;0.25;4.E+4;4.E+4;3.E+4;100.;5.E+4;5.E+4;1.E+5;"
        "0.3;0.3;0."
        "25;2.E+4;2.E+4;1.E+4");
    const ViscoElasticMatCard fixed = cardAt<ViscoElasticMatTable>(model, 0);
    REQUIRE(fixed.model == ViscoElasticMatModel::Iso);
    REQUIRE(fixed.solidThermalExpansion == 1.0e-5);
    REQUIRE(fixed.liquidThermalExpansion == 2.0e-5);
    REQUIRE(fixed.terms.size() == 2);
    REQUIRE(fixed.terms[0] == std::array<double, 4>{1000.0, 10.0, 2000.0, 20.0});
    REQUIRE(fixed.terms[1] ==
            std::array<double, 4>{500.0, 100.0, 800.0, 200.0}); // G2, TD2 on line 2, K2, TV2 on line 3
    const ViscoElasticMatCard lines = cardAt<ViscoElasticMatTable>(model, 1);
    REQUIRE(lines.model == ViscoElasticMatModel::Iso1); // the default
    REQUIRE(lines.terms.size() == 2);
    REQUIRE(lines.terms[1] == std::array<double, 4>{500.0, 100.0, 0.0, 0.0});
    REQUIRE(cardAt<ViscoElasticMatTable>(model, 2).model == ViscoElasticMatModel::Mooney);
    REQUIRE(cardAt<ViscoElasticMatTable>(model, 3).model ==
            ViscoElasticMatModel::Iso1); // LINEAR is another name of ISO1
    const ViscoElasticMatCard ortho = cardAt<ViscoElasticMatTable>(model, 4);
    REQUIRE(ortho.model == ViscoElasticMatModel::Ortho);
    REQUIRE(ortho.terms.empty());
    REQUIRE(ortho.orthoTerms.size() == 2);
    REQUIRE(ortho.orthoTerms[0][0] == 10.0);
    REQUIRE(ortho.orthoTerms[0][6] == 0.25);
    REQUIRE(ortho.orthoTerms[0][7] == 4.0e4);
    REQUIRE(ortho.orthoTerms[1][0] == 100.0);
    REQUIRE(ortho.orthoTerms[1][9] == 1.0e4);
    requireRoundTrip(model);
    expectInvalid("ViscoElasticMat;1;BAD", "model");
    expectInvalid("ViscoElasticMat;1;IsotropicForm1;;;-1.;2.", "shearModulus");
    expectInvalid("ViscoElasticMat;1;OrthotropicForm;-1.", "deviatoricRelaxationTime");
    // The fixed layouts hold five terms: more need ISO1 and the others.
    ViscoElasticMatCard six;
    six.id = 1;
    six.model = ViscoElasticMatModel::Iso;
    six.terms.assign(6, std::array<double, 4>{1.0, 1.0, 1.0, 1.0});
    REQUIRE(ViscoElasticMatFormat::check(six).has_value());
    six.model = ViscoElasticMatModel::Iso1;
    REQUIRE_FALSE(ViscoElasticMatFormat::check(six).has_value());
}

TEST_CASE("every companion holds the fields it should in the table and lists them for renumbering", "[model][cards]")
{
    // The model writes what it reads for the companions of materials that are renumbered, and the Python-visible
    // columns exist: a quick check on the sizes of the groups.
    Model model = readBulk("AnisoMat3D;1;1.\nAnisoMat3DTemperature;1;2\nAnisoMat2DTemperature;2;2\nAnisoMat2D;2;1.");
    REQUIRE(model.table<AnisoMat3DTemperatureTable>().stiffnessTableIdsColumn().size() == 1);
    REQUIRE(model.table<AnisoMat2DTemperatureTable>().structuralDampingTermTableIdsColumn().size() == 1);
}

TEST_CASE("the FID of a MATEP names a TABLES1 or a TABL3D, and follows the renumbering of the one it names",
          "[model][cards]")
{
    // format reference MATEP (SOL 400): FID is the ID of a TABLES1 or a TABL3D entry.
    Model model = readBulk(
        "IsoMat;1;2.1E+5;;0.3\n"
        "IsoMat;2;2.1E+5;;0.3\n"
        "ElastoPlasticMat;1;Table;;5\n"
        "ElastoPlasticMat;2;Table;;6\n"
        "TableStress1;5;2;0.;250.;0.5;500.;EndTable\n"
        "TableMultiVariable0;6;73;;;;;0.;250.;0.5;500.");
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    REQUIRE(ReferenceChecker::check(model, sink) == 0);
    Renumbering::apply(model, RenumberSpace::MultiDimensionalTable, Renumbering::offset(100));
    REQUIRE((model.table<ElastoPlasticMatTable>().card(0).stressStrainTableId == 5 &&
             model.table<ElastoPlasticMatTable>().card(1).stressStrainTableId == 106));
    Renumbering::apply(model, RenumberSpace::StressTable, Renumbering::offset(10));
    REQUIRE((model.table<ElastoPlasticMatTable>().card(0).stressStrainTableId == 15 &&
             model.table<ElastoPlasticMatTable>().card(1).stressStrainTableId == 106));
    REQUIRE(ReferenceChecker::check(model, sink) == 0);
}
