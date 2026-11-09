// Checks of the brake, contact and state cards: the brake squeal cards of SOL 400 (BRKPROP, BRKSYS, BSQUEAL, MOTION),
// the glue release UNGLUE, the load stepping control table TABSCTL with the CRITTID of an NLSTEP that names it, the
// cards of the SOL 400 initial and deactivated states (DEACTEL, IPSTRAIN, ISTRESS), the tire selection TIRE, the
// connector override CONCTL and the RBE3 perturbation override GMOVR3. Per card: the fields in their format reference
// positions, the defaults, the references it lists and what each one points to, the renumbering of the spaces the card
// owns or names, the refused values and the round trip through a model file. The generic tests of
// CardRoundTripTest.cpp read, write and copy every sample; this file covers what each card means.

#include "oor/model/cards/elements/connectorSearchControl/ConnectorSearchControlTable.hpp"
#include "oor/model/cards/nonlinear_contact/brakeMotion/BrakeMotionTable.hpp"
#include "oor/model/cards/nonlinear_contact/brakeProp/BrakePropTable.hpp"
#include "oor/model/cards/nonlinear_contact/brakeSqueal/BrakeSquealTable.hpp"
#include "oor/model/cards/nonlinear_contact/brakeSystem/BrakeSystemTable.hpp"
#include "oor/model/cards/nonlinear_contact/contactUnglueNodes/ContactUnglueNodesTable.hpp"
#include "oor/model/cards/nonlinear_contact/nonlinearStepControl/NonlinearStepControlTable.hpp"
#include "oor/model/cards/nonlinear_contact/stepControlCriteria/StepControlCriteriaTable.hpp"
#include "oor/model/cards/rigid_elements/rigidInterpolationOverride/RigidInterpolationOverrideTable.hpp"
#include "oor/model/cards/solution_control/deactivatedElems/DeactivatedElemsTable.hpp"
#include "oor/model/cards/solution_control/initialPlasticStrain/InitialPlasticStrainTable.hpp"
#include "oor/model/cards/solution_control/initialStress/InitialStressTable.hpp"
#include "oor/model/cards/solution_control/tireSelection/TireSelectionTable.hpp"
#include "oor/model/cards/tables/tableMultiVariable0/TableMultiVariable0Table.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

bool contains(const std::string& text, const std::string& part)
{
    return text.find(part) != std::string::npos;
}

// The number of references that name nothing in the model that the cards make.
std::size_t unresolvedOf(const std::string& cards)
{
    Model model = readBulk(cards);
    return unresolvedIn(model);
}

// A brake system: two bodies (BCBODY1 on one BSURF), two motions, a property, a BRKSYS of two disks and a BSQUEAL. The
// second disk leaves its MT2_ID blank, so it uses the first motion for both bodies.
const std::string BRAKE_TEXT =
    "ContactElemSurface;200;1;Through;4\n"
    "ContactBody1;110;;ThreeDimensional;Deformable;200\n"
    "ContactBody1;111;;ThreeDimensional;Deformable;200\n"
    "BrakeMotion;30;;RotationMotion;0.;0.;1.\n"
    "BrakeMotion;31;;TranslationMotion;1.;0.;0.\n"
    "BrakeProp;40;AverageStiffnessMethod;1.E+7\n"
    "BrakeSystem;900;0.5;0;Yes;;INNER;110;111;30;31;40;OUTER;110;111;30;;40\n"
    "BrakeSqueal;901;;1.E+7\n";

} // namespace

// ---------------------------------------------------------------------------------------------------------------------
// BRKPROP
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a BRKPROP is a list of AVSTIF, TAVSTIF and FRICT pairs and names a TABL3Di", "[model][cards][brake]")
{
    REQUIRE((BrakePropTable::INFO.category == CardCategory::BrakeProperty &&
             BrakePropTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(
        "BrakeProp;28;AverageStiffnessMethod;3.75E+6;Friction;0.35\n"
        "BrakeProp;29;TemperatureAverageStiffness;201\n"
        "BrakeProp;30;Friction;0.3;TemperatureAverageStiffness;202;AverageStiffnessMethod;2.5E+7\n"
        "BrakeProp;31;AverageStiffnessMethod;1.E+6;;;;;TemperatureAverageStiffness;203;Friction;0.1\n"
        "TableMultiVariable0;201;12;;;;;0.;1.;1.;2.");
    const BrakePropCard example = cardAt<BrakePropTable>(model, 0);
    REQUIRE((example.id == 28 && example.areaStiffness == 3.75E6 && example.areaStiffnessTableId == 0 &&
             example.friction == 0.35));
    // A table gives the stiffness, and a blank FRICT is 0.0, which takes the friction from the contact table.
    const BrakePropCard tabled = cardAt<BrakePropTable>(model, 1);
    REQUIRE((!tabled.areaStiffness.has_value() && tabled.areaStiffnessTableId == 201 && tabled.friction == 0.0));
    // The pairs may come in any order, and a continuation line starts its pairs in its first field.
    const BrakePropCard reordered = cardAt<BrakePropTable>(model, 2);
    REQUIRE((reordered.areaStiffness == 2.5E7 && reordered.areaStiffnessTableId == 202 && reordered.friction == 0.3));
    const BrakePropCard continued = cardAt<BrakePropTable>(model, 3);
    REQUIRE((continued.areaStiffness == 1.E6 && continued.areaStiffnessTableId == 203 && continued.friction == 0.1));
    requireRoundTrip(model);

    // The pairs are written in the order AVSTIF, TAVSTIF, FRICT; the friction coefficient is left out when it is 0.0.
    const Model plain = readBulk("BrakeProp;40;Friction;0.;AverageStiffnessMethod;1.E+6");
    REQUIRE_FALSE(contains(writeModelText(plain), "Friction"));
    REQUIRE(contains(writeModelText(plain, false), "Friction"));
    REQUIRE(contains(writeModelText(readBulk(
                         "BrakeProp;41;Friction;0.2;TemperatureAverageStiffness;5;AverageStiffnessMethod;1.E+6")),
                     "BrakeProp;41;AverageStiffnessMethod"));
    REQUIRE(contains(writeModelText(readBulk("BrakeProp;42;TemperatureAverageStiffness;5")),
                     "BrakeProp;42;TemperatureAverageStiffness;5"));

    // Only TAVSTIF names a card: a TABL3Di of the model.
    RecordingVisitor visitor;
    model.table<BrakePropTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"areaStiffnessTableId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::MultiDimensionalTable});
    RecordingVisitor blank;
    model.table<BrakePropTable>().visitReferences(0, blank);
    REQUIRE(blank.fields.empty());
    REQUIRE(unresolvedIn(model) == 2); // the tables 202 and 203 do not exist

    // Editing: the values are checked as the reader checks them, and a card needs the stiffness or its table.
    BrakePropCard edited = cardAt<BrakePropTable>(model, 0);
    edited.friction = 0.4;
    REQUIRE_FALSE(BrakePropFormat::check(edited).has_value());
    model.table<BrakePropTable>().store(0, edited);
    REQUIRE(cardAt<BrakePropTable>(model, 0).friction == 0.4);
    edited.areaStiffness.reset();
    REQUIRE(BrakePropFormat::check(edited)->field == std::string("areaStiffness"));
    edited.areaStiffnessTableId = 5;
    REQUIRE_FALSE(BrakePropFormat::check(edited).has_value());
    edited.friction = -1.0;
    REQUIRE(BrakePropFormat::check(edited)->field == std::string("friction"));

    // Renumbering: the TABL3Di the card names, and the numbers of the BRKPROP cards.
    Renumbering::apply(model, RenumberSpace::MultiDimensionalTable, Renumbering::offset(1000));
    REQUIRE((cardAt<BrakePropTable>(model, 1).areaStiffnessTableId == 1201 &&
             cardAt<BrakePropTable>(model, 3).areaStiffnessTableId == 1203));
    REQUIRE(cardAt<TableMultiVariable0Table>(model).id == 1201);
    Renumbering::apply(model, RenumberSpace::BrakeProperty, Renumbering::offset(500));
    REQUIRE((cardAt<BrakePropTable>(model, 0).id == 528 && cardAt<BrakePropTable>(model, 3).id == 531));
    REQUIRE(unresolvedIn(model) == 2);

    const std::size_t before = model.cardCount();
    model.remove(*model.locateInType(typeIndexOf<BrakePropTable>(), 529));
    REQUIRE(model.cardCount() == before - 1);
    REQUIRE_FALSE(model.locateInType(typeIndexOf<BrakePropTable>(), 529).has_value());

    REQUIRE_THROWS_AS(readBulk("BrakeProp;28;AverageStiffnessMethod;1.E+6\nBrakeProp;28;AverageStiffnessMethod;2.E+6"),
                      base::DeckError);
    expectInvalid("BrakeProp;0;AverageStiffnessMethod;1.E+7", "id");
    expectInvalid("BrakeProp;-3;AverageStiffnessMethod;1.E+7", "id");
    expectInvalid("BrakeProp;1", "areaStiffness"); // the stiffness or its table is required
    expectInvalid("BrakeProp;1;Friction;0.2", "areaStiffness");
    expectInvalid("BrakeProp;1;AverageStiffnessMethod;-5.", "areaStiffness");
    expectInvalid("BrakeProp;1;AverageStiffnessMethod;0.", "areaStiffness");
    expectInvalid("BrakeProp;1;AverageStiffnessMethod;x", "areaStiffness");
    expectInvalid("BrakeProp;1;AverageStiffnessMethod;1.E+7;Friction;-0.1", "friction");
    expectInvalid("BrakeProp;1;AverageStiffnessMethod;1.E+7;Friction;x", "friction");
    expectInvalid("BrakeProp;1;TemperatureAverageStiffness;-3;AverageStiffnessMethod;1.E+6", "areaStiffnessTableId");
    expectInvalid("BrakeProp;1;TemperatureAverageStiffness;1.5", "areaStiffnessTableId");
    expectInvalid("BrakeProp;1;AverageStiffnessMethod;1.E+7;AverageStiffnessMethod;2.E+7",
                  "parameterName"); // a parameter is given once
    expectInvalid("BrakeProp;1;AverageStiffnessMethod;1.E+7;STIFF;3.",
                  "parameterName");                    // a name that is not on the format
    expectInvalid("BrakeProp;1;;5.", "parameterName"); // a value with no name
}

// ---------------------------------------------------------------------------------------------------------------------
// BRKSYS
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a BRKSYS has the flags of its first line and a disk on each line, with a blank MT2_ID resolved to MT1_ID",
          "[model][cards][brake]")
{
    REQUIRE((BrakeSystemTable::INFO.category == CardCategory::BrakeSqueal &&
             BrakeSystemTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(BRAKE_TEXT + "BrakeSystem;902\nBrakeSystem;903;;1;No;1;FRONT;110;111;30;31;40\n");
    const BrakeSystemCard card = cardAt<BrakeSystemTable>(model, 0);
    REQUIRE((card.id == 900 && card.loadFactor == 0.5 && card.frictionVectorMode == 0 &&
             card.squealOnly == BrakeSquealOnly::Yes && card.slidingContactOption == 0));
    REQUIRE(card.disks.size() == 2);
    const BrakeSystemDisk inner = card.disks[0];
    REQUIRE((inner.name == "INNER" && inner.firstBodyId == 110 && inner.secondBodyId == 111 &&
             inner.firstMotionId == 30 && inner.secondMotionId == 31 && inner.brakePropId == 40));
    const BrakeSystemDisk outer = card.disks[1];
    REQUIRE(
        (outer.name == "OUTER" && outer.firstMotionId == 30 && outer.secondMotionId == 30)); // MT2_ID: blank is MT1_ID
    // The defaults of the first line, and a card with no disk line (the format does not require one).
    const BrakeSystemCard bare = cardAt<BrakeSystemTable>(model, 1);
    REQUIRE((bare.id == 902 && bare.loadFactor == 0.0 && bare.frictionVectorMode == 0 &&
             bare.squealOnly == BrakeSquealOnly::Yes && bare.slidingContactOption == 0 && bare.disks.empty()));
    const BrakeSystemCard flags = cardAt<BrakeSystemTable>(model, 2);
    REQUIRE((flags.frictionVectorMode == 1 && flags.squealOnly == BrakeSquealOnly::No &&
             flags.slidingContactOption == 1 && flags.disks.size() == 1));
    requireRoundTrip(model);

    // A written card has the MT2_ID that was resolved, and a first line of defaults has only the ID.

    REQUIRE(cardAt<BrakeSystemTable>(readBulk("BrakeSystem;5;;;;;a;1;2;3;;4"), 0).disks[0].secondMotionId == 3);

    // Each disk names two contact bodies, two motions and a property, in that order.
    RecordingVisitor visitor;
    model.table<BrakeSystemTable>().visitReferences(0, visitor);
    const std::vector<std::string> oneDisk{"firstBodyId", "secondBodyId", "firstMotionId", "secondMotionId",
                                           "brakePropId"};
    std::vector<std::string> expectedFields = oneDisk;
    expectedFields.insert(expectedFields.end(), oneDisk.begin(), oneDisk.end());
    REQUIRE(visitor.fields == expectedFields);
    const std::vector<ReferenceTarget> oneDiskTargets{ReferenceTarget::ContactBody, ReferenceTarget::ContactBody,
                                                      ReferenceTarget::BrakeMotion, ReferenceTarget::BrakeMotion,
                                                      ReferenceTarget::BrakeProperty};
    std::vector<ReferenceTarget> expectedTargets = oneDiskTargets;
    expectedTargets.insert(expectedTargets.end(), oneDiskTargets.begin(), oneDiskTargets.end());
    REQUIRE(visitor.targets == expectedTargets);
    RecordingVisitor none;
    model.table<BrakeSystemTable>().visitReferences(1, none);
    REQUIRE(none.fields.empty());

    // The bodies, the motions and the property exist in the model.
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("a BRKSYS follows the renumbering of the bodies, the motions, the property and the analysis number",
          "[model][cards][brake]")
{
    Model model = readBulk(BRAKE_TEXT);
    REQUIRE(unresolvedIn(model) == 0);

    Renumbering::apply(model, RenumberSpace::BrakeMotion, Renumbering::offset(1000));
    REQUIRE((cardAt<BrakeMotionTable>(model, 0).id == 1030 && cardAt<BrakeMotionTable>(model, 1).id == 1031));
    BrakeSystemCard card = cardAt<BrakeSystemTable>(model);
    REQUIRE((card.disks[0].firstMotionId == 1030 && card.disks[0].secondMotionId == 1031));
    REQUIRE((card.disks[1].firstMotionId == 1030 &&
             card.disks[1].secondMotionId == 1030)); // the resolved default moves with MT1_ID
    Renumbering::apply(model, RenumberSpace::BrakeProperty, Renumbering::offset(500));
    REQUIRE(cardAt<BrakePropTable>(model).id == 540);
    REQUIRE((cardAt<BrakeSystemTable>(model).disks[0].brakePropId == 540 &&
             cardAt<BrakeSystemTable>(model).disks[1].brakePropId == 540));
    Renumbering::apply(model, RenumberSpace::ContactBody, Renumbering::offset(5000));
    card = cardAt<BrakeSystemTable>(model);
    REQUIRE((card.disks[0].firstBodyId == 5110 && card.disks[0].secondBodyId == 5111));
    // The BSQUEAL and the BRKSYS are one number space: both move.
    Renumbering::apply(model, RenumberSpace::BrakeSqueal, Renumbering::offset(100));
    REQUIRE((cardAt<BrakeSystemTable>(model).id == 1000 && cardAt<BrakeSquealTable>(model).id == 1001));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    // A body, a motion or a property that no card defines is reported for each name.
    Model broken = readBulk("BrakeSystem;5;;;;;X;998;999;997;996;995");
    REQUIRE(unresolvedIn(broken) == 5);

    const std::size_t before = model.cardCount();
    model.remove(*model.locateInType(typeIndexOf<BrakeSystemTable>(), 1000));
    REQUIRE(model.cardCount() == before - 1);
    REQUIRE(unresolvedIn(model) == 0);
    // A deck cannot give two BRKSYS the same number (the check `contact.brake-squeal-id-repeated` is for a BSQUEAL).
    REQUIRE_THROWS_AS(readBulk("BrakeSystem;5\nBrakeSystem;5"), base::DeckError);
}

TEST_CASE("a BRKSYS edited through its value keeps the rules of the reader", "[model][cards][brake]")
{
    Model model = readBulk(BRAKE_TEXT);
    BrakeSystemCard edited = cardAt<BrakeSystemTable>(model);
    edited.loadFactor = 2.0;
    edited.disks[0].name = "FRONT";
    REQUIRE_FALSE(BrakeSystemFormat::check(edited).has_value());
    model.table<BrakeSystemTable>().store(0, edited);
    REQUIRE((cardAt<BrakeSystemTable>(model).loadFactor == 2.0 &&
             cardAt<BrakeSystemTable>(model).disks[0].name == "FRONT"));
    edited.frictionVectorMode = 2;
    REQUIRE(BrakeSystemFormat::check(edited)->field == std::string("frictionVectorMode"));
    edited.frictionVectorMode = 1;
    edited.slidingContactOption = -1;
    REQUIRE(BrakeSystemFormat::check(edited)->field == std::string("slidingContactOption"));
    edited.slidingContactOption = 0;
    edited.disks[1].secondBodyId = 0;
    REQUIRE(BrakeSystemFormat::check(edited)->field == std::string("secondBodyId"));
}

TEST_CASE("a BRKSYS refuses the values that its format refuses", "[model][cards][brake]")
{
    expectInvalid("BrakeSystem;0", "id");
    expectInvalid("BrakeSystem;-5", "id");
    expectInvalid("BrakeSystem;1;x", "loadFactor");
    expectInvalid("BrakeSystem;1;;2", "frictionVectorMode");
    expectInvalid("BrakeSystem;1;;x", "frictionVectorMode");
    expectInvalid("BrakeSystem;1;;;MAYBE", "squealOnly");
    expectInvalid("BrakeSystem;1;;;;3", "slidingContactOption");
    expectInvalid("BrakeSystem;1;;;;;;8;9;18;19;28", "diskName");            // the name is required
    REQUIRE_NOTHROW(readBulk("BrakeSystem;1;;;;;TOOLONGNAME;8;9;18;19;28")); // at most eight characters
    expectInvalid("BrakeSystem;1;;;;;INNER;;9;18;19;28", "firstBodyId");
    expectInvalid("BrakeSystem;1;;;;;INNER;-8;9;18;19;28", "firstBodyId");
    expectInvalid("BrakeSystem;1;;;;;INNER;8;;18;19;28", "secondBodyId");
    expectInvalid("BrakeSystem;1;;;;;INNER;8;9;;19;28", "firstMotionId");
    expectInvalid("BrakeSystem;1;;;;;INNER;8;9;18;x;28", "secondMotionId");
    expectInvalid("BrakeSystem;1;;;;;INNER;8;9;18;19", "brakePropId");
    // Two disks, the second with a bad body.
    expectInvalid("BrakeSystem;1;;;;;INNER;8;9;18;19;28;OUTER;0;9;18;19;28", "firstBodyId");
}

// ---------------------------------------------------------------------------------------------------------------------
// BSQUEAL
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a BSQUEAL has the stiffness, the flag and an axis on a second line", "[model][cards][brake]")
{
    REQUIRE((BrakeSquealTable::INFO.category == CardCategory::BrakeSqueal &&
             BrakeSquealTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(
        "BrakeSqueal;100;0.35;6.25E+6;No;0.;0.;1.;1.5;2.5;-3.5\n"
        "BrakeSqueal;101;;1.E+7\n"
        "BrakeSqueal;102;0.;2.5E+6;Yes;;;1.\n");
    const BrakeSquealCard example = cardAt<BrakeSquealTable>(model, 0);
    REQUIRE((example.id == 100 && example.loadFactor == 0.35 && example.areaStiffness == 6.25E6 &&
             example.squealOnly == BrakeSquealOnly::No));
    REQUIRE((example.axisX == 0.0 && example.axisY == 0.0 && example.axisZ == 1.0));
    REQUIRE((example.xCoord == 1.5 && example.yCoord == 2.5 && example.zCoord == -3.5));
    // The defaults: OMETH is 0.0, BSONLY is YES and the axis is blank, which the card keeps blank.
    const BrakeSquealCard minimal = cardAt<BrakeSquealTable>(model, 1);
    REQUIRE((minimal.loadFactor == 0.0 && minimal.squealOnly == BrakeSquealOnly::Yes));
    REQUIRE_FALSE(
        (minimal.axisX || minimal.axisY || minimal.axisZ || minimal.xCoord || minimal.yCoord || minimal.zCoord));
    // Each member of the axis is blank on its own (a zero is not a blank), and the word is read in any case.
    const BrakeSquealCard partial = cardAt<BrakeSquealTable>(model, 2);
    REQUIRE_FALSE((partial.axisX || partial.axisY));
    REQUIRE((partial.axisZ == 1.0 && !partial.xCoord && !partial.yCoord && !partial.zCoord &&
             partial.squealOnly == BrakeSquealOnly::Yes));
    requireRoundTrip(model);

    // The second line is written only for a card that has an axis; the blank members stay blank.

    REQUIRE_FALSE(contains(writeModelText(readBulk("BrakeSqueal;103;;1.E+7")), "Yes"));
    REQUIRE(contains(writeModelText(readBulk("BrakeSqueal;103;;1.E+7"), false), "Yes"));

    // The card names no other card.
    RecordingVisitor visitor;
    model.table<BrakeSquealTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());
    REQUIRE(unresolvedIn(model) == 0);

    // The BSQUEAL and the BRKSYS are one number space of the analysis (the check `contact.brake-squeal-id-repeated`
    // reports a number that both have); the reader accepts both.
    REQUIRE_NOTHROW(readBulk("BrakeSqueal;900;;1.E+7\nBrakeSystem;900"));
    REQUIRE_THROWS_AS(readBulk("BrakeSqueal;900;;1.E+7\nBrakeSqueal;900;;2.E+7"), base::DeckError);
    Renumbering::apply(model, RenumberSpace::BrakeSqueal, Renumbering::offset(500));
    REQUIRE((cardAt<BrakeSquealTable>(model, 0).id == 600 && cardAt<BrakeSquealTable>(model, 2).id == 602));

    BrakeSquealCard edited = cardAt<BrakeSquealTable>(model, 0);
    edited.axisX.reset();
    edited.areaStiffness = 8.0E6;
    REQUIRE_FALSE(BrakeSquealFormat::check(edited).has_value());
    model.table<BrakeSquealTable>().store(0, edited);
    REQUIRE((!cardAt<BrakeSquealTable>(model, 0).axisX && cardAt<BrakeSquealTable>(model, 0).areaStiffness == 8.0E6));
    edited.areaStiffness = 0.0;
    REQUIRE(BrakeSquealFormat::check(edited)->field == std::string("areaStiffness"));

    const std::size_t before = model.cardCount();
    model.remove(*model.locateInType(typeIndexOf<BrakeSquealTable>(), 601));
    REQUIRE(model.cardCount() == before - 1);

    expectInvalid("BrakeSqueal;0;;1.E+6", "id");
    expectInvalid("BrakeSqueal;-1;;1.E+6", "id");
    expectInvalid("BrakeSqueal;1", "areaStiffness"); // required
    expectInvalid("BrakeSqueal;1;;-5.", "areaStiffness");
    expectInvalid("BrakeSqueal;1;;0.", "areaStiffness");
    expectInvalid("BrakeSqueal;1;x;1.E+6", "loadFactor");
    expectInvalid("BrakeSqueal;1;;1.E+6;MAYBE", "squealOnly");
    expectInvalid("BrakeSqueal;1;;1.E+6;;x", "axisX");
    expectInvalid("BrakeSqueal;1;;1.E+6;;;x", "axisY");
    expectInvalid("BrakeSqueal;1;;1.E+6;;;;x", "axisZ");
    expectInvalid("BrakeSqueal;1;;1.E+6;;;;;x", "xCoord");
    expectInvalid("BrakeSqueal;1;;1.E+6;;;;;;x", "yCoord");
    expectInvalid("BrakeSqueal;1;;1.E+6;;;;;;;x", "zCoord");
}

// ---------------------------------------------------------------------------------------------------------------------
// MOTION
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a MOTION is a rotation about an axis through a center, or a translation along a direction",
          "[model][cards][brake]")
{
    REQUIRE((BrakeMotionTable::INFO.category == CardCategory::BrakeMotion &&
             BrakeMotionTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(
        "BrakeMotion;18;50.;RotationMotion;0.5;0.2;0.9;12.;7.5;-2.\n"
        "BrakeMotion;19;100.;TranslationMotion;0.7;0.3;0.9\n"
        "BrakeMotion;20;;RotationMotion;0.;0.;1.\n");
    const BrakeMotionCard rotation = cardAt<BrakeMotionTable>(model, 0);
    REQUIRE((rotation.id == 18 && rotation.velocity == 50.0 && rotation.form == BrakeMotionForm::Rotat));
    REQUIRE((rotation.cosineX == 0.5 && rotation.cosineY == 0.2 && rotation.cosineZ == 0.9));
    REQUIRE((rotation.xCoord == 12.0 && rotation.yCoord == 7.5 && rotation.zCoord == -2.0));
    const BrakeMotionCard translation = cardAt<BrakeMotionTable>(model, 1);
    REQUIRE((translation.form == BrakeMotionForm::Trans && translation.velocity == 100.0));
    REQUIRE((translation.cosineX == 0.7 && translation.cosineY == 0.3 && translation.cosineZ == 0.9));
    REQUIRE((translation.xCoord == 0.0 && translation.yCoord == 0.0 &&
             translation.zCoord == 0.0)); // a translation has no center
    // A blank velocity, center and cosine are 0.0, and the form is read in any case.
    const BrakeMotionCard bare = cardAt<BrakeMotionTable>(model, 2);
    REQUIRE(
        (bare.velocity == 0.0 && bare.form == BrakeMotionForm::Rotat && bare.cosineX == 0.0 && bare.cosineZ == 1.0));
    REQUIRE((bare.xCoord == 0.0 && bare.yCoord == 0.0 && bare.zCoord == 0.0));
    requireRoundTrip(model);

    // A zero is written as a blank real, and a translation has no center fields.

    REQUIRE_FALSE(contains(writeModelText(readBulk("BrakeMotion;21;;TranslationMotion;1.;0.;0."), false), ",0.,0.,0."));

    RecordingVisitor visitor;
    model.table<BrakeMotionTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());

    // A TRANS has no center, so a translation with a center is refused, and so is a direction that is not one.
    BrakeMotionCard edited = cardAt<BrakeMotionTable>(model, 0);
    edited.form = BrakeMotionForm::Trans;
    REQUIRE(BrakeMotionFormat::check(edited)->field == std::string("xCoord"));
    edited.xCoord = 0.0;
    edited.yCoord = 0.0;
    edited.zCoord = 0.0;
    REQUIRE_FALSE(BrakeMotionFormat::check(edited).has_value());
    model.table<BrakeMotionTable>().store(0, edited);
    REQUIRE(cardAt<BrakeMotionTable>(model, 0).form == BrakeMotionForm::Trans);
    edited.cosineX = edited.cosineY = edited.cosineZ = 0.0;
    REQUIRE(BrakeMotionFormat::check(edited)->field == std::string("cosineX"));

    Renumbering::apply(model, RenumberSpace::BrakeMotion, Renumbering::offset(100));
    REQUIRE((cardAt<BrakeMotionTable>(model, 0).id == 118 && cardAt<BrakeMotionTable>(model, 2).id == 120));
    const std::size_t before = model.cardCount();
    model.remove(*model.locateInType(typeIndexOf<BrakeMotionTable>(), 119));
    REQUIRE(model.cardCount() == before - 1);
    REQUIRE_THROWS_AS(readBulk("BrakeMotion;18;;RotationMotion;1.;0.;0.\nBrakeMotion;18;;TranslationMotion;1.;0.;0."),
                      base::DeckError);

    expectInvalid("BrakeMotion;0;;RotationMotion;1.;0.;0.", "id");
    expectInvalid("BrakeMotion;1", "form"); // required
    expectInvalid("BrakeMotion;1;;SPIN;1.;0.;0.", "form");
    expectInvalid("BrakeMotion;1;x;RotationMotion;1.;0.;0.", "velocity");
    expectInvalid("BrakeMotion;1;;RotationMotion", "cosineX"); // a direction is needed
    expectInvalid("BrakeMotion;1;;RotationMotion;0.;0.;0.;5.", "cosineX");
    expectInvalid("BrakeMotion;1;;RotationMotion;x;0.;0.", "cosineX");
    expectInvalid("BrakeMotion;1;;RotationMotion;1.;y;0.", "cosineY");
    expectInvalid("BrakeMotion;1;;RotationMotion;1.;0.;z", "cosineZ");
    expectInvalid("BrakeMotion;1;;RotationMotion;1.;0.;0.;x", "xCoord");
    expectInvalid("BrakeMotion;1;;TranslationMotion;1.;0.;0.;5.", "xCoord"); // the center of a rotation
    expectInvalid("BrakeMotion;1;;TranslationMotion;1.;0.;0.;0.;5.", "yCoord");
    expectInvalid("BrakeMotion;1;;TranslationMotion;1.;0.;0.;0.;0.;5.", "zCoord");
}

// ---------------------------------------------------------------------------------------------------------------------
// UNGLUE
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an UNGLUE lists the grid points of a body with THRU and BY, and its ID is shared and may be zero",
          "[model][cards][unglue]")
{
    REQUIRE((ContactUnglueNodesTable::INFO.category == CardCategory::Other &&
             ContactUnglueNodesTable::INFO.idSpace == base::IdSpace::Group));
    Model model = readBulk(
        "Node;40\n"
        "Node;400\n"
        "Node;5120\n"
        "Node;6230\n"
        "Node;7340\n"
        "Node;1500\n"
        "Node;2500\n"
        "ContactElemSurface;200;1;Through;4\n"
        "ContactBody1;101;;ThreeDimensional;Deformable;200\n"
        "ContactUnglueNodes;10;101;40;Through;400;By;4;;5120;6230;7340;1500;Through;2500\n"
        "ContactUnglueNodes;10;101;5\n"
        "ContactUnglueNodes;0;101;7;8;9\n");
    REQUIRE(rowCount<ContactUnglueNodesTable>(model) == 3); // the cards of one ID are all kept
    const ContactUnglueNodesCard example = cardAt<ContactUnglueNodesTable>(model, 0);
    REQUIRE((example.id == 10 && example.bodyId == 101));
    REQUIRE(example.grids ==
            std::vector<ContactRun>{{40, 400, 4}, {5120, 5120, 1}, {6230, 6230, 1}, {7340, 7340, 1}, {1500, 2500, 1}});
    REQUIRE(cardAt<ContactUnglueNodesTable>(model, 1).grids == std::vector<ContactRun>{{5, 5, 1}});
    // ID 0 is the load case 0, which always acts.
    const ContactUnglueNodesCard always = cardAt<ContactUnglueNodesTable>(model, 2);
    REQUIRE((always.id == 0 && always.grids.size() == 3));
    requireRoundTrip(model);

    // A range with no BY has the increment 1, which is not written.
    const Model plain = readBulk("ContactUnglueNodes;1;101;5;Through;9\nContactUnglueNodes;2;101;5;Through;9;By;2");
    REQUIRE(cardAt<ContactUnglueNodesTable>(plain, 0).grids == std::vector<ContactRun>{{5, 9, 1}});

    oor::testing::requireModelFileRoundTrip(plain);

    // The body is a contact body; a single point is a grid point, the ends of a range are the ends of a THRU list.
    RecordingVisitor visitor;
    model.table<ContactUnglueNodesTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields ==
            std::vector<std::string>{"bodyId", "nodes", "nodes", "nodes", "nodes", "nodes", "nodes", "nodes"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{
                                   ReferenceTarget::ContactBody, ReferenceTarget::NodeInRange,
                                   ReferenceTarget::NodeInRange, ReferenceTarget::Node, ReferenceTarget::Node,
                                   ReferenceTarget::Node, ReferenceTarget::NodeInRange, ReferenceTarget::NodeInRange});
    // The grid 5 and the grid 7 to 9 do not exist; the first card finds everything.
    REQUIRE(unresolvedIn(model) == 4);
    REQUIRE(unresolvedOf("ContactElemSurface;200;1\nContactBody1;101;;ThreeDimensional;Deformable;"
                         "200\nContactUnglueNodes;10;102;5;Through;9;40") == 3); // and the element 1 of the BSURF

    ContactUnglueNodesCard edited = example;
    edited.grids = {ContactRun{5, 3, -1}};
    REQUIRE(ContactUnglueNodesFormat::check(edited)->field == std::string("nodes")); // a range goes up
    edited.grids = {ContactRun{5, 9, 2}};
    REQUIRE_FALSE(ContactUnglueNodesFormat::check(edited).has_value());
    model.table<ContactUnglueNodesTable>().store(0, edited);
    REQUIRE(cardAt<ContactUnglueNodesTable>(model, 0).grids == std::vector<ContactRun>{{5, 9, 2}});
    edited.id = -1;
    REQUIRE(ContactUnglueNodesFormat::check(edited)->field == std::string("id"));

    // The body follows the renumbering of the bodies, the points that of the nodes, ends of a range included.
    Renumbering::apply(model, RenumberSpace::ContactBody, Renumbering::offset(1000));
    REQUIRE((cardAt<ContactUnglueNodesTable>(model, 0).bodyId == 1101 &&
             cardAt<ContactUnglueNodesTable>(model, 2).bodyId == 1101));
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100000));
    REQUIRE(cardAt<ContactUnglueNodesTable>(model, 0).grids == std::vector<ContactRun>{{100005, 100009, 2}});
    REQUIRE(cardAt<ContactUnglueNodesTable>(model, 2).grids ==
            std::vector<ContactRun>{{100007, 100007, 1}, {100008, 100008, 1}, {100009, 100009, 1}});
    const std::size_t before = model.cardCount();
    model.remove(CardLocation{typeIndexOf<ContactUnglueNodesTable>(), 1});
    REQUIRE(model.cardCount() == before - 1);

    expectInvalid("ContactUnglueNodes;-1;101;5", "id");
    expectInvalid("ContactUnglueNodes", "id");
    expectInvalid("ContactUnglueNodes;1;;5", "bodyId"); // required
    expectInvalid("ContactUnglueNodes;1;0;5", "bodyId");
    expectInvalid("ContactUnglueNodes;1;101", "nodes"); // the grid points are required
    expectInvalid("ContactUnglueNodes;1;101;0", "nodes");
    expectInvalid("ContactUnglueNodes;1;101;x", "nodes");
    expectInvalid("ContactUnglueNodes;1;101;20;Through", "Through");
    expectInvalid("ContactUnglueNodes;1;101;Through;20", "Through");
    expectInvalid("ContactUnglueNodes;1;101;By;2", "By");
    expectInvalid("ContactUnglueNodes;1;101;400;Through;40", "nodes"); // the increment is positive: a range goes up
    expectInvalid("ContactUnglueNodes;1;101;40;Through;400;By;0", "By");
    expectInvalid("ContactUnglueNodes;1;101;40;Through;400;By;-2", "By");
    expectInvalid("ContactUnglueNodes;1;101;40;Through;400;By", "By");
}

// ---------------------------------------------------------------------------------------------------------------------
// TABSCTL and the CRITTID of an NLSTEP
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TABSCTL has criteria with a SET3, up to four ranges and the format's own compact layout",
          "[model][cards][tabsctl]")
{
    REQUIRE((StepControlCriteriaTable::INFO.category == CardCategory::LoadStepControl &&
             StepControlCriteriaTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(
        "StepControlCriteria;17;StrainIncrement;8400;0.05;12.5;0.25;40.;0.75;;;;StrainEnergyIncrement;1800;0.05;12.5;;;"
        ";;;;PlasticStrainIncrement;All;1.25\n"
        "StepControlCriteria;18;TemperatureIncrement;203;5.;100.;10.;200.;20.;300.;40.;0.;DisplacementIncrement;;1.\n"
        "StepControlCriteria;19;StrainIncrement;5;0.03;21.;;;;;;;StrainEnergyIncrement;6;0.04\n");
    const StepControlCriteriaCard example = cardAt<StepControlCriteriaTable>(model, 0);
    REQUIRE((example.id == 17 && example.criteria.size() == 3));
    REQUIRE((example.criteria[0].kind == LoadStepCriterion::Esi && example.criteria[0].setId == 8400));
    REQUIRE(example.criteria[0].ranges ==
            std::vector<StepControlCriteriaRange>{{0.05, 12.5}, {0.25, 40.0}, {0.75, std::nullopt}});
    REQUIRE((example.criteria[1].kind == LoadStepCriterion::Esri && example.criteria[1].setId == 1800));
    REQUIRE(example.criteria[1].ranges == std::vector<StepControlCriteriaRange>{{0.05, 12.5}});
    // ALL and a blank SET3 are the same: every element or grid point.
    REQUIRE((example.criteria[2].kind == LoadStepCriterion::Epsi && example.criteria[2].setId == 0));
    REQUIRE(example.criteria[2].ranges == std::vector<StepControlCriteriaRange>{{1.25, std::nullopt}});

    // The fourth range is on a line of its own; a criterion with three ranges or fewer takes one line.
    const StepControlCriteriaCard four = cardAt<StepControlCriteriaTable>(model, 1);
    REQUIRE((four.criteria.size() == 2 && four.criteria[0].kind == LoadStepCriterion::Gti));
    REQUIRE(four.criteria[0].ranges ==
            std::vector<StepControlCriteriaRange>{{5.0, 100.0}, {10.0, 200.0}, {20.0, 300.0}, {40.0, 0.0}});
    REQUIRE((four.criteria[1].kind == LoadStepCriterion::Gdi && four.criteria[1].setId == 0));
    // The layout of the format (a blank second line for each criterion) reads too, and a keyword is read in any case.
    const StepControlCriteriaCard spaced = cardAt<StepControlCriteriaTable>(model, 2);
    REQUIRE((spaced.criteria.size() == 2 && spaced.criteria[0].kind == LoadStepCriterion::Esi));
    REQUIRE((spaced.criteria[1].kind == LoadStepCriterion::Esri && spaced.criteria[1].setId == 6));
    requireRoundTrip(model);

    const std::string text = writeModelText(model);
    REQUIRE(contains(text,
                     "StepControlCriteria;17;StrainIncrement;8400;0.05;12.5;0.25;40.;0.75;;;;StrainEnergyIncrement;"
                     "1800;0.05;12.5;;;;;;;PlasticStrainIncrement;;1.25\n"));
    REQUIRE(contains(text,
                     "StepControlCriteria;18;TemperatureIncrement;203;5.;100.;10.;200.;20.;300.;40.;0.;"
                     "DisplacementIncrement;;1.\n"));
    oor::testing::requireModelFileRoundTrip(model, false); // include the default criteria explicitly

    // Every kind of criterion of the format.
    const Model kinds = readBulk(
        "StepControlCriteria;1;StrainIncrement;;0.01;;;;;;;;PlasticStrainIncrement;;0.02;;;;;;;;CreepStrainIncrement;;"
        "0.03;;;;;;;;NormalizedCreepStrainIncrement;;0.04;;;;;;;;StressIncrement;;0.05;;;;;"
        ";;;StrainEnergyIncrement;;0.06;;;;;;;;TemperatureIncrement;;0.07;;;;;;;;DisplacementIncrement;;0.08;;;;;;;;"
        "RotationIncrement;;0.09;;;;;;;;NormalizedStressIncrement;;0.1");
    const StepControlCriteriaCard all = cardAt<StepControlCriteriaTable>(kinds);
    REQUIRE(all.criteria.size() == 10);
    for (std::size_t index = 0; index < all.criteria.size(); ++index)
        REQUIRE(static_cast<std::size_t>(all.criteria[index].kind) == index);
    requireRoundTrip(kinds);

    // The SET3 of a criterion is a set; ALL names nothing.
    RecordingVisitor visitor;
    model.table<StepControlCriteriaTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"setId", "setId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Set, ReferenceTarget::Set});
    RecordingVisitor none;
    model.table<StepControlCriteriaTable>().visitReferences(1, none); // the set 203 only; the GDI is ALL
    REQUIRE(none.fields == std::vector<std::string>{"setId"});
    REQUIRE(unresolvedOf("StepControlCriteria;17;StrainIncrement;8400;0.05;;;;;;;;StrainEnergyIncrement;1800;0.05;;;;;;"
                         ";;PlasticStrainIncrement;All;1.25") == 2);
    REQUIRE(unresolvedOf("TypedListSet3;8400;Elements;1;2\nTypedListSet3;1800;Elements;3\nStepControlCriteria;17;"
                         "StrainIncrement;8400;0.05;"
                         ";;;;;;;StrainEnergyIncrement;1800;0.05;;;;;;;;PlasticStrainIncrement;All;1.25") ==
            3); // the elements 1, 2 and 3 of the two SET3 cards are not in the deck

    // A criterion has four ranges at most: the first criterion has three, so one more is allowed and two more are not.
    StepControlCriteriaCard edited = example;
    edited.criteria[0].ranges.push_back(StepControlCriteriaRange{0.8, 90.0});
    REQUIRE_FALSE(StepControlCriteriaFormat::check(edited).has_value());
    edited.criteria[0].ranges.push_back(StepControlCriteriaRange{0.9, std::nullopt});
    REQUIRE(StepControlCriteriaFormat::check(edited)->field == std::string("limit4"));
    edited = example;
    edited.criteria[1].ranges.clear();
    REQUIRE(StepControlCriteriaFormat::check(edited)->field == std::string("limit1"));
    edited = example;
    edited.criteria[1].setId = 22;
    REQUIRE_FALSE(StepControlCriteriaFormat::check(edited).has_value());
    model.table<StepControlCriteriaTable>().store(0, edited);
    REQUIRE(cardAt<StepControlCriteriaTable>(model, 0).criteria[1].setId == 22);
    edited.criteria.clear();
    REQUIRE(StepControlCriteriaFormat::check(edited)->field == std::string("criterion"));
}

TEST_CASE("a TABSCTL is named by the CRITTID of an NLSTEP and its SET3 by the criteria", "[model][cards][tabsctl]")
{
    Model model = readBulk(
        "TypedListSet3;8400;Elements;1;2\n"
        "TypedListSet3;1800;Node;1;2\n"
        "StepControlCriteria;17;StrainIncrement;8400;0.05;12.5;;;;;;;TemperatureIncrement;1800;1.;;;;;;;;"
        "PlasticStrainIncrement;All;1.25\n"
        "NonlinearStepControl;5;1.;;AdaptiveStepping;0.02;1.E-5;;5;;20;;4;2.E-4;17\n"
        "NonlinearStepControl;6;1.;;FixedStepping;10\n"
        "NonlinearStepControl;7;1.;;AdaptiveStepping;0.02\n");
    // The CRITTID is a field of the second line of ADAPT; an NLSTEP with no ADAPT, or none on it, names no table.
    REQUIRE(NonlinearStepControlFormat::criteriaTable(cardAt<NonlinearStepControlTable>(model, 0)) == 17);
    REQUIRE(NonlinearStepControlFormat::criteriaTable(cardAt<NonlinearStepControlTable>(model, 1)) == 0);
    REQUIRE(NonlinearStepControlFormat::criteriaTable(cardAt<NonlinearStepControlTable>(model, 2)) == 0);
    RecordingVisitor visitor;
    model.table<NonlinearStepControlTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"criterionTableId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::LoadStepControl});
    RecordingVisitor none;
    model.table<NonlinearStepControlTable>().visitReferences(1, none);
    model.table<NonlinearStepControlTable>().visitReferences(2, none);
    REQUIRE(none.fields.empty());
    REQUIRE(unresolvedIn(model) == 4); // the elements the SET3 cards list are not in the deck
    // A CRITTID that no TABSCTL has is unresolved.
    REQUIRE(unresolvedOf("NonlinearStepControl;5;1.;;AdaptiveStepping;0.02;1.E-5;;5;;20;;4;2.E-4;99") == 1);

    // The TABSCTL and the CRITTID follow one another through the renumbering of the space.
    Renumbering::apply(model, RenumberSpace::LoadStepControl, Renumbering::offset(100));
    REQUIRE(cardAt<StepControlCriteriaTable>(model).id == 117);
    REQUIRE(NonlinearStepControlFormat::criteriaTable(cardAt<NonlinearStepControlTable>(model, 0)) == 117);
    REQUIRE(unresolvedIn(model) == 4); // the SET3 cards list elements and grid points that are not in the deck
    // The sets the criteria name follow the renumbering of the sets, ALL stays.
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(1000));
    const StepControlCriteriaCard card = cardAt<StepControlCriteriaTable>(model);
    REQUIRE((card.criteria[0].setId == 9400 && card.criteria[1].setId == 2800 && card.criteria[2].setId == 0));
    REQUIRE(unresolvedIn(model) == 4); // the SET3 cards list elements and grid points that are not in the deck
    requireRoundTrip(model);

    const std::size_t before = model.cardCount();
    model.remove(*model.locateInType(typeIndexOf<StepControlCriteriaTable>(), 117));
    REQUIRE(model.cardCount() == before - 1);
    REQUIRE(unresolvedIn(model) == 5); // the NLSTEP names a table that is gone, and the four of the SET3 cards
    REQUIRE_THROWS_AS(
        readBulk("StepControlCriteria;1;StrainIncrement;;0.03\nStepControlCriteria;1;StrainIncrement;;0.03"),
        base::DeckError);
}

TEST_CASE("a TABSCTL refuses the values that its format refuses", "[model][cards][tabsctl]")
{
    expectInvalid("StepControlCriteria;0;StrainIncrement;;0.03", "id");
    expectInvalid("StepControlCriteria;-2;StrainIncrement;;0.03", "id");
    expectInvalid("StepControlCriteria;1", "criterion"); // a criterion is needed
    expectInvalid("StepControlCriteria;1;StrainIncrement", "limit1");
    expectInvalid("StepControlCriteria;1;StrainIncrement;x;0.03", "setId");
    expectInvalid("StepControlCriteria;1;StrainIncrement;-4;0.03", "setId");
    expectInvalid("StepControlCriteria;1;StrainIncrement;;x", "limit1");
    expectInvalid("StepControlCriteria;1;StrainIncrement;;0.03;x", "rangeEnd1");
    expectInvalid("StepControlCriteria;1;StrainIncrement;;;5.", "limit1"); // an MTk needs its YTk
    expectInvalid("StepControlCriteria;1;StrainIncrement;;0.03;5.;;7.", "limit2");
    expectInvalid("StepControlCriteria;1;StrainIncrement;;0.03;;;;0.04", "limit3"); // the ranges follow one another
    expectInvalid("StepControlCriteria;1;StrainIncrement;;0.03;5.;0.04;6.;0.05;7.;0.06;x", "rangeEnd4");
}

// ---------------------------------------------------------------------------------------------------------------------
// DEACTEL
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a DEACTEL names a SET3 of elements and is selected by its number", "[model][cards][deactel]")
{
    REQUIRE((DeactivatedElemsTable::INFO.category == CardCategory::DeactivatedElementSet &&
             DeactivatedElemsTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(
        "TypedListSet3;200;Elements;1;2\nTypedListSet3;201;Elements;3\nDeactivatedElems;2;200\nDeactivatedElems;3;"
        "201\n");
    const DeactivatedElemsCard card = cardAt<DeactivatedElemsTable>(model, 0);
    REQUIRE((card.id == 2 && card.elemSetId == 200));
    REQUIRE(cardAt<DeactivatedElemsTable>(model, 1).elemSetId == 201);
    requireRoundTrip(model);

    RecordingVisitor visitor;
    model.table<DeactivatedElemsTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"elemSetId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Set});
    REQUIRE(unresolvedIn(model) == 3); // the elements 1, 2 and 3 of the SET3 cards are not in the deck
    REQUIRE(unresolvedOf("DeactivatedElems;2;200") == 1);

    DeactivatedElemsCard edited = card;
    edited.elemSetId = 201;
    REQUIRE_FALSE(DeactivatedElemsFormat::check(edited).has_value());
    model.table<DeactivatedElemsTable>().store(0, edited);
    REQUIRE(cardAt<DeactivatedElemsTable>(model, 0).elemSetId == 201);
    edited.elemSetId = 0;
    REQUIRE(DeactivatedElemsFormat::check(edited)->field == std::string("elemSetId"));

    // The number is the space of the Case Control command; the SET3 follows the renumbering of the sets.
    Renumbering::apply(model, RenumberSpace::DeactivatedElementSet, Renumbering::offset(10));
    REQUIRE((cardAt<DeactivatedElemsTable>(model, 0).id == 12 && cardAt<DeactivatedElemsTable>(model, 1).id == 13));
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(1000));
    REQUIRE((cardAt<DeactivatedElemsTable>(model, 0).elemSetId == 1201 &&
             cardAt<DeactivatedElemsTable>(model, 1).elemSetId == 1201));
    REQUIRE(unresolvedIn(model) == 3); // the elements of the SET3 cards are not in the deck

    const std::size_t before = model.cardCount();
    model.remove(*model.locateInType(typeIndexOf<DeactivatedElemsTable>(), 13));
    REQUIRE(model.cardCount() == before - 1);
    REQUIRE_THROWS_AS(readBulk("DeactivatedElems;2;200\nDeactivatedElems;2;201"), base::DeckError);

    expectInvalid("DeactivatedElems;0;200", "id");
    expectInvalid("DeactivatedElems;-2;200", "id");
    expectInvalid("DeactivatedElems;2", "elemSetId"); // required
    expectInvalid("DeactivatedElems;2;0", "elemSetId");
    expectInvalid("DeactivatedElems;2;x", "elemSetId");
}

// ---------------------------------------------------------------------------------------------------------------------
// IPSTRAIN
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an IPSTRAIN is a strain for a range of elements, integration points and layers", "[model][cards][initial]")
{
    REQUIRE((InitialPlasticStrainTable::INFO.category == CardCategory::Other &&
             InitialPlasticStrainTable::INFO.idSpace == base::IdSpace::None));
    Model model = readBulk(
        "Node;1\n"
        "Node;2\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "RodProp;1;1;1.\n"
        "Rod2;3101;1;1;2\n"
        "Rod2;3140;1;1;2\n"
        "InitialPlasticStrain;3101;3140;1;4;2;6;0.035\n"
        "InitialPlasticStrain;3101\n"
        "InitialPlasticStrain;3102;3110;2;3;;;-0.01\n"
        "InitialPlasticStrain;3103;3103;;;0;0\n");
    const InitialPlasticStrainCard example = cardAt<InitialPlasticStrainTable>(model, 0);
    REQUIRE((example.range.firstElemId == 3101 && example.range.lastElemId == 3140 && example.range.firstPoint == 1 &&
             example.range.lastPoint == 4 && example.range.firstLayer == 2 && example.range.lastLayer == 6 &&
             example.strain == 0.035));
    // The defaults: EID2 is EID1 (stored resolved), INT1 is 1, INTN is 4, the layers are blank and STRAIN is 0.0.
    const InitialPlasticStrainCard bare = cardAt<InitialPlasticStrainTable>(model, 1);
    REQUIRE((bare.range.firstElemId == 3101 && bare.range.lastElemId == 3101 && bare.range.firstPoint == 1 &&
             bare.range.lastPoint == 4 && bare.range.firstLayer == 0 && bare.range.lastLayer == 0 &&
             bare.strain == 0.0));
    const InitialPlasticStrainCard points = cardAt<InitialPlasticStrainTable>(model, 2);
    REQUIRE((points.range.firstPoint == 2 && points.range.lastPoint == 3 && points.strain == -0.01));
    REQUIRE((cardAt<InitialPlasticStrainTable>(model, 3).range.firstLayer == 0 &&
             cardAt<InitialPlasticStrainTable>(model, 3).range.lastLayer == 0));
    requireRoundTrip(model);

    // EID2 depends on EID1, so it is always written; the other defaults are blanks, unless they are asked for.
    REQUIRE(contains(writeModelText(readBulk("InitialPlasticStrain;3101")), "InitialPlasticStrain;3101;3101\n"));
    REQUIRE(contains(writeModelText(readBulk("InitialPlasticStrain;3101"), false),
                     "InitialPlasticStrain;3101;3101;1;4;0;0"));
    // A zero EID2 is a blank one.
    REQUIRE(cardAt<InitialPlasticStrainTable>(readBulk("InitialPlasticStrain;3101;0"), 0).range.lastElemId == 3101);

    // The ends of the range of elements are references that may name nothing (the range skips the elements it lacks).
    RecordingVisitor visitor;
    model.table<InitialPlasticStrainTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"firstElemId", "lastElemId"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::ElementInRange, ReferenceTarget::ElementInRange});
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(unresolvedOf("InitialPlasticStrain;3101;3140") == 0);

    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(1000));
    REQUIRE((cardAt<InitialPlasticStrainTable>(model, 0).range.firstElemId == 4101 &&
             cardAt<InitialPlasticStrainTable>(model, 0).range.lastElemId == 4140));
    REQUIRE((cardAt<InitialPlasticStrainTable>(model, 2).range.firstElemId == 4102 &&
             cardAt<InitialPlasticStrainTable>(model, 2).range.lastElemId == 4110));
    REQUIRE(cardAt<InitialPlasticStrainTable>(model, 1).range.lastElemId ==
            4101); // the resolved EID2 moves with its element

    InitialPlasticStrainCard edited = cardAt<InitialPlasticStrainTable>(model, 0);
    edited.range.lastElemId = edited.range.firstElemId - 1;
    REQUIRE(InitialPlasticStrainFormat::check(edited)->field == std::string("lastElemId"));
    edited = cardAt<InitialPlasticStrainTable>(model, 0);
    edited.strain = 0.05;
    REQUIRE_FALSE(InitialPlasticStrainFormat::check(edited).has_value());
    model.table<InitialPlasticStrainTable>().store(0, edited);
    REQUIRE(cardAt<InitialPlasticStrainTable>(model, 0).strain == 0.05);

    // A card has no number: it is deleted by its place.
    const std::size_t before = model.cardCount();
    model.remove(CardLocation{typeIndexOf<InitialPlasticStrainTable>(), 1});
    REQUIRE(model.cardCount() == before - 1);

    expectInvalid("InitialPlasticStrain", "firstElemId");
    expectInvalid("InitialPlasticStrain;0", "firstElemId");
    expectInvalid("InitialPlasticStrain;-5", "firstElemId");
    expectInvalid("InitialPlasticStrain;5.5", "firstElemId");
    expectInvalid("InitialPlasticStrain;5;3", "lastElemId"); // the last element is not below the first
    expectInvalid("InitialPlasticStrain;5;6;0", "firstPoint");
    expectInvalid("InitialPlasticStrain;5;6;-1", "firstPoint");
    expectInvalid("InitialPlasticStrain;5;6;3;2", "lastPoint");
    expectInvalid("InitialPlasticStrain;5;6;;;-1", "firstLayer");
    expectInvalid("InitialPlasticStrain;5;6;;;;-2", "lastLayer");
    expectInvalid("InitialPlasticStrain;5;6;;;4;2", "lastLayer");
    expectInvalid("InitialPlasticStrain;5;6;;;;;x", "strain");
}

// ---------------------------------------------------------------------------------------------------------------------
// ISTRESS
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("an ISTRESS is a stress of up to seven components for a range, in the element or the basic system",
          "[model][cards][initial]")
{
    REQUIRE((InitialStressTable::INFO.category == CardCategory::Other &&
             InitialStressTable::INFO.idSpace == base::IdSpace::None));
    Model model = readBulk(
        "Node;1\n"
        "Node;2\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "RodProp;1;1;1.\n"
        "Rod2;3101;1;1;2\n"
        "Rod2;3140;1;1;2\n"
        "InitialStress;3101;3140;1;4;2;6;4.2E+4;-1800.;0.;3150.;0.;950.\n"
        "InitialStress;3102\n"
        "InitialStress;3103;3104;;;;;100.;200.;300.;400.;500.;600.;700.;Coordinates;-1\n"
        "InitialStress;3105;;;;;;;;;;;;;Coordinates;0\n");
    const InitialStressCard example = cardAt<InitialStressTable>(model, 0);
    REQUIRE((example.range.firstElemId == 3101 && example.range.lastElemId == 3140 && example.range.firstLayer == 2 &&
             example.range.lastLayer == 6));
    REQUIRE(example.stress ==
            std::array<double, INITIAL_STRESS_COMPONENTS>{42000.0, -1800.0, 0.0, 3150.0, 0.0, 950.0, 0.0});
    REQUIRE_FALSE(example.frameId.has_value()); // the element system, for a CQUAD4 and a CTRIA3
    // The defaults: EID2 is EID1, the points are 1 to 4 and every component is 0.0.
    const InitialStressCard bare = cardAt<InitialStressTable>(model, 1);
    REQUIRE((bare.range.lastElemId == 3102 && bare.range.firstPoint == 1 && bare.range.lastPoint == 4 &&
             !bare.frameId.has_value()));
    REQUIRE(bare.stress == std::array<double, INITIAL_STRESS_COMPONENTS>{});
    // Seven components over two lines, and the COORD line on the third.
    const InitialStressCard coord = cardAt<InitialStressTable>(model, 2);
    REQUIRE(coord.stress ==
            std::array<double, INITIAL_STRESS_COMPONENTS>{100.0, 200.0, 300.0, 400.0, 500.0, 600.0, 700.0});
    REQUIRE(coord.frameId == -1);
    REQUIRE(cardAt<InitialStressTable>(model, 3).frameId == 0);
    requireRoundTrip(model);

    // The COORD line is written in its own line, after a blank one when the second line is empty.

    REQUIRE_FALSE(contains(writeModelText(readBulk("InitialStress;3102")), "Coordinates"));

    RecordingVisitor visitor;
    model.table<InitialStressTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"firstElemId", "lastElemId"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::ElementInRange, ReferenceTarget::ElementInRange});
    REQUIRE(unresolvedIn(model) == 0);
    Renumbering::apply(model, RenumberSpace::Element, Renumbering::offset(500));
    REQUIRE((cardAt<InitialStressTable>(model, 0).range.firstElemId == 3601 &&
             cardAt<InitialStressTable>(model, 0).range.lastElemId == 3640));
    REQUIRE(cardAt<InitialStressTable>(model, 2).range.lastElemId == 3604);
    // The CID is -1 or 0: it names no coordinate system card, so a renumbering of the systems leaves it alone.
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE((cardAt<InitialStressTable>(model, 2).frameId == -1 && cardAt<InitialStressTable>(model, 3).frameId == 0));

    InitialStressCard edited = cardAt<InitialStressTable>(model, 0);
    edited.stress[6] = 10.0;
    edited.frameId = -1;
    REQUIRE_FALSE(InitialStressFormat::check(edited).has_value());
    model.table<InitialStressTable>().store(0, edited);
    REQUIRE(
        (cardAt<InitialStressTable>(model, 0).stress[6] == 10.0 && cardAt<InitialStressTable>(model, 0).frameId == -1));
    edited.frameId = 1;
    REQUIRE(InitialStressFormat::check(edited)->field == std::string("frameId"));
    edited.frameId = -2;
    REQUIRE(InitialStressFormat::check(edited)->field == std::string("frameId"));

    const std::size_t before = model.cardCount();
    model.remove(CardLocation{typeIndexOf<InitialStressTable>(), 1});
    REQUIRE(model.cardCount() == before - 1);

    expectInvalid("InitialStress", "firstElemId");
    expectInvalid("InitialStress;0", "firstElemId");
    expectInvalid("InitialStress;5;3", "lastElemId");
    expectInvalid("InitialStress;5;6;0", "firstPoint");
    expectInvalid("InitialStress;5;6;3;2", "lastPoint");
    expectInvalid("InitialStress;5;6;;;4;2", "lastLayer");
    expectInvalid("InitialStress;5;;;;;;x", "stress1");
    expectInvalid("InitialStress;5;;;;;;1.;x", "stress2");
    expectInvalid("InitialStress;5;;;;;;;;x", "stress3");
    expectInvalid("InitialStress;5;;;;;;;;1.;2.;3.;4.;x", "stress7");
    expectInvalid("InitialStress;5;;;;;;;;;;;;;Coordinates;1", "frameId");
    expectInvalid("InitialStress;5;;;;;;;;;;;;;Coordinates;-2", "frameId");
    expectInvalid("InitialStress;5;;;;;;;;;;;;;Coordinates;x", "frameId");
    expectInvalid("InitialStress;5;;;;;;;;;;;;;NOCOORD;0", "coordinate");
    expectInvalid("InitialStress;5;;;;;;;;;;;;;;0", "coordinate"); // a CID needs the word COORD
}

// ---------------------------------------------------------------------------------------------------------------------
// TIRE
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a TIRE is the tire-model word and the names of the tires over as many lines as needed",
          "[model][cards][tire]")
{
    REQUIRE((TireSelectionTable::INFO.category == CardCategory::Other &&
             TireSelectionTable::INFO.idSpace == base::IdSpace::PerCardType));
    Model model = readBulk(
        "TireSelection;100;ExternalTireModel;TYRLF;TYRRF;TYRLB;TYRRB\n"
        "TireSelection;101;ExternalTireModel;A;B;C;D;E;F;G;H;I\n"
        "TireSelection;102;ExternalTireModel;ONLY\n");
    const TireSelectionCard example = cardAt<TireSelectionTable>(model, 0);
    REQUIRE((example.id == 100 && example.names == std::vector<std::string>{"TYRLF", "TYRRF", "TYRLB", "TYRRB"}));
    // Seven names fill the first line after the word, and the next line goes on.
    REQUIRE(cardAt<TireSelectionTable>(model, 1).names ==
            std::vector<std::string>{"A", "B", "C", "D", "E", "F", "G", "H", "I"});
    // The word is read in any case; a name is kept as written.
    REQUIRE(cardAt<TireSelectionTable>(model, 2).names == std::vector<std::string>{"ONLY"});
    requireRoundTrip(model);

    // The names are those of DTI entries that no card types: nothing is looked up. The Case Control TIRE is text.
    RecordingVisitor visitor;
    model.table<TireSelectionTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());
    REQUIRE(unresolvedIn(model) == 0);

    TireSelectionCard edited = example;
    edited.names.push_back("TYRSP");
    REQUIRE_FALSE(TireSelectionFormat::check(edited).has_value());
    model.table<TireSelectionTable>().store(0, edited);
    REQUIRE(cardAt<TireSelectionTable>(model, 0).names.size() == 5);
    edited.names.push_back("TOOLONGNAME");
    REQUIRE_FALSE(TireSelectionFormat::check(edited).has_value());
    edited.names.clear();
    REQUIRE(TireSelectionFormat::check(edited)->field == std::string("tireNames"));

    const std::size_t before = model.cardCount();
    model.remove(*model.locateInType(typeIndexOf<TireSelectionTable>(), 101));
    REQUIRE(model.cardCount() == before - 1);
    REQUIRE_THROWS_AS(readBulk("TireSelection;100;ExternalTireModel;A\nTireSelection;100;ExternalTireModel;B"),
                      base::DeckError);

    expectInvalid("TireSelection;0;ExternalTireModel;A", "id");
    expectInvalid("TireSelection;-1;ExternalTireModel;A", "id");
    expectInvalid("TireSelection;1", "tireFileKeyword"); // the word is required
    expectInvalid("TireSelection;1;TIRES;A", "tireFileKeyword");
    expectInvalid("TireSelection;1;ExternalTireModel", "tireNames"); // and at least one tire
    REQUIRE_NOTHROW(readBulk("TireSelection;1;ExternalTireModel;TOOLONGNAME"));
}

// ---------------------------------------------------------------------------------------------------------------------
// CONCTL
// ---------------------------------------------------------------------------------------------------------------------

TEST_CASE("a CONCTL is a SET3 and the SWLDPRM parameters that override the global ones for its connectors",
          "[model][cards][conctl]")
{
    REQUIRE((ConnectorSearchControlTable::INFO.category == CardCategory::Other &&
             ConnectorSearchControlTable::INFO.idSpace == base::IdSpace::None));
    Model model = readBulk(
        "TypedListSet3;64;Elements;1;2\n"
        "TypedListSet3;87;Elements;3\n"
        "ConnectorSearchControl;64;ProjectionTolerance;0.15;GeneratedDataPrint;2;MaxEndPointMoves;6;"
        "MaxDiameterHalvings;4\n"
        "ConnectorSearchControl;87;MoveEndPointsToProjection;1\n"
        "ConnectorSearchControl;64;MYPARAM;5;MaxProjectionDistance;0.5\n");
    const ConnectorSearchControlCard example = cardAt<ConnectorSearchControlTable>(model, 0);
    REQUIRE((example.elementSetId == 64 && example.parameters.size() == 4));
    REQUIRE((example.parameters[0].name == "ProjectionTolerance" && !example.parameters[0].value.isInteger() &&
             example.parameters[0].value.asReal() == 0.15));
    REQUIRE((example.parameters[1].name == "GeneratedDataPrint" && example.parameters[1].value.isInteger() &&
             std::get<std::int32_t>(example.parameters[1].value.number) == 2));
    REQUIRE((example.parameters[2].name == "MaxEndPointMoves" &&
             example.parameters[3].name == "MaxDiameterHalvings")); // across the line
    REQUIRE((cardAt<ConnectorSearchControlTable>(model, 1).elementSetId == 87 &&
             cardAt<ConnectorSearchControlTable>(model, 1).parameters.size() == 1));
    // A name that SWLDPRM does not list is kept as written, in upper case; a listed one has its neutral
    // name.
    const ConnectorSearchControlCard other = cardAt<ConnectorSearchControlTable>(model, 2);
    REQUIRE((other.parameters[0].name == "MYPARAM" && other.parameters[1].name == "MaxProjectionDistance"));
    // Several cards may name the same SET3: the format reference allows many CONCTL and uses all of them.
    REQUIRE(cardAt<ConnectorSearchControlTable>(model, 0).elementSetId ==
            cardAt<ConnectorSearchControlTable>(model, 2).elementSetId);
    requireRoundTrip(model);

    RecordingVisitor visitor;
    model.table<ConnectorSearchControlTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"elementSetId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Set});
    REQUIRE(unresolvedIn(model) == 3); // the elements 1, 2 and 3 of the SET3 cards are not in the deck
    REQUIRE(unresolvedOf("ConnectorSearchControl;64;MoveEndPointsToProjection;1") == 1);

    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(1000));
    REQUIRE((cardAt<ConnectorSearchControlTable>(model, 0).elementSetId == 1064 &&
             cardAt<ConnectorSearchControlTable>(model, 1).elementSetId == 1087));
    REQUIRE(unresolvedIn(model) == 3); // the elements of the SET3 cards are not in the deck

    ConnectorSearchControlCard edited = cardAt<ConnectorSearchControlTable>(model, 1);
    edited.parameters.push_back(
        ConnectorSearchParamsParameter{"GeneratedDataPrint", ConnectorSearchParamsValue{std::int32_t{2}}});
    REQUIRE_FALSE(ConnectorSearchControlFormat::check(edited).has_value());
    model.table<ConnectorSearchControlTable>().store(1, edited);
    REQUIRE(cardAt<ConnectorSearchControlTable>(model, 1).parameters.size() == 2);
    edited.parameters.push_back(
        ConnectorSearchParamsParameter{"GeneratedDataPrint", ConnectorSearchParamsValue{std::int32_t{7}}});
    REQUIRE(ConnectorSearchControlFormat::check(edited)->field == std::string("parameterValue"));
    edited.parameters.clear();
    REQUIRE(ConnectorSearchControlFormat::check(edited)->field == std::string("parameterName1"));
    edited.elementSetId = 0;
    REQUIRE(ConnectorSearchControlFormat::check(edited)->field == std::string("elementSetId"));

    const std::size_t before = model.cardCount();
    model.remove(CardLocation{typeIndexOf<ConnectorSearchControlTable>(), 2});
    REQUIRE(model.cardCount() == before - 1);

    expectInvalid("ConnectorSearchControl;0;GeneratedDataPrint;1", "elementSetId");
    expectInvalid("ConnectorSearchControl;-5;GeneratedDataPrint;1", "elementSetId");
    expectInvalid("ConnectorSearchControl;64", "parameterName1"); // a parameter is needed
    expectInvalid("ConnectorSearchControl;64;GeneratedDataPrint", "parameterValue1");
    expectInvalid("ConnectorSearchControl;64;;1", "parameterName");
    expectInvalid("ConnectorSearchControl;64;GeneratedDataPrint;x", "parameterValue1");
    expectInvalid("ConnectorSearchControl;64;GeneratedDataPrint;1;MaxEndPointMoves", "parameterValue2");
    expectInvalid("ConnectorSearchControl;64;3;1", "parameterName"); // a name starts with a letter
    oor::testing::requireModelFileRoundTrip(readBulk("ConnectorSearchControl;64;TOOLONGNAME;1"));
    expectInvalid("ConnectorSearchControl;64;GeneratedDataPrint;7",
                  "parameterValue"); // 0, 1, 2, 11, 12, 100, 101, 111, 200, 201 or 211
    expectInvalid("ConnectorSearchControl;64;GeneratedDataPrint;1.5", "parameterValue"); // an integer parameter
    expectInvalid("ConnectorSearchControl;64;MoveEndPointsToProjection;2", "parameterValue");
    expectInvalid("ConnectorSearchControl;64;MaxProjectionDistance;-1.", "parameterValue");
}

// ---------------------------------------------------------------------------------------------------------------------
// GMOVR3
// ---------------------------------------------------------------------------------------------------------------------
