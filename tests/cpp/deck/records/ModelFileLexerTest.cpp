#include "oor/deck/records/ModelFileLexer.hpp"
#include "oor/deck/records/RecordText.hpp"
#include "support/TestFiles.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using namespace oor;
using deck::ModelItemKind;
using deck::RecordText;

namespace
{

std::vector<std::string> split(const std::string& line)
{
    std::vector<std::string> values;
    std::string problem;
    REQUIRE(RecordText::split(line, values, problem));
    return values;
}

} // namespace

TEST_CASE("a record is cut at its separators, spaces around the values dropped", "[deck][model-file]")
{
    CHECK(split("Node;1;;0.0; 2.5 ;3.") == std::vector<std::string>{"Node", "1", "", "0.0", "2.5", "3."});
    CHECK(split("Title") == std::vector<std::string>{"Title"});
    CHECK(split("A;;") == std::vector<std::string>{"A", "", ""});
}

TEST_CASE("a quoted value keeps its separators, quotes and spaces", "[deck][model-file]")
{
    CHECK(split(R"(TitleText;"a; b";" x ";"say ""hi""")") ==
          std::vector<std::string>{"TitleText", "a; b", " x ", R"(say "hi")"});
    std::vector<std::string> values;
    std::string problem;
    CHECK_FALSE(RecordText::split(R"(TitleText;"open)", values, problem));
    CHECK_FALSE(problem.empty());
    CHECK_FALSE(RecordText::split(R"(TitleText;"a"b)", values, problem));
}

TEST_CASE("a record is written with quotes where a value needs them and no blank at its end", "[deck][model-file]")
{
    CHECK(RecordText::format({"Node", "1", "", "0.0", "", ""}) == "Node;1;;0.0");
    CHECK(RecordText::format({"TitleText", "a; b", " x", R"(say "hi")"}) == R"(TitleText;"a; b";" x";"say ""hi""")");
    for (const std::string& line : {std::string("A;\"1;2\";;\" \""), std::string("B;x y;\"\"\"\"")})
        CHECK(RecordText::format(split(line)) == line);
}

TEST_CASE("a model file is cut into sections, records and the comments before them", "[deck][model-file]")
{
    const std::string text =
        "# the model\n"
        "[executive]\n"
        "SolutionSequence;LinearStatics\n"
        "\n"
        "[case]\n"
        "  # first subcase\n"
        "SubcaseStart;1\n"
        "[model]\n"
        "Node;1;;0.;0.;0.\n"
        "#end of the model\n";
    base::DiagnosticSink sink;
    const deck::ModelText model = deck::ModelFileLexer::readText(text, {}, sink);
    REQUIRE_FALSE(sink.hasErrors());
    REQUIRE(model.items.size() == 8);
    CHECK(model.items[0].kind == ModelItemKind::Comments);
    CHECK(model.items[0].comments == " the model");
    CHECK(model.items[1].kind == ModelItemKind::Section);
    CHECK(model.items[1].values == std::vector<std::string>{"executive"});
    CHECK(model.items[2].values == std::vector<std::string>{"SolutionSequence", "LinearStatics"});
    CHECK(model.items[2].line == 3);
    CHECK(model.items[4].kind == ModelItemKind::Record);
    CHECK(model.items[4].comments == " first subcase");
    CHECK(model.items[6].values.front() == "Node");
    CHECK(model.items[7].kind == ModelItemKind::Comments);
    CHECK(model.items[7].comments == "end of the model");
}

TEST_CASE("a part section line holds its kind and number", "[deck][model-file]")
{
    base::DiagnosticSink sink;
    const deck::ModelText model = deck::ModelFileLexer::readText("[model;SuperelementPart;5]\n", {}, sink);
    REQUIRE(model.items.size() == 1);
    CHECK(model.items[0].values == std::vector<std::string>{"model", "SuperelementPart", "5"});
}

TEST_CASE("an Include record puts the items of the included file in place", "[deck][model-file]")
{
    const testing::TestFiles files;
    files.write("parts/mesh.oos", "# nodes\nNode;2;;1.;0.;0.\n");
    const auto main = files.write("main.oos", "[model]\nNode;1;;0.;0.;0.\nInclude;parts/mesh.oos\nNode;3;;2.;0.;0.\n");
    base::DiagnosticSink sink;
    const deck::ModelText model = deck::ModelFileLexer::read(main, {}, sink);
    REQUIRE_FALSE(sink.hasErrors());
    REQUIRE(model.files.size() == 2);
    REQUIRE(model.items.size() == 6);
    CHECK(model.items[2].kind == ModelItemKind::IncludeBegin);
    CHECK(model.items[2].values == std::vector<std::string>{"parts/mesh.oos"});
    CHECK(model.items[2].includedFileIndex == 1);
    CHECK(model.items[3].fileIndex == 1);
    CHECK(model.items[3].comments == " nodes");
    CHECK(model.items[4].kind == ModelItemKind::IncludeEnd);
    CHECK(model.items[5].values[1] == "3");
}

TEST_CASE("a bad line of a model file is reported and skipped", "[deck][model-file]")
{
    base::DiagnosticSink sink;
    const deck::ModelText model =
        deck::ModelFileLexer::readText("[model\nNode;\"1\nInclude;missing.oos\nNode;2\n", {}, sink);
    CHECK(sink.diagnostics().size() == 3);
    REQUIRE(model.items.size() == 1);
    CHECK(model.items[0].values[1] == "2");
}

TEST_CASE("a model file is known by its extension", "[deck][model-file]")
{
    CHECK(deck::isModelFile("a/plate.oos"));
    CHECK(deck::isModelFile("PLATE.OOS"));
    CHECK_FALSE(deck::isModelFile("plate.bdf"));
}
