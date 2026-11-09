#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "support/ModelTestSupport.hpp"
#include "support/TestFiles.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using namespace oor;

namespace
{

// A flat plate of `size` x `size` ShellQuad4 on (size + 1)^2 nodes, shifted by `offset` along x (a model file).
std::string plateModel(int size, int offset)
{
    std::string text = "[model]\nIsoMat;1;2.1E+5;;0.3;7.8E-9\nShellProp;1;1;0.1\n";
    const int columns = size + 1;
    for (int row = 0; row <= size; ++row)
        for (int column = 0; column <= size; ++column)
            text += "Node;" + std::to_string(row * columns + column + 1) + ";;" + std::to_string(offset + column) +
                    ".;" + std::to_string(row) + ".;0.\n";
    for (int row = 0; row < size; ++row)
        for (int column = 0; column < size; ++column)
        {
            const int first = row * columns + column + 1;
            text += "ShellQuad4;" + std::to_string(row * size + column + 1) + ";1;" + std::to_string(first) + ";" +
                    std::to_string(first + 1) + ";" + std::to_string(first + columns + 1) + ";" +
                    std::to_string(first + columns) + "\n";
        }
    return text;
}

} // namespace

// A Model supports concurrent reads and the readers are re-entrant. The ThreadSanitizer run of
// scripts/check_sanitizers.sh is what checks that nothing here races; the test itself checks the results.
TEST_CASE("two model files are read at the same time and one model is read from several threads at once",
          "[model][threads][robustness]")
{
    const testing::TestFiles files;
    const std::array<std::filesystem::path, 2> paths{files.write("a.oos", plateModel(20, 0)),
                                                     files.write("b.oos", plateModel(20, 100))};
    std::array<std::optional<model::Model>, 2> models;
    {
        std::vector<std::thread> readers;
        for (std::size_t which = 0; which < paths.size(); ++which)
            readers.emplace_back(
                [&, which] { models[which] = model::ModelFileReader::read(paths[which], model::ModelReadOptions{}); });
        for (std::thread& reader : readers)
            reader.join();
    }
    REQUIRE(models[0]->cardCount() == 2 + 21 * 21 + 20 * 20);
    REQUIRE(models[1]->cardCount() == models[0]->cardCount());

    // Four threads read the same model: every grid point and element found by its ID, at its row, with its values.
    const model::Model& model = *models[1];
    std::atomic<int> mismatches = 0;
    std::vector<std::thread> threads;
    for (int thread = 0; thread < 4; ++thread)
        threads.emplace_back(
            [&]
            {
                const model::NodeTable& grids = *model.findTable<model::NodeTable>();
                for (std::uint32_t row = 0; row < grids.rows().rowCount(); ++row)
                {
                    const model::NodeCard card = grids.card(row);
                    const std::optional<model::CardLocation> found = model.locate(base::IdSpace::Node, card.id);
                    if (!found || found->row != row || card.xyz.x < 100.0)
                        ++mismatches;
                }
                const model::ShellQuad4Table& quads = *model.findTable<model::ShellQuad4Table>();
                for (std::uint32_t row = 0; row < quads.rows().rowCount(); ++row)
                {
                    const std::optional<model::CardLocation> found =
                        model.locate(base::IdSpace::Element, quads.idAtRow(row));
                    if (!found || found->row != row)
                        ++mismatches;
                }
            });
    for (std::thread& thread : threads)
        thread.join();
    REQUIRE(mismatches == 0);
}
