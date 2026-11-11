#include "oor/model/core/ModelFileReader.hpp"
#include "oor/model/core/ModelFileWriter.hpp"
#include "support/ModelFileRoundTrip.hpp"
#include "support/TestFiles.hpp"
#include "support/ValidationModels.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

// Every validation model (validation/models) goes through model files and back: the model is read, written as model
// files with its Include structure, read again from them; the card tables must be the same, and writing the model read
// back must give the same files (a fixed point).

namespace
{
constexpr std::size_t MODEL_ROUND_TRIP_PARTS = 24;
constexpr std::size_t MAX_VALIDATION_MODELS = 256;

// The files under a folder, by their path relative to it.
std::map<std::string, std::string> filesUnder(const std::filesystem::path& folder)
{
    std::map<std::string, std::string> files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(folder))
    {
        if (!entry.is_regular_file())
            continue;
        std::ifstream file(entry.path(), std::ios::binary);
        files[entry.path().lexically_relative(folder).generic_string()] =
            std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }
    return files;
}

void checkModels(std::size_t first, std::size_t stride)
{
    using namespace oor;
    const std::vector<std::filesystem::path> models = testing::validationModels(OOR_VALIDATION_DIR);
    REQUIRE(models.size() > 200);
    REQUIRE(models.size() <= MAX_VALIDATION_MODELS);
    for (std::size_t index = first; index < models.size(); index += stride)
    {
        INFO(models[index].string());
        const testing::TestFiles files;
        const model::Model model = model::ModelFileReader::read(models[index], {});
        model::ModelFileWriteOptions options;
        options.splitIncludes = true;
        options.includeOutside = true;
        const std::filesystem::path firstFile = files.root() / "first" / "model.oos";
        model::ModelFileWriter::write(model, firstFile, options);
        const model::Model back = model::ModelFileReader::read(firstFile, {});
        std::string errors;
        for (const base::Diagnostic& diagnostic : back.diagnostics().diagnostics())
            if (diagnostic.severity == base::Severity::Error)
                errors += diagnostic.toString() + "\n";
        INFO(errors);
        CHECK(errors.empty());
        CHECK(testing::cardTablesText(back) == testing::cardTablesText(model));
        model::ModelFileWriter::write(back, files.root() / "second" / "model.oos", options);
        CHECK(filesUnder(files.root() / "second") == filesUnder(files.root() / "first"));
        // One text, the Include structure flattened, is a fixed point too.
        const testing::ModelFileRoundTrip flat = testing::modelFileRoundTrip(model, false);
        INFO(flat.problem);
        CHECK(flat.problem.empty());
    }
}

} // namespace

#define OOR_MODEL_ROUND_TRIP_PART(index) \
    TEST_CASE("validation model file round trip part " #index, "[model][validation][model-file]") \
    { \
        checkModels(index, MODEL_ROUND_TRIP_PARTS); \
    }

OOR_MODEL_ROUND_TRIP_PART(0)
OOR_MODEL_ROUND_TRIP_PART(1)
OOR_MODEL_ROUND_TRIP_PART(2)
OOR_MODEL_ROUND_TRIP_PART(3)
OOR_MODEL_ROUND_TRIP_PART(4)
OOR_MODEL_ROUND_TRIP_PART(5)
OOR_MODEL_ROUND_TRIP_PART(6)
OOR_MODEL_ROUND_TRIP_PART(7)
OOR_MODEL_ROUND_TRIP_PART(8)
OOR_MODEL_ROUND_TRIP_PART(9)
OOR_MODEL_ROUND_TRIP_PART(10)
OOR_MODEL_ROUND_TRIP_PART(11)
OOR_MODEL_ROUND_TRIP_PART(12)
OOR_MODEL_ROUND_TRIP_PART(13)
OOR_MODEL_ROUND_TRIP_PART(14)
OOR_MODEL_ROUND_TRIP_PART(15)
OOR_MODEL_ROUND_TRIP_PART(16)
OOR_MODEL_ROUND_TRIP_PART(17)
OOR_MODEL_ROUND_TRIP_PART(18)
OOR_MODEL_ROUND_TRIP_PART(19)
OOR_MODEL_ROUND_TRIP_PART(20)
OOR_MODEL_ROUND_TRIP_PART(21)
OOR_MODEL_ROUND_TRIP_PART(22)
OOR_MODEL_ROUND_TRIP_PART(23)

#undef OOR_MODEL_ROUND_TRIP_PART
