#include "oor/deck/records/RecordText.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/matrices/directMatrix/DirectMatrixTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/core/ModelFileReader.hpp"
#include "oor/model/core/ModelFileWriter.hpp"
#include "oor/model/core/UnknownCardTable.hpp"
#include "support/ModelTestSupport.hpp"
#include "support/TestFiles.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <string>

using namespace oor;
using namespace oor::model;
using Catch::Matchers::ContainsSubstring;

namespace
{

// A model file with the three sections, comments, a part, alternatives and a repeated group.
const std::string MODEL_TEXT =
    "# a plate under load\n"
    "[executive]\n"
    "SolutionSequence;LinearStatics\n"
    "TimeLimit;10\n"
    "[case]\n"
    "TitleText;\"Plate; under load\"\n"
    "DisplacementOutput;All;PrintListing;PlotOnly\n"
    "SetDefinition;3;1;2;5;Through;10\n"
    "SubcaseParameter;PostprocessingFile;-1\n"
    "# the first load case\n"
    "SubcaseStart;1\n"
    "StaticLoadSelection;10\n"
    "SubcaseStart;2\n"
    "StaticLoadSelection;20\n"
    "[model]\n"
    "# the corners\n"
    "Node;1;;0.;0.;0.\n"
    "Node;2;;1.;0.;0.\n"
    "Node;3;;1.;1.;0.\n"
    "Node;4;;0.;1.;0.\n"
    "ShellQuad4;12;101;1;2;3;4;;7\n"
    "ShellQuad4;13;101;1;2;3;4;30.;;0.01\n"
    "ShellProp;101;1;0.002\n"
    "LoadCombination;10;1.;0.5;1;2.;2\n"
    "[model;SuperelementPart;5]\n"
    "Node;100;;5.;0.;0.\n";

Model readText(const std::string& text, bool strict = false)
{
    base::DiagnosticSink lexing;
    ModelReadOptions options;
    options.strict = strict;
    Model model = ModelFileReader::fromItems(deck::ModelFileLexer::readText(text, {}, lexing), options);
    for (const base::Diagnostic& diagnostic : lexing.diagnostics())
        model.diagnostics().report(diagnostic);
    return model;
}

std::string errorsOf(const Model& model)
{
    std::string errors;
    for (const base::Diagnostic& diagnostic : model.diagnostics().diagnostics())
        errors += diagnostic.toString() + "\n";
    return errors;
}

} // namespace

TEST_CASE("a model file is read into the card tables, the control sections and the parts", "[model][model-file]")
{
    const Model model = readText(MODEL_TEXT, true);
    INFO(errorsOf(model));
    REQUIRE_FALSE(model.diagnostics().hasErrors());
    CHECK(model.cardCount() == 8);
    const auto* quads = model.findTable<ShellQuad4Table>();
    REQUIRE(quads != nullptr);
    // the alternatives: a material frame on the first, an angle on the second
    CHECK(quads->rows().rowCount() == 2);
    const std::string written = ModelFileWriter::toString(model);
    CHECK_THAT(written, ContainsSubstring("ShellQuad4;12;101;1;2;3;4;;7\n"));
    CHECK_THAT(written, ContainsSubstring("ShellQuad4;13;101;1;2;3;4;30.;;0.01\n"));
    CHECK(model.commentOf(model.findTable<NodeTable>()->rows().keyAt(0)) == " the corners");
    REQUIRE(model.parts().size() == 1);
    CHECK(model.parts()[0]->partInfo()->kind == "SuperelementPart");
    CHECK(model.parts()[0]->partInfo()->id == 5);
    CHECK(model.parts()[0]->cardCount() == 1);

    const ControlSections& control = model.control();
    CHECK(control.executive.solutionNumber() == 101);
    const CaseControl& caseControl = control.caseControl;
    REQUIRE(caseControl.segments().size() == 3);
    const Subcase& global = caseControl.global();
    REQUIRE(global.entries.size() == 4);
    CHECK(global.entries[0].value.text == "Plate; under load");
    CHECK(global.entries[1].kind == EntryKind::Output);
    CHECK(global.entries[1].options == std::vector<std::string>{"PrintListing", "PlotOnly"});
    CHECK(global.entries[2].set.integers == std::vector<std::int64_t>{1, 2, 5, 6, 7, 8, 9, 10});
    CHECK(global.entries[3].variant == "PostprocessingFile");
    CHECK(global.entries[3].value.integer == -1);
    CHECK(caseControl.segments()[1].id == 1);
    CHECK(caseControl.segments()[1].header.leadingComments == " the first load case");
    CHECK(caseControl.segments()[2].entries.at(0).value.integer == 20);
}

TEST_CASE("a model file written from a model read from one is the same text", "[model][model-file]")
{
    const Model model = readText(MODEL_TEXT, true);
    const std::string written = ModelFileWriter::toString(model);
    CHECK(written == MODEL_TEXT);
}

TEST_CASE("a record that does not fit its card is reported with the card and the line", "[model][model-file]")
{
    const Model model = readText("[model]\nNode;1;;0.;0.;0.;0;0;0;9\nShellQuad4;1;1;1;2;3;4;30.;7\n");
    const std::string errors = errorsOf(model);
    CHECK_THAT(errors, ContainsSubstring("Node has 8 fields"));
    CHECK_THAT(errors, ContainsSubstring("materialAngle and matFrameId are alternatives"));
    CHECK_THAT(errors, ContainsSubstring(":2"));
    CHECK(model.cardCount() == 0);
}

TEST_CASE("a card the library does not know is kept as it is", "[model][model-file]")
{
    const Model model = readText("[model]\nMYCARD;1;;x\n");
    REQUIRE(model.cardCount() == 1);
    CHECK(ModelFileWriter::toString(model) == "[model]\nMYCARD;1;;x\n");
    const std::string text =
        "[model]\nAVeryLongCardName;1\n\"Plugin card;Name\";payload\n\"#Future\";x\n\"[Future]\";y\n\" Node "
        "\";z\nnode;1\n";
    const Model unknown = readText(text);
    REQUIRE(unknown.cardCount() == 6);
    const auto* table = unknown.findTable<UnknownCardTable>();
    REQUIRE(table != nullptr);
    CHECK(table->nameAt(1).view() == "Plugin card;Name");
    CHECK(table->nameAt(4).view() == " Node ");
    CHECK_FALSE(unknown.locate(base::IdSpace::Node, 1).has_value());
    CHECK(ModelFileWriter::toString(unknown) == text);
    CHECK(ModelFileWriter::toString(readText(ModelFileWriter::toString(unknown))) == text);
    CHECK_FALSE(deck::CardName::fromText("Bad\tName").has_value());
}

TEST_CASE("a model file keeps its Include structure when it is written to files", "[model][model-file]")
{
    const testing::TestFiles files;
    files.write("in/mesh.oos", "# the mesh\nNode;2;;1.;0.;0.\n");
    const auto main = files.write("in/main.oos", "[model]\nNode;1;;0.;0.;0.\nInclude;mesh.oos\nNode;3;;2.;0.;0.\n");
    const Model model = ModelFileReader::read(main, {});
    REQUIRE_FALSE(model.diagnostics().hasErrors());
    CHECK(model.cardCount() == 3);
    ModelFileWriteOptions options;
    options.splitIncludes = true;
    ModelFileWriter::write(model, files.root() / "out" / "main.oos", options);
    CHECK(files.read("out/main.oos") == "[model]\nNode;1;;0.;0.;0.\nInclude;mesh.oos\nNode;3;;2.;0.;0.\n");
    CHECK(files.read("out/mesh.oos") == "# the mesh\nNode;2;;1.;0.;0.\n");
    CHECK(ModelFileWriter::toString(model) ==
          "[model]\nNode;1;;0.;0.;0.\n# the mesh\nNode;2;;1.;0.;0.\nNode;3;;2.;0.;0.\n");
}

TEST_CASE("a file assignment goes through a model file in the executive section", "[model][model-file]")
{
    const std::string text =
        "[executive]\n"
        "FileAssignment;INPUTT4='matrices.op4';FileUnit=12;FormattedFile\n"
        "SolutionSequence;101\n"
        "[model]\n"
        "Node;1;;0.;0.;0.\n";
    const Model model = readText(text, true);
    INFO(errorsOf(model));
    REQUIRE(model.control().fileManagement.findAll("FileAssignment").size() == 1);
    CHECK(model.control().fileManagement.findAll("FileAssignment").front()->parameters.size() == 3);
    CHECK(ModelFileWriter::toString(model) == text);
}

TEST_CASE("model numeric fields refuse implicit and nonstandard exponents", "[model][model-file][numbers]")
{
    for (const char* value : {"1.-3", "1.5D3", "2.0+5"})
    {
        const Model model = readText(std::string("Node;1;;") + value + ";0.;0.\n");
        REQUIRE(model.diagnostics().hasErrors());
        CHECK(model.diagnostics().firstError()->code == "card.bad-real");
        CHECK(model.diagnostics().firstError()->location.cardName == "Node");
    }
}

TEST_CASE("model records preserve surrounding spaces in typed literal names", "[model][model-file]")
{
    const Model model = readText("DirectMatrix;\"  User matrix  \";0;2;1;0;1;1\n", true);
    const std::string name = "  User matrix  ";
    REQUIRE(model.findTable<DirectMatrixTable>() != nullptr);
    REQUIRE(model.findTable<DirectMatrixTable>()->card(0).entryName == name);
    const std::string text = ModelFileWriter::toString(model);
    CHECK_THAT(text, ContainsSubstring("\"  User matrix  \""));
    const Model again = readText(text, true);
    REQUIRE(again.findTable<DirectMatrixTable>() != nullptr);
    CHECK(again.findTable<DirectMatrixTable>()->card(0).entryName == name);
    CHECK(ModelFileWriter::toString(again) == text);
}

TEST_CASE("a line break in a record value is refused before replacing output files", "[model][model-file]")
{
    const testing::TestFiles files;
    const auto path = files.write("out.oos", "original contents");
    for (const std::string& broken : {std::string("bad\rname"), std::string("bad\nname")})
    {
        CHECK_THROWS_AS(deck::RecordText::format({"FutureCard", broken}), base::UsageError);
        Model model = readText("DirectMatrix;ValidName;0;2;1;0;1;1\n", true);
        auto card = model.table<DirectMatrixTable>().card(0);
        card.entryName = broken;
        model.table<DirectMatrixTable>().store(0, card);
        CHECK_THROWS_AS(ModelFileWriter::toString(model), base::UsageError);
        CHECK_THROWS_AS(ModelFileWriter::write(model, path, {}), base::UsageError);
        CHECK(files.read("out.oos") == "original contents");
    }
}
