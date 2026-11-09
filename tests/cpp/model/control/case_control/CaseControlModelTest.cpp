#include "oor/model/control/case_control/CommandCatalogue.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/model/references/Renumbering.hpp"
#include "support/ControlTestSupport.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <fstream>
#include <iterator>

using namespace oor;
using namespace oor::model;
using oor::testing::readModelText;
using oor::testing::writeModelText;

namespace
{

const char* const DECK =
    "[executive]\n"
    "SolutionSequence;101\n"
    "[case]\n"
    "TitleText;a plate\n"
    "# the constraints\n"
    "SinglePointConstrSelection;10\n"
    "DisplacementOutput;All;PlotOnly\n"
    "SetDefinition;5;1;Through;4;9\n"
    "SubcaseStart;1\n"
    "LabelText;first\n"
    "StaticLoadSelection;5\n"
    "EigenMethodSelection;3\n"
    "SubcaseStart;2\n"
    "SinglePointConstrSelection;20\n"
    "DisplacementOutput;5;PrintListing\n"
    "OutputBlock;;Plot\n"
    "PlotDraw;SET 1\n"
    "[model]\n"
    "Node;1;;0.;0.;0.\n"
    "Node;2;;1.;0.;0.\n"
    "SinglePointConstrList;10;123456;1\n"
    "SinglePointConstrList;20;123;2\n"
    "PointForceLoad;5;2;;1.;;;1.\n"
    "PointForceLoad;6;2;;1.;;;1.\n"
    "LanczosEigenMethod;3;;;4\n";

} // namespace

TEST_CASE("a model file with a case section reads and writes back unchanged", "[control][model]")
{
    const Model model = readModelText(DECK);
    const CaseControl& control = model.control().caseControl;
    REQUIRE(control.segments().size() == 3);
    REQUIRE(model.diagnostics().diagnostics().empty());
    const Subcase& first = control.segments()[1];
    REQUIRE(control.effectiveCommand(first.key, "SinglePointConstrSelection")->value.integer == 10);
    REQUIRE(writeModelText(model) == DECK);
}

TEST_CASE("a Case Control read, written and read again is the same", "[control][model]")
{
    const Model model = readModelText(DECK);
    const Model again = readModelText(writeModelText(model));
    const auto textOf = [](const Model& value) { return testing::caseText(value.control().caseControl); };
    REQUIRE(textOf(again) == textOf(model));
    REQUIRE(textOf(model) ==
            "TitleText;a plate\n# the "
            "constraints\nSinglePointConstrSelection;10\nDisplacementOutput;All;PlotOnly\nSetDefinition;5;1;Through;4;"
            "9\nSubcaseStart;1\nLabelText;first\nStaticLoadSelection;5\nEigenMethodSelection;3\nSubcaseStart;"
            "2\nSinglePointConstrSelection;20\nDisplacementOutput;5;PrintListing\nOutputBlock;;Plot\nPlotDraw;SET 1\n");
}

TEST_CASE("a model without case commands preserves its executive and model sections", "[control][model]")
{
    const std::string deck = "[executive]\nSolutionSequence;101\n[model]\nNode;1;;0.;0.;0.\n";
    const Model model = readModelText(deck);
    REQUIRE(model.control().caseControl.isEmpty());
    REQUIRE(writeModelText(model).find("[executive]\nSolutionSequence;101\n[model]\n") == 0);
}

TEST_CASE("adding case commands writes a case section before model records", "[control][model]")
{
    Model model = readModelText("Node;1;;0.;0.;0.\n");
    REQUIRE(writeModelText(model).find("[case]") == std::string::npos);
    CaseControl& control = model.control().caseControl;
    Subcase& subcase = control.addSubcase(SubcaseKind::Subcase, 1);
    control.setCommand(subcase, *CommandCatalogue::instance().findByName("SinglePointConstrSelection"), "", {},
                       CaseValue::ofInteger(1));
    const std::string written = writeModelText(model);
    REQUIRE(written.find("[case]\nSubcaseStart;1\nSinglePointConstrSelection;1\n[model]\nNode") == 0);
    REQUIRE(readModelText(writeModelText(model)).control().caseControl.segments().size() == 2);
}

TEST_CASE("a model built from nothing writes its case records", "[control][model]")
{
    Model model;
    CaseControl& control = model.control().caseControl;
    control.setCommand(control.global(), *CommandCatalogue::instance().findByName("TitleText"), "", {},
                       CaseValue::ofText("scratch"));
    const std::string written = writeModelText(model);
    REQUIRE(written == "[case]\nTitleText;scratch\n");
    const Model again = readModelText(writeModelText(model));
    REQUIRE(CaseControl::ownCommand(again.control().caseControl.global(), "TitleText")->value.text == "scratch");
}

TEST_CASE("a model without any Case Control still writes what it wrote before", "[control][model]")
{
    Model model;
    REQUIRE(writeModelText(model) == "");
}

TEST_CASE("renumbering a set ID rewrites the Case Control commands that hold it", "[control][model][references]")
{
    Model model = readModelText(DECK);
    Renumbering::apply(model, RenumberSpace::ConstraintSet, Renumbering::offset(100));
    Renumbering::apply(model, RenumberSpace::LoadCombinationSet, Renumbering::offset(50));
    const CaseControl& control = model.control().caseControl;
    REQUIRE(control.global().entries[1].value.integer == 110); // SPC = 10
    REQUIRE(control.segments()[2].entries[0].value.integer == 120);
    REQUIRE(CaseControl::ownCommand(control.segments()[1], "StaticLoadSelection")->value.integer == 55);
    REQUIRE(CaseControl::ownCommand(control.segments()[1], "EigenMethodSelection")->value.integer == 3);
    const std::string text = testing::caseText(control);
    REQUIRE(text.find("SinglePointConstrSelection;110\n") != std::string::npos);
    REQUIRE(text.find("SinglePointConstrSelection;120\n") != std::string::npos);
    REQUIRE(text.find("StaticLoadSelection;55\n") != std::string::npos);
    REQUIRE(text.find("EigenMethodSelection;3\n") != std::string::npos);
    REQUIRE(text.find("DisplacementOutput;All;PlotOnly\n") != std::string::npos);
    // The SET of IDs is a SET of the Case Control, not a Bulk Data set: left alone.
    REQUIRE(text.find("SetDefinition;5;1;Through;4;9\n") != std::string::npos);

    Renumbering::apply(model, RenumberSpace::Method, Renumbering::offset(7));
    REQUIRE(CaseControl::ownCommand(model.control().caseControl.segments()[1], "EigenMethodSelection")->value.integer ==
            10);
}

TEST_CASE("a failed renumbering leaves the Case Control as it was", "[control][model][references]")
{
    Model model = readModelText(DECK);
    const std::string before = testing::caseText(model.control().caseControl);
    const auto collapse = Renumbering::fromFunction([](base::EntityId) -> std::optional<base::EntityId> { return 7; });
    REQUIRE_THROWS(Renumbering::apply(model, RenumberSpace::ConstraintSet, collapse));
    REQUIRE(testing::caseText(model.control().caseControl) == before);
}

TEST_CASE("the reference check reports Case Control commands that name a set that does not exist", "[control][model]")
{
    Model model = readModelText(DECK);
    base::DiagnosticSink clean;
    REQUIRE(ReferenceChecker::check(model, clean) == 0);

    const CommandDefinition& spc = *CommandCatalogue::instance().findByName("SinglePointConstrSelection");
    CaseControl& control = model.control().caseControl;
    control.setCommand(control.segments()[1], spc, "", {}, CaseValue::ofInteger(77));
    base::DiagnosticSink sink;
    REQUIRE(ReferenceChecker::check(model, sink) == 1);
    REQUIRE(sink.diagnostics().size() == 1);
    REQUIRE(sink.diagnostics()[0].severity == base::Severity::Warning);
    REQUIRE(sink.diagnostics()[0].code == "model.unresolved-case-control-reference");
    REQUIRE(sink.diagnostics()[0].message.find("SinglePointConstrSelection of SubcaseStart 1 refers to 77") !=
            std::string::npos);
}

TEST_CASE("a copy of a model has its own Case Control", "[control][model]")
{
    Model model = readModelText(DECK);
    Model copy = model.copy();
    CaseControl& control = copy.control().caseControl;
    control.setCommand(control.global(), *CommandCatalogue::instance().findByName("SinglePointConstrSelection"), "", {},
                       CaseValue::ofInteger(99));
    REQUIRE(
        CaseControl::ownCommand(model.control().caseControl.global(), "SinglePointConstrSelection")->value.integer ==
        10);
    REQUIRE(CaseControl::ownCommand(copy.control().caseControl.global(), "SinglePointConstrSelection")->value.integer ==
            99);
}

TEST_CASE("an Include in the case section survives reading and writing", "[control][model][include]")
{
    const oor::testing::TestFiles files;
    const std::string included =
        "SubcaseStart;2\n# inside the include\nStaticLoadSelection;4\nDisplacementOutput;All\n";
    files.write("main.oos",
                "[executive]\nSolutionSequence;101\n[case]\nTitleText;with include\nSubcaseStart;1\n"
                "SinglePointConstrSelection;1\nInclude;more.oos\nSubcaseStart;3\nStaticLoadSelection;5\n"
                "[model]\nNode;1;;0.;0.;0.\n");
    files.write("more.oos", included);
    const Model model = ModelFileReader::read(files.root() / "main.oos", ModelReadOptions{});
    const CaseControl& control = model.control().caseControl;
    REQUIRE(control.segments().size() == 4);
    REQUIRE(control.segments()[2].id == 2);
    REQUIRE(control.segments()[1].entries.size() == 2);
    REQUIRE(control.segments()[1].entries[1].kind == EntryKind::IncludeBegin);
    REQUIRE(control.segments()[2].entries[1].name == "DisplacementOutput");
    REQUIRE(control.segments()[2].entries.size() == 3); // LOAD, DISP and the end of the included file
    REQUIRE(control.segments()[2].entries.back().kind == EntryKind::IncludeEnd);

    // One file: the included records are in the text.
    const std::string flat = writeModelText(model);
    REQUIRE(flat.find("SinglePointConstrSelection;1\n" + included + "SubcaseStart;3\n") != std::string::npos);

    // With the Include structure: the Include record in the main file, the records in the included file.
    ModelFileWriteOptions options;
    options.splitIncludes = true;
    const std::filesystem::path outDirectory = files.root() / "out";
    ModelFileWriter::write(model, outDirectory / "main.oos", options);
    const auto readFile = [](const std::filesystem::path& path)
    {
        std::ifstream stream(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
    };
    const std::string main = readFile(outDirectory / "main.oos");
    REQUIRE(main.find("SinglePointConstrSelection;1\nInclude;more.oos\nSubcaseStart;3\nStaticLoadSelection;5\n") !=
            std::string::npos);
    REQUIRE(readFile(outDirectory / "more.oos") == included);
}
