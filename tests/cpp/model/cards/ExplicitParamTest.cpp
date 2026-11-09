// Checks of the DYPARAM card: one card type with 80 documented entries, typed in a catalogue. The catalogue
// is checked against a line typed for every entry, the card keeps unknown names as written, the references of its
// values are visited and renumbered, and each rule of an entry (kinds, words, ranges, the values that go together)
// refuses what the format reference forbids.

#include "oor/model/cards/explicit/explicitParam/ExplicitParamCatalogue.hpp"
#include "oor/model/cards/explicit/explicitParam/ExplicitParamTable.hpp"
#include "oor/model/cards/explicit/explicitParam/ExplicitParamValues.hpp"
#include "support/ExplicitTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <limits>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// One line for every entry of the format reference, and one for every option of CONTACT and every keyword of SMP: each
// gives values of the right kind for its entry, chosen for these tests. Every setting is given once, so that a deck of
// all the lines has no repeated setting.
const std::vector<std::string>& entryLines()
{
    static const std::vector<std::string> LINES = {
        "ExplicitParam;OccupantAccelerationOutput;0.006",
        "ExplicitParam;OccupantHistoryOutput;Yes",
        "ExplicitParam;OccupantTimeOutput;3.E-4",
        "ExplicitParam;AutoCoupling;On;On;On;0.002;AverageDisplacement;PorousKind",
        "ExplicitParam;AxialSymmetry;RectRegion;YAxis;PlaneZx;4.5;No;0.8",
        "ExplicitParam;AxialRemap;2.;0.4;0.6;0.;1.;0.;35.",
        "ExplicitParam;LinearBulkViscosity;0.04",
        "ExplicitParam;QuadraticBulkViscosity;1.4",
        "ExplicitParam;BulkViscosityKind;ExternalExplicitSolver",
        "ExplicitParam;FullRigidCoupling;Yes",
        "ExplicitParam;ClumpFlowLimit;0.15",
        "ExplicitParam;ClumpEnergy;Average",
        "ExplicitParam;PointMassOutput;Yes",
        "ExplicitParam;ContactDefaults;VersionKind;Belt",
        "ExplicitParam;ContactDefaults;ThicknessKind;0.02",
        "ExplicitParam;ContactDefaults;GapKind;0.002",
        "ExplicitParam;ContactDefaults;LimitsKind;-80.;90.;-40.;45.;0.;12.",
        "ExplicitParam;ContactDefaults;DampingKind;Yes",
        "ExplicitParam;ContactDefaults;CouplingPorosity;Yes",
        "ExplicitParam;ContactDefaults;DampingForce;No",
        "ExplicitParam;ContactDefaults;ExternalExplicitProgram",
        "ExplicitParam;ContactDefaults;InformationKind;104;106;108",
        "ExplicitParam;ContactDefaults;ElemView;25.",
        "ExplicitParam;ContactDefaults;Force;40;12.;MaxForceFactor",
        "ExplicitParam;CouplingSubcycleMaxTime;34",
        "ExplicitParam;CouplingFriction;NonMetal",
        "ExplicitParam;CohesionOption;4.E+10;2.E+5;8.",
        "ExplicitParam;DistributedParallelOptions;3;2;4;-1",
        "ExplicitParam;DummySegments;InternalKind",
        "ExplicitParam;SmallestTimeStepElemCount;0",
        "ExplicitParam;EulerCubeBlocks;800;3;2;2",
        "ExplicitParam;EulerPartitioning;User;3;2;2",
        "ExplicitParam;EulerTransport;ImpulseLoad;FailureCriterion",
        "ExplicitParam;EulerSubcyclesMax;5;0.02;None",
        "ExplicitParam;EulerSubcycles;40",
        "ExplicitParam;FailureTimeStep;0.003",
        "ExplicitParam;FailureOutput;Yes",
        "ExplicitParam;FlowMethod;FacetSurface",
        "ExplicitParam;GeometryCheckOption;Off",
        "ExplicitParam;FastCoupling;NoOffset;FailureCriterion;CheckEulerElems;NearOnlyKind;PolynomialPackingShort;Off;"
        "Off",
        "ExplicitParam;HydrostaticBody;-150.;0.;30.",
        "ExplicitParam;HourglassMembraneCoefficient;0.07",
        "ExplicitParam;HourglassCoefficient;0.11",
        "ExplicitParam;HourglassSolidCoefficient;0.06",
        "ExplicitParam;HourglassTwistCoefficient;0.04",
        "ExplicitParam;HourglassWarpCoefficient;0.02",
        "ExplicitParam;HourglassShell;FlanaganBelytschkoViscous",
        "ExplicitParam;HourglassSolid;ExternalExplicitProgram",
        "ExplicitParam;HourglassKind;FlanaganBelytschkoViscous",
        "ExplicitParam;HydrostaticInitialGravity;32.17",
        "ExplicitParam;HydroVolumeFailure;No",
        "ExplicitParam;InitialMetricMethod;Zero;Yes;0.003;0.004",
        "ExplicitParam;InfoObjects;Yes",
        "ExplicitParam;InitialStep;5.E-7",
        "ExplicitParam;LagrangePartitioning;6;0.;0.;1.;4.;0.;2.",
        "ExplicitParam;CubeLimit;1800",
        "ExplicitParam;RigidBodyMerge;RigidAssembly(2);RigidMat(7);RigidMat(8);RigidAssembly(9)",
        "ExplicitParam;RigidBodyMerge;Automatic",
        "ExplicitParam;RigidBodyMerge1;RigidMat(6);RigidMat(7);RigidAssembly(4)",
        "ExplicitParam;MaxStepSize;4.E-4",
        "ExplicitParam;MeshPlane;4",
        "ExplicitParam;MinStepSize;2.E-7",
        "ExplicitParam;GasMixing;No",
        "ExplicitParam;ZeroNodeVelocity;No",
        "ExplicitParam;OldLagrangeTime;0",
        "ExplicitParam;JonesWilkinsLeeDetonation;Link",
        "ExplicitParam;LimiterKind;RoeScheme;None",
        "ExplicitParam;ParallelOption;InfoParameters;Off",
        "ExplicitParam;PlasticCoverCut;0.004",
        "ExplicitParam;MinPressureFailure;No",
        "ExplicitParam;RigidBodyStarInfo;NodeOutputOff",
        "ExplicitParam;DensityCutoff;3.E-9",
        "ExplicitParam;RigidJointStiffness;15.",
        "ExplicitParam;MassScaling;3.E-6;40.;3",
        "ExplicitParam;ShellFormulation;KeywordHoff",
        "ExplicitParam;ShellElemSystem;MidsideNodes",
        "ExplicitParam;ShellPlasticity;Radial",
        "ExplicitParam;ShellStrainDefinition;Fiber",
        "ExplicitParam;ShellThicknessUpdate;No",
        "ExplicitParam;SelectedElems;Yes",
        "ExplicitParam;SharedMemoryParallel;ShellEntities;BatchSize;AutomaticSize;ProcessorLoopCount;3",
        "ExplicitParam;SharedMemoryParallel;SolidEntities;ProcessorInformation;Off",
        "ExplicitParam;SoundSpeedLimit;2.E-5",
        "ExplicitParam;SphericalRemap;2.;1.;4.;12.",
        "ExplicitParam;SphericalSymmetry;SphericalKind;YAxis;2.5",
        "ExplicitParam;StrainOutput;No",
        "ExplicitParam;ToleranceCheck;3.E-8",
        "ExplicitParam;VelocityDamping;0.004",
        "ExplicitParam;MaxVelocity;2.E+7;No",
        "ExplicitParam;MaxVelocity1;3.E+6;No;4.E+9",
        "ExplicitParam;Viscoplasticity;0",
    };
    return LINES;
}

// The lines joined into the card records of one model.
std::string entryDeck()
{
    std::string deck;
    for (const std::string& line : entryLines())
        deck += line + "\n";
    return deck;
}

// What distinguishes a setting from the others of its entry, as the catalogue sees it.
std::string settingOf(const ExplicitParamEntry& entry, const ExplicitParamForm& form)
{
    return std::string(entry.name) + "/" + std::string(form.selector);
}

ExplicitParamCard cardWith(const std::string& name, std::vector<ExplicitParamValue> values)
{
    ExplicitParamCard card;
    card.entryName = name;
    card.values = std::move(values);
    return card;
}

std::string fieldOf(const std::optional<CardProblem>& problem)
{
    REQUIRE(problem.has_value());
    return problem->field;
}

} // namespace

TEST_CASE("the DYPARAM catalogue lists the names of the format reference with their forms",
          "[model][cards][explicit][dyparam]")
{
    const ExplicitParamCatalogue& catalogue = ExplicitParamCatalogue::instance();
    // 79 names: 78 headings of one name each, and DYPARAM,SMP with its two headings (BATCHSIZ and CPUINFO).
    REQUIRE(catalogue.entries().size() == 79);
    std::size_t forms = 0;
    for (const ExplicitParamEntry& entry : catalogue.entries())
        forms += entry.forms.size();
    REQUIRE(forms == 90); // CONTACT has 11 options and SMP 2 keywords, the other 77 names one layout

    const ExplicitParamEntry* contact = catalogue.find("ContactDefaults");
    REQUIRE(contact != nullptr);
    REQUIRE((contact->forms.size() == 11 && contact->selectorIndex == 0 && contact->identityValues == 1));
    const ExplicitParamEntry* smp = catalogue.find("SharedMemoryParallel");
    REQUIRE(smp != nullptr);
    REQUIRE((smp->forms.size() == 2 && smp->selectorIndex == 1 && smp->identityValues == 2));
    REQUIRE(catalogue.find("RigidBodyMerge")->identityValues == -1); // may be given as often as needed
    REQUIRE(catalogue.find("NOSUCH") == nullptr);
    REQUIRE(catalogue.find("SHELLFORM") == nullptr); // the heading of the format has the eight characters of SHELLFRM
    REQUIRE(catalogue.find("ShellFormulation") != nullptr);

    // The structure of every entry holds together: a selector word is the word its form's selector slot accepts, a
    // blank that stands for another value points at an earlier slot, a default meets the rules of its own slot.
    for (const ExplicitParamEntry& entry : catalogue.entries())
    {
        INFO(entry.name);
        REQUIRE_FALSE(entry.forms.empty());
        REQUIRE((entry.forms.size() == 1) == (entry.selectorIndex < 0));
        for (const ExplicitParamForm& form : entry.forms)
        {
            REQUIRE_FALSE(form.slots.empty());
            if (entry.selectorIndex >= 0)
            {
                const ExplicitParamSlot& selector = form.slots[static_cast<std::size_t>(entry.selectorIndex)];
                REQUIRE(form.selector == selector.words);
                REQUIRE(selector.required());
            }
            for (std::size_t index = 0; index < form.slots.size(); ++index)
            {
                const ExplicitParamSlot& slot = form.slots[index];
                if (slot.defaultValue.kind == ExplicitParamDefaultKind::SameAs)
                    REQUIRE(slot.defaultValue.slot < index);
                if (slot.defaultValue.kind == ExplicitParamDefaultKind::Word)
                    REQUIRE(slot.kind == ExplicitParamSlotKind::Word);
                if (slot.defaultValue.kind == ExplicitParamDefaultKind::Real)
                    REQUIRE(
                        (slot.kind == ExplicitParamSlotKind::Real && slot.range.contains(slot.defaultValue.realValue)));
                if (slot.defaultValue.kind == ExplicitParamDefaultKind::Integer)
                    REQUIRE(slot.range.contains(slot.defaultValue.integerValue));
            }
        }
    }
}

TEST_CASE("every entry of the format reference reads from its line, in the form of its name",
          "[model][cards][explicit][dyparam]")
{
    const std::vector<std::string>& lines = entryLines();
    const Model model = readBulk(entryDeck());
    REQUIRE(model.diagnostics().diagnostics().empty());
    REQUIRE(rowCount<ExplicitParamTable>(model) == lines.size());

    const ExplicitParamCatalogue& catalogue = ExplicitParamCatalogue::instance();
    std::set<std::string> covered;
    for (std::uint32_t row = 0; row < lines.size(); ++row)
    {
        INFO(lines[row]);
        const ExplicitParamCard card = cardAt<ExplicitParamTable>(model, row);
        const ExplicitParamEntry* entry = catalogue.find(card.entryName);
        REQUIRE(entry != nullptr);
        const ExplicitParamForm* form = ExplicitParamCatalogue::formOf(*entry, card.values);
        REQUIRE(form != nullptr);
        covered.insert(settingOf(*entry, *form));

        // The values with every blank replaced by its default are values of the entry as well. (Not for SMP, where the
        // default of VALUE2 applies without the word CPULOOPS that goes with it.)
        if (entry->name != "SharedMemoryParallel")
        {
            const ExplicitParamCard resolved = cardWith(card.entryName, resolveExplicitParamValues(card));
            REQUIRE_FALSE(catalogue.problemWith(resolved).has_value());
        }
    }
    // A line was typed for every form of every entry.
    for (const ExplicitParamEntry& entry : catalogue.entries())
    {
        for (const ExplicitParamForm& form : entry.forms)
            REQUIRE(covered.contains(settingOf(entry, form)));
    }
    requireRoundTrip(model);
}

TEST_CASE("a name alone is a card exactly where no value is required", "[model][cards][explicit][dyparam]")
{
    const ExplicitParamCatalogue& catalogue = ExplicitParamCatalogue::instance();
    for (const ExplicitParamEntry& entry : catalogue.entries())
    {
        if (entry.forms.size() != 1)
            continue; // the option or the keyword is required
        INFO(entry.name);
        const ExplicitParamForm& form = entry.forms.front();
        bool needsValue = form.tailMinimum > 0;
        for (const ExplicitParamSlot& slot : form.slots)
            needsValue = needsValue || slot.required();
        REQUIRE(catalogue.problemWith(cardWith(std::string(entry.name), {})).has_value() == needsValue);
    }
}

TEST_CASE("a DYPARAM keeps its values as given, with the kind of each, and an unknown name as written",
          "[model][cards][explicit][dyparam]")
{
    const Model model = readBulk(
        "ExplicitParam;AutoCoupling;On;;;0.005;Average\n"
        "ExplicitParam;OccupantAccelerationOutput;1\n"
        "ExplicitParam;myparam;1;2.5;text;;7\n"
        "ExplicitParam;MYBLANK");
    const ExplicitParamCard coupling = cardAt<ExplicitParamTable>(model, 0);
    REQUIRE(coupling.entryName == "AutoCoupling"); // the neutral name of the dictionary
    REQUIRE(coupling.values.size() ==
            5); // the blank fields between the values are kept, the ones after the last are not
    REQUIRE((coupling.values[0].kind == ExplicitParamValueKind::Text && coupling.values[0].textValue == "On"));
    REQUIRE((coupling.values[1].isBlank() && coupling.values[2].isBlank()));
    REQUIRE((coupling.values[3].kind == ExplicitParamValueKind::Real && coupling.values[3].realValue == 5.0e-3));
    REQUIRE(coupling.values[4].textValue == "Average"); // the words use exact neutral names

    // An integer where the entry takes a real is the real of the same value.
    const ExplicitParamCard step = cardAt<ExplicitParamTable>(model, 1);
    REQUIRE((step.values.size() == 1 && step.values[0].kind == ExplicitParamValueKind::Real &&
             step.values[0].realValue == 1.0));

    // A name the format reference does not list is any number of values of any kind.
    const ExplicitParamCard mine = cardAt<ExplicitParamTable>(model, 2);
    REQUIRE(mine.entryName == "myparam");
    REQUIRE(mine.values.size() == 5);
    REQUIRE((mine.values[0].kind == ExplicitParamValueKind::Integer && mine.values[0].integerValue == 1));
    REQUIRE((mine.values[1].kind == ExplicitParamValueKind::Real && mine.values[1].realValue == 2.5));
    REQUIRE((mine.values[2].kind == ExplicitParamValueKind::Text && mine.values[2].textValue == "text"));
    REQUIRE(mine.values[3].isBlank());
    REQUIRE((mine.values[4].kind == ExplicitParamValueKind::Integer && mine.values[4].integerValue == 7));
    REQUIRE(ExplicitParamCatalogue::instance().find("MYPARAM") == nullptr);
    REQUIRE(cardAt<ExplicitParamTable>(model, 3).values.empty());
    requireRoundTrip(model);

    // A long list of values, past the eight fields of a line of the old layout.
    const Model grids = readBulk("ExplicitParam;ContactDefaults;InformationKind;1;2;3;4;5;6;7;8;9;10");
    REQUIRE(cardAt<ExplicitParamTable>(grids).values.size() == 11);
    REQUIRE(cardAt<ExplicitParamTable>(grids).values.back().integerValue == 10);
    requireRoundTrip(grids);

    // A card made in code is made like a card read: normalised before it is stored.
    ExplicitParamCard built =
        cardWith("OccupantAccelerationOutput",
                 {ExplicitParamValue::integer(5), ExplicitParamValue::blank(), ExplicitParamValue::blank()});
    normaliseExplicitParam(built);
    REQUIRE(built.entryName == "OccupantAccelerationOutput");
    REQUIRE(built.values == std::vector<ExplicitParamValue>{ExplicitParamValue::real(5.0)});
    // An integer left in a real slot is written as the real, so that the text reads back as the same card.
    Model made;
    made.add<ExplicitParamTable>(cardWith("OccupantAccelerationOutput", {ExplicitParamValue::integer(5)}));
    const Model again = readModelText(writeModelText(made), true);
    REQUIRE(cardAt<ExplicitParamTable>(again).values[0].kind == ExplicitParamValueKind::Real);
}

TEST_CASE("a setting is the last card of its name with the format reference defaults for the blank values",
          "[model][cards][explicit][dyparam]")
{
    const Model model = readBulk(
        "ExplicitParam;EulerCubeBlocks;1200;3\nExplicitParam;HydrostaticInitialGravity;9.8\nExplicitParam;"
        "HydrostaticInitialGravity;9.81\nExplicitParam;MYPARAM;3");

    // NBY and NBZ are NBX when blank.
    const std::optional<ExplicitParamSetting> cubes = explicitParamSetting(model, "EulerCubeBlocks");
    REQUIRE(cubes.has_value());
    REQUIRE(cubes->fromDeck);
    REQUIRE(cubes->values.size() == 4);
    REQUIRE((cubes->values[0].integerValue == 1200 && cubes->values[1].integerValue == 3));
    REQUIRE((cubes->values[2].integerValue == 3 && cubes->values[3].integerValue == 3));

    // The second of two cards is the one the solver ends with.
    REQUIRE(explicitParamSetting(model, "HydrostaticInitialGravity")->values[0].realValue == 9.81);

    // No card: the default of the format.
    const std::optional<ExplicitParamSetting> longest = explicitParamSetting(model, "MaxStepSize");
    REQUIRE(longest.has_value());
    REQUIRE_FALSE(longest->fromDeck);
    REQUIRE((longest->values.size() == 1 && longest->values[0].kind == ExplicitParamValueKind::Real));
    REQUIRE(longest->values[0].realValue == 1.0e20);
    REQUIRE(explicitParamSetting(model, "OccupantHistoryOutput")->values[0].textValue == "Yes");

    // No value at all: a value that is required, one that the format computes, a name with several settings, an unknown
    // name.
    REQUIRE_FALSE(explicitParamSetting(model, "InitialStep").has_value());
    REQUIRE_FALSE(explicitParamSetting(model, "HourglassCoefficient").has_value());
    REQUIRE_FALSE(explicitParamSetting(model, "ContactDefaults").has_value());
    REQUIRE_FALSE(explicitParamSetting(model, "NOSUCH").has_value());
    // An unknown name that a card gives is its values as written.
    // An unknown name is the deck word.
    const std::optional<ExplicitParamSetting> mine = explicitParamSetting(model, "MYPARAM");
    REQUIRE((mine.has_value() && mine->fromDeck && mine->values.size() == 1 && mine->values[0].integerValue == 3));
}

TEST_CASE("the tables, the grid points and the MATRIGs a DYPARAM names are references",
          "[model][cards][explicit][dyparam]")
{
    Model model = readBulk(
        "ExplicitParam;CouplingSubcycleMaxTime;30\n"
        "ExplicitParam;ContactDefaults;InformationKind;10;20\n"
        "ExplicitParam;RigidBodyMerge1;RigidMat(1);RigidAssembly(5);RigidMat(8)\n"
        "ExplicitParam;RigidBodyMerge;RigidAssembly(9);RigidMat(1)\n"
        "ExplicitParam;OccupantAccelerationOutput;0.001\n"
        "TableDynamic1;30;;;0.;1.;1.;1.;EndTable\n"
        "Node;10;;0.;0.;0.\n"
        "Node;20;;1.;0.;0.\n"
        "RigidMat;1\n"
        "RigidMat;8");
    REQUIRE(unresolvedIn(model) == 0);

    RecordingVisitor table;
    model.table<ExplicitParamTable>().visitReferences(0, table);
    REQUIRE(table.fields == std::vector<std::string>{"maxTimeTable"});
    REQUIRE(table.targets == std::vector<ReferenceTarget>{ReferenceTarget::DynamicTable});
    RecordingVisitor grids;
    model.table<ExplicitParamTable>().visitReferences(1, grids);
    REQUIRE(grids.fields == std::vector<std::string>{"monitoredGrid", "monitoredGrid"});
    REQUIRE(grids.targets == std::vector<ReferenceTarget>{ReferenceTarget::Node, ReferenceTarget::Node});
    // A MATRIG is named MR<id>; an FR<id> is an assembly (the new one of a MATRMERG does not exist) and is not visited.
    RecordingVisitor merge;
    model.table<ExplicitParamTable>().visitReferences(2, merge);
    REQUIRE(merge.fields == std::vector<std::string>{"firstBodyName", "mergedBodyNames"});
    REQUIRE(merge.targets == std::vector<ReferenceTarget>{ReferenceTarget::Material, ReferenceTarget::Material});
    RecordingVisitor assembly;
    model.table<ExplicitParamTable>().visitReferences(3, assembly);
    REQUIRE(assembly.fields == std::vector<std::string>{"mergedBodyNames"});
    RecordingVisitor none;
    model.table<ExplicitParamTable>().visitReferences(4, none);
    REQUIRE(none.fields.empty());

    Renumbering::apply(model, RenumberSpace::DynamicTable, Renumbering::offset(500));
    REQUIRE(cardAt<ExplicitParamTable>(model, 0).values[0].integerValue == 530);
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE((cardAt<ExplicitParamTable>(model, 1).values[1].integerValue == 110 &&
             cardAt<ExplicitParamTable>(model, 1).values[2].integerValue == 120));
    Renumbering::apply(model, RenumberSpace::Material, Renumbering::offset(1000));
    const ExplicitParamCard merged = cardAt<ExplicitParamTable>(model, 2);
    REQUIRE((merged.values[0].textValue == "RigidMat(1001)" && merged.values[1].textValue == "RigidAssembly(5)" &&
             merged.values[2].textValue == "RigidMat(1008)")); // the prefix stays, the FR name is left alone
    REQUIRE((cardAt<ExplicitParamTable>(model, 3).values[0].textValue == "RigidAssembly(9)" &&
             cardAt<ExplicitParamTable>(model, 3).values[1].textValue == "RigidMat(1001)"));
    REQUIRE(unresolvedIn(model) == 0);
    requireRoundTrip(model);

    Model dangling = readBulk(
        "ExplicitParam;CouplingSubcycleMaxTime;99\nExplicitParam;ContactDefaults;InformationKind;5\nExplicitParam;"
        "RigidBodyMerge1;"
        "RigidMat(3);RigidMat(4)\nRigidMat;3");
    REQUIRE(unresolvedIn(dangling) == 3); // the table, the grid point and RigidMat(4)
}

TEST_CASE("a card tells the setting it gives, and the settings given several times are kept as cards",
          "[model][cards][explicit][dyparam]")
{
    // CONTACT is one setting per option, SMP one per entity and keyword, a merge of rigid bodies as many as needed.
    const Model model = readBulk(
        "ExplicitParam;HydrostaticInitialGravity;9.8\n"
        "ExplicitParam;HydrostaticInitialGravity;9.81\n"
        "ExplicitParam;ContactDefaults;GapKind;0.1\n"
        "ExplicitParam;ContactDefaults;GapKind;0.2\n"
        "ExplicitParam;SharedMemoryParallel;BeamEntities;BatchSize;63\n"
        "ExplicitParam;SharedMemoryParallel;ShellEntities;BatchSize;63\n"
        "ExplicitParam;SharedMemoryParallel;BeamEntities;ProcessorInformation;On\n"
        "ExplicitParam;RigidBodyMerge;RigidAssembly(1);RigidMat(2)\n"
        "ExplicitParam;RigidBodyMerge;RigidAssembly(3);RigidMat(4)\n"
        "ExplicitParam;MYPARAM;1\n"
        "ExplicitParam;MYPARAM;2");
    // Repeating a setting is not an error: every card is kept.
    REQUIRE_FALSE(model.diagnostics().hasErrors());
    REQUIRE(rowCount<ExplicitParamTable>(model) == 11);

    const auto identity = [&model](std::uint32_t row)
    { return explicitParamIdentity(cardAt<ExplicitParamTable>(model, row)); };
    REQUIRE(identity(0) == "HydrostaticInitialGravity");
    REQUIRE(identity(0) == identity(1));
    REQUIRE(identity(2) == "ContactDefaults,GapKind");
    REQUIRE(identity(2) == identity(3));
    REQUIRE(identity(4) == "SharedMemoryParallel,BeamEntities,BatchSize");
    REQUIRE(identity(5) == "SharedMemoryParallel,ShellEntities,BatchSize");
    REQUIRE(identity(6) == "SharedMemoryParallel,BeamEntities,ProcessorInformation");
    REQUIRE_FALSE(identity(7).has_value()); // a merge may be given as often as needed
    REQUIRE_FALSE(identity(9).has_value()); // a name the catalogue does not know has no setting
}

TEST_CASE("the values of every entry are refused when they break its rules", "[model][cards][explicit][dyparam]")
{
    // The name and the number of values.
    expectInvalid("ExplicitParam", "entryName");
    expectInvalid("ExplicitParam;OccupantAccelerationOutput;1.;2.", ""); // a second value is not in the layout
    expectInvalid("ExplicitParam;MYPARAM;99999999999", "fieldValue");

    // A number: its kind and its limits.
    expectInvalid("ExplicitParam;OccupantAccelerationOutput;-1.", "outputInterval");
    expectInvalid("ExplicitParam;OccupantAccelerationOutput;Yes", "outputInterval");
    expectInvalid("ExplicitParam;InitialStep", "timeStep");           // required
    expectInvalid("ExplicitParam;ToleranceCheck;0.001", "tolerance"); // the format caps it at 1.E-6
    expectInvalid("ExplicitParam;HourglassCoefficient;0.2", "coefficient");
    expectInvalid("ExplicitParam;LinearBulkViscosity;-0.1", "coefficient");
    expectInvalid("ExplicitParam;EulerCubeBlocks;2.5", "elemsPerCube"); // an integer
    expectInvalid("ExplicitParam;EulerSubcycles;0", "subcycleCount");
    expectInvalid("ExplicitParam;Viscoplasticity;2", "viscoplasticOn");

    // A word.
    expectInvalid("ExplicitParam;OccupantHistoryOutput;MAYBE", "historyOutput");
    expectInvalid("ExplicitParam;OccupantHistoryOutput;5", "historyOutput");
    expectInvalid("ExplicitParam;AutoCoupling;Yes", "activeState");
    expectInvalid("ExplicitParam;AutoCoupling;;;;-1.", "areaTolerance");
    expectInvalid("ExplicitParam;AutoCoupling;On;Off;Off;0.;Zero;BLOW", "failureHandling");
    expectInvalid("ExplicitParam;AxialSymmetry;AxialMesh;XAxis", "symmetryPlane"); // required
    expectInvalid("ExplicitParam;ParallelOption;On", "settingWord");
    expectInvalid("ExplicitParam;ShellFormulation;BELY", "formulation");

    // The values of an entry that go together.
    expectInvalid("ExplicitParam;EulerPartitioning;User", "cubeCountX");
    expectInvalid("ExplicitParam;EulerSubcyclesMax;5;;None", "maxUncoveredFractionChange");
    expectInvalid("ExplicitParam;LagrangePartitioning;4;1.", "directionY");
    expectInvalid("ExplicitParam;LagrangePartitioning;6;1.;0.;0.;0.;-100.", "centreZ");
    expectInvalid("ExplicitParam;LagrangePartitioning;8", "partitionMethod");

    // A table.
    expectInvalid("ExplicitParam;CouplingSubcycleMaxTime", "maxTimeTable");
    expectInvalid("ExplicitParam;CouplingSubcycleMaxTime;0", "maxTimeTable");
    expectInvalid("ExplicitParam;CouplingSubcycleMaxTime;1.5", "maxTimeTable");

    // CONTACT: the option picks the layout.
    expectInvalid("ExplicitParam;ContactDefaults", "contactOption");
    expectInvalid("ExplicitParam;ContactDefaults;NOPE", "contactOption");
    expectInvalid("ExplicitParam;ContactDefaults;VersionKind", "contactVersion");
    expectInvalid("ExplicitParam;ContactDefaults;VersionKind;V9", "contactVersion");
    expectInvalid("ExplicitParam;ContactDefaults;ThicknessKind;-1.", "thickness");
    expectInvalid("ExplicitParam;ContactDefaults;InformationKind", "monitoredGrid");
    expectInvalid("ExplicitParam;ContactDefaults;InformationKind;1;;3", "monitoredGrid");
    expectInvalid("ExplicitParam;ContactDefaults;InformationKind;0", "monitoredGrid");
    expectInvalid("ExplicitParam;ContactDefaults;ExternalExplicitProgram;Yes", "");
    expectInvalid("ExplicitParam;ContactDefaults;Force;0", "cycleCount");
    expectInvalid("ExplicitParam;ContactDefaults;Force;10;10.;SOME", "forceKind");
    expectInvalid("ExplicitParam;ContactDefaults;LimitsKind;1.;2.;3.;4.;5.;6.;7.", ""); // a value too many

    // SMP: the keyword picks the layout, the entity is one of the words of that layout.
    expectInvalid("ExplicitParam;SharedMemoryParallel;BeamEntities", "settingKey");
    expectInvalid("ExplicitParam;SharedMemoryParallel;BeamEntities;OTHER", "settingKey");
    expectInvalid("ExplicitParam;SharedMemoryParallel;WALLS;BatchSize", "entityKind");
    expectInvalid("ExplicitParam;SharedMemoryParallel;MembraneKind;BatchSize",
                  "entityKind"); // only the CPU information knows the membranes
    expectInvalid("ExplicitParam;SharedMemoryParallel;BeamEntities;BatchSize;0", "batchSize");
    expectInvalid("ExplicitParam;SharedMemoryParallel;BeamEntities;BatchSize;BIG", "batchSize");
    expectInvalid("ExplicitParam;SharedMemoryParallel;BeamEntities;BatchSize;8;ProcessorLoopCount", "cpuLoopCount");
    expectInvalid("ExplicitParam;SharedMemoryParallel;BeamEntities;BatchSize;AutomaticSize;;4", "cpuLoopsWord");
    expectInvalid("ExplicitParam;SharedMemoryParallel;BeamEntities;ProcessorInformation;MAYBE", "printCpuInfo");

    // The merges of rigid bodies.
    expectInvalid("ExplicitParam;RigidBodyMerge", "newAssemblyName");
    expectInvalid("ExplicitParam;RigidBodyMerge;RigidAssembly(1)", "mergedBodyNames"); // nothing to merge
    expectInvalid("ExplicitParam;RigidBodyMerge;RigidMat(1);RigidMat(2)",
                  "newAssemblyName"); // the new assembly is an FR
    expectInvalid("ExplicitParam;RigidBodyMerge;Automatic;RigidMat(1)", "mergedBodyNames");
    expectInvalid("ExplicitParam;RigidBodyMerge;RigidAssembly(1);X2", "mergedBodyNames");
    expectInvalid("ExplicitParam;RigidBodyMerge;RigidAssembly(1);;RigidMat(2)", "mergedBodyNames");
    expectInvalid("ExplicitParam;RigidBodyMerge;RigidAssembly(1);RigidMat(0)", "mergedBodyNames");
    expectInvalid("ExplicitParam;RigidBodyMerge1;RigidMat(1)", "mergedBodyNames");
    expectInvalid("ExplicitParam;RigidBodyMerge1;Automatic;RigidMat(1)", "firstBodyName");
}

TEST_CASE("the check of a DYPARAM refuses what would not read back as the same card",
          "[model][cards][explicit][dyparam]")
{
    // User-defined names and values retain their spelling and length; text remains distinct from numbers.
    REQUIRE_FALSE(ExplicitParamFormat::check(cardWith("TOOLONGNAME", {})).has_value());
    REQUIRE(fieldOf(ExplicitParamFormat::check(cardWith("", {}))) == "entryName");
    REQUIRE_FALSE(ExplicitParamFormat::check(cardWith("lower", {})).has_value());
    REQUIRE_FALSE(ExplicitParamFormat::check(cardWith("HAS,COMMA", {})).has_value());
    REQUIRE_FALSE(ExplicitParamFormat::check(cardWith("MYPARAM", {ExplicitParamValue::text("lower")})).has_value());
    REQUIRE_FALSE(ExplicitParamFormat::check(cardWith("MYPARAM", {ExplicitParamValue::text("NINELETTR")})).has_value());
    REQUIRE(fieldOf(ExplicitParamFormat::check(cardWith("MYPARAM", {ExplicitParamValue::text("123")}))) ==
            "fieldValue");
    REQUIRE(fieldOf(ExplicitParamFormat::check(cardWith("MYPARAM", {ExplicitParamValue::text("1.5E3")}))) ==
            "fieldValue");
    REQUIRE(fieldOf(ExplicitParamFormat::check(cardWith(
                "MYPARAM", {ExplicitParamValue::real(std::numeric_limits<double>::infinity())}))) == "fieldValue");
    // The problem is named by the field of the entry where there is one.
    const auto problemField = [](const char* name, std::vector<ExplicitParamValue> values)
    { return fieldOf(ExplicitParamFormat::check(cardWith(name, std::move(values)))); };
    REQUIRE(problemField("OccupantHistoryOutput", {ExplicitParamValue::text("lower")}) == "historyOutput");
    REQUIRE(problemField("OccupantAccelerationOutput", {ExplicitParamValue::real(std::nan(""))}) == "outputInterval");

    // An integer where the entry takes a real is accepted (the card writes and reads it back as the real).
    REQUIRE_FALSE(ExplicitParamFormat::check(cardWith("OccupantAccelerationOutput", {ExplicitParamValue::integer(5)}))
                      .has_value());
    REQUIRE_FALSE(
        ExplicitParamFormat::check(cardWith("MYPARAM", {ExplicitParamValue::integer(5), ExplicitParamValue::blank(),
                                                        ExplicitParamValue::text("ANY")}))
            .has_value());
    // A value of the wrong kind in a slot is refused, a blank value after the last is not a value.
    REQUIRE(problemField("EulerCubeBlocks", {ExplicitParamValue::real(2.0)}) == "elemsPerCube");
    REQUIRE(problemField("OccupantHistoryOutput", {ExplicitParamValue::text("No"), ExplicitParamValue::integer(1)})
                .empty());
}
