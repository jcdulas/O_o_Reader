#include "oor/base/errors/Errors.hpp"
#include "oor/model/control/executive_control/SolutionSequences.hpp"
#include "support/ControlTestSupport.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace oor;
using namespace oor::model;
using oor::testing::readModelText;
using oor::testing::writeModelText;

// The executive section of a native model: its statements read from a model file, written back, edited and copied.

namespace
{

const char* const DECK =
    "[executive]\n"
    "# banner\n"
    "JobIdentifier;PLATE;EXAMPLE\n"
    "SolutionSequence;101\n"
    "TimeLimit;10;20\n"
    "DiagnosticSelection;8;53\n"
    "ApplicationKind;HeatTransferApplication\n"
    "[case]\n"
    "TitleText;x\n"
    "[model]\n"
    "Node;1;;0.;0.;0.\n";

std::string textOf(const Model& model)
{
    return testing::executiveText(model.control().executive);
}

// The statements of a section as names: DMAP text, Include markers and raw text as <dmap>, <include>, <raw NAME>.
std::vector<std::string> namesOf(const ControlStatementList& list)
{
    std::vector<std::string> names;
    for (const ControlStatement& statement : list.entries())
    {
        switch (statement.kind)
        {
        case ControlKind::Comment:
        case ControlKind::IncludeEnd:
            break;
        case ControlKind::IncludeBegin:
            names.push_back("<include>");
            break;
        case ControlKind::Raw:
            names.push_back("<raw " + statement.name + ">");
            break;
        default:
            names.push_back(statement.name);
            break;
        }
    }
    return names;
}

using Names = std::vector<std::string>;

// A model with this executive section (its records) and nothing else.
Model readExecutive(const std::string& executive)
{
    return readModelText("[executive]\n" + executive + "[model]\nNode;1;;0.;0.;0.\n");
}

} // namespace

TEST_CASE("an executive section is read into typed statements", "[control][executive]")
{
    const Model model = readModelText(DECK);
    const ExecutiveControl& control = model.control().executive;
    REQUIRE(model.diagnostics().diagnostics().empty());
    REQUIRE(namesOf(control) ==
            Names{"JobIdentifier", "SolutionSequence", "TimeLimit", "DiagnosticSelection", "ApplicationKind"});
    REQUIRE(control.findLast("JobIdentifier")->words == std::vector<std::string>{"PLATE", "EXAMPLE"});
    REQUIRE(control.findLast("SolutionSequence")->words == std::vector<std::string>{"101"});
    REQUIRE(control.solutionNumber() == 101);
    REQUIRE(control.findLast("TimeLimit")->words == std::vector<std::string>{"10", "20"});
    REQUIRE(control.diagnosticSelectionNumbers() == std::vector<std::int64_t>{8, 53});
    REQUIRE(control.findLast("ApplicationKind")->parameters.front().name == "HeatTransferApplication");
}

TEST_CASE("an executive section goes through a model file unchanged", "[control][executive]")
{
    const Model model = readModelText(DECK);
    REQUIRE(writeModelText(model) == DECK);
    const Model again = readModelText(writeModelText(model));
    REQUIRE(textOf(again) == textOf(model));
    REQUIRE(namesOf(again.control().executive) == namesOf(model.control().executive));
}

TEST_CASE("DIAG statements are replaced by one", "[control][executive]")
{
    Model model = readExecutive("SolutionSequence;101\nDiagnosticSelection;8\nDiagnosticSelection;12;13\n");
    ExecutiveControl& control = model.control().executive;
    control.setDiagnosticSelection({8, 12, 13});
    REQUIRE(namesOf(control) == Names{"SolutionSequence", "DiagnosticSelection"});
    REQUIRE(textOf(model).find("SolutionSequence;101\nDiagnosticSelection;8;12;13\n") == 0);
    control.setDiagnosticSelection({});
    REQUIRE(namesOf(control) == Names{"SolutionSequence"});
}

TEST_CASE("values that cannot be written are refused", "[control][executive]")
{
    Model model;
    ExecutiveControl& control = model.control().executive;
    REQUIRE_THROWS_AS(control.setJobIdentifier({}), base::UsageError);
    REQUIRE_THROWS_AS(control.setJobIdentifier({"A", "B", "C"}), base::UsageError);
    REQUIRE_THROWS_AS(control.setSolution(""), base::UsageError);
    REQUIRE_THROWS_AS(control.setSolution("101", "a,b"), base::UsageError);
    REQUIRE_THROWS_AS(control.setTimeLimit({"ten"}), base::UsageError);
    REQUIRE_THROWS_AS(control.setTimeLimit({}), base::UsageError);
    REQUIRE_THROWS_AS(control.addKeyword("SolutionSequence", {}), base::UsageError);
    REQUIRE_THROWS_AS(control.addKeyword("ExecutiveEnd", {}), base::UsageError);
    REQUIRE_THROWS_AS(control.addKeyword("NOSUCH", {}), base::UsageError);
    REQUIRE(control.isEmpty()); // a refused call leaves nothing behind
}

TEST_CASE("a model built from nothing writes its executive control as neutral records", "[control][executive]")
{
    Model model;
    model.control().executive.setSolution("101");
    model.control().executive.setTimeLimit({"10"});
    REQUIRE(model.control().executive.entries().size() == 2);
    REQUIRE(writeModelText(model) == "[executive]\nSolutionSequence;101\nTimeLimit;10\n");
    const Model again = readModelText(writeModelText(model));
    REQUIRE(again.control().executive.solutionNumber() == 101);
    REQUIRE(again.control().executive.findLast("TimeLimit")->words == std::vector<std::string>{"10"});
}

TEST_CASE("copying a model copies its executive control", "[control][executive]")
{
    const Model model = readModelText(DECK);
    Model copy = model.copy();
    copy.control().executive.setSolution("103");
    REQUIRE(model.control().executive.solutionNumber() == 101);
    REQUIRE(copy.control().executive.solutionNumber() == 103);
}

TEST_CASE("replacing a section with a copy gives new keys", "[control][executive]")
{
    const Model model = readModelText(DECK);
    ExecutiveControl other;
    other.replaceWith(model.control().executive);
    REQUIRE(namesOf(other) == namesOf(model.control().executive));
    REQUIRE(other.entries().size() == model.control().executive.entries().size());
}
