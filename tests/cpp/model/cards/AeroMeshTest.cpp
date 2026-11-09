// Checks of the cards of the aerodynamic mesh and of the parametric loads of the static aeroelastic trim:
// AESCALE and AEGRID (a grid point and the scale it names), AEQUAD4 and AETRIA3 (the elements that connect the points),
// ACTRIM (a card found by its name), AEDW, AEPRESS and AEFORCE (a load vector of one control vector, with the matrix
// names that are not references). The ID spaces, the references, the renumbering of the spaces they own and the refused
// values are tested here.

#include "oor/model/cards/aero/aeroDownwash/AeroDownwashTable.hpp"
#include "oor/model/cards/aero/aeroForceVector/AeroForceVectorTable.hpp"
#include "oor/model/cards/aero/aeroNode/AeroNodeTable.hpp"
#include "oor/model/cards/aero/aeroNodeScale/AeroNodeScaleTable.hpp"
#include "oor/model/cards/aero/aeroPressureVector/AeroPressureVectorTable.hpp"
#include "oor/model/cards/aero/aeroQuad4/AeroQuad4Table.hpp"
#include "oor/model/cards/aero/aeroTria3/AeroTria3Table.hpp"
#include "oor/model/cards/aero/trimmedMaterialMatrices/TrimmedMaterialMatricesTable.hpp"
#include "support/AeroTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

TEST_CASE("an AESCALE has three factors that default to 1.0 and is named by the ASID of an AEGRID",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroNodeScale;100;1;1;1\nAeroNodeScale;101\nAeroNodeScale;102;2.5;;-3.\nAeroNode;1;;0.;0.;0.;;100\nAeroNode;2;"
        ";1.;1.;1.;;103");
    const AeroNodeScaleCard full = cardAt<AeroNodeScaleTable>(model, 0);
    REQUIRE((full.id == 100 && full.referenceLength1 == 1.0 && full.referenceLength2 == 1.0 &&
             full.referenceLength3 == 1.0));
    const AeroNodeScaleCard blank = cardAt<AeroNodeScaleTable>(model, 1);
    REQUIRE((blank.id == 101 && blank.referenceLength1 == 1.0 && blank.referenceLength2 == 1.0 &&
             blank.referenceLength3 == 1.0));
    const AeroNodeScaleCard mixed = cardAt<AeroNodeScaleTable>(model, 2);
    REQUIRE((mixed.id == 102 && mixed.referenceLength1 == 2.5 && mixed.referenceLength2 == 1.0 &&
             mixed.referenceLength3 == -3.0));
    REQUIRE((AeroNodeScaleTable::INFO.category == CardCategory::AeroScale &&
             AeroNodeScaleTable::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    // A factor that is 1.0 is a blank field, and a blank in the middle is kept as one (the third factor is in field 4).

    // The card names nothing; the ASID of an AEGRID names it (103 is not in the deck).
    RecordingVisitor visitor;
    model.table<AeroNodeScaleTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());
    model.table<AeroNodeTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"scaleId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroScale});
    REQUIRE(unresolvedIn(model) == 1);

    // The scales are renumbered with the references to them, the dangling one with the mapping.
    const std::size_t changed = Renumbering::apply(model, RenumberSpace::AeroScale, Renumbering::offset(10));
    REQUIRE(changed == 3);
    REQUIRE((cardAt<AeroNodeScaleTable>(model, 0).id == 110 && cardAt<AeroNodeScaleTable>(model, 2).id == 112));
    REQUIRE((cardAt<AeroNodeTable>(model, 0).scaleId == 110 && cardAt<AeroNodeTable>(model, 1).scaleId == 113));

    REQUIRE_THROWS_AS(readBulk("AeroNodeScale;5\nAeroNodeScale;5;2."), base::DeckError);
    expectInvalid("AeroNodeScale;0", "id");
    expectInvalid("AeroNodeScale;-4", "id");
    expectInvalid("AeroNodeScale;5;LEFT", "referenceLength1");
    expectInvalid("AeroNodeScale;5;;RIGHT", "referenceLength2");
    expectInvalid("AeroNodeScale;5;;;UP", "referenceLength3");
}

TEST_CASE("an AEGRID is a point of the aerodynamic mesh with its own ID space and its three references",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "RectFrame2;1;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "RectFrame2;3;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "AeroNodeScale;4;2.;2.;2.\n"
        "AeroNode;1;1;1.4;2.7;3.5;3;4\n"
        "AeroNode;2\n"
        "AeroNode;3;;1.;2.;3.");
    const AeroNodeCard full = cardAt<AeroNodeTable>(model, 0);
    REQUIRE((full.id == 1 && full.positionFrameId == 1 && full.displacementFrameId == 3 && full.scaleId == 4));
    REQUIRE((full.xCoord1 == 1.4 && full.xCoord2 == 2.7 && full.xCoord3 == 3.5));
    const AeroNodeCard bare = cardAt<AeroNodeTable>(model, 1);
    REQUIRE((bare.id == 2 && bare.positionFrameId == 0 && bare.displacementFrameId == 0 && bare.scaleId == 0));
    REQUIRE((bare.xCoord1 == 0.0 && bare.xCoord2 == 0.0 && bare.xCoord3 == 0.0)); // a blank coordinate is 0.0
    const AeroNodeCard basic = cardAt<AeroNodeTable>(model, 2);
    REQUIRE((basic.positionFrameId == 0 && basic.displacementFrameId == 0 && basic.scaleId == 0));
    REQUIRE((basic.xCoord1 == 1.0 && basic.xCoord2 == 2.0 && basic.xCoord3 == 3.0));
    REQUIRE((AeroNodeTable::INFO.category == CardCategory::AeroNode &&
             AeroNodeTable::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    // The three coordinates are always written; the blank systems and the missing scale are not.

    // The aerodynamic points are not the structural ones: a GRID 1 and an AEGRID 1 coexist, two AEGRID 1 do not.
    REQUIRE_NOTHROW(readBulk("Node;1;;0.;0.;0.\nAeroNode;1"));
    REQUIRE_THROWS_AS(readBulk("AeroNode;1\nAeroNode;1;;1.;1.;1."), base::DeckError);

    RecordingVisitor visitor;
    model.table<AeroNodeTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"positionFrameId", "displacementFrameId", "scaleId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Coordinate, ReferenceTarget::Coordinate,
                                                            ReferenceTarget::AeroScale});
    visitor = RecordingVisitor{};
    model.table<AeroNodeTable>().visitReferences(1, visitor);
    model.table<AeroNodeTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields.empty()); // the basic system and no scale: nothing to look up
    REQUIRE(unresolvedIn(model) == 0);
    Model dangling = readBulk("AeroNode;1;9;1.;1.;1.;8;7");
    REQUIRE(unresolvedIn(dangling) == 3);

    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE((cardAt<AeroNodeTable>(model, 0).positionFrameId == 11 &&
             cardAt<AeroNodeTable>(model, 0).displacementFrameId == 13));
    Renumbering::apply(model, RenumberSpace::AeroScale, Renumbering::offset(100));
    REQUIRE((cardAt<AeroNodeTable>(model, 0).scaleId == 104 && cardAt<AeroNodeScaleTable>(model, 0).id == 104));
    const std::size_t changed = Renumbering::apply(model, RenumberSpace::AeroNode, Renumbering::offset(100));
    REQUIRE(changed == 3);
    REQUIRE((cardAt<AeroNodeTable>(model, 0).id == 101 && cardAt<AeroNodeTable>(model, 2).id == 103));
    REQUIRE(model.findTable<AeroNodeTable>()->idAtRow(1) == 102);

    expectInvalid("AeroNode;0", "id");
    expectInvalid("AeroNode;1;-1", "positionFrameId");
    expectInvalid("AeroNode;1;;LEFT", "xCoord1");
    expectInvalid("AeroNode;1;;1.;LEFT", "xCoord2");
    expectInvalid("AeroNode;1;;1.;1.;LEFT", "xCoord3");
    expectInvalid("AeroNode;1;;1.;1.;1.;-2", "displacementFrameId");
    expectInvalid("AeroNode;1;;1.;1.;1.;;-3", "scaleId");
}

TEST_CASE("an AEQUAD4 and an AETRIA3 connect AEGRID points and name a component that no card owns",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroNode;14\n"
        "AeroNode;27\n"
        "AeroNode;43\n"
        "AeroNode;58\n"
        "AeroQuad4;106;2;14;27;43;58\n"
        "AeroQuad4;107;1;58;43;27;14\n"
        "AeroTria3;2416;7;14;27;43");
    const AeroQuad4Card quad = cardAt<AeroQuad4Table>(model, 0);
    REQUIRE((quad.id == 106 && quad.componentId == 2 && quad.node1 == 14 && quad.node2 == 27 && quad.node3 == 43 &&
             quad.node4 == 58));
    REQUIRE((cardAt<AeroQuad4Table>(model, 1).node1 == 58 && cardAt<AeroQuad4Table>(model, 1).node4 == 14));
    const AeroTria3Card triangle = cardAt<AeroTria3Table>(model, 0);
    REQUIRE((triangle.id == 2416 && triangle.componentId == 7 && triangle.node1 == 14 && triangle.node2 == 27 &&
             triangle.node3 == 43));
    // The mesh elements are neither structural elements nor aerodynamic panels: they have no category of their own.
    REQUIRE((AeroQuad4Table::INFO.category == CardCategory::Aero &&
             AeroTria3Table::INFO.category == CardCategory::Aero));
    REQUIRE((AeroQuad4Table::INFO.idSpace == base::IdSpace::PerCardType &&
             AeroTria3Table::INFO.idSpace == base::IdSpace::PerCardType));
    requireRoundTrip(model);

    // The grid points are references to AEGRID cards; the component number is not looked up.
    RecordingVisitor visitor;
    model.table<AeroQuad4Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"node1", "node2", "node3", "node4"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(4, ReferenceTarget::AeroNode));
    visitor = RecordingVisitor{};
    model.table<AeroTria3Table>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"node1", "node2", "node3"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>(3, ReferenceTarget::AeroNode));
    REQUIRE(unresolvedIn(model) == 0);
    Model dangling = readBulk("AeroQuad4;1;1;1;2;3;4\nAeroTria3;2;1;1;2;3");
    REQUIRE(unresolvedIn(dangling) == 7);

    // Renumbering the points follows them in both cards; the component number and the element numbers stay.
    const std::size_t changed = Renumbering::apply(model, RenumberSpace::AeroNode, Renumbering::offset(1000));
    REQUIRE(changed == 4);
    const AeroQuad4Card moved = cardAt<AeroQuad4Table>(model, 0);
    REQUIRE((moved.id == 106 && moved.componentId == 2 && moved.node1 == 1014 && moved.node2 == 1027 &&
             moved.node3 == 1043 && moved.node4 == 1058));
    const AeroTria3Card movedTriangle = cardAt<AeroTria3Table>(model, 0);
    REQUIRE((movedTriangle.id == 2416 && movedTriangle.componentId == 7 && movedTriangle.node1 == 1014 &&
             movedTriangle.node3 == 1043));

    // Each type has its own element numbers (a check of the model compares them); two cards of one type may not share
    // one.
    REQUIRE_THROWS_AS(readBulk("AeroQuad4;1;1;1;2;3;4\nAeroQuad4;1;1;5;6;7;8"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("AeroTria3;1;1;1;2;3\nAeroTria3;1;1;5;6;7"), base::DeckError);

    expectInvalid("AeroQuad4;0;2;1;2;3;4", "id");
    expectInvalid("AeroQuad4;1;0;1;2;3;4", "componentId");
    expectInvalid("AeroQuad4;1;2;0;2;3;4", "node1");
    expectInvalid("AeroQuad4;1;2;1;-2;3;4", "node2");
    expectInvalid("AeroQuad4;1;2;1;2;3", "node4");   // a point is missing
    expectInvalid("AeroQuad4;1;2;1;2;3;1", "node4"); // the four points are different
    expectInvalid("AeroQuad4;1;2;1;2;2;4", "node3");
    expectInvalid("AeroTria3;0;2;1;2;3", "id");
    expectInvalid("AeroTria3;1;0;1;2;3", "componentId");
    expectInvalid("AeroTria3;1;2;0;2;3", "node1");
    expectInvalid("AeroTria3;1;2;1;2", "node3");
    expectInvalid("AeroTria3;1;2;1;2;1", "node3"); // a triangle with a repeated point has no area
}

TEST_CASE("an ACTRIM is found by its name, has a complex factor that defaults to 1 and refers to no card",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "TrimmedMaterialMatrices;floor;36;37;2.5;0.75\nTrimmedMaterialMatrices;WALL;36;37\nTrimmedMaterialMatrices;"
        "SIDE;41;42;0.5\nTrimmedMaterialMatrices;TOP;43;44;;0.25");
    const TrimmedMaterialMatricesCard full = cardAt<TrimmedMaterialMatricesTable>(model, 0);
    REQUIRE((full.name == "floor" && full.mappedDataUnit == 36 && full.propertyMatrixUnit == 37 &&
             full.scaleReal == 2.5 && full.scaleImaginary == 0.75));
    const TrimmedMaterialMatricesCard minimal = cardAt<TrimmedMaterialMatricesTable>(model, 1);
    REQUIRE((minimal.name == "WALL" && minimal.scaleReal == 1.0 && minimal.scaleImaginary == 0.0));
    const TrimmedMaterialMatricesCard real = cardAt<TrimmedMaterialMatricesTable>(model, 2);
    REQUIRE((real.scaleReal == 0.5 && real.scaleImaginary == 0.0));
    const TrimmedMaterialMatricesCard imaginary = cardAt<TrimmedMaterialMatricesTable>(model, 3);
    REQUIRE((imaginary.mappedDataUnit == 43 && imaginary.propertyMatrixUnit == 44 && imaginary.scaleReal == 1.0 &&
             imaginary.scaleImaginary == 0.25));
    REQUIRE((TrimmedMaterialMatricesTable::INFO.category == CardCategory::Aero &&
             TrimmedMaterialMatricesTable::INFO.idSpace == base::IdSpace::None));
    requireRoundTrip(model);

    // A factor that is the default is a blank field; a blank in front of a real part is kept (SCLI is field 5).

    // The unit numbers belong to the FMS section, the name to the Case Control: nothing to follow.
    RecordingVisitor visitor;
    for (std::uint32_t row = 0; row < 4; ++row)
        model.table<TrimmedMaterialMatricesTable>().visitReferences(row, visitor);
    REQUIRE(visitor.fields.empty());
    REQUIRE(unresolvedIn(model) == 0);

    expectInvalid("TrimmedMaterialMatrices;;31;32", "name");
    requireLiteralRoundTrip("TrimmedMaterialMatrices;1ABC;31;32", "1ABC");
    requireLiteralRoundTrip("TrimmedMaterialMatrices;Long mixedCase.label;31;32", "Long mixedCase.label");
    expectInvalid("TrimmedMaterialMatrices;A;0;32", "mappedDataUnit");
    expectInvalid("TrimmedMaterialMatrices;A;;32", "mappedDataUnit");
    expectInvalid("TrimmedMaterialMatrices;A;31;-1", "propertyMatrixUnit");
    expectInvalid("TrimmedMaterialMatrices;A;31", "propertyMatrixUnit");
    expectInvalid("TrimmedMaterialMatrices;A;31;31", "propertyMatrixUnit"); // a unit number is assigned once
    expectInvalid("TrimmedMaterialMatrices;A;31;32;LEFT", "scaleReal");
    expectInvalid("TrimmedMaterialMatrices;A;31;32;1.;RIGHT", "scaleImaginary");
}

TEST_CASE("an AEDW has a Mach number, two symmetry words, a control vector and matrix names", "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroDownwash;0.85;Symmetric;Asymmetric;105;WSH1\nAeroDownwash;1.5;Antisymmetric;Symmetric;102;alp2;inter");
    const AeroDownwashCard first = cardAt<AeroDownwashTable>(model, 0);
    REQUIRE((first.machNumber == 0.85 && first.symmetricXz == ForceSymmetry::Symmetric &&
             first.symmetricXy == ForceSymmetry::Asymmetric));
    REQUIRE((first.controlVectorId == 105 && first.matrixName == "WSH1" && !first.interpolationMatrixName.has_value()));
    // The words are read in either case and written as the format reference writes them; the matrix names are kept as
    // written.
    const AeroDownwashCard second = cardAt<AeroDownwashTable>(model, 1);
    REQUIRE((second.machNumber == 1.5 && second.symmetricXz == ForceSymmetry::Antisymmetric &&
             second.symmetricXy == ForceSymmetry::Symmetric));
    REQUIRE((second.controlVectorId == 102 && second.matrixName == "alp2" &&
             second.interpolationMatrixName == std::optional<std::string>("inter")));
    REQUIRE((AeroDownwashTable::INFO.category == CardCategory::Aero &&
             AeroDownwashTable::INFO.idSpace == base::IdSpace::None));
    requireRoundTrip(model);

    // The UXID is the only reference (the control vector); the matrix names are looked up by a check of the model.
    RecordingVisitor visitor;
    model.table<AeroDownwashTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"controlVectorId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::ControlVector});
    REQUIRE(unresolvedIn(model) == 2); // no UXVEC in the deck
    Renumbering::apply(model, RenumberSpace::ControlVector, Renumbering::offset(100));
    REQUIRE((cardAt<AeroDownwashTable>(model, 0).controlVectorId == 205 &&
             cardAt<AeroDownwashTable>(model, 1).controlVectorId == 202));
    REQUIRE(model.table<AeroDownwashTable>().controlVectorIdColumn().get(1) ==
            202); // the column the engine reads follows the card

    expectInvalid("AeroDownwash;;Symmetric;Symmetric;1;ALP", "machNumber");
    expectInvalid("AeroDownwash;-0.5;Symmetric;Symmetric;1;ALP", "machNumber");
    expectInvalid("AeroDownwash;1.;Symmetric;Symmetric;1;ALP", "machNumber"); // neither subsonic nor supersonic
    expectInvalid("AeroDownwash;0.9;;Symmetric;1;ALP", "symmetricXz");
    expectInvalid("AeroDownwash;0.9;SIDE;Symmetric;1;ALP", "symmetricXz");
    expectInvalid("AeroDownwash;0.9;Symmetric;;1;ALP", "symmetricXy");
    expectInvalid("AeroDownwash;0.9;Symmetric;Both;1;ALP", "symmetricXy");
    expectInvalid("AeroDownwash;0.9;Symmetric;Symmetric;0;ALP", "controlVectorId");
    expectInvalid("AeroDownwash;0.9;Symmetric;Symmetric;;ALP", "controlVectorId");
    expectInvalid("AeroDownwash;0.9;Symmetric;Symmetric;1", "matrixName");
    requireLiteralRoundTrip("AeroDownwash;0.9;Symmetric;Symmetric;1;1ALP", "1ALP");
    requireLiteralRoundTrip("AeroDownwash;0.9;Symmetric;Symmetric;1;ALP;Long mixedCase.label", "Long mixedCase.label");
}

TEST_CASE("an AEPRESS has the layout of an AEDW for a pressure per unit dynamic pressure", "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroPressureVector;0.85;Symmetric;Asymmetric;105;PRS1\nAeroPressureVector;0.;Asymmetric;Antisymmetric;103;"
        "pres;presi");
    const AeroPressureVectorCard first = cardAt<AeroPressureVectorTable>(model, 0);
    REQUIRE((first.machNumber == 0.85 && first.symmetricXz == ForceSymmetry::Symmetric &&
             first.symmetricXy == ForceSymmetry::Asymmetric));
    REQUIRE((first.controlVectorId == 105 && first.matrixName == "PRS1" && !first.interpolationMatrixName.has_value()));
    const AeroPressureVectorCard second = cardAt<AeroPressureVectorTable>(model, 1);
    REQUIRE((second.machNumber == 0.0 && second.symmetricXz == ForceSymmetry::Asymmetric &&
             second.symmetricXy == ForceSymmetry::Antisymmetric)); // a zero Mach number is allowed
    REQUIRE((second.controlVectorId == 103 && second.matrixName == "pres" &&
             second.interpolationMatrixName == std::optional<std::string>("presi")));
    REQUIRE((AeroPressureVectorTable::INFO.category == CardCategory::Aero &&
             AeroPressureVectorTable::INFO.idSpace == base::IdSpace::None));
    requireRoundTrip(model);

    RecordingVisitor visitor;
    model.table<AeroPressureVectorTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"controlVectorId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::ControlVector});
    REQUIRE(unresolvedIn(model) == 2);
    Renumbering::apply(model, RenumberSpace::ControlVector, Renumbering::offset(100));
    REQUIRE((cardAt<AeroPressureVectorTable>(model, 0).controlVectorId == 205 &&
             cardAt<AeroPressureVectorTable>(model, 1).controlVectorId == 203));
    REQUIRE(model.table<AeroPressureVectorTable>().controlVectorIdColumn().get(0) == 205);

    expectInvalid("AeroPressureVector;;Symmetric;Symmetric;1;P", "machNumber");
    expectInvalid("AeroPressureVector;-0.5;Symmetric;Symmetric;1;P", "machNumber");
    expectInvalid("AeroPressureVector;1.;Symmetric;Symmetric;1;P", "machNumber");
    expectInvalid("AeroPressureVector;0.9;;Symmetric;1;P", "symmetricXz");
    expectInvalid("AeroPressureVector;0.9;Symmetric;SIDE;1;P", "symmetricXy");
    expectInvalid("AeroPressureVector;0.9;Symmetric;Symmetric;0;P", "controlVectorId");
    expectInvalid("AeroPressureVector;0.9;Symmetric;Symmetric;1", "matrixName");
    requireLiteralRoundTrip("AeroPressureVector;0.9;Symmetric;Symmetric;1;P;1PRESI", "1PRESI");
}

TEST_CASE("an AEFORCE is given on the aerodynamic mesh by a DMIK name or on the structural mesh by a load set",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroForceVector;0.85;Symmetric;Asymmetric;105;AeroMesh;;FRC1\n"
        "AeroForceVector;0.8;Asymmetric;Symmetric;102;Structure;25\n"
        "AeroForceVector;1.2;Antisymmetric;Antisymmetric;103;Structure;26;;PerDynamicPressure\n"
        "AeroForceVector;0.5;Symmetric;Symmetric;104;AeroMesh;;GAMMA;PerDynamicPressure");
    const AeroForceVectorCard aero = cardAt<AeroForceVectorTable>(model, 0);
    REQUIRE((aero.machNumber == 0.85 && aero.symmetricXz == ForceSymmetry::Symmetric &&
             aero.symmetricXy == ForceSymmetry::Asymmetric));
    REQUIRE((aero.controlVectorId == 105 && aero.mesh == AeroForceVectorMesh::Aero && aero.loadSetId == 0));
    REQUIRE((aero.matrixName == std::optional<std::string>("FRC1") && !aero.perUnitPressure));
    const AeroForceVectorCard structural = cardAt<AeroForceVectorTable>(model, 1);
    REQUIRE(
        (structural.symmetricXz == ForceSymmetry::Asymmetric && structural.symmetricXy == ForceSymmetry::Symmetric));
    REQUIRE((structural.mesh == AeroForceVectorMesh::Struct && structural.loadSetId == 25 &&
             !structural.matrixName.has_value()));
    REQUIRE(!structural.perUnitPressure);
    const AeroForceVectorCard perUnit = cardAt<AeroForceVectorTable>(model, 2);
    REQUIRE(
        (perUnit.symmetricXz == ForceSymmetry::Antisymmetric && perUnit.loadSetId == 26 && perUnit.perUnitPressure));
    // PERQ is ignored for an AERO mesh but kept as written.
    const AeroForceVectorCard ignored = cardAt<AeroForceVectorTable>(model, 3);
    REQUIRE((ignored.mesh == AeroForceVectorMesh::Aero && ignored.matrixName == std::optional<std::string>("GAMMA") &&
             ignored.perUnitPressure));
    REQUIRE((AeroForceVectorTable::INFO.category == CardCategory::Aero &&
             AeroForceVectorTable::INFO.idSpace == base::IdSpace::None));
    requireRoundTrip(model);

    // The fields of the layout: LSET is field 6, DMIK field 7 and PERQ field 8, so the blanks are kept in between.

    // The UXID names a control vector and the LSET a load set (the STRUCT mesh only); the DMIK is a name.
    RecordingVisitor visitor;
    model.table<AeroForceVectorTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"controlVectorId"});
    visitor = RecordingVisitor{};
    model.table<AeroForceVectorTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"controlVectorId", "loadSetId"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::ControlVector, ReferenceTarget::LoadCombinationSet});
    REQUIRE(unresolvedIn(model) == 6); // four control vectors and two load sets that the deck does not have
    Renumbering::apply(model, RenumberSpace::ControlVector, Renumbering::offset(100));
    REQUIRE((cardAt<AeroForceVectorTable>(model, 0).controlVectorId == 205 &&
             cardAt<AeroForceVectorTable>(model, 3).controlVectorId == 204));
    REQUIRE(model.table<AeroForceVectorTable>().controlVectorIdColumn().get(2) == 203);
    Renumbering::apply(model, RenumberSpace::LoadCombinationSet, Renumbering::offset(100));
    REQUIRE((cardAt<AeroForceVectorTable>(model, 1).loadSetId == 125 &&
             cardAt<AeroForceVectorTable>(model, 2).loadSetId == 126));
    REQUIRE((cardAt<AeroForceVectorTable>(model, 0).loadSetId == 0 &&
             cardAt<AeroForceVectorTable>(model, 0).controlVectorId == 205));

    expectInvalid("AeroForceVector;;Symmetric;Symmetric;1;AeroMesh;;B", "machNumber");
    expectInvalid("AeroForceVector;1.;Symmetric;Symmetric;1;AeroMesh;;B", "machNumber");
    expectInvalid("AeroForceVector;0.9;;Symmetric;1;AeroMesh;;B", "symmetricXz");
    expectInvalid("AeroForceVector;0.9;Symmetric;SIDE;1;AeroMesh;;B", "symmetricXy");
    expectInvalid("AeroForceVector;0.9;Symmetric;Symmetric;0;AeroMesh;;B", "controlVectorId");
    expectInvalid("AeroForceVector;0.9;Symmetric;Symmetric;1;;;B", "mesh"); // no default for the mesh
    expectInvalid("AeroForceVector;0.9;Symmetric;Symmetric;1;Both;5", "mesh");
    expectInvalid("AeroForceVector;0.9;Symmetric;Symmetric;1;AeroMesh", "matrixName"); // an AERO mesh needs its matrix
    requireLiteralRoundTrip("AeroForceVector;0.9;Symmetric;Symmetric;1;AeroMesh;;1B", "1B");
    expectInvalid("AeroForceVector;0.9;Symmetric;Symmetric;1;AeroMesh;5;B", "loadSetId"); // the load set is for STRUCT
    expectInvalid("AeroForceVector;0.9;Symmetric;Symmetric;1;Structure",
                  "loadSetId"); // a STRUCT mesh needs its load set
    expectInvalid("AeroForceVector;0.9;Symmetric;Symmetric;1;Structure;-5", "loadSetId");
    expectInvalid("AeroForceVector;0.9;Symmetric;Symmetric;1;Structure;5;B", "matrixName"); // the matrix is for AERO
    expectInvalid("AeroForceVector;0.9;Symmetric;Symmetric;1;AeroMesh;;B;NOPE", "perUnitPressure");
}
