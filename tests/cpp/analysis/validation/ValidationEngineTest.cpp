#include "oor/analysis/validation/engine/CheckRegistry.hpp"
#include "oor/analysis/validation/engine/ValidationEngine.hpp"
#include "oor/analysis/validation/engine/ValidationError.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/base/threading/ProgressCounter.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::analysis;
using oor::testing::readModelText;

namespace
{

// A deck with a finding in several families: a reference that does not resolve, a zero load, a material that is not
// stable, a Case Control command that selects a set that does not exist, and a rigid element loop.
const char* const MIXED_DECK = R"([executive]
SolutionSequence;101
[case]
SinglePointConstrSelection;1
StaticLoadSelection;77
SubcaseStart;1
DisplacementOutput;All
[model]
IsoMat;1;2.1E+5;;0.5;7.8E-9
ShellProp;1;1;0.1;1
Node;1;;0.;0.;0.
Node;2;;1.;0.;0.
Node;3;;1.;1.;0.
Node;4;;0.;1.;0.
ShellQuad4;10;9;1;2;3;4
SinglePointConstrList;1;123456;3
PointForceLoad;1;3;;0.;0.;0.;1.
RigidBodyStar;60;1;123456;2
RigidBodyStar;61;2;123456;1
)";

std::vector<std::string> texts(const ValidationReport& report)
{
    std::vector<std::string> lines;
    for (const ValidationFinding& finding : report.findings)
        lines.push_back(finding.toString());
    return lines;
}

std::set<std::string> codes(const ValidationReport& report)
{
    std::set<std::string> found;
    for (const ValidationFinding& finding : report.findings)
        found.insert(finding.code);
    return found;
}

} // namespace

TEST_CASE("the checks are registered once, by family, under codes that start with the prefix of the family",
          "[validation]")
{
    const CheckRegistry& registry = CheckRegistry::instance();
    REQUIRE(registry.checks().size() >= 80);
    std::set<std::string_view> seen;
    std::size_t lastFamily = 0;
    for (const auto& check : registry.checks())
    {
        const CheckInfo& info = check->info();
        REQUIRE(seen.insert(info.code).second);
        const std::size_t family = CheckRegistry::familyIndex(info.family);
        REQUIRE(family < CheckRegistry::families().size());
        REQUIRE(family >= lastFamily);
        lastFamily = family;
        const std::string prefix = std::string(CheckRegistry::families()[family].prefix) + ".";
        REQUIRE(info.code.starts_with(prefix));
        REQUIRE_FALSE(info.description.empty());
        REQUIRE(registry.find(info.code) == check.get());
    }
    REQUIRE(registry.find("nothing.here") == nullptr);
    // The contact, design and aeroelastic cards are typed and have their checks.
    for (const std::string_view family : {"contact", "design", "aero"})
    {
        REQUIRE(std::any_of(registry.checks().begin(), registry.checks().end(),
                            [family](const auto& check) { return check->info().family == family; }));
    }
}

TEST_CASE("a name matches a check by its code, its family or the prefix of its code", "[validation]")
{
    const CheckRegistry& registry = CheckRegistry::instance();
    const CheckInfo& info = registry.find("ref.unresolved")->info();
    REQUIRE(CheckRegistry::matches("ref.unresolved", info));
    REQUIRE(CheckRegistry::matches("references", info));
    REQUIRE(CheckRegistry::matches("ref", info));
    REQUIRE(CheckRegistry::matches("all", info));
    REQUIRE_FALSE(CheckRegistry::matches("load", info));
    REQUIRE_FALSE(CheckRegistry::matches("ref.", info));
    REQUIRE(registry.isKnownName("constraints"));
    REQUIRE(registry.isKnownName("constraint"));
    REQUIRE(registry.isKnownName("aero"));
    REQUIRE(registry.isKnownName("all"));
    REQUIRE_FALSE(registry.isKnownName("Loads"));
    REQUIRE_FALSE(registry.isKnownName(""));
}

TEST_CASE("select and ignore choose the checks, and the opt-in checks need their own code or all", "[validation]")
{
    const auto codesOf = [](const ValidationOptions& options)
    {
        std::set<std::string> found;
        for (const ValidationCheck* check : ValidationEngine::select(options))
            found.insert(std::string(check->info().code));
        return found;
    };
    ValidationOptions options;
    const std::set<std::string> defaults = codesOf(options);
    REQUIRE(defaults.contains("ref.unresolved"));
    REQUIRE_FALSE(defaults.contains("geometry.coincident"));
    REQUIRE_FALSE(defaults.contains("ref.orphan-node"));

    options.select = {"references"};
    REQUIRE(codesOf(options) == std::set<std::string>{"ref.unresolved", "ref.duplicate-id"});
    options.select = {"ref.orphan-node"};
    REQUIRE(codesOf(options) == std::set<std::string>{"ref.orphan-node"});
    options.select = {"geometry"};
    REQUIRE(codesOf(options).empty()); // the family of an opt-in check does not run it
    options.select = {"all"};
    REQUIRE(codesOf(options).contains("geometry.coincident"));
    options.ignore = {"geometry", "load"};
    REQUIRE_FALSE(codesOf(options).contains("geometry.coincident"));
    REQUIRE_FALSE(codesOf(options).contains("load.zero-gravity"));
    options.select = {"loads"};
    options.ignore = {"load.zero-gravity"};
    REQUIRE(codesOf(options).contains("load.zero-magnitude"));
    REQUIRE_FALSE(codesOf(options).contains("load.zero-gravity"));

    options.select = {"refs"};
    REQUIRE_THROWS_AS(ValidationEngine::select(options), base::UsageError);
    options.select.clear();
    options.ignore = {"typo"};
    REQUIRE_THROWS_AS(ValidationEngine::select(options), base::UsageError);

    REQUIRE(ValidationEngine::defaultMinimum({}) == base::Severity::Warning);
    REQUIRE(ValidationEngine::defaultMinimum({"ref.unresolved"}) == base::Severity::Warning);
    REQUIRE(ValidationEngine::defaultMinimum({"solution.no-output-request"}) == base::Severity::Info);
}

TEST_CASE("the findings are stamped by the check, sorted errors first, and carry the card", "[validation]")
{
    const model::Model model = readModelText(MIXED_DECK);
    ValidationOptions options;
    const ValidationReport report = ValidationEngine::run(model, options);
    REQUIRE(codes(report) == std::set<std::string>{"ref.unresolved", "compat.iso-mat-poisson", "load.zero-magnitude",
                                                   "constraint.rigid-loop", "case.unresolved-reference"});
    // Errors, then warnings.
    bool seenWarning = false;
    for (const ValidationFinding& finding : report.findings)
    {
        if (finding.severity == base::Severity::Warning)
            seenWarning = true;
        else
            REQUIRE_FALSE(seenWarning);
    }
    const ValidationFinding& first = report.findings.front();
    REQUIRE(first.code == "ref.unresolved");
    REQUIRE(first.family == "references");
    // The finding holds the names of the code; its text, the keywords of the deck.
    REQUIRE(first.cardName == "ShellQuad4");
    REQUIRE(first.cardId == 10);
    REQUIRE(first.field == "propId");
    REQUIRE(first.hasCardKey());
    REQUIRE(first.part.empty());
    REQUIRE_THAT(first.toString(),
                 Catch::Matchers::ContainsSubstring("error[ref.unresolved] ShellQuad4 10, field propId"));
    REQUIRE(report.errorCount() == 4);
    REQUIRE(report.warningCount() == 2);
    REQUIRE(report.infoCount() == 0);
    // A Case Control finding names its statement and has no card key.
    const auto caseFinding =
        std::find_if(report.findings.begin(), report.findings.end(),
                     [](const ValidationFinding& finding) { return finding.code == "case.unresolved-reference"; });
    REQUIRE(caseFinding != report.findings.end());
    REQUIRE(caseFinding->cardName == "global"); // the LOAD command is above the first SUBCASE
    REQUIRE_FALSE(caseFinding->hasCardKey());

    options.minSeverity = base::Severity::Error;
    REQUIRE(ValidationEngine::run(model, options).warningCount() == 0);
}

TEST_CASE("the report is the same for any number of threads", "[validation]")
{
    const model::Model model = readModelText(MIXED_DECK);
    ValidationOptions options;
    options.select = {"all"};
    options.minSeverity = base::Severity::Info;
    const std::vector<std::string> reference = texts(ValidationEngine::run(model, options));
    REQUIRE(reference.size() >= 6);
    for (const std::size_t workers : {0U, 1U, 3U, 8U})
    {
        base::ThreadPool pool(workers);
        base::ProgressCounter progress;
        REQUIRE(texts(ValidationEngine::run(model, options, &pool, &progress)) == reference);
        const base::ProgressCounter::Snapshot state = progress.snapshot();
        REQUIRE(state.stage == "validation");
        REQUIRE(state.done == state.total);
        REQUIRE(state.total > 0);
    }
}

TEST_CASE("raiseIfErrors throws a ValidationError that carries the report", "[validation]")
{
    const model::Model bad = readModelText(MIXED_DECK);
    const ValidationReport report = ValidationEngine::run(bad, {});
    try
    {
        ValidationEngine::raiseIfErrors(report);
        FAIL("an error was expected");
    }
    catch (const ValidationError& error)
    {
        REQUIRE(error.report().findings.size() == report.findings.size());
        REQUIRE_THAT(std::string(error.what()), Catch::Matchers::ContainsSubstring("validation found 4 errors"));
    }
    ValidationReport warningsOnly;
    ValidationFinding warning;
    warning.severity = base::Severity::Warning;
    warningsOnly.findings.push_back(warning);
    REQUIRE_NOTHROW(ValidationEngine::raiseIfErrors(warningsOnly));
}

TEST_CASE("each part is checked with its own IDs and its findings name it", "[validation]")
{
    const std::string deck =
        std::string(MIXED_DECK) +
        "[model;SuperelementPart;3]\nIsoMat;1;-5.;;0.3\nNode;1;;0.;0.;0.\nNode;2;;1.;0.;0.\nRod2;5;3;1;2\n";
    const model::Model model = readModelText(deck);
    const ValidationReport report = ValidationEngine::run(model, {});
    std::vector<std::string> inPart;
    for (const ValidationFinding& finding : report.findings)
    {
        if (finding.part == "SuperelementPart 3")
            inPart.push_back(finding.code);
    }
    // The part has a material with a negative modulus, and a rod that names a property and no grid points of its own...
    // no: the grid points are its own, the property is the residual structure's (PSHELL 1 only: PROD 3 does not exist
    // anywhere).
    REQUIRE(std::find(inPart.begin(), inPart.end(), "compat.iso-mat-modulus") != inPart.end());
    REQUIRE(std::find(inPart.begin(), inPart.end(), "ref.unresolved") != inPart.end());
    // The residual structure's findings do not carry a part.
    REQUIRE(report.findings.front().part.empty());
}

TEST_CASE("ReferenceChecker::scan shows every reference field, with whether it resolves, and does not change the model",
          "[validation]")
{
    const model::Model model = readModelText(MIXED_DECK);
    std::size_t references = 0;
    std::size_t unresolved = 0;
    std::size_t caseControl = 0;
    bool sawPid = false;
    model::ReferenceChecker::scan(model,
                                  [&](const model::ReferenceUse& use)
                                  {
                                      ++references;
                                      unresolved += use.resolved ? 0U : 1U;
                                      caseControl += use.inCaseControl ? 1U : 0U;
                                      if (!use.inCaseControl && use.field == "propId" && use.id == 9)
                                      {
                                          sawPid = true;
                                          REQUIRE_FALSE(use.resolved);
                                          REQUIRE(use.target == model::ReferenceTarget::Property);
                                      }
                                  });
    REQUIRE(sawPid);
    REQUIRE(unresolved == 2);  // the property 9 and the LOAD 77
    REQUIRE(caseControl == 2); // SPC = 1 and LOAD = 77
    REQUIRE(references > caseControl);
    REQUIRE(model.cardCount() > 0);
}

TEST_CASE("fatigue event labels do not inherit fixed-field character restrictions", "[validation]")
{
    const model::Model model = readModelText(
        "FatigueEvent;5;1;1;Name;1StartsWithDigit\n"
        "FatigueEvent;6;1;1;Name;ABCDEFGH1Continues\n",
        true);
    REQUIRE_FALSE(model.diagnostics().hasErrors());
    ValidationOptions options;
    options.select = {"compatibility"};
    const ValidationReport report = ValidationEngine::run(model, options);
    for (const ValidationFinding& finding : report.findings)
        CHECK(finding.code != "compat.fatigue-event-name");
}
