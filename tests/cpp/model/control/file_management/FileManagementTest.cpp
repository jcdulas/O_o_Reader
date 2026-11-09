#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace oor;
using namespace oor::model;
using oor::testing::readModelText;
using oor::testing::writeModelText;

TEST_CASE("a model built from nothing writes file assignments before executive records",
          "[control][file-management][executive]")
{
    Model model;
    model.control().executive.setTimeLimit({"10"});
    model.control().executive.setSolution("101");
    model.control().fileManagement.addKeyword(
        "FileAssignment",
        {StatementParameter::withQuotedValue("RESULTS", "x.bin"), StatementParameter::withValue("FileUnit", "12")});
    REQUIRE(writeModelText(model) ==
            "[executive]\nFileAssignment;RESULTS='x.bin';FileUnit=12\nSolutionSequence;101\nTimeLimit;10\n");
}

TEST_CASE("copying a model copies its File Management", "[control][file-management]")
{
    const Model model = readModelText(
        "[executive]\nFileAssignment;RESULTS='x.bin';FileUnit=12\nSolutionSequence;101\n[model]\n"
        "Node;1;;0.;0.;0.\n");
    REQUIRE(model.control().fileManagement.findAll("FileAssignment").size() == 1);
    Model copy = model.copy();
    REQUIRE(copy.control().fileManagement.findAll("FileAssignment").size() == 1);
    copy.control().fileManagement.findLast("FileAssignment")->parameters[0].value = "other.bin";
    REQUIRE(model.control().fileManagement.findLast("FileAssignment")->parameters[0].value == "x.bin");
    REQUIRE(copy.control().fileManagement.findLast("FileAssignment")->parameters[0].value == "other.bin");
}
