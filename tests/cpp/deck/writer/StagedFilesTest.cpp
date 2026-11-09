#include "oor/deck/writer/StagedFiles.hpp"
#include "support/TestFiles.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace oor;

TEST_CASE("staged files are written by pieces and replace their targets at the commit", "[deck][writer]")
{
    const testing::TestFiles files;
    const auto target = files.write("model.oos", "the old text\n");
    {
        deck::StagedFiles staged;
        const std::size_t main = staged.open(target);
        const std::size_t included = staged.open(files.root() / "sub" / "part.oos");
        staged.append(main, "Node;1\n");
        staged.append(included, "Rod2;2\n");
        staged.append(main, "Node;3\n");
        REQUIRE(files.read("model.oos") == "the old text\n"); // nothing replaced before the commit
        staged.commit(nullptr);
    }
    REQUIRE(files.read("model.oos") == "Node;1\nNode;3\n");
    REQUIRE(files.read("sub/part.oos") == "Rod2;2\n");
    REQUIRE_FALSE(files.exists("model.oos.oor_tmp"));
}

TEST_CASE("staged files dropped without a commit leave the targets as they were", "[deck][writer]")
{
    const testing::TestFiles files;
    const auto target = files.write("model.oos", "the old text\n");
    base::CancellationToken token;
    {
        deck::StagedFiles staged;
        staged.append(staged.open(target), "half a model\n");
        token.requestCancellation();
        REQUIRE_THROWS_AS(staged.commit(&token), base::OperationCancelled);
    }
    REQUIRE(files.read("model.oos") == "the old text\n");
    REQUIRE_FALSE(files.exists("model.oos.oor_tmp"));
}
