#include "oor/base/errors/Errors.hpp"
#include "oor/model/control/case_control/CaseControl.hpp"
#include "oor/model/control/case_control/CaseValueParser.hpp"
#include "oor/model/control/case_control/CommandCatalogue.hpp"
#include "support/ControlTestSupport.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <utility>
#include <vector>

using namespace oor;
using namespace oor::model;

namespace
{

const CommandDefinition& command(const char* name)
{
    const CommandDefinition* definition = CommandCatalogue::instance().findByName(name);
    REQUIRE(definition != nullptr);
    return *definition;
}

// The case section of a model file holding the records of [case] (docs/model-format.md).
CaseControl parse(const std::string& records)
{
    return testing::readModelText("[case]\n" + records, true).control().caseControl;
}

std::string textOf(const CaseControl& control)
{
    return testing::caseText(control);
}

} // namespace

TEST_CASE("a Case Control made in memory writes neutral records in subcase order", "[control][edit]")
{
    CaseControl control;
    REQUIRE(control.isEmpty());
    control.setCommand(control.global(), command("TitleText"), "", {}, CaseValue::ofText("a plate"));
    control.setOutput(control.global(), command("DisplacementOutput"), {"PlotOnly", "SortByPoint"},
                      CaseValue::ofKeyword("All"));
    Subcase& first = control.addSubcase(SubcaseKind::Subcase, 1);
    control.setCommand(first, command("SinglePointConstrSelection"), "", {}, CaseValue::ofInteger(10));
    control.setCommand(first, command("TemperatureSelection"), "InitialTemperature", {}, CaseValue::ofInteger(3));
    control.setParam(first, "PostprocessingFile", CaseValue::ofInteger(-1));
    control.setCommand(first, command("PlotSuperelements"), "", {}, CaseValue::ofText("0,1"));
    REQUIRE_FALSE(control.isEmpty());
    REQUIRE(textOf(control) ==
            "TitleText;a "
            "plate\nDisplacementOutput;All;PlotOnly;SortByPoint\nSubcaseStart;1\nSinglePointConstrSelection;"
            "10\nTemperatureSelection;3;InitialTemperature\nSubcaseParameter;PostprocessingFile;-1\nPlotSuperelements;"
            "0,1\n");
}

TEST_CASE("setting a command changes the value in place and drops only that entry's original text", "[control][edit]")
{
    CaseControl control = parse("# fixed\nSinglePointConstrSelection;10\nStaticLoadSelection;4\nSubcaseStart;1\n");
    CaseEntry& spc =
        control.setCommand(control.global(), command("SinglePointConstrSelection"), "", {}, CaseValue::ofInteger(11));
    REQUIRE(spc.original.empty());
    REQUIRE(textOf(control) == "# fixed\nSinglePointConstrSelection;11\nStaticLoadSelection;4\nSubcaseStart;1\n");

    // The same value again changes nothing.
    const std::string before = textOf(control);
    control.setCommand(control.global(), command("StaticLoadSelection"), "", {}, CaseValue::ofInteger(4));
    REQUIRE(textOf(control) == before);
    REQUIRE(control.global().entries.size() == 2);
}

TEST_CASE("an output request with the same name and options is replaced, another is added", "[control][edit]")
{
    CaseControl control = parse("SubcaseStart;1\nDisplacementOutput;All;PlotOnly\n");
    Subcase& subcase = control.segments()[1];
    control.setOutput(subcase, command("DisplacementOutput"), {"PlotOnly"}, CaseValue::ofInteger(5));
    control.setOutput(subcase, command("DisplacementOutput"), {"PrintListing", "SortByPoint"},
                      CaseValue::ofKeyword("All"));
    REQUIRE(CaseControl::ownOutputs(subcase, "DisplacementOutput").size() == 2);
    REQUIRE(textOf(control) ==
            "SubcaseStart;1\nDisplacementOutput;5;PlotOnly\nDisplacementOutput;All;PrintListing;SortByPoint\n");

    // The options are compared without regard to their order.
    control.setOutput(subcase, command("DisplacementOutput"), {"SortByPoint", "PrintListing"},
                      CaseValue::ofKeyword("None"));
    REQUIRE(CaseControl::ownOutputs(subcase, "DisplacementOutput").size() == 2);
    REQUIRE(CaseControl::ownOutputs(subcase, "DisplacementOutput")[1]->value.text == "None");

    REQUIRE(control.removeOutputs(subcase, "DisplacementOutput", nullptr));
    REQUIRE(CaseControl::ownOutputs(subcase, "DisplacementOutput").empty());
    REQUIRE_FALSE(control.removeOutputs(subcase, "DisplacementOutput", nullptr));
}

TEST_CASE("a new subcase goes after the last subcase and before the output blocks", "[control][edit]")
{
    CaseControl control = parse("SubcaseStart;1\nSinglePointConstrSelection;1\nOutputBlock;;Plot\nPlotDraw;SET 1\n");
    control.addSubcase(SubcaseKind::SubcaseCombination, 30);
    REQUIRE(control.segments().size() == 3);
    REQUIRE(control.segments()[2].kind == SubcaseKind::SubcaseCombination);
    REQUIRE(control.segments()[1].entries.size() == 1);
    REQUIRE(control.segments()[2].entries.size() == 2); // the block moved to the last subcase
    REQUIRE(textOf(control) ==
            "SubcaseStart;1\nSinglePointConstrSelection;1\nSubcaseCombination;30\nOutputBlock;;Plot\nPlotDraw;SET 1\n");
    // A command added to that subcase goes before its block.
    control.setCommand(control.segments()[2], command("SinglePointConstrSelection"), "", {}, CaseValue::ofInteger(4));
    REQUIRE(textOf(control) ==
            "SubcaseStart;1\nSinglePointConstrSelection;1\nSubcaseCombination;30\nSinglePointConstrSelection;"
            "4\nOutputBlock;;Plot\nPlotDraw;SET 1\n");
    REQUIRE_THROWS_AS(control.addSubcase(SubcaseKind::Step, 1), base::UsageError);
}

TEST_CASE("an added subcase is undone from its addition point, its moved block back in place", "[control][edit]")
{
    CaseControl control = parse("SubcaseStart;1\nSinglePointConstrSelection;1\nOutputBlock;;Plot\nPlotDraw;SET 1\n");
    const std::string before = textOf(control);
    const std::uint32_t firstKey = control.segments()[1].key;
    CaseControl::AdditionPoint point = control.additionPoint();
    Subcase& added = control.addSubcase(SubcaseKind::Subcase, 2);
    control.setCommand(added, command("StaticLoadSelection"), "", {}, CaseValue::ofInteger(5));
    const std::uint32_t nextKey = control.newKey();
    control.restore(std::move(point));
    REQUIRE(textOf(control) == before);
    REQUIRE(control.segments().size() == 2);
    REQUIRE(control.findSubcase(firstKey) == &control.segments()[1]);
    REQUIRE(control.newKey() < nextKey); // the keys of the undone subcase are given again, as with a copy put back
    REQUIRE_THROWS_AS(control.restore(CaseControl::AdditionPoint{}), base::UsageError);
}

TEST_CASE("a subcase is found by bisection, a step and a stale key by walking the sections", "[control][edit]")
{
    CaseControl control;
    std::vector<std::uint32_t> keys;
    for (std::int64_t id = 1; id <= 9; ++id)
        keys.push_back(control.addSubcase(SubcaseKind::Subcase, id).key);
    const std::uint32_t stepKey = control.addChild(keys[3], SubcaseKind::Step, 1).key;
    keys.push_back(control.addSubcase(SubcaseKind::Subcase, 10).key);
    for (std::size_t index = 0; index < keys.size(); ++index)
        REQUIRE(control.findSubcase(keys[index]) == &control.segments()[index + 1]);
    REQUIRE(control.findSubcase(stepKey) == &control.segments()[4].children[0]);
    REQUIRE(control.findSubcase(control.global().key) == &control.global());
    REQUIRE(control.removeSubcase(keys[5]));
    REQUIRE(control.findSubcase(keys[5]) == nullptr);
    REQUIRE(control.findSubcase(keys[6])->id == 7);
    // Sections out of key order (a list edited through segments()) are still found, by the walk.
    std::swap(control.segments()[1], control.segments()[8]);
    for (const std::uint32_t key : {keys[0], keys[8], keys[9]})
        REQUIRE(control.findSubcase(key)->key == key);
}

TEST_CASE("steps are added to a subcase and a block at the end", "[control][edit]")
{
    CaseControl control;
    Subcase& subcase = control.addSubcase(SubcaseKind::Subcase, 1);
    const std::uint32_t subcaseKey = subcase.key;
    Subcase& step = control.addChild(subcaseKey, SubcaseKind::Step, 1);
    const std::uint32_t stepKey = step.key;
    control.setCommand(*control.findSubcase(stepKey), command("NonlinearStaticParamsSelection"), "", {},
                       CaseValue::ofInteger(7));
    control.addChild(stepKey, SubcaseKind::Substep, 1);
    const std::uint32_t blockKey = control.addBlock("CurvePlot").key;
    control.addBlockCommand(blockKey, command("XyXTitle"), "TIME");
    control.addBlockCommand(blockKey, command("XyYTitle"), "FORCE");
    REQUIRE(textOf(control) ==
            "SubcaseStart;1\nStepStart;1\nNonlinearStaticParamsSelection;7\nSubstepStart;1\nOutputBlock;;"
            "CurvePlot\nXyXTitle;TIME\nXyYTitle;FORCE\n");
    REQUIRE(control.blockKeys() == std::vector<std::uint32_t>{blockKey});
    REQUIRE_THROWS_AS(control.addBlockCommand(stepKey, command("XyXTitle"), "x"), base::UsageError);
    REQUIRE(control.removeBlock(blockKey));
    REQUIRE_FALSE(control.removeBlock(blockKey));
    REQUIRE(textOf(control) == "SubcaseStart;1\nStepStart;1\nNonlinearStaticParamsSelection;7\nSubstepStart;1\n");
    REQUIRE(control.lineage(stepKey).size() == 3);
}

TEST_CASE("commands, sets and parameters are removed; a subcase with INCLUDE markers is not", "[control][edit]")
{
    CaseControl control = parse(
        "SinglePointConstrSelection;1\nSetDefinition;3;1;2\nSubcaseParameter;PostprocessingFile;-1\nSubcaseStart;1\n"
        "SubcaseStart;2\n");
    REQUIRE(control.removeCommand(control.global(), "SinglePointConstrSelection"));
    REQUIRE_FALSE(control.removeCommand(control.global(), "SinglePointConstrSelection"));
    REQUIRE(control.removeSet(control.global(), 3));
    REQUIRE(control.removeParam(control.global(), "PostprocessingFile"));
    REQUIRE(control.global().entries.empty());
    REQUIRE(control.removeSubcase(control.segments()[1].key));
    REQUIRE(textOf(control) == "SubcaseStart;2\n");
    REQUIRE_FALSE(control.removeSubcase(control.global().key));

    CaseEntry begin;
    begin.kind = EntryKind::IncludeBegin;
    begin.text = "INCLUDE 'a.oos'";
    control.segments()[1].entries.push_back(begin);
    REQUIRE_FALSE(control.removeSubcase(control.segments()[1].key));
    REQUIRE(control.segments().size() == 2);
}

TEST_CASE("the numbers of the commands that hold a set ID go through the visitor", "[control][edit][references]")
{
    struct Shift final : ReferenceVisitor
    {
        base::EntityId visit(std::string_view field, ReferenceTarget target, base::EntityId id) override
        {
            seen.push_back(std::string(field) + ":" + std::to_string(id));
            return target == ReferenceTarget::ConstraintSet ? id + 100 : id;
        }
        std::vector<std::string> seen;
    };
    CaseControl control = parse(
        "SinglePointConstrSelection;1\nEigenMethodSelection;5\nTimeStepSelection;9\nTitleText;x\nSubcaseStart;1\n"
        "SinglePointConstrSelection;2\nMultiPointConstrSelection;2\nStaticLoadSelection;3\n");
    Shift shift;
    std::vector<std::string> scopes;
    control.visitReferences(shift, [&scopes](SubcaseKind kind, std::int64_t number)
                            { scopes.push_back(std::string(subcaseKindName(kind)) + " " + std::to_string(number)); });
    REQUIRE(scopes == std::vector<std::string>{"global 0", "subcase 1"});
    // The field of a command is its neutral name.
    REQUIRE(shift.seen == std::vector<std::string>{"SinglePointConstrSelection:1", "EigenMethodSelection:5",
                                                   "TimeStepSelection:9", "SinglePointConstrSelection:2",
                                                   "MultiPointConstrSelection:2", "StaticLoadSelection:3"});
    REQUIRE(textOf(control) ==
            "SinglePointConstrSelection;101\nEigenMethodSelection;5\nTimeStepSelection;9\nTitleText;x\nSubcaseStart;"
            "1\nSinglePointConstrSelection;102\nMultiPointConstrSelection;102\nStaticLoadSelection;3\n");
}

TEST_CASE("values are checked against the syntax of their command", "[control][edit]")
{
    const auto problem = [](const char* name, const CaseValue& value)
    { return CaseValueParser::check(command(name), value); };
    REQUIRE_FALSE(problem("SinglePointConstrSelection", CaseValue::ofInteger(3)).has_value());
    REQUIRE(problem("SinglePointConstrSelection", CaseValue::ofText("3"))->isWrongKind);
    REQUIRE_FALSE(problem("SinglePointConstrSelection", CaseValue::ofInteger(-1))->isWrongKind);
    REQUIRE(problem("XyXPaperSize", CaseValue::ofInteger(3))->isWrongKind);
    REQUIRE(problem("TitleText", CaseValue::ofKeyword("X"))->isWrongKind);
    REQUIRE_FALSE(problem("AeroSymmetryXz", CaseValue::ofKeyword("SymmetricFlow")).has_value());
    const auto unlisted = problem("AeroSymmetryXz", CaseValue::ofKeyword("MIRROR"));
    REQUIRE(unlisted.has_value());
    REQUIRE_FALSE(unlisted->isWrongKind);
    REQUIRE(unlisted->message.find("SymmetricFlow") != std::string::npos);
    REQUIRE(problem("SubcaseCombinationFactors", CaseValue::ofIntegers({1}))->isWrongKind);
    REQUIRE_FALSE(problem("SuperelementAllSteps", CaseValue::ofKeyword("All")).has_value());
    REQUIRE_FALSE(problem("DisplacementOutput", CaseValue::ofInteger(4)).has_value());
    REQUIRE(problem("DisplacementOutput", CaseValue::ofInteger(0))->isWrongKind == false);
}
