// Checks of the cards of the control surfaces and the trim: AESURF, AESURFS, AESTAT and AEPARM (the trim
// variables, found by label), AELINK (the ID or the word ALWAYS), the components AECOMP and AECOMPL (found by name),
// TRIM2 (values that are numbers or words), UXVEC, CSSCHD and AELISTC. Each card is tested on the fields of its format
// reference layout, its defaults, the references it lists and the renumbering of its ID space, the values it refuses
// and the round trip through the three formats.

#include "oor/model/cards/aero/aeroComponent/AeroComponentTable.hpp"
#include "oor/model/cards/aero/aeroComponentUnion/AeroComponentUnionTable.hpp"
#include "oor/model/cards/aero/aeroControlSchedule/AeroControlScheduleTable.hpp"
#include "oor/model/cards/aero/aeroControlSurface/AeroControlSurfaceTable.hpp"
#include "oor/model/cards/aero/aeroControlSurfaceNodes/AeroControlSurfaceNodesTable.hpp"
#include "oor/model/cards/aero/aeroControlVector/AeroControlVectorTable.hpp"
#include "oor/model/cards/aero/aeroController/AeroControllerTable.hpp"
#include "oor/model/cards/aero/aeroLabelList/AeroLabelListTable.hpp"
#include "oor/model/cards/aero/aeroRigidTrimVariable/AeroRigidTrimVariableTable.hpp"
#include "oor/model/cards/aero/aeroTrim2/AeroTrim2Table.hpp"
#include "oor/model/cards/aero/aeroTrimLink/AeroTrimLinkTable.hpp"
#include "oor/model/control/case_control/CaseControl.hpp"
#include "support/AeroTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <variant>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The number a command of the global Case Control section holds.
std::int64_t globalCommand(const Model& model, const char* name)
{
    const CaseEntry* entry = CaseControl::ownCommand(model.control().caseControl.global(), name);
    REQUIRE(entry != nullptr);
    return entry->value.integer;
}

} // namespace

TEST_CASE("an AESTAT and an AEPARM name a trim variable by label and share an ID the solver ignores",
          "[model][cards][aero]")
{
    const Model model = readBulk(
        "AeroRigidTrimVariable;4001;AngleOfAttack\nAeroRigidTrimVariable;4001;SideSlip\nAeroController;4101;FANPWR;"
        "KN\nAeroController;4102;YAWCTL");
    const AeroRigidTrimVariableCard angle = cardAt<AeroRigidTrimVariableTable>(model, 0);
    REQUIRE((angle.id == 4001 && angle.label == "AngleOfAttack"));
    REQUIRE(cardAt<AeroRigidTrimVariableTable>(model, 1).label == "SideSlip"); // the same ID is no duplicate
    const AeroControllerCard power = cardAt<AeroControllerTable>(model, 0);
    REQUIRE((power.id == 4101 && power.label == "FANPWR" && power.units == "KN"));
    REQUIRE(cardAt<AeroControllerTable>(model, 1).units.empty());
    requireRoundTrip(model);

    expectInvalid("AeroRigidTrimVariable;0;AngleOfAttack", "id");
    expectInvalid("AeroRigidTrimVariable;4001", "label");
    requireLiteralRoundTrip("AeroRigidTrimVariable;4001;ANGLEA123", "ANGLEA123");
    expectInvalid("AeroController;0;FANPWR", "id");
    expectInvalid("AeroController;4101", "label");
    requireLiteralRoundTrip("AeroController;4101;FANPWR;KN12345678", "KN12345678");
}

TEST_CASE("an AESURF has its fields in the positions of the format reference and keeps its limits resolved",
          "[model][cards][aero]")
{
    const Model model = readBulk(
        "AeroControlSurface;7001;ELEV;7300;7400;7301;7401;;;12.5;240.;;;-2.5E+4;1.5E+4;7501;7502\n"
        "AeroControlSurface;7002;AILERON;7310;7410\n"
        "AeroControlSurface;7003;rudder;7320;7420;0;;1.;;0.25;0.75;-1.25;1.25\n"
        "AeroControlSurface;6002;FLAP;101;201;102;202;0.8;NoLinearDownwash;2.5;3.5;-0.5;0.6;-100.;200.");
    const AeroControlSurfaceCard sampleCard = cardAt<AeroControlSurfaceTable>(model, 0);
    REQUIRE((sampleCard.id == 7001 && sampleCard.label == "ELEV" && sampleCard.frameId1 == 7300 &&
             sampleCard.boxListId1 == 7400));
    REQUIRE((sampleCard.frameId2 == 7301 && sampleCard.boxListId2 == 7401 && sampleCard.effectiveness == 1.0 &&
             sampleCard.linearDownwash == AeroControlSurfaceDownwash::Computed));
    REQUIRE((sampleCard.referenceChord == 12.5 && sampleCard.referenceArea == 240.0));
    // A blank PLLIM or PULIM is the quarter turn: the card holds the number, and writes a blank for it.
    REQUIRE((sampleCard.deflectionLowerLimit == AeroControlSurfaceCard::DEFAULT_DEFLECTION_LOWER_LIMIT &&
             sampleCard.deflectionUpperLimit == AeroControlSurfaceCard::DEFAULT_DEFLECTION_UPPER_LIMIT));
    REQUIRE((sampleCard.hingeMomentLowerLimit == -2.5e4 && sampleCard.hingeMomentUpperLimit == 1.5e4 &&
             sampleCard.lowerLimitTableId == 7501 && sampleCard.upperLimitTableId == 7502));

    const AeroControlSurfaceCard minimal = cardAt<AeroControlSurfaceTable>(model, 1);
    REQUIRE((minimal.frameId2 == 0 && minimal.boxListId2 == 0 && minimal.referenceChord == 1.0 &&
             minimal.referenceArea == 1.0));
    REQUIRE((!minimal.hingeMomentLowerLimit && !minimal.hingeMomentUpperLimit && minimal.lowerLimitTableId == 0 &&
             minimal.upperLimitTableId == 0));

    // A CID2 of 0 with no ALID2 is no second component, like a blank.
    const AeroControlSurfaceCard fromDeck = cardAt<AeroControlSurfaceTable>(model, 2);
    REQUIRE((fromDeck.label == "rudder" && fromDeck.frameId1 == 7320 && fromDeck.boxListId1 == 7420 &&
             fromDeck.frameId2 == 0));
    REQUIRE(fromDeck.boxListId2 == 0);
    REQUIRE((fromDeck.effectiveness == 1.0 && fromDeck.referenceChord == 0.25 && fromDeck.referenceArea == 0.75));
    REQUIRE((fromDeck.deflectionLowerLimit == -1.25 && fromDeck.deflectionUpperLimit == 1.25));

    const AeroControlSurfaceCard full = cardAt<AeroControlSurfaceTable>(model, 3);
    REQUIRE((full.effectiveness == 0.8 && full.linearDownwash == AeroControlSurfaceDownwash::Given &&
             full.deflectionLowerLimit == -0.5 && full.deflectionUpperLimit == 0.6));
    REQUIRE((full.hingeMomentLowerLimit == -100.0 && full.hingeMomentUpperLimit == 200.0 &&
             full.lowerLimitTableId == 0 && full.upperLimitTableId == 0));
    requireRoundTrip(model);

    // The continuation line is optional: it is written only when one of its fields is not blank.
}

TEST_CASE("an AESURF lists its coordinate systems, boxes and tables, which follow a renumbering",
          "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroControlSurface;7001;ELEV;7300;7400;7301;7401;;;12.5;240.;;;-2.5E+4;1.5E+4;7501;7502\n"
        "AeroControlSurface;7002;AILERON;7310;7410\n"
        "RectFrame2;7300;;0.;0.;0.;0.;0.;1.;1.;0.;0.\n"
        "AeroBoxList;7400;1001");
    RecordingVisitor visitor;
    model.table<AeroControlSurfaceTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"frameId1", "boxListId1", "frameId2", "boxListId2",
                                                       "lowerLimitTableId", "upperLimitTableId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Coordinate, ReferenceTarget::AeroList,
                                                            ReferenceTarget::Coordinate, ReferenceTarget::AeroList,
                                                            ReferenceTarget::DynamicTable,
                                                            ReferenceTarget::DynamicTable});
    visitor = RecordingVisitor{};
    model.table<AeroControlSurfaceTable>().visitReferences(1, visitor); // the fields left blank are not visited
    REQUIRE(visitor.fields == std::vector<std::string>{"frameId1", "boxListId1"});
    // The system 7300 and the list 7400 exist: the second component and the two tables of the first surface, and the
    // system and the list of the second one, do not.
    REQUIRE(unresolvedIn(model) == 6);

    Renumbering::apply(model, RenumberSpace::AeroList, Renumbering::offset(1000));
    REQUIRE((cardAt<AeroControlSurfaceTable>(model, 0).boxListId1 == 8400 &&
             cardAt<AeroControlSurfaceTable>(model, 1).boxListId1 == 8410));
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE((cardAt<AeroControlSurfaceTable>(model, 0).frameId1 == 7310 &&
             cardAt<AeroControlSurfaceTable>(model, 0).frameId2 == 7311));
    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(5));
    REQUIRE((cardAt<AeroControlSurfaceTable>(model, 0).lowerLimitTableId == 7506 &&
             cardAt<AeroControlSurfaceTable>(model, 0).upperLimitTableId == 7507));
    // The ID is the number of the surface: a CSSCHD names it, and it is unique among the AESURF cards.
    REQUIRE(model.locateInType(typeIndexOf<AeroControlSurfaceTable>(), 7001).has_value());
    REQUIRE_THROWS_AS(readBulk("AeroControlSurface;1;A;100;100\nAeroControlSurface;1;B;100;100"), base::DeckError);
}

TEST_CASE("an AESURF refuses what its rules and ranges forbid", "[model][cards][aero]")
{
    expectInvalid("AeroControlSurface;0;ELEV;100;100", "id");
    expectInvalid("AeroControlSurface;1;;100;100", "label");
    requireLiteralRoundTrip("AeroControlSurface;1;Long mixedCase.label;100;100", "Long mixedCase.label");
    expectInvalid("AeroControlSurface;1;ELEV;;100", "frameId1");
    expectInvalid("AeroControlSurface;1;ELEV;0;100", "frameId1"); // a coordinate system of the model, not the basic one
    expectInvalid("AeroControlSurface;1;ELEV;100", "boxListId1");
    expectInvalid("AeroControlSurface;1;ELEV;100;100;200", "boxListId2"); // a second component is a pair
    expectInvalid("AeroControlSurface;1;ELEV;100;100;;201", "frameId2");
    expectInvalid("AeroControlSurface;1;ELEV;100;100;0;201", "frameId2"); // CID2 written as 0 is no system
    expectInvalid("AeroControlSurface;1;ELEV;100;100;;;0.", "effectiveness");
    expectInvalid("AeroControlSurface;1;ELEV;100;100;;;;SIDEWAYS", "linearDownwash");
    expectInvalid("AeroControlSurface;1;ELEV;100;100;;;;;0.", "referenceChord");
    expectInvalid("AeroControlSurface;1;ELEV;100;100;;;;;;-1.", "referenceArea");
    // A limit is a constant or a table, side by side: a table is refused when its side has a constant.
    expectInvalid("AeroControlSurface;1;ELEV;100;100;;;;;;;-0.5;;;;5", "lowerLimitTableId");
    expectInvalid("AeroControlSurface;1;ELEV;100;100;;;;;;;;0.5;;;;6", "upperLimitTableId");
    expectInvalid("AeroControlSurface;1;ELEV;100;100;;;;;;;;;;;-5", "lowerLimitTableId");
    // The upper limit from a table with a lower constant is allowed.
    const Model mixed = readBulk("AeroControlSurface;1;ELEV;100;100;;;;;;;-0.5;;;;;6");
    REQUIRE((cardAt<AeroControlSurfaceTable>(mixed).deflectionLowerLimit == -0.5 &&
             cardAt<AeroControlSurfaceTable>(mixed).upperLimitTableId == 6));
}

TEST_CASE("an AESURFS lists the grids of a control surface in SET1 cards", "[model][cards][aero]")
{
    Model model = readBulk("AeroControlSurfaceNodes;6101;ELEV;6106;6107\nAeroControlSurfaceNodes;6101;RUDDER;6108");
    const AeroControlSurfaceNodesCard both = cardAt<AeroControlSurfaceNodesTable>(model, 0);
    REQUIRE((both.id == 6101 && both.label == "ELEV" && both.nodeSetId1 == 6106 && both.nodeSetId2 == 6107));
    const AeroControlSurfaceNodesCard one =
        cardAt<AeroControlSurfaceNodesTable>(model, 1); // the same ID: the solver ignores it
    REQUIRE((one.label == "RUDDER" && one.nodeSetId1 == 6108 && one.nodeSetId2 == 0));
    requireRoundTrip(model);
    // The layout has blank cells in fields 3 and 5.

    RecordingVisitor visitor;
    model.table<AeroControlSurfaceNodesTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"nodeSetId1", "nodeSetId2"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Set, ReferenceTarget::Set});
    visitor = RecordingVisitor{};
    model.table<AeroControlSurfaceNodesTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"nodeSetId1"});
    REQUIRE(unresolvedIn(model) == 3); // no SET1 is in this deck

    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(10));
    REQUIRE((cardAt<AeroControlSurfaceNodesTable>(model, 0).nodeSetId1 == 6116 &&
             cardAt<AeroControlSurfaceNodesTable>(model, 0).nodeSetId2 == 6117));

    expectInvalid("AeroControlSurfaceNodes;0;ELEV;6106", "id");
    expectInvalid("AeroControlSurfaceNodes;6101;;6106", "label");
    expectInvalid("AeroControlSurfaceNodes;6101;ELEV", "nodeSetId1");
    expectInvalid("AeroControlSurfaceNodes;6101;ELEV;0", "nodeSetId1");
    expectInvalid("AeroControlSurfaceNodes;6101;ELEV;6106;-1", "nodeSetId2");
}

TEST_CASE("an AELINK has its ID as a number or the word ALWAYS and keeps which one it was", "[model][cards][aero]")
{
    const Model model = readBulk(
        "AeroTrimLink;4201;FLAPIN;FLAPOUT;-1.5\n"
        "AeroTrimLink;Always;DROLL;AILL;1.;AILR;0.75;SPOIL;0.125;TABL;-1.;ANGLEB;2.5\n"
        "AeroTrimLink;0;AIL;ELEV;1.\n"
        "AeroTrimLink;4201;ailr_l;ailr_r;-1.");
    const AeroTrimLinkCard sampleCard = cardAt<AeroTrimLinkTable>(model, 0);
    REQUIRE((sampleCard.id == 4201 && !sampleCard.always && sampleCard.dependentLabel == "FLAPIN"));
    REQUIRE(sampleCard.terms == std::vector<LabeledValue>{{"FLAPOUT", -1.5}});
    const AeroTrimLinkCard every = cardAt<AeroTrimLinkTable>(model, 1);
    REQUIRE((every.id == 0 && every.always && every.dependentLabel == "DROLL" && every.terms.size() == 5));
    REQUIRE((every.terms[2] == LabeledValue{"SPOIL", 0.125} && every.terms[3] == LabeledValue{"TABL", -1.0}));
    REQUIRE(every.terms[4].label == "ANGLEB");
    const AeroTrimLinkCard zero = cardAt<AeroTrimLinkTable>(model, 2);
    REQUIRE((zero.id == 0 && !zero.always));                 // 0 and ALWAYS mean the same and are written as they were
    REQUIRE(cardAt<AeroTrimLinkTable>(model, 3).id == 4201); // several cards share an ID: one relation per card
    requireRoundTrip(model);

    // The labels are names and the ID names a trim by selection: nothing is visited.
    Model editable = model;
    RecordingVisitor visitor;
    editable.table<AeroTrimLinkTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields.empty());
    // A positive ID is no longer the word ALWAYS.
    editable.table<AeroTrimLinkTable>().setIdAtRow(1, 7);
    REQUIRE((cardAt<AeroTrimLinkTable>(editable, 1).id == 7 && !cardAt<AeroTrimLinkTable>(editable, 1).always));

    expectInvalid("AeroTrimLink;-1;AIL;ELEV;1.", "id");
    expectInvalid("AeroTrimLink;;AIL;ELEV;1.", "id");
    expectInvalid("AeroTrimLink;1.5;AIL;ELEV;1.", "id");
    expectInvalid("AeroTrimLink;10;;ELEV;1.", "dependentLabel");
    requireLiteralRoundTrip("AeroTrimLink;10;Long mixedCase.label;ELEV;1.", "Long mixedCase.label");
    expectInvalid("AeroTrimLink;10;AIL", "independentLabels"); // at least one independent variable
    expectInvalid("AeroTrimLink;10;AIL;ELEV", "coefficients");
    expectInvalid("AeroTrimLink;10;AIL;;1.", "independentLabels");
    expectInvalid("AeroTrimLink;10;AIL;ELEV;LEFT", "coefficients");
    requireLiteralRoundTrip("AeroTrimLink;10;AIL;Long mixedCase.label;1.", "Long mixedCase.label");
}

TEST_CASE("an AECOMP is found by its name and its IDs name the cards of its LISTTYPE", "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroComponent;FIN;AeroBoxList;4301;4302\n"
        "AeroComponent;WgCA;AeroPanels;100001;101001\n"
        "AeroComponent;R_FLAPS;IdListSet1;4310\n"
        "AeroComponent;CMPA;ComponentId;4501;4502\n"
        "AeroComponent;Wing2;AeroBoxList;4303");
    const AeroComponentCard fin = cardAt<AeroComponentTable>(model, 0);
    REQUIRE((fin.name == "FIN" && fin.listKind == AeroComponentListType::AeroBoxList));
    REQUIRE(fin.ids == std::vector<base::EntityId>{4301, 4302});
    REQUIRE(cardAt<AeroComponentTable>(model, 1).listKind == AeroComponentListType::AeroPanel);
    REQUIRE((cardAt<AeroComponentTable>(model, 2).listKind == AeroComponentListType::NodeSet &&
             cardAt<AeroComponentTable>(model, 2).name == "R_FLAPS"));
    REQUIRE(cardAt<AeroComponentTable>(model, 3).listKind == AeroComponentListType::ComponentId);
    REQUIRE((cardAt<AeroComponentTable>(model, 4).name == "Wing2" &&
             cardAt<AeroComponentTable>(model, 4).listKind == AeroComponentListType::AeroBoxList));
    requireRoundTrip(model);
    REQUIRE(model.findTable<AeroComponentTable>()->nameAt(4) == "Wing2"); // kept as written

    const Model longList = readBulk("AeroComponent;BIG;AeroBoxList;1;2;3;4;5;6;7;8;9");
    REQUIRE(cardAt<AeroComponentTable>(longList).ids == std::vector<base::EntityId>{1, 2, 3, 4, 5, 6, 7, 8, 9});
    requireRoundTrip(longList);

    // What the IDs refer to depends on the LISTTYPE; component numbers belong to no card.
    RecordingVisitor visitor;
    model.table<AeroComponentTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"listIds", "listIds"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroList, ReferenceTarget::AeroList});
    visitor = RecordingVisitor{};
    model.table<AeroComponentTable>().visitReferences(1, visitor);
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::AeroElement, ReferenceTarget::AeroElement});
    visitor = RecordingVisitor{};
    model.table<AeroComponentTable>().visitReferences(2, visitor);
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Set});
    visitor = RecordingVisitor{};
    model.table<AeroComponentTable>().visitReferences(3, visitor);
    REQUIRE(visitor.fields.empty());
    REQUIRE(!aeroComponentTarget(AeroComponentListType::ComponentId).has_value());
    REQUIRE(aeroComponentTarget(AeroComponentListType::NodeSet).value() == ReferenceTarget::Set);

    Model dangling = readBulk("AeroComponent;WgCA;AeroPanels;100001;101001\nAeroComponent;CMPA;ComponentId;4501;4502");
    REQUIRE(unresolvedIn(dangling) == 2);

    Renumbering::apply(model, RenumberSpace::AeroList, Renumbering::offset(100));
    REQUIRE(cardAt<AeroComponentTable>(model, 0).ids == std::vector<base::EntityId>{4401, 4402});
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(10));
    REQUIRE(cardAt<AeroComponentTable>(model, 2).ids == std::vector<base::EntityId>{4320});
    Renumbering::apply(model, RenumberSpace::AeroElement, Renumbering::offset(500));
    REQUIRE(cardAt<AeroComponentTable>(model, 1).ids == std::vector<base::EntityId>{100501, 101501});
    REQUIRE(cardAt<AeroComponentTable>(model, 3).ids == std::vector<base::EntityId>{4501, 4502}); // no card owns them

    expectInvalid("AeroComponent;;AeroBoxList;1", "name");
    requireLiteralRoundTrip("AeroComponent;Long mixedCase.label;AeroBoxList;1", "Long mixedCase.label");
    expectInvalid("AeroComponent;WING;;1", "listKind");
    expectInvalid("AeroComponent;WING;PANEL;1", "listKind");
    expectInvalid("AeroComponent;WING;AeroBoxList", "listIds");
    expectInvalid("AeroComponent;WING;AeroBoxList;0", "listIds");
    expectInvalid("AeroComponent;WING;AeroBoxList;1;;3", "listIds");     // no blank between two IDs
    expectInvalid("AeroComponent;WING;AeroBoxList;1;THRU;5", "listIds"); // THRU is not part of the layout
}

TEST_CASE("an AECOMPL joins components by name", "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroComponentUnion;TAILS;FIN;RUDDER;TABS\nAeroComponentUnion;ALLC;C1;C2;C3;C4;C5;C6;C7;C8;"
        "C9\nAeroComponentUnion;WgAR;WgAROb;WgARIb");
    const AeroComponentUnionCard tails = cardAt<AeroComponentUnionTable>(model, 0);
    REQUIRE((tails.name == "TAILS" && tails.labels == std::vector<std::string>{"FIN", "RUDDER", "TABS"}));
    const AeroComponentUnionCard all = cardAt<AeroComponentUnionTable>(model, 1);
    REQUIRE((all.labels.size() == 9 && all.labels[7] == "C8" && all.labels[8] == "C9"));
    REQUIRE((cardAt<AeroComponentUnionTable>(model, 2).name == "WgAR" &&
             cardAt<AeroComponentUnionTable>(model, 2).labels[0] == "WgAROb"));
    requireRoundTrip(model);

    // The labels are names of components, not IDs.
    RecordingVisitor visitor;
    model.table<AeroComponentUnionTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());
    REQUIRE(unresolvedIn(model) == 0);

    expectInvalid("AeroComponentUnion;;FIN", "name");
    expectInvalid("AeroComponentUnion;TAILS", "labels");
    expectInvalid("AeroComponentUnion;TAILS;FIN;;RUDDER", "labels"); // no blank between two labels
    requireLiteralRoundTrip("AeroComponentUnion;TAILS;Long mixedCase.label", "Long mixedCase.label");
}

TEST_CASE("a TRIM2 has AEQR in field 8 and its variables from field 9, with numbers or words", "[model][cards][aero]")
{
    Model model = readModelText(
        "[case]\n"
        "TrimSelection;4622\n"
        "[model]\n"
        "AeroTrim2;4621;0.75;450.\n"
        "AeroTrim2;4622;0.75;450.;;RigidAccelZ;1.5;AngleOfAttack;Free;ELEV;0.125\n"
        "AeroTrim2;4623;0.5;200.;0.;PitchRate;0.;RigidAccelZ;2.5;RigidAngularAccelY;Linked;ELEV;Scheduled;"
        "AngleOfAttack;Free\n",
        true);
    const AeroTrim2Card minimal = cardAt<AeroTrim2Table>(model, 0);
    REQUIRE((minimal.id == 4621 && minimal.machNumber == 0.75 && minimal.dynamicPressure == 450.0 &&
             minimal.equivalentAspectRatio == 1.0));
    REQUIRE(minimal.variables.empty());
    const AeroTrim2Card sampleCard = cardAt<AeroTrim2Table>(model, 1);
    REQUIRE(sampleCard.variables.size() == 3);
    REQUIRE((sampleCard.variables[0].label == "RigidAccelZ" && std::get<double>(sampleCard.variables[0].value) == 1.5));
    REQUIRE((sampleCard.variables[1].label == "AngleOfAttack" &&
             std::get<TrimState>(sampleCard.variables[1].value) == TrimState::Free));
    REQUIRE(std::get<double>(sampleCard.variables[2].value) == 0.125);
    const AeroTrim2Card words = cardAt<AeroTrim2Table>(model, 2);
    REQUIRE((words.equivalentAspectRatio == 0.0 && words.variables.size() == 5));
    REQUIRE(std::get<double>(words.variables[1].value) == 2.5);
    REQUIRE(std::get<TrimState>(words.variables[2].value) == TrimState::Linked); // read in any case
    REQUIRE(std::get<TrimState>(words.variables[3].value) == TrimState::Scheduled);
    REQUIRE(std::get<TrimState>(words.variables[4].value) == TrimState::Free);
    requireRoundTrip(model);

    // AEQR is field 8 of the first line and the pairs start the continuation line: not the layout of a TRIM.

    // The Case Control TRIM selects a TRIM2 as it selects a TRIM, and a renumbering follows it.
    REQUIRE(unresolvedIn(model) == 0);
    Model dangling = readModelText("[case]\nTrimSelection;9\n[model]\nAeroTrim2;4621;0.5;10.\n", true);
    REQUIRE(unresolvedIn(dangling) == 1);
    RecordingVisitor visitor;
    model.table<AeroTrim2Table>().visitReferences(1, visitor);
    REQUIRE(visitor.fields.empty()); // the variables are named by label
    Renumbering::apply(model, RenumberSpace::AeroTrimSet, Renumbering::offset(10));
    REQUIRE((cardAt<AeroTrim2Table>(model, 0).id == 4631 && cardAt<AeroTrim2Table>(model, 2).id == 4633));
    REQUIRE(globalCommand(model, "TrimSelection") == 4632);

    // MACH may be 0.0 (the format reference says not negative), but not 1.0.
    REQUIRE(cardAt<AeroTrim2Table>(readBulk("AeroTrim2;31;0.;100.")).machNumber == 0.0);

    expectInvalid("AeroTrim2;0;0.9;100.", "id");
    expectInvalid("AeroTrim2;1;-0.5;100.", "machNumber");
    expectInvalid("AeroTrim2;1;1.;100.", "machNumber");
    expectInvalid("AeroTrim2;1;0.9;0.", "dynamicPressure");
    expectInvalid("AeroTrim2;1;0.9;100.;2.", "equivalentAspectRatio");
    expectInvalid("AeroTrim2;1;0.9;100.;;RigidAccelZ", "value");
    expectInvalid("AeroTrim2;1;0.9;100.;;;1.", "label");
    expectInvalid("AeroTrim2;1;0.9;100.;;RigidAccelZ;LOCKED", "value");
    requireLiteralRoundTrip("AeroTrim2;1;0.9;100.;;Long mixedCase.label;1.", "Long mixedCase.label");
    expectInvalid("AeroTrim2;1;0.9;100.;;RigidAccelZ;1.;RigidAccelZ;Free",
                  "label"); // a variable is given once, whatever the case
}

TEST_CASE("a UXVEC has its pairs from field 9 and may repeat a label", "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroControlVector;4701;FANPWR;2500.;AngleOfAttack;0.125\n"
        "AeroControlVector;4702\n"
        "AeroControlVector;4703;A1;1.;A2;2.;A3;3.;A4;4.;Intercept;0.\n"
        "AeroControlVector;4704;THRUST;1.;THRUST;2.");
    REQUIRE(cardAt<AeroControlVectorTable>(model, 0).variables ==
            std::vector<LabeledValue>{{"FANPWR", 2.5e3}, {"AngleOfAttack", 0.125}});
    REQUIRE((cardAt<AeroControlVectorTable>(model, 1).id == 4702 &&
             cardAt<AeroControlVectorTable>(model, 1).variables.empty()));
    const AeroControlVectorCard many = cardAt<AeroControlVectorTable>(model, 2);
    REQUIRE((many.variables.size() == 5 && many.variables[4] == LabeledValue{"Intercept", 0.0}));
    REQUIRE(cardAt<AeroControlVectorTable>(model, 3).variables.size() == 2); // several values of one controller
    requireRoundTrip(model);

    RecordingVisitor visitor;
    model.table<AeroControlVectorTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields.empty());
    Renumbering::apply(model, RenumberSpace::ControlVector, Renumbering::offset(100));
    REQUIRE(
        (cardAt<AeroControlVectorTable>(model, 0).id == 4801 && cardAt<AeroControlVectorTable>(model, 3).id == 4804));

    expectInvalid("AeroControlVector;0", "id");
    expectInvalid("AeroControlVector;1001;THRUST", "values");
    expectInvalid("AeroControlVector;1001;;1.", "labels");
    expectInvalid("AeroControlVector;1001;THRUST;LEFT", "values");
    requireLiteralRoundTrip("AeroControlVector;1001;Long mixedCase.label;1.", "Long mixedCase.label");
}

TEST_CASE("a CSSCHD names a control surface and the AEFACT lists of its schedule", "[model][cards][aero]")
{
    Model model = readBulk(
        "AeroControlSchedule;4901;4950;4932;4934;4938\n"
        "AeroControlSchedule;4902;4950;;4934;4939\n"
        "AeroControlSchedule;4903;4951;4933;;4940\n"
        "AeroControlSurface;4950;ELEV;1;2\n"
        "AeroFactorList;4932;0.;0.1\n"
        "AeroFactorList;4934;0.3;0.7\n"
        "AeroFactorList;4938;0.;0.1;0.2;0.3");
    const AeroControlScheduleCard sampleCard = cardAt<AeroControlScheduleTable>(model, 0);
    REQUIRE((sampleCard.id == 4901 && sampleCard.controlSurfaceId == 4950 && sampleCard.angleOfAttackListId == 4932 &&
             sampleCard.machListId == 4934 && sampleCard.scheduleListId == 4938));
    const AeroControlScheduleCard mach = cardAt<AeroControlScheduleTable>(model, 1);
    REQUIRE((mach.angleOfAttackListId == 0 && mach.machListId == 4934 && mach.scheduleListId == 4939));
    const AeroControlScheduleCard alpha = cardAt<AeroControlScheduleTable>(model, 2);
    REQUIRE((alpha.controlSurfaceId == 4951 && alpha.angleOfAttackListId == 4933 && alpha.machListId == 0));
    requireRoundTrip(model);

    RecordingVisitor visitor;
    model.table<AeroControlScheduleTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields ==
            std::vector<std::string>{"controlSurfaceId", "angleOfAttackListId", "machListId", "scheduleListId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::AeroControlSurface,
                                                            ReferenceTarget::AeroFactor, ReferenceTarget::AeroFactor,
                                                            ReferenceTarget::AeroFactor});
    visitor = RecordingVisitor{};
    model.table<AeroControlScheduleTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"controlSurfaceId", "machListId", "scheduleListId"});
    // The AESURF 4950 has a system 1 and an AELIST 2 that are not in the deck; the schedule 4902 lacks its AEFACT 4939
    // and the schedule 4903 the surface 4951 and the AEFACT 4933 and 4940.
    REQUIRE(unresolvedIn(model) == 6);

    Renumbering::apply(model, RenumberSpace::AeroControlSurface, Renumbering::offset(10));
    REQUIRE((cardAt<AeroControlSurfaceTable>(model, 0).id == 4960 &&
             cardAt<AeroControlScheduleTable>(model, 0).controlSurfaceId == 4960));
    Renumbering::apply(model, RenumberSpace::AeroFactor, Renumbering::offset(100));
    REQUIRE((cardAt<AeroControlScheduleTable>(model, 0).angleOfAttackListId == 5032 &&
             cardAt<AeroControlScheduleTable>(model, 0).machListId == 5034));
    REQUIRE(cardAt<AeroControlScheduleTable>(model, 0).scheduleListId == 5038);
    Renumbering::apply(model, RenumberSpace::ControlSchedule, Renumbering::offset(100));
    REQUIRE((cardAt<AeroControlScheduleTable>(model, 0).id == 5001 &&
             cardAt<AeroControlScheduleTable>(model, 2).id == 5003));

    expectInvalid("AeroControlSchedule;0;4950;4932;4934;4938", "id");
    expectInvalid("AeroControlSchedule;4901;;4932;4934;4938", "controlSurfaceId");
    expectInvalid("AeroControlSchedule;4901;4950;4932;4934", "scheduleListId");
    expectInvalid("AeroControlSchedule;4901;4950;-1;4934;4938", "angleOfAttackListId");
    expectInvalid("AeroControlSchedule;4901;4950;;;4938", "angleOfAttackListId"); // not both blank
}

TEST_CASE("an AELISTC is a list of names with no blank inside", "[model][cards][aero]")
{
    const Model model =
        readBulk("AeroLabelList;4401;FLAPS;INBOARD1\nAeroLabelList;102;ONE\nAeroLabelList;103;A;B;C;D;E;F;G;H;I");
    REQUIRE(cardAt<AeroLabelListTable>(model, 0).names == std::vector<std::string>{"FLAPS", "INBOARD1"});
    REQUIRE(cardAt<AeroLabelListTable>(model, 1).names == std::vector<std::string>{"ONE"});
    REQUIRE(cardAt<AeroLabelListTable>(model, 2).names ==
            std::vector<std::string>{"A", "B", "C", "D", "E", "F", "G", "H", "I"});
    requireRoundTrip(model);
    REQUIRE_THROWS_AS(readBulk("AeroLabelList;101;A\nAeroLabelList;101;B"), base::DeckError); // the SID is unique

    expectInvalid("AeroLabelList;0;A", "id");
    expectInvalid("AeroLabelList;101", "names");
    expectInvalid("AeroLabelList;101;A;;B", "names");
    requireLiteralRoundTrip("AeroLabelList;101;Long mixedCase.label", "Long mixedCase.label");
}

TEST_CASE("the Case Control CSSCHD and AEUXREF select a schedule and a control vector", "[model][cards][aero]")
{
    const std::string cards =
        "AeroControlSchedule;4901;4950;4932;4934;4938\n"
        "AeroControlSurface;4950;ELEV;1;2\n"
        "AeroFactorList;4932;0.;0.1\n"
        "AeroFactorList;4934;0.3;0.7\n"
        "AeroFactorList;4938;0.;0.1;0.2;0.3\n"
        "AeroControlVector;4702;FANPWR;2500.\n";
    Model model = readModelText(
        "[case]\nControlScheduleSelection;4901\nAeroReferenceControlVector;4702\n[model]\n" + cards, true);
    // The commands name cards of the deck: only the system and the AELIST of the AESURF are missing.
    const std::size_t missing = unresolvedIn(model);
    REQUIRE(missing == 2);
    Model dangling =
        readModelText("[case]\nControlScheduleSelection;9\nAeroReferenceControlVector;7\n[model]\n" + cards, true);
    REQUIRE(unresolvedIn(dangling) == missing + 2);

    Renumbering::apply(model, RenumberSpace::ControlSchedule, Renumbering::offset(100));
    REQUIRE((cardAt<AeroControlScheduleTable>(model).id == 5001 &&
             globalCommand(model, "ControlScheduleSelection") == 5001));
    Renumbering::apply(model, RenumberSpace::ControlVector, Renumbering::offset(100));
    REQUIRE((cardAt<AeroControlVectorTable>(model).id == 4802 &&
             globalCommand(model, "AeroReferenceControlVector") == 4802));
}
