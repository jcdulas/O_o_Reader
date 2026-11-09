// Checks of the property extensions and the composite and section properties: the nonlinear
// extensions PBARN1, PBEMN1, PRODN1, PSHEARN and PSLDN1, the composite cards PCOMPFQ, PCOMPF, PCOMPLS and PLCOMP, and
// the arbitrary cross-sections PBMSECT and PBRSECT. Per card: the fields in their format reference positions, the
// keyword lines and their defaults, the references the card lists, the renumbering of the space it owns, the refused
// values and the round trip through a model file.

#include "oor/model/cards/elements/barArbitrarySectionProp/BarArbitrarySectionPropTable.hpp"
#include "oor/model/cards/elements/beamArbitrarySectionProp/BeamArbitrarySectionPropTable.hpp"
#include "oor/model/cards/elements/beamPropNonlinear/BeamPropNonlinearTable.hpp"
#include "oor/model/cards/elements/solidPropNonlinear/SolidPropNonlinearTable.hpp"
#include "oor/model/cards/properties/barPropNonlinear/BarPropNonlinearTable.hpp"
#include "oor/model/cards/properties/compPropFreqDamping/CompPropFreqDampingTable.hpp"
#include "oor/model/cards/properties/compPropIntegration/CompPropIntegrationTable.hpp"
#include "oor/model/cards/properties/compSolidPropNonlinear/CompSolidPropNonlinearTable.hpp"
#include "oor/model/cards/properties/planeCompProp/PlaneCompPropTable.hpp"
#include "oor/model/cards/properties/rodPropNonlinear/RodPropNonlinearTable.hpp"
#include "oor/model/cards/properties/shearPanelPropNonlinear/ShearPanelPropNonlinearTable.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// The lines of a written deck that continue a card (they start with the comma of their blank marker field).

bool contains(const std::string& text, const std::string& part)
{
    return text.find(part) != std::string::npos;
}

// The group of a keyword line from its four words.
BehaviorGroup group(ElementBehavior behavior, IntegrationScheme integration, ElementBehavior heatBehavior,
                    IntegrationScheme heatIntegration)
{
    return BehaviorGroup{behavior, integration, heatBehavior, heatIntegration};
}

} // namespace

TEST_CASE("a PBARN1 and a PBEMN1 extend the PBAR or PBEAM with the same PID: C2 line, defaults and references",
          "[model][cards]")
{
    Model model = readBulk(
        "BarPropNonlinear;36;64\n"
        "BarPropNonlinear;37;;Numerical;TwoNodeElems;Bar;LinearShear\n"
        "BarPropNonlinear;38;5;Smeared;TwoNodeElems;;LinearCubicClosed\n"
        "BeamPropNonlinear;34;86\n"
        "BeamPropNonlinear;35;;Numerical;TwoNodeElems;Beam;LinearCubicOpen\n"
        "BarProp;36;64;0.01;1.E-8;2.E-8;1.E-8\n"
        "BarProp;37;64;0.01;1.E-8;2.E-8;1.E-8\n"
        "BarProp;38;64;0.01;1.E-8;2.E-8;1.E-8\n"
        "BeamProp;34;86;3.4;2.6;4.81\n"
        "BeamProp;35;86;3.4;2.6;4.81\n"
        "IsoMat;64;2.1E+11;;0.3\n"
        "IsoMat;5;2.1E+11;;0.3\n"
        "IsoMat;86;2.1E+11;;0.3");
    const BarPropNonlinearCard plain = cardAt<BarPropNonlinearTable>(model, 0);
    REQUIRE((plain.id == 36 && plain.matId == 64 && plain.sectionIntegration == SectionIntegration::Smeared));
    // A missing C2 line gives the defaults of the format reference: BAR and linear/cubic.
    REQUIRE(plain.groups == defaultGroups(BAR_PROP_NONLINEAR_GROUPS));
    REQUIRE((plain.groups[0].behavior == ElementBehavior::Bar &&
             plain.groups[0].integration == IntegrationScheme::LinearCubic));
    const BarPropNonlinearCard numerical = cardAt<BarPropNonlinearTable>(model, 1);
    REQUIRE((numerical.matId == 0 && numerical.sectionIntegration == SectionIntegration::Numerical));
    REQUIRE((numerical.groups[0].behavior == ElementBehavior::Bar &&
             numerical.groups[0].integration == IntegrationScheme::LinearShear));
    const BarPropNonlinearCard closed = cardAt<BarPropNonlinearTable>(model, 2);
    REQUIRE((closed.matId == 5 && closed.sectionIntegration == SectionIntegration::Smeared));
    REQUIRE(closed.groups[0].integration == IntegrationScheme::LinearCubicClosed); // a blank BEH2 is the default
    const BeamPropNonlinearCard beam = cardAt<BeamPropNonlinearTable>(model, 0);
    REQUIRE((beam.id == 34 && beam.matId == 86 && beam.groups == defaultGroups(BEAM_PROP_NONLINEAR_GROUPS)));
    const BeamPropNonlinearCard open = cardAt<BeamPropNonlinearTable>(model, 1);
    REQUIRE((open.sectionIntegration == SectionIntegration::Numerical &&
             open.groups[0].behavior == ElementBehavior::Beam &&
             open.groups[0].integration == IntegrationScheme::LinearCubicOpen)); // LCO is the beam's, not the bar's
    REQUIRE(unresolvedIn(model) == 0);

    // The words are read in any case, and the C2 keyword too.
    const Model lower = readBulk("BarPropNonlinear;36;;;TwoNodeElems;Bar;LinearShear");
    REQUIRE(cardAt<BarPropNonlinearTable>(lower).groups[0].integration == IntegrationScheme::LinearShear);

    RecordingVisitor visitor;
    model.table<BarPropNonlinearTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"id", "matId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Material});
    visitor = RecordingVisitor{};
    model.table<BarPropNonlinearTable>().visitReferences(1, visitor); // a blank MID keeps the material of the PBAR
    REQUIRE(visitor.fields == std::vector<std::string>{"id"});
    visitor = RecordingVisitor{};
    model.table<BeamPropNonlinearTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"id", "matId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Material});

    // The defaults are not written; a word that is not the default is, on the C2 line, with the others left blank.
    const std::string text = writeModelText(model);
    REQUIRE(contains(text, "BarPropNonlinear;36;64\n"));
    REQUIRE(contains(text, "BarPropNonlinear;37;;Numerical;TwoNodeElems;;LinearShear\n"));
    REQUIRE(contains(text, "BarPropNonlinear;38;5;;TwoNodeElems;;LinearCubicClosed\n"));
    REQUIRE(contains(text, "BeamPropNonlinear;35;;Numerical;TwoNodeElems;;LinearCubicOpen\n"));
    // Written in full, the card has every word.
    REQUIRE(contains(writeModelText(model, false), "BarPropNonlinear;36;64;Smeared;TwoNodeElems;Bar;LinearCubic\n"));
    requireRoundTrip(model);

    // The ID is in the space of its own card type, and is the one of the PBAR: renumbering the properties follows.
    REQUIRE_THROWS_AS(readBulk("BarPropNonlinear;36;64\nBarPropNonlinear;36;74"), base::DeckError);
    REQUIRE_NOTHROW(readBulk("BarPropNonlinear;36\nBeamPropNonlinear;36\nBarProp;36;64;0.01;1.E-8;2.E-8;1.E-8"));
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<BarPropNonlinearTable>(model, 0).id == 1036 && cardAt<BarPropNonlinearTable>(model, 2).id == 1038));
    REQUIRE(
        (cardAt<BeamPropNonlinearTable>(model, 0).id == 1034 && cardAt<BeamPropNonlinearTable>(model, 1).id == 1035));
    REQUIRE(model.locateInType(typeIndexOf<BarPropNonlinearTable>(), 1036).has_value());
    REQUIRE_FALSE(model.locateInType(typeIndexOf<BarPropNonlinearTable>(), 36).has_value());
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(100));
    REQUIRE((cardAt<BarPropNonlinearTable>(model, 0).matId == 164 &&
             cardAt<BarPropNonlinearTable>(model, 1).matId == 0 &&
             cardAt<BarPropNonlinearTable>(model, 2).matId == 105));
    REQUIRE(cardAt<BeamPropNonlinearTable>(model, 0).matId == 186);
    REQUIRE(unresolvedIn(model) == 0);

    expectInvalid("BarPropNonlinear;0", "id");
    expectInvalid("BarPropNonlinear;36;-1", "matId");
    expectInvalid("BarPropNonlinear;36;;X", "sectionIntegration");
    expectInvalid("BarPropNonlinear;36;;;TwoNodeElems;Rod", "behavior2");
    expectInvalid("BarPropNonlinear;36;;;TwoNodeElems;;LinearCubicOpen",
                  "integration2"); // the open section is for beams
    expectInvalid("BarPropNonlinear;36;;;TwoNodeElems;Bar;LinearCubic;TwoNodeElems;Bar;LinearShear",
                  "TwoNodeElems"); // the field is named by its key word
    expectInvalid("BeamPropNonlinear;34;;;TwoNodeElems;Bar", "behavior2");
    expectInvalid("BeamPropNonlinear;34;;;TwoNodeElems;;LCX", "integration2");
    expectInvalid("BeamPropNonlinear;34;;Z", "sectionIntegration");
    expectInvalid("BeamPropNonlinear;0", "id");
}

TEST_CASE("a PRODN1 has structural and heat words and a PSHEARN is for the structure only", "[model][cards]")
{
    Model model = readBulk(
        "RodPropNonlinear;25;87\n"
        "RodPropNonlinear;26;;ImplicitHeat;TwoNodeElems;Rod;Linear;Rod;Linear\n"
        "RodPropNonlinear;27;7;ImplicitStructural\n"
        "ShearPanelPropNonlinear;41;16\n"
        "ShearPanelPropNonlinear;42;;ImplicitStructural;FourNodeElems;Membrane;Linear\n"
        "RodProp;25;87;2.5E-4;1.E-8;0.;0.1\n"
        "RodProp;26;87;2.5E-4;1.E-8;0.;0.1\n"
        "RodProp;27;87;2.5E-4;1.E-8;0.;0.1\n"
        "ShearPanelProp;41;16;0.5\n"
        "ShearPanelProp;42;16;0.5\n"
        "IsoMat;87;2.1E+11;;0.3\n"
        "IsoMat;7;2.1E+11;;0.3\n"
        "IsoMat;16;2.1E+11;;0.3");
    const RodPropNonlinearCard both = cardAt<RodPropNonlinearTable>(model, 0);
    REQUIRE((both.id == 25 && both.matId == 87 && both.analysisType == PropertyAnalysis::StructuralAndHeat));
    // The one line the format reference gives a rod has ROD and linear integration for the structure and for the heat.
    REQUIRE(both.groups == defaultGroups(ROD_PROP_NONLINEAR_GROUPS));
    REQUIRE(both.groups[0] ==
            group(ElementBehavior::Rod, IntegrationScheme::Linear, ElementBehavior::Rod, IntegrationScheme::Linear));
    REQUIRE(cardAt<RodPropNonlinearTable>(model, 1).analysisType == PropertyAnalysis::Heat);
    REQUIRE(cardAt<RodPropNonlinearTable>(model, 1).groups == defaultGroups(ROD_PROP_NONLINEAR_GROUPS));
    REQUIRE((cardAt<RodPropNonlinearTable>(model, 2).analysisType == PropertyAnalysis::Structural &&
             cardAt<RodPropNonlinearTable>(model, 2).matId == 7));
    const ShearPanelPropNonlinearCard shear = cardAt<ShearPanelPropNonlinearTable>(model, 0);
    REQUIRE((shear.id == 41 && shear.matId == 16 && shear.analysisType == PropertyAnalysis::Structural));
    REQUIRE(shear.groups[0] == group(ElementBehavior::Shear, IntegrationScheme::Linear, ElementBehavior::Bar,
                                     IntegrationScheme::Linear)); // the heat words of a card without them stay at rest
    REQUIRE(cardAt<ShearPanelPropNonlinearTable>(model, 1).groups == shear.groups);
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<RodPropNonlinearTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"id", "matId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Material});
    visitor = RecordingVisitor{};
    model.table<ShearPanelPropNonlinearTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"id"});

    const std::string text = writeModelText(model);
    REQUIRE(contains(text, "RodPropNonlinear;26;;ImplicitHeat\n"));
    REQUIRE(contains(text, "ShearPanelPropNonlinear;42\n"));
    REQUIRE_FALSE(contains(text, "TwoNodeElems"));
    REQUIRE_FALSE(contains(text, "FourNodeElems"));
    REQUIRE(contains(writeModelText(model, false),
                     "ShearPanelPropNonlinear;42;0;ImplicitStructural;FourNodeElems;Membrane;Linear\n"));
    requireRoundTrip(model);

    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(500));
    REQUIRE((cardAt<RodPropNonlinearTable>(model, 1).id == 526 &&
             cardAt<ShearPanelPropNonlinearTable>(model, 0).id == 541));
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(10));
    REQUIRE((cardAt<RodPropNonlinearTable>(model, 0).matId == 97 &&
             cardAt<ShearPanelPropNonlinearTable>(model, 0).matId == 26));
    REQUIRE(unresolvedIn(model) == 0);

    expectInvalid("RodPropNonlinear;0", "id");
    expectInvalid("RodPropNonlinear;25;-3", "matId");
    expectInvalid("RodPropNonlinear;25;;XX", "analysisType");
    expectInvalid("RodPropNonlinear;25;;;TwoNodeElems;Bar", "behavior2");
    expectInvalid("RodPropNonlinear;25;;;TwoNodeElems;;Quadratic", "integration2");
    expectInvalid("RodPropNonlinear;25;;;TwoNodeElems;;;Beam", "heatBehavior2");
    expectInvalid("RodPropNonlinear;25;;;TwoNodeElems;;;;Quadratic", "heatIntegration2");
    expectInvalid("ShearPanelPropNonlinear;25;;ImplicitHeat", "analysisType"); // a shear panel has no heat analysis
    expectInvalid("ShearPanelPropNonlinear;25;;ImplicitStructuralHeat", "analysisType");
    expectInvalid("ShearPanelPropNonlinear;25;;;FourNodeElems;Bar", "behavior4");
    expectInvalid("ShearPanelPropNonlinear;25;;;FourNodeElems;;LinearCubic", "integration4");
    expectInvalid("ShearPanelPropNonlinear;0", "id");
}

TEST_CASE("a PSLDN1 has a line for each keyword, in any order, and the words of the C8 line go together",
          "[model][cards]")
{
    Model model = readBulk(
        "SolidPropNonlinear;31;47;2\n"
        "SolidPropNonlinear;32;;;;EightNodeElems;SolidComposite;AssumedStrain;;;FourNodeElems;IncompressibleSolid;;;;"
        "TwentyNodeElems;;QuadraticReduced\n"
        "SolidPropNonlinear;33;;3;ImplicitHeat;TenNodeElems;;LinearReducedHourglass;;;FiveNodeElems;;"
        "LinearReducedHourglass\n"
        "SolidProp;31;47\n"
        "SolidProp;32;47\n"
        "SolidProp;33;47\n"
        "IsoMat;47;2.1E+11;;0.3");
    const SolidPropNonlinearCard plain = cardAt<SolidPropNonlinearTable>(model, 0);
    REQUIRE((plain.id == 31 && plain.matId == 47 && plain.direction == 2 &&
             plain.analysisType == PropertyAnalysis::StructuralAndHeat));
    REQUIRE(plain.groups == defaultGroups(SOLID_PROP_NONLINEAR_GROUPS)); // no line: every keyword has its defaults
    REQUIRE(plain.groups[SOLID_PROP_NONLINEAR_C8] == group(ElementBehavior::Solid, IntegrationScheme::Linear,
                                                           ElementBehavior::Solid, IntegrationScheme::Linear));
    REQUIRE(plain.groups[SOLID_PROP_NONLINEAR_C10] == group(ElementBehavior::Solid, IntegrationScheme::Quadratic,
                                                            ElementBehavior::Solid, IntegrationScheme::Quadratic));
    REQUIRE(plain.groups[SOLID_PROP_NONLINEAR_C5] == group(ElementBehavior::Solid, IntegrationScheme::Linear,
                                                           ElementBehavior::Solid, IntegrationScheme::Linear));

    // Lines in an order of their own: each one finds its place.
    const SolidPropNonlinearCard lines = cardAt<SolidPropNonlinearTable>(model, 1);
    REQUIRE((lines.id == 32 && lines.matId == 0 && lines.direction == 1));
    REQUIRE(lines.groups[SOLID_PROP_NONLINEAR_C8] == group(ElementBehavior::SolidComposite,
                                                           IntegrationScheme::AssumedStrain, ElementBehavior::Solid,
                                                           IntegrationScheme::Linear));
    REQUIRE(lines.groups[SOLID_PROP_NONLINEAR_C4] == group(ElementBehavior::IncompressibleSolid,
                                                           IntegrationScheme::Linear, ElementBehavior::Solid,
                                                           IntegrationScheme::Linear));
    REQUIRE(lines.groups[SOLID_PROP_NONLINEAR_C20] == group(ElementBehavior::Solid, IntegrationScheme::QuadraticReduced,
                                                            ElementBehavior::Solid, IntegrationScheme::Quadratic));
    REQUIRE(lines.groups[SOLID_PROP_NONLINEAR_C6] == plain.groups[SOLID_PROP_NONLINEAR_C6]);
    const SolidPropNonlinearCard heat = cardAt<SolidPropNonlinearTable>(model, 2);
    REQUIRE((heat.direction == 3 && heat.analysisType == PropertyAnalysis::Heat));
    REQUIRE(heat.groups[SOLID_PROP_NONLINEAR_C10].integration == IntegrationScheme::LinearReducedHourglass);
    REQUIRE(heat.groups[SOLID_PROP_NONLINEAR_C5].integration == IntegrationScheme::LinearReducedHourglass);
    REQUIRE(unresolvedIn(model) == 0);

    // Written in the order of the keywords, a word at its default left blank.

    requireRoundTrip(model);

    RecordingVisitor visitor;
    model.table<SolidPropNonlinearTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"id", "matId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::Material});
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE(
        (cardAt<SolidPropNonlinearTable>(model, 0).id == 1031 && cardAt<SolidPropNonlinearTable>(model, 2).id == 1033));
    REQUIRE(model.locateInType(typeIndexOf<SolidPropNonlinearTable>(), 1032).has_value());
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE_THROWS_AS(readBulk("SolidPropNonlinear;31\nSolidPropNonlinear;31;47"), base::DeckError);

    expectInvalid("SolidPropNonlinear;0", "id");
    expectInvalid("SolidPropNonlinear;31;-2", "matId");
    expectInvalid("SolidPropNonlinear;31;47;0", "direction");
    expectInvalid("SolidPropNonlinear;31;47;4", "direction");
    expectInvalid("SolidPropNonlinear;31;;;XX", "analysisType");
    expectInvalid("SolidPropNonlinear;31;;;;EightNodeElems;Solid;;;;EightNodeElems;Solid", "EightNodeElems");
    expectInvalid("SolidPropNonlinear;31;;;;EightNodeElems;IncompressibleSolid",
                  "behavior8"); // the incompressible solid is a tetrahedron
    expectInvalid("SolidPropNonlinear;31;;;;FourNodeElems;;Quadratic", "integration4");
    expectInvalid("SolidPropNonlinear;31;;;;SixNodeElems;;Quadratic", "integration6");
    expectInvalid("SolidPropNonlinear;31;;;;TenNodeElems;;Linear", "integration10");
    expectInvalid("SolidPropNonlinear;31;;;;ThirteenNodeElems;;Linear", "integration13");
    expectInvalid("SolidPropNonlinear;31;;;;TwentyNodeElems;;Linear", "integration20");
    expectInvalid("SolidPropNonlinear;31;;;;FourNodeElems;;;;Quadratic", "heatIntegration4");
    expectInvalid("SolidPropNonlinear;31;;;;EightNodeElems;SolidComposite;LinearReducedHourglass",
                  "integration8"); // the layered solid is L or ASTN
    expectInvalid("SolidPropNonlinear;31;;;;EightNodeElems;Solid;AssumedStrain",
                  "integration8"); // ASTN is the solid shell
    expectInvalid("SolidPropNonlinear;31;;2;;EightNodeElems;SolidComposite;AssumedStrain", "direction");

    // The same rules apply to the values of a card that is edited.
    SolidPropNonlinearCard card;
    card.id = 1;
    card.direction = 2;
    card.groups[SOLID_PROP_NONLINEAR_C8] = group(ElementBehavior::SolidComposite, IntegrationScheme::AssumedStrain,
                                                 ElementBehavior::Solid, IntegrationScheme::Linear);
    const std::optional<CardProblem> problem = SolidPropNonlinearFormat::check(card);
    REQUIRE(problem.has_value());
    REQUIRE(std::string(problem->field) == "direction");
    card.direction = 1;
    REQUIRE_FALSE(SolidPropNonlinearFormat::check(card).has_value());
}

TEST_CASE("a PCOMPFQ replaces the damping of a composite by a table, in the column of the GE of the PCOMP",
          "[model][cards]")
{
    Model model = readBulk(
        "CompPropFreqDamping;44;17\n"
        "CompPropFreqDamping;45\n"
        "CompPropGlobal;44;;;;;;;;1;1;0.1\n"
        "CompPropGlobal;45;;;;;;;;1;1;0.1\n"
        "IsoMat;1;2.1E+11;;0.3\n"
        "TableDynamic1;17;;;0.;1.;1.;1.;EndTable");
    REQUIRE((cardAt<CompPropFreqDampingTable>(model, 0).id == 44 &&
             cardAt<CompPropFreqDampingTable>(model, 0).structuralDamping == 17));
    REQUIRE((cardAt<CompPropFreqDampingTable>(model, 1).id == 45 &&
             cardAt<CompPropFreqDampingTable>(model, 1).structuralDamping == 0));
    REQUIRE(unresolvedIn(model) == 0);
    RecordingVisitor visitor;
    model.table<CompPropFreqDampingTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"id", "structuralDamping"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Property, ReferenceTarget::DynamicTable});
    visitor = RecordingVisitor{};
    model.table<CompPropFreqDampingTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"id"});
    requireRoundTrip(model);

    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(100));
    REQUIRE(cardAt<CompPropFreqDampingTable>(model, 0).structuralDamping == 117);
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<CompPropFreqDampingTable>(model, 0).id == 1044 &&
             cardAt<CompPropFreqDampingTable>(model, 1).id == 1045));
    REQUIRE(model.locateInType(typeIndexOf<CompPropFreqDampingTable>(), 1044).has_value());
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE_THROWS_AS(readBulk("CompPropFreqDamping;44\nCompPropFreqDamping;44;5"), base::DeckError);

    expectInvalid("CompPropFreqDamping;0", "id");
    expectInvalid("CompPropFreqDamping;44;-1", "structuralDamping");
}

TEST_CASE("a PCOMPF lists properties and ranges with THRU, TO and BY, or says ALL", "[model][cards]")
{
    std::string deck =
        "CompPropIntegration;2;100;Through;200;By;10\n"
        "CompPropIntegration;1;63;ToKeyword;66;;72;;;83;Through;91;By;4;;;;122\n"
        "CompPropIntegration;3;All\n"
        "CompPropIntegration;;5;10;12\n"
        "CompPropIntegration;2;50;Through;40\n"
        "IsoMat;1;2.1E+11;;0.3\n";
    for (const int pid : {5, 10, 12, 72, 122}) // the properties that stand alone
        deck += "CompPropGlobal;" + std::to_string(pid) + ";;;;;;;;1;1;0.1\n";
    Model model = readBulk(deck);
    REQUIRE(rowCount<CompPropIntegrationTable>(model) == 5);

    const CompPropIntegrationCard stepped = cardAt<CompPropIntegrationTable>(model, 0);
    REQUIRE((stepped.integration == 2 && !stepped.all));
    REQUIRE(stepped.runs == std::vector<CompPropIntegrationRun>{{100, 200, 10}});
    // Any mix of PIDs and ranges, blank fields for readability, TO for THRU; BY left out is 1.
    const CompPropIntegrationCard mixed = cardAt<CompPropIntegrationTable>(model, 1);
    REQUIRE(mixed.integration == 1);
    REQUIRE(mixed.runs == std::vector<CompPropIntegrationRun>{{63, 66, 1}, {72, 72, 1}, {83, 91, 4}, {122, 122, 1}});
    REQUIRE((cardAt<CompPropIntegrationTable>(model, 2).all &&
             cardAt<CompPropIntegrationTable>(model, 2).integration == 3));
    REQUIRE(cardAt<CompPropIntegrationTable>(model, 2).runs.empty());
    // A blank INT is 1.
    REQUIRE(cardAt<CompPropIntegrationTable>(model, 3).integration == 1);
    REQUIRE(cardAt<CompPropIntegrationTable>(model, 3).runs ==
            std::vector<CompPropIntegrationRun>{{5, 5, 1}, {10, 10, 1}, {12, 12, 1}});
    // A range may go down, and then BY left out is -1.
    REQUIRE(cardAt<CompPropIntegrationTable>(model, 4).runs == std::vector<CompPropIntegrationRun>{{50, 40, -1}});
    REQUIRE(unresolvedIn(model) == 0);

    // A PID that stands alone must exist; the ends of a range may name nothing, the properties between them are the
    // ones that exist.
    RecordingVisitor visitor;
    model.table<CompPropIntegrationTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>(6, "propId"));
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::PropertyInRange, ReferenceTarget::PropertyInRange,
                                         ReferenceTarget::Property, ReferenceTarget::PropertyInRange,
                                         ReferenceTarget::PropertyInRange, ReferenceTarget::Property});
    visitor = RecordingVisitor{};
    model.table<CompPropIntegrationTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields.empty()); // ALL names no property

    // INT is always written; a range stays on one line, and a BY that is the default is left out.

    requireRoundTrip(model);

    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE(cardAt<CompPropIntegrationTable>(model, 0).runs == std::vector<CompPropIntegrationRun>{{1100, 1200, 10}});
    REQUIRE(cardAt<CompPropIntegrationTable>(model, 1).runs ==
            std::vector<CompPropIntegrationRun>{{1063, 1066, 1}, {1072, 1072, 1}, {1083, 1091, 4}, {1122, 1122, 1}});
    REQUIRE(unresolvedIn(model) == 0);

    // A deck may have many of them: the card has no ID.
    REQUIRE_NOTHROW(readBulk("CompPropIntegration;1;5\nCompPropIntegration;2;5"));

    expectInvalid("CompPropIntegration;4;10", "integration");
    expectInvalid("CompPropIntegration;0;10", "integration");
    expectInvalid("CompPropIntegration;1", "propId");
    expectInvalid("CompPropIntegration;1;x", "propId");
    expectInvalid("CompPropIntegration;1;0", "propId");
    expectInvalid("CompPropIntegration;1;10000000", "propId");
    expectInvalid("CompPropIntegration;1;All;5", "propId");
    expectInvalid("CompPropIntegration;1;5;All", "propId");
    expectInvalid("CompPropIntegration;1;Through;5", "through");
    expectInvalid("CompPropIntegration;1;5;Through", "through");
    expectInvalid("CompPropIntegration;1;5;Through;9;Through;12", "through");
    expectInvalid("CompPropIntegration;1;5;By;2", "increment");
    expectInvalid("CompPropIntegration;1;5;Through;9;By;2;By;3", "increment");
    expectInvalid("CompPropIntegration;1;5;Through;9;By;0", "increment");
    expectInvalid("CompPropIntegration;1;5;Through;9;By;-1", "increment"); // the increment goes the wrong way
    expectInvalid("CompPropIntegration;1;9;Through;5;By;1", "increment");
}

TEST_CASE("a PCOMPLS has plies with global IDs, the C8 and C20 lines and a material coordinate system",
          "[model][cards]")
{
    Model model = readBulk(
        "CompSolidPropNonlinear;6601;1;;;;;3010;1420;0.25;17.5;;3020;1460;0.75;72.5\n"
        "CompSolidPropNonlinear;6602;-1;2;12.5;ImplicitStructural;TwentyNodeElems;;Quadratic;;;EightNodeElems;"
        "SolidComposite;AssumedStrain;;;;1;1420;0.5;;;2;1460;0.5;-45.\n"
        "CompSolidPropNonlinear;6603;;-1;;;;5;1420;1.\n"
        "IsoMat;1420;2.1E+11;;0.3\n"
        "IsoMat;1460;2.1E+11;;0.3\n"
        "RectFrame2;2");
    const CompSolidPropNonlinearCard sampleCard = cardAt<CompSolidPropNonlinearTable>(model, 0);
    REQUIRE((sampleCard.id == 6601 && sampleCard.direction == 1 && sampleCard.matFrameId == 0 &&
             !sampleCard.bondAllowable.has_value()));
    REQUIRE(sampleCard.analysisType == PropertyAnalysis::StructuralAndHeat);
    REQUIRE(sampleCard.groups == defaultGroups(COMP_SOLID_PROP_NONLINEAR_GROUPS));
    REQUIRE(sampleCard.groups[COMP_SOLID_PROP_NONLINEAR_C8] ==
            group(ElementBehavior::SolidComposite, IntegrationScheme::Linear, ElementBehavior::SolidComposite,
                  IntegrationScheme::Linear));
    REQUIRE(sampleCard.groups[COMP_SOLID_PROP_NONLINEAR_C20] ==
            group(ElementBehavior::SolidComposite, IntegrationScheme::Quadratic, ElementBehavior::SolidComposite,
                  IntegrationScheme::Quadratic));
    REQUIRE(sampleCard.plies ==
            std::vector<CompSolidPropNonlinearPly>{{3010, 1420, 0.25, 17.5}, {3020, 1460, 0.75, 72.5}});
    REQUIRE(sampleCard.totalThickness() == 0.25 + 0.75);

    const CompSolidPropNonlinearCard full = cardAt<CompSolidPropNonlinearTable>(model, 1);
    REQUIRE((full.direction == -1 && full.matFrameId == 2 && full.bondAllowable == 12.5 &&
             full.analysisType == PropertyAnalysis::Structural));
    REQUIRE(full.groups[COMP_SOLID_PROP_NONLINEAR_C8].integration == IntegrationScheme::AssumedStrain);
    REQUIRE(full.plies ==
            std::vector<CompSolidPropNonlinearPly>{{1, 1420, 0.5, 0.}, {2, 1460, 0.5, -45.}}); // THETA defaults to 0
    REQUIRE(cardAt<CompSolidPropNonlinearTable>(model, 2).matFrameId == -1);                   // the element system
    REQUIRE(unresolvedIn(model) == 0);

    // A material for each ply, a coordinate system only when it is a card (0 is basic and -1 the element system).
    RecordingVisitor visitor;
    model.table<CompSolidPropNonlinearTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId1", "matId1"});
    visitor = RecordingVisitor{};
    model.table<CompSolidPropNonlinearTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId1", "matId1", "matFrameId"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::Material,
                                                            ReferenceTarget::Coordinate});
    visitor = RecordingVisitor{};
    model.table<CompSolidPropNonlinearTable>().visitReferences(2, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId1"});

    // A line at its defaults is not written; the words of one that is not are, the others blank.

    requireRoundTrip(model);

    // A PCOMPLS is a property: it shares the number space with the PSOLID, the PLCOMP and the others.
    REQUIRE_THROWS_AS(readBulk("CompSolidPropNonlinear;6601;;;;;;1;1420;0.5\nSolidProp;6601;1420"), base::DeckError);
    REQUIRE_THROWS_AS(readBulk("CompSolidPropNonlinear;6601;;;;;;1;1420;0.5\nPlaneCompProp;6601;;;;;;1;1420;0.5"),
                      base::DeckError);
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<CompSolidPropNonlinearTable>(model, 0).id == 7601 &&
             cardAt<CompSolidPropNonlinearTable>(model, 2).id == 7603));
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(100));
    REQUIRE(cardAt<CompSolidPropNonlinearTable>(model, 1).plies[0].matId == 1520);
    REQUIRE(cardAt<CompSolidPropNonlinearTable>(model, 1).plies[1].matId == 1560);
    Renumbering::apply(model, RenumberSpace::Coordinate, Renumbering::offset(10));
    REQUIRE((cardAt<CompSolidPropNonlinearTable>(model, 1).matFrameId == 12 &&
             cardAt<CompSolidPropNonlinearTable>(model, 2).matFrameId == -1));
    REQUIRE(unresolvedIn(model) == 0);

    expectInvalid("CompSolidPropNonlinear;0;;;;;;1;1;0.5", "id");
    expectInvalid("CompSolidPropNonlinear;6601;0;;;;;1;1;0.5", "direction");
    expectInvalid("CompSolidPropNonlinear;6601;4;;;;;1;1;0.5", "direction");
    expectInvalid("CompSolidPropNonlinear;6601;;-2;;;;1;1;0.5", "matFrameId");
    expectInvalid("CompSolidPropNonlinear;6601;;;-1.;;;1;1;0.5", "bondAllowable");
    expectInvalid("CompSolidPropNonlinear;6601;;;;XX;;1;1;0.5", "analysisType");
    expectInvalid("CompSolidPropNonlinear;6601;2;;;;EightNodeElems;SolidComposite;AssumedStrain;;;;1;1;0.5",
                  "direction"); // the solid shell takes DIRECT 1
    expectInvalid("CompSolidPropNonlinear;6601;;;;;EightNodeElems;;Quadratic;;;;1;1;0.5", "integration8");
    expectInvalid("CompSolidPropNonlinear;6601;;;;;TwentyNodeElems;;Linear;;;;1;1;0.5", "integration20");
    expectInvalid("CompSolidPropNonlinear;6601;;;;;EightNodeElems;Solid;;;;;1;1;0.5", "behavior8");
    expectInvalid(
        "CompSolidPropNonlinear;6601;;;;;EightNodeElems;SolidComposite;;;;EightNodeElems;SolidComposite;;;;;1;1;0.5",
        "EightNodeElems");
    expectInvalid("CompSolidPropNonlinear;6601", "globalPlyId1");                        // a laminate has plies
    expectInvalid("CompSolidPropNonlinear;6601;;;;;;1;1;0.5;;;1;2;0.5", "globalPlyId1"); // global ply IDs are distinct
    expectInvalid("CompSolidPropNonlinear;6601;;;;;;0;1;0.5", "globalPlyId1");
    expectInvalid("CompSolidPropNonlinear;6601;;;;;;1;0;0.5", "matId1");
    expectInvalid("CompSolidPropNonlinear;6601;;;;;;1;1;0.", "thickness1");
    expectInvalid("CompSolidPropNonlinear;6601;;;;;;1;1", "thickness1");
}

TEST_CASE("a PLCOMP has an out-of-plane thickness and the plane words COMPS and AXCOMP", "[model][cards]")
{
    Model model = readBulk(
        "PlaneCompProp;6611;1;;;;;3010;1420;0.25;17.5;;3020;1460;0.75;72.5\n"
        "PlaneCompProp;6612;-2;0.5;3.5;ImplicitHeat;EightNodeElems;AxisymComposite;;AxisymComposite;;FourNodeElems;"
        "PlaneStrainComposite;;;;;1;1420;1.\n"
        "IsoMat;1420;2.1E+11;;0.3\n"
        "IsoMat;1460;2.1E+11;;0.3");
    const PlaneCompPropCard sampleCard = cardAt<PlaneCompPropTable>(model, 0);
    REQUIRE((sampleCard.id == 6611 && sampleCard.direction == 1 && sampleCard.thicknessOption == 1.0 &&
             !sampleCard.bondAllowable.has_value()));
    REQUIRE(sampleCard.groups == defaultGroups(PLANE_COMP_PROP_GROUPS));
    REQUIRE(sampleCard.groups[PLANE_COMP_PROP_C4] ==
            group(ElementBehavior::PlaneStrainComposite, IntegrationScheme::Linear,
                  ElementBehavior::PlaneStrainComposite, IntegrationScheme::Linear));
    REQUIRE(sampleCard.groups[PLANE_COMP_PROP_C8] ==
            group(ElementBehavior::PlaneStrainComposite, IntegrationScheme::Quadratic,
                  ElementBehavior::PlaneStrainComposite, IntegrationScheme::Quadratic));
    REQUIRE(sampleCard.plies == std::vector<PlaneCompPropPly>{{3010, 1420, 0.25, 17.5}, {3020, 1460, 0.75, 72.5}});
    const PlaneCompPropCard axisymmetric = cardAt<PlaneCompPropTable>(model, 1);
    REQUIRE((axisymmetric.direction == -2 && axisymmetric.thicknessOption == 0.5 && axisymmetric.bondAllowable == 3.5 &&
             axisymmetric.analysisType == PropertyAnalysis::Heat));
    REQUIRE(axisymmetric.groups[PLANE_COMP_PROP_C8] ==
            group(ElementBehavior::AxisymmetricComposite, IntegrationScheme::Quadratic,
                  ElementBehavior::AxisymmetricComposite, IntegrationScheme::Quadratic));
    REQUIRE(axisymmetric.groups[PLANE_COMP_PROP_C4] ==
            sampleCard.groups[PLANE_COMP_PROP_C4]); // a C4 line at its defaults
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<PlaneCompPropTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId1", "matId1"});
    REQUIRE(visitor.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::Material});

    requireRoundTrip(model);

    REQUIRE_THROWS_AS(readBulk("PlaneCompProp;6611;;;;;;1;1420;0.5\nShellProp;6611;1420;0.1"), base::DeckError);
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(1000));
    REQUIRE((cardAt<PlaneCompPropTable>(model, 0).id == 7611 && cardAt<PlaneCompPropTable>(model, 1).id == 7612));
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(100));
    REQUIRE(cardAt<PlaneCompPropTable>(model, 0).plies[1].matId == 1560);
    REQUIRE(unresolvedIn(model) == 0);

    expectInvalid("PlaneCompProp;0;;;;;;1;1;0.5", "id");
    expectInvalid("PlaneCompProp;6611;3;;;;;1;1;0.5", "direction"); // a plane element has two directions
    expectInvalid("PlaneCompProp;6611;0;;;;;1;1;0.5", "direction");
    expectInvalid("PlaneCompProp;6611;;x;;;;1;1;0.5", "thicknessOption");
    expectInvalid("PlaneCompProp;6611;;;-1.;;;1;1;0.5", "bondAllowable");
    expectInvalid("PlaneCompProp;6611;;;;XX;;1;1;0.5", "analysisType");
    expectInvalid("PlaneCompProp;6611;;;;;FourNodeElems;SolidComposite;;;;;1;1;0.5", "behavior4");
    expectInvalid("PlaneCompProp;6611;;;;;FourNodeElems;;Quadratic;;;;1;1;0.5", "integration4");
    expectInvalid("PlaneCompProp;6611;;;;;EightNodeElems;;Linear;;;;1;1;0.5", "integration8");
    expectInvalid("PlaneCompProp;6611;;;;;FourNodeElems;;;Solid;;;1;1;0.5", "heatBehavior4");
    expectInvalid(
        "PlaneCompProp;6611;;;;;FourNodeElems;PlaneStrainComposite;;;;FourNodeElems;PlaneStrainComposite;;;;;1;1;0.5",
        "FourNodeElems");
    expectInvalid("PlaneCompProp;6611", "globalPlyId1");
    expectInvalid("PlaneCompProp;6611;;;;;;1;1;0.5;;;1;2;0.5", "globalPlyId1");
    expectInvalid("PlaneCompProp;6611;;;;;;1;1;-0.5", "thickness1");
    expectInvalid("PlaneCompProp;6611;;;;;;1;0;0.5", "matId1");
}

namespace
{

// A deck that holds everything the sections below name, so that none of their references is left dangling.
const std::string SECTION_PREREQUISITES =
    "IsoMat;10;2.1E+11;;0.3\n"
    "Node;1\n"
    "Node;2\n"
    "Node;3\n"
    "Node;4\n"
    "Node;5\n"
    "Node;6\n"
    "Node;7\n"
    "Node;131\n"
    "Node;212\n"
    "IdListSet1;10;1;2;3\n"
    "IdListSet1;20;4;5\n"
    "IdListSet1;21;6;7\n"
    "CompPropGlobal;40;;;;;;;;1;10;0.1\n"
    "CompPropGlobal;41;;;;;;;;1;10;0.1\n"
    "CompPropGlobal;42;;;;;;;;1;10;0.1\n";

} // namespace

TEST_CASE("a PBRSECT is the section of a bar: the same items, without the core and the layers, and a mass field",
          "[model][cards]")
{
    Model model =
        readBulk(SECTION_PREREQUISITES +
                 "BarArbitrarySectionProp;7731;10;GeneralSection;;outerPerimeter=10,innerPerimeter=20\n"
                 "BarArbitrarySectionProp;7732;10;ClosedProfile;0.02;outerPerimeter=10,branch=20,thickness=1.0,"
                 "thickness(9)=[1.4,points=(131,212)]\n"
                 "BarArbitrarySectionProp;7733;10;OpenProfile;;outerPerimeter=10,thickness=1.,nonStructMass=0.03\n");
    const BarArbitrarySectionPropCard general = cardAt<BarArbitrarySectionPropTable>(model, 0);
    REQUIRE((general.id == 7731 && general.matId == 10 && general.form == ArbitrarySectionForm::General));
    REQUIRE((general.nonStructMass == 0.0 &&
             printSectionItems(general.items) == std::vector<std::string>{"outerPerimeter=10", "innerPerimeter=20"}));
    const BarArbitrarySectionPropCard closed = cardAt<BarArbitrarySectionPropTable>(model, 1);
    REQUIRE((closed.form == ArbitrarySectionForm::ClosedProfile && closed.nonStructMass == 0.02));
    REQUIRE(printSectionItems(closed.items) == std::vector<std::string>{"outerPerimeter=10", "branch=20",
                                                                        "thickness=1.",
                                                                        "thickness(9)=[1.4,points=(131,212)]"});
    // The mass may be in the text instead (the format reference lists it in both places).
    const BarArbitrarySectionPropCard text = cardAt<BarArbitrarySectionPropTable>(model, 2);
    REQUIRE((text.nonStructMass == 0.0 && text.items.back().key == ArbitrarySectionKey::NonstructuralMass));
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor visitor;
    model.table<BarArbitrarySectionPropTable>().visitReferences(1, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId", "outerPerimeter", "branch", "startPoint", "endPoint"});
    visitor = RecordingVisitor{};
    model.table<BarArbitrarySectionPropTable>().visitReferences(0, visitor);
    REQUIRE(visitor.fields == std::vector<std::string>{"matId", "outerPerimeter", "innerPerimeter"});
    REQUIRE(visitor.targets ==
            std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::Set, ReferenceTarget::Set});

    requireRoundTrip(model);

    REQUIRE_THROWS_AS(readBulk("BarArbitrarySectionProp;7731;10;GeneralSection;;outerPerimeter=10\nBarProp;7731;10;0."
                               "01;1.E-8;2.E-8;1.E-8"),
                      base::DeckError);
    Renumbering::apply(model, RenumberSpace::Property, Renumbering::offset(100));
    REQUIRE(cardAt<BarArbitrarySectionPropTable>(model, 1).id == 7832);
    Renumbering::apply(model, RenumberSpace::Set, Renumbering::offset(1000));
    REQUIRE(cardAt<BarArbitrarySectionPropTable>(model, 0).items[1].target == 1020);
    REQUIRE(unresolvedIn(model) == 0);

    expectInvalid("BarArbitrarySectionProp;0;10;GeneralSection;;outerPerimeter=10", "id");
    expectInvalid("BarArbitrarySectionProp;7731;0;GeneralSection;;outerPerimeter=10", "matId");
    expectInvalid("BarArbitrarySectionProp;7731;10;;;outerPerimeter=10", "form");
    expectInvalid("BarArbitrarySectionProp;7731;10;GeneralSection;-1.;outerPerimeter=10", "nonStructMass");
    expectInvalid("BarArbitrarySectionProp;7731;10;GeneralSection;0.02;outerPerimeter=10,nonStructMass=0.01",
                  "nonStructMass"); // in the field or in the text
    expectInvalid("BarArbitrarySectionProp;7731;10;GeneralSection;;outerPerimeter=10,core=5",
                  "core"); // a bar has no core, layers, OUTM
    expectInvalid("BarArbitrarySectionProp;7731;10;GeneralSection;;outerPerimeter=10,layer=[1,2]", "layer");
    expectInvalid("BarArbitrarySectionProp;7731;10;GeneralSection;;outerMesh=3", "outerMesh");
    expectInvalid("BarArbitrarySectionProp;7731;10;GeneralSection", "outerPerimeter");
    expectInvalid("BarArbitrarySectionProp;7731;10;ClosedProfile;;outerPerimeter=10,innerPerimeter=20",
                  "innerPerimeter");
    expectInvalid("BarArbitrarySectionProp;7731;10;GeneralSection;;outerPerimeter=10,thickness=1.", "thickness");
    expectInvalid("BarArbitrarySectionProp;7731;10;GeneralSection;;OUTP=10", "description");
    expectInvalid("BarArbitrarySectionProp;7731;10;GeneralSection;;OuterPerimeter=10", "description");
}

TEST_CASE("the text of a section is one field of the record and a long one is written over several lines",
          "[model][cards]")
{
    // The items of the description are separated by commas in one field.
    const Model fixed =
        readBulk("BeamArbitrarySectionProp;7725;10;ClosedProfile;outerPerimeter=10,branch=20,thickness=1.0");
    const BeamArbitrarySectionPropCard card = cardAt<BeamArbitrarySectionPropTable>(fixed);
    REQUIRE((card.id == 7725 && card.matId == 10 && card.form == ArbitrarySectionForm::ClosedProfile));
    REQUIRE(printSectionItems(card.items) ==
            std::vector<std::string>{"outerPerimeter=10", "branch=20", "thickness=1."});
    REQUIRE(cardAt<BeamArbitrarySectionPropTable>(
                readBulk("BeamArbitrarySectionProp;7726;10;GeneralSection;outerPerimeter=12345678"))
                .items[0]
                .target == 12345678);

    // The tokens of the text are the pieces between commas, brackets and parentheses included.
    const BarArbitrarySectionPropCard commas =
        cardAt<BarArbitrarySectionPropTable>(readBulk("BarArbitrarySectionProp;1;2;ClosedProfile;;outerPerimeter=10,"
                                                      "thickness(3)=[1.5,points=(7,8)],nonStructMass=1.E-3"));
    REQUIRE(printSectionItems(commas.items) ==
            std::vector<std::string>{"outerPerimeter=10", "thickness(3)=[1.5,points=(7,8)]", "nonStructMass=0.001"});

    // A long section description remains one complete text value.
    std::string longCard = "BarArbitrarySectionProp;7741;10;OpenProfile;;outerPerimeter=10";
    for (int index = 1; index <= 12; ++index)
        longCard += ",branch(" + std::to_string(index) + ")=" + std::to_string(30 + index);
    longCard += ",thickness=1.\n";
    const Model longModel = readBulk(longCard);
    const BarArbitrarySectionPropCard longSection = cardAt<BarArbitrarySectionPropTable>(longModel);
    REQUIRE(longSection.items.size() == 14);
    REQUIRE(cardAt<BarArbitrarySectionPropTable>(readModelText(writeModelText(longModel), true)).items ==
            longSection.items);
}
