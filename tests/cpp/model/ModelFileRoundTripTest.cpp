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

TEST_CASE("the validation models go through model files and back, part 1 of 6", "[model][validation][model-file]")
{
    checkModels(0, 6);
}

TEST_CASE("the validation models go through model files and back, part 2 of 6", "[model][validation][model-file]")
{
    checkModels(1, 6);
}

TEST_CASE("the validation models go through model files and back, part 3 of 6", "[model][validation][model-file]")
{
    checkModels(2, 6);
}

TEST_CASE("the validation models go through model files and back, part 4 of 6", "[model][validation][model-file]")
{
    checkModels(3, 6);
}

TEST_CASE("the validation models go through model files and back, part 5 of 6", "[model][validation][model-file]")
{
    checkModels(4, 6);
}

TEST_CASE("the validation models go through model files and back, part 6 of 6", "[model][validation][model-file]")
{
    checkModels(5, 6);
}
