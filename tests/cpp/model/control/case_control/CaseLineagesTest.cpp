#include "oor/model/control/case_control/CaseControl.hpp"
#include "oor/model/control/case_control/CaseLineages.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;

namespace
{

// The case section of a model file given as its records.
CaseControl parse(const std::string& text)
{
    return testing::readModelText("[case]\n" + text, true).control().caseControl;
}

void collectSections(const std::vector<Subcase>& sections, std::vector<const Subcase*>& found)
{
    for (const Subcase& section : sections)
    {
        found.push_back(&section);
        collectSections(section.children, found);
    }
}

// Global commands, subcases with steps and substeps, and two auxiliary models.
const char* const TEXT =
    "SinglePointConstrSelection;1\n"
    "SetDefinition;5;1;Through;10\n"
    "DisplacementOutput;All\n"
    "SubcaseStart;1\n"
    "StaticLoadSelection;10\n"
    "StepStart;1\n"
    "NonlinearStaticParamsSelection;11\n"
    "SubstepStart;1\n"
    "NonlinearStaticParamsSelection;12\n"
    "StressOutput;5\n"
    "SubstepStart;2\n"
    "StepStart;2\n"
    "StaticLoadSelection;4\n"
    "SubcaseStart;2\n"
    "SetDefinition;5;20\n"
    "AuxiliaryCase\n"
    "AuxiliaryModelSelection;5\n"
    "SubcaseStart;1\n"
    "StaticLoadSelection;30\n"
    "StepStart;1\n"
    "SubcaseStart;2\n"
    "AuxiliaryCase\n"
    "AuxiliaryModelSelection;7\n"
    "SubcaseStart;3\n";

} // namespace

TEST_CASE("the lineages made in one walk are those CaseControl finds section by section", "[control][lineage]")
{
    const CaseControl control = parse(TEXT);
    std::vector<const Subcase*> sections;
    collectSections(control.segments(), sections);
    REQUIRE(sections.size() == 13);
    const CaseLineages lineages(control);
    for (const Subcase* section : sections)
    {
        const std::vector<const Subcase*> expected = control.lineage(section->key);
        const std::span<const Subcase* const> lineage = lineages.of(section->key);
        REQUIRE(std::vector<const Subcase*>(lineage.begin(), lineage.end()) == expected);
        REQUIRE(lineages.auxiliaryCase(section->key) == control.auxiliaryCaseOf(section->key));
        for (const char* name : {"SinglePointConstrSelection", "StaticLoadSelection", "NonlinearStaticParamsSelection",
                                 "AuxiliaryModelSelection"})
            REQUIRE(lineages.command(section->key, name) == control.effectiveCommand(section->key, name));
        for (const char* name : {"DisplacementOutput", "StressOutput"})
            REQUIRE(lineages.outputs(section->key, name) == control.effectiveOutputs(section->key, name));
        REQUIRE(lineages.set(section->key, 5) == control.effectiveSet(section->key, 5));
    }
}

TEST_CASE("the lineages follow the inheritance of the Case Control", "[control][lineage]")
{
    const CaseControl control = parse(TEXT);
    const std::vector<Subcase>& segments = control.segments();
    const CaseLineages lineages(control);
    const Subcase& substep = segments[1].children[0].children[0];
    REQUIRE(lineages.of(substep.key).size() == 4); // the substep, its step, its subcase, the global section
    REQUIRE(lineages.command(substep.key, "NonlinearStaticParamsSelection")->value.integer == 12);
    REQUIRE(lineages.command(substep.key, "StaticLoadSelection")->value.integer == 10);
    REQUIRE(lineages.command(substep.key, "SinglePointConstrSelection")->value.integer == 1);
    REQUIRE(lineages.set(segments[2].key, 5) != lineages.set(segments[1].key, 5)); // the subcase's own SET 5
    // an auxiliary subcase sees its AUXCASE section and stops: no global SPC, its own AUXMODEL
    REQUIRE(segments[3].kind == SubcaseKind::AuxiliaryCase);
    REQUIRE(lineages.command(segments[4].key, "SinglePointConstrSelection") == nullptr);
    REQUIRE(lineages.command(segments[4].children[0].key, "AuxiliaryModelSelection")->value.integer == 5);
    REQUIRE(lineages.auxiliaryCase(segments[4].children[0].key) == &segments[3]);
    REQUIRE(lineages.auxiliaryCase(segments[7].key) == &segments[6]);
    REQUIRE(lineages.auxiliaryCase(segments[1].key) == nullptr);
    REQUIRE(lineages.of(segments[0].key).size() == 1);
}

TEST_CASE("an unknown key has an empty lineage and nothing effective", "[control][lineage]")
{
    const CaseControl control = parse(TEXT);
    const CaseLineages lineages(control);
    REQUIRE(lineages.of(999'999).empty());
    REQUIRE(lineages.command(999'999, "SinglePointConstrSelection") == nullptr);
    REQUIRE(lineages.outputs(999'999, "DisplacementOutput").empty());
    REQUIRE(lineages.set(999'999, 5) == nullptr);
    REQUIRE(lineages.auxiliaryCase(999'999) == nullptr);
}
