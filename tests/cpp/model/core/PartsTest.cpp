// Parts ([model;SuperelementPart;n] and the other part sections) are models of their own: their own cards, their own
// ID spaces, their own versions, written back where the model file had them.

#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/superelements/superelementConnection/SuperelementConnectionTable.hpp"
#include "oor/model/cards/superelements/superelementInteriorNodes/SuperelementInteriorNodesTable.hpp"
#include "support/SuperelementTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>
#include <vector>

using namespace oor;
using namespace oor::model;
using namespace oor::testing;

namespace
{

// One model section, from its header to the next model or part header.
std::string section(const std::string& text, const std::string& prefix)
{
    const std::size_t begin = text.find(prefix);
    if (begin == std::string::npos)
        return {};
    const std::size_t end = text.find("\n[model", begin + prefix.size());
    return text.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
}

Model readFile(const TestFiles& files, const std::string& name, const std::string& text)
{
    const std::filesystem::path path = files.write(name, text);
    ModelReadOptions options;
    options.strict = true;
    return ModelFileReader::read(path, options);
}

} // namespace

TEST_CASE("two parts that reuse the same IDs do not collide", "[model][parts]")
{
    const Model model = readModelText(
        "[model]\n"
        "Node;1;;0.;0.;0.\n"
        "Node;2;;1.;0.;0.\n"
        "[model;SuperelementPart;1]\n"
        "Node;1;;10.;0.;0.\n"
        "Node;2;;11.;0.;0.\n"
        "[model;SuperelementPart;2]\n"
        "Node;1;;20.;0.;0.\n"
        "Node;2;;21.;0.;0.\n",
        true);
    REQUIRE(model.parts().size() == 2);
    const std::vector<double> expected = {0.0, 10.0, 20.0};
    std::vector<const Model*> models = {&model, model.parts()[0].get(), model.parts()[1].get()};
    for (std::size_t index = 0; index < models.size(); ++index)
    {
        const NodeTable& grids = *models[index]->findTable<NodeTable>();
        REQUIRE(grids.rows().aliveCount() == 2);
        REQUIRE(grids.xyz(models[index]->locate(base::IdSpace::Node, 1)->row).x == expected[index]);
    }
    REQUIRE(model.parts()[0]->partInfo()->id == 1);
    REQUIRE(model.parts()[1]->partInfo()->id == 2);
    REQUIRE_FALSE(model.diagnostics().hasErrors());
    requireRoundTrip(model);
}

TEST_CASE("a part knows the model that holds it, also after the model is copied or moved", "[model][parts]")
{
    Model model = readModelText(
        "[model]\n"
        "SuperelementInteriorNodes;1;1\n"
        "[model;SuperelementPart;1]\n"
        "Node;1;;0.;0.;0.\n"
        "[model;AuxiliaryModel;7]\n"
        "Node;1;;1.;0.;0.\n",
        true);
    Model& part = *model.parts()[0];
    REQUIRE(model.owner() == nullptr);
    REQUIRE(part.owner() == &model);
    REQUIRE(&part.root() == &model);
    REQUIRE(&model.root() == &model);
    REQUIRE(model.findPart("SuperelementPart", 1) == &part);
    REQUIRE(part.findPart("AuxiliaryModel", 7) ==
            model.parts()[1].get()); // a part looks among the parts of its root model
    REQUIRE(model.findPart("SuperelementPart", 2) == nullptr);
    REQUIRE(&model.superelementModel(1) == &part);
    REQUIRE(&model.superelementModel(0) == &model);
    REQUIRE(&model.superelementModel(9) == &model); // a list superelement keeps its IDs in the main model
    REQUIRE(&part.superelementModel(1) == &part);
    REQUIRE(&part.superelementModel(0) == &model);

    const Model copy = model.copy();
    REQUIRE(copy.parts()[0]->owner() == &copy);
    REQUIRE(copy.parts()[0].get() != &part);
    const Model standalone = part.copy(); // a copy of a part is a model of its own
    REQUIRE(standalone.owner() == nullptr);
    REQUIRE(standalone.isPart());

    Model moved(std::move(model));
    REQUIRE(moved.parts()[0]->owner() == &moved);
    Model assigned;
    assigned = std::move(moved);
    REQUIRE(assigned.parts()[1]->owner() == &assigned);
}

TEST_CASE("a part put back by an undo is the same object and still has its owner", "[model][parts]")
{
    Model model = readModelText("[model]\nNode;1;;0.;0.;0.\n[model;SuperelementPart;1]\nNode;1;;0.;0.;0.\n", true);
    Model* part = model.parts()[0].get();
    Model backup(model);
    model.replaceContents(std::move(backup));
    REQUIRE(model.parts()[0].get() == part);
    REQUIRE(part->owner() == &model);
    REQUIRE(part->partInfo()->id == 1);
}

TEST_CASE("every registered kind of model part is retained", "[model][parts]")
{
    const Model model = readModelText(
        "[model;AcousticFieldPointMesh;1]\n"
        "Node;1;;0.;0.;0.\n"
        "[model;ArbitraryModel;2]\n"
        "Node;1;;0.;0.;0.\n"
        "[model;MassModel;3]\n"
        "Node;1;;0.;0.;0.\n"
        "[model;FlexibleBody;4]\n"
        "Node;1;;0.;0.;0.\n"
        "[model;TrimComponent;5]\n"
        "Node;1;;0.;0.;0.\n",
        true);
    std::vector<std::string> kinds;
    for (const auto& part : model.parts())
        kinds.push_back(part->partInfo()->kind);
    REQUIRE(kinds == std::vector<std::string>{"AcousticFieldPointMesh", "ArbitraryModel", "MassModel", "FlexibleBody",
                                              "TrimComponent"});
}

TEST_CASE("a module is a part; a section that appends opens it again", "[model][parts]")
{
    const Model model = readModelText(
        "[model]\nNode;1;;0.;0.;0.\nNode;2;;1.;0.;0.\n[model;ModuleSection;1]\n"
        "Node;1;;10.;0.;0.\n[model;ModuleSection;1;append]\nNode;2;;11.;0.;0.\n",
        true);
    REQUIRE(model.parts().size() == 2);
    const PartInfo& first = *model.parts()[0]->partInfo();
    const PartInfo& second = *model.parts()[1]->partInfo();
    REQUIRE((first.kind == "ModuleSection" && first.id == 1 && !first.appends));
    REQUIRE((second.kind == "ModuleSection" && second.id == 1 && second.appends));
    REQUIRE(model.findTable<NodeTable>()->rows().aliveCount() == 2);
    REQUIRE(writeModelText(model).find("[model;ModuleSection;1;append]\nNode;2;;11.;0.;0.\n") != std::string::npos);
    // the appended section names a grid point of the first section of its module
    Model appended = readModelText(
        "[model;ModuleSection;1]\n"
        "Node;1;;0.;0.;0.\n"
        "Node;2;;1.;0.;0.\n"
        "[model;ModuleSection;1;append]\n"
        "Rod2;5;3;1;2\n"
        "RodProp;3;4;1.\n"
        "IsoMat;4;2.1E+5;;0.3\n",
        true);
    REQUIRE(unresolvedIn(*appended.parts()[1]) == 0);
}

TEST_CASE("the active and passive sub-models of a TPA are parts with no number that share their IDs", "[model][parts]")
{
    Model model = readModelText(
        "[model]\n"
        "TypedListSet3;1;Node;1\n"
        "[model;PassivePart]\n"
        "Node;1;;0.;0.;0.\n"
        "[model;ActivePart]\n"
        "Node;2;;0.;0.;1.\n"
        "Rod2;5;3;1;2\n"
        "RodProp;3;4;1.\n"
        "IsoMat;4;2.1E+5;;0.3\n",
        true);
    REQUIRE(model.parts().size() == 2);
    REQUIRE((model.parts()[0]->partInfo()->kind == "PassivePart" && model.parts()[0]->partInfo()->id == 0));
    REQUIRE((model.parts()[1]->partInfo()->kind == "ActivePart" && model.parts()[1]->partInfo()->id == 0));
    REQUIRE(unresolvedIn(model) == 0);
    REQUIRE(unresolvedIn(*model.parts()[1]) == 0);
}

TEST_CASE("a mass increment names the grid points of its base model", "[model][parts]")
{
    Model model = readModelText(
        "[model]\n"
        "Node;8;;0.;0.;0.\n"
        "[model;SuperelementPart;10]\n"
        "Node;1001;;1.;0.;0.\n"
        "[model;MassModel;101]\n"
        "PointMassInertia;9001;8;;1.E-5\n"
        "[model;SuperelementPart;10;massId=101]\n"
        "PointMassInertia;9002;1001;;1.E-5\n",
        true);
    REQUIRE(model.parts().size() == 3);
    REQUIRE(unresolvedIn(*model.parts()[1]) == 0);
    REQUIRE(unresolvedIn(*model.parts()[2]) == 0);
    // what the base model does not have is still missing
    Model lacking = readModelText("[model]\nNode;8;;0.;0.;0.\n[model;MassModel;101]\nPointMassInertia;9001;9;;1.E-5\n", true);
    REQUIRE(unresolvedIn(*lacking.parts()[0]) == 1);
}

TEST_CASE("editing a part leaves the other parts and the residual structure as they were", "[model][parts]")
{
    Model model = readModelText(
        "[model]\n"
        "Node;1;;0.;0.;0.\n"
        "[model;SuperelementPart;1]\n"
        "Node;1;;10.;0.;0.\n"
        "Node;2;;11.;0.;0.\n"
        "[model;SuperelementPart;2]\n"
        "Node;1;;20.;0.;0.\n"
        "Node;2;;21.;0.;0.\n"
        "[model;SuperelementPart;3]\n"
        "Node;1;;30.;0.;0.\n",
        true);
    const std::string before = writeModelText(model);
    Model& second = *model.parts()[1];
    NodeCard grid = second.findTable<NodeTable>()->card(0);
    grid.xyz.x = 99.5;
    second.table<NodeTable>().store(0, grid);
    second.noteContentChange();
    const std::string after = writeModelText(model);
    REQUIRE(after != before);
    for (const char* prefix : {"[model]", "[model;SuperelementPart;1]", "[model;SuperelementPart;3]"})
        REQUIRE(section(after, prefix) == section(before, prefix));
    REQUIRE(section(after, "[model;SuperelementPart;2]") != section(before, "[model;SuperelementPart;2]"));
    REQUIRE(after.find("99.5") != std::string::npos);
}

TEST_CASE("the versions of a part are its own", "[model][parts]")
{
    Model model = readModelText(
        "[model]\n"
        "Node;1;;0.;0.;0.\n"
        "[model;SuperelementPart;1]\n"
        "Node;1;;10.;0.;0.\n"
        "[model;SuperelementPart;2]\n"
        "Node;1;;20.;0.;0.\n",
        true);
    Model& first = *model.parts()[0];
    Model& second = *model.parts()[1];
    const std::uint64_t rootStructure = model.structureVersion();
    const std::uint64_t rootContent = model.contentVersion();
    const std::uint64_t secondStructure = second.structureVersion();
    const std::uint64_t secondContent = second.contentVersion();
    const std::uint64_t firstContent = first.contentVersion();

    // A card added to a part changes the content of that part, and its connectivity when it is an element.
    NodeCard grid;
    grid.id = 7;
    first.add<NodeTable>(grid);
    REQUIRE(first.contentVersion() != firstContent);
    REQUIRE(model.contentVersion() == rootContent);
    REQUIRE(model.structureVersion() == rootStructure);
    REQUIRE(second.contentVersion() == secondContent);
    REQUIRE(second.structureVersion() == secondStructure);

    // Renumbering the nodes of a part changes that part's versions (and the cards of the residual structure that name
    // them), not those of the other part.
    Renumbering::apply(second, RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(second.contentVersion() != secondContent);
    REQUIRE(second.structureVersion() != secondStructure);
    REQUIRE(model.structureVersion() == rootStructure); // no element or node of the residual structure changed
    // The versions come from one counter: no two states of content ever share a number.
    REQUIRE(second.contentVersion() != first.contentVersion());
    REQUIRE(second.contentVersion() != model.contentVersion());
}

TEST_CASE("renumbering a part leaves the others and the residual structure alone", "[model][parts]")
{
    Model model = readModelText(
        "[model]\n"
        "Node;1;;0.;0.;0.\n"
        "Node;2;;1.;0.;0.\n"
        "[model;SuperelementPart;1]\n"
        "Node;1;;10.;0.;0.\n"
        "Node;2;;11.;0.;0.\n"
        "[model;SuperelementPart;2]\n"
        "Node;1;;20.;0.;0.\n"
        "Node;2;;21.;0.;0.\n",
        true);
    const std::string before = writeModelText(model);
    Renumbering::apply(*model.parts()[0], RenumberSpace::Node, Renumbering::offset(100));
    const std::string after = writeModelText(model);
    for (const char* prefix : {"[model]", "[model;SuperelementPart;2]"})
        REQUIRE(section(after, prefix) == section(before, prefix));
    REQUIRE(model.parts()[0]->locate(base::IdSpace::Node, 101).has_value());
    REQUIRE_FALSE(model.parts()[0]->locate(base::IdSpace::Node, 1).has_value());
    REQUIRE(model.parts()[1]->locate(base::IdSpace::Node, 1).has_value());
    REQUIRE(model.locate(base::IdSpace::Node, 1).has_value());
    // The residual structure alone leaves every part as it is, unless the parts are asked for.
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(5000));
    REQUIRE(model.locate(base::IdSpace::Node, 5001).has_value());
    REQUIRE(model.parts()[1]->locate(base::IdSpace::Node, 1).has_value());
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1), true);
    REQUIRE(model.locate(base::IdSpace::Node, 5002).has_value());
    REQUIRE(model.parts()[1]->locate(base::IdSpace::Node, 2).has_value());
    REQUIRE(model.parts()[0]->locate(base::IdSpace::Node, 102).has_value());
}

TEST_CASE("the references of the residual structure to a part follow the renumbering of that part", "[model][parts]")
{
    Model model = readModelText(
        "[model]\n"
        "Node;1;;0.;0.;0.\n"
        "Node;2;;1.;0.;0.\n"
        "SuperelementConnection;10;0;;;11;1;12;2\n"
        "[model;SuperelementPart;10]\n"
        "Node;11;;10.;0.;0.\n"
        "Node;12;;11.;0.;0.\n"
        "Node;13;;12.;0.;0.\n",
        true);
    const auto pairsOf = [&]
    {
        std::vector<std::pair<base::EntityId, base::EntityId>> pairs;
        for (const SuperelementConnectionPair& pair : cardAt<SuperelementConnectionTable>(model).pairs)
            pairs.emplace_back(pair.a.first, pair.b.first);
        return pairs;
    };
    REQUIRE(unresolvedIn(model) == 0);
    // The part alone: the connection points that are in it follow, those of the residual structure do not.
    Renumbering::apply(*model.parts()[0], RenumberSpace::Node, Renumbering::offset(100));
    REQUIRE(pairsOf() == std::vector<std::pair<base::EntityId, base::EntityId>>{{111, 1}, {112, 2}});
    REQUIRE(unresolvedIn(model) == 0);
    // The residual structure alone: the other way round.
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(1000));
    REQUIRE(pairsOf() == std::vector<std::pair<base::EntityId, base::EntityId>>{{111, 1001}, {112, 1002}});
    REQUIRE(unresolvedIn(model) == 0);
    // Both: each side with the mapping of its own model.
    Renumbering::apply(model, RenumberSpace::Node, Renumbering::offset(10), true);
    REQUIRE(pairsOf() == std::vector<std::pair<base::EntityId, base::EntityId>>{{121, 1011}, {122, 1012}});
    REQUIRE(unresolvedIn(model) == 0);
}

TEST_CASE("a refused renumbering of a part restores the whole model", "[model][parts]")
{
    Model model = readModelText(
        "[model]\n"
        "Node;1;;0.;0.;0.\n"
        "SuperelementConnection;10;0;;;11;1\n"
        "[model;SuperelementPart;10]\n"
        "Node;11;;10.;0.;0.\n"
        "Node;12;;11.;0.;0.\n",
        true);
    const std::string before = writeModelText(model);
    REQUIRE_THROWS_AS(
        Renumbering::apply(*model.parts()[0], RenumberSpace::Node,
                           Renumbering::fromFunction([](base::EntityId) { return std::optional<base::EntityId>(5); })),
        base::DuplicateIdError);
    REQUIRE(writeModelText(model) == before);
    REQUIRE(model.parts()[0]->owner() == &model);
}

TEST_CASE("an Include inside a part reads into that part and is written back to its own file", "[model][parts]")
{
    const TestFiles files;
    files.write("part1.oos", "Node;1;;1.;0.;0.\nNode;2;;2.;0.;0.\n");
    const Model withInclude =
        readFile(files, "main.oos",
                 "[model]\nNode;1;;0.;0.;0.\n[model;SuperelementPart;1]\nInclude;part1.oos\nNode;3;;0.;0.;0.\n"
                 "[model;SuperelementPart;2]\nNode;1;;5.;0.;0.\n");
    REQUIRE(withInclude.parts().size() == 2);
    REQUIRE(withInclude.parts()[0]->cardCount() == 3); // the two of the include and the grid after it
    REQUIRE(withInclude.cardCount() == 1);
    REQUIRE(withInclude.parts()[1]->cardCount() == 1);
    REQUIRE(withInclude.parts()[0]->markers().size() == 2); // the Include begins and ends in the part
    REQUIRE(withInclude.markers().empty());

    ModelFileWriteOptions options;
    options.splitIncludes = true;
    ModelFileWriter::write(withInclude, files.root() / "out" / "main.oos", options);
    const std::string mainText = files.read("out/main.oos");
    const std::string includeText = files.read("out/part1.oos");
    REQUIRE(mainText.find("[model;SuperelementPart;1]\nInclude;part1.oos\n") != std::string::npos);
    REQUIRE(includeText.find("Node") != std::string::npos);
    REQUIRE(includeText.find("[model") == std::string::npos);
    // The node after the include is still in the part: between the include and the next part line.
    const std::size_t nodeAfterInclude = mainText.find("Node", mainText.find("Include;part1.oos"));
    REQUIRE(nodeAfterInclude != std::string::npos);
    REQUIRE(nodeAfterInclude < mainText.find("[model;SuperelementPart;2]"));
}

TEST_CASE("the number of a part can change and its part line follows", "[model][parts]")
{
    Model model = readModelText(
        "[model;SuperelementPart;1]\n"
        "Node;1;;0.;0.;0.\n"
        "[model;SuperelementPart;2;massId=40]\n"
        "Node;1;;0.;0.;0.\n"
        "[model;AuxiliaryModel;3]\n"
        "Node;1;;0.;0.;0.\n",
        true);
    model.parts()[0]->setPartId(11);
    model.parts()[1]->setPartId(12);
    model.parts()[2]->setPartId(103);
    REQUIRE(model.parts()[1]->partInfo()->massId == 40);
    const std::string text = writeModelText(model);
    for (const char* line :
         {"[model;SuperelementPart;11]\n", "[model;SuperelementPart;12;massId=40]\n", "[model;AuxiliaryModel;103]\n"})
        REQUIRE(text.find(line) != std::string::npos);
    const Model again = readModelText(text, true);
    REQUIRE(again.parts()[0]->partInfo()->id == 11);
    REQUIRE(again.parts()[1]->partInfo()->id == 12);
    REQUIRE(again.parts()[2]->partInfo()->id == 103);
}

TEST_CASE("a part is validated, summed and copied as a model", "[model][parts]")
{
    Model model = readModelText(
        "[model]\n"
        "Node;1;;0.;0.;0.\n"
        "[model;SuperelementPart;1]\n"
        "Node;1;;10.;0.;0.\n"
        "Node;2;;11.;0.;0.\n",
        true);
    REQUIRE(model.parts()[0]->cardCount() == 2);
    Model copy = model.parts()[0]->copy();
    REQUIRE(copy.cardCount() == 2);
    // The text of a part is a model of its own: read as a stand-alone model it has the same cards.
    const std::string text = writeModelText(*model.parts()[0]);
    const Model standalone = readModelText(text, true);
    REQUIRE(standalone.cardCount() == 2);
    REQUIRE(standalone.parts().empty());
    REQUIRE(unresolvedIn(*model.parts()[0]) == 0);
}
