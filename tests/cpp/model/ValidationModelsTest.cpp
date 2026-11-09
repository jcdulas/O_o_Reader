#include "oor/model/core/ModelFileReader.hpp"
#include "support/ValidationModels.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>
#include <vector>

// The model files of validation/models (written for the project) read without an error diagnostic. The folder is given
// by the build (OOR_VALIDATION_DIR).

namespace
{

// Reads the models whose index is `first`, `first + stride`, ...: the models are spread over several test cases so
// that each stays well under the time limit of a test.
void checkModels(std::size_t first, std::size_t stride)
{
    const std::vector<std::filesystem::path> models = oor::testing::validationModels(OOR_VALIDATION_DIR);
    REQUIRE(models.size() > 200);
    for (std::size_t index = first; index < models.size(); index += stride)
    {
        INFO(models[index].string());
        const oor::model::Model model = oor::model::ModelFileReader::read(models[index], {});
        std::string errors;
        for (const oor::base::Diagnostic& diagnostic : model.diagnostics().diagnostics())
            if (diagnostic.severity == oor::base::Severity::Error)
                errors += diagnostic.toString() + "\n";
        INFO(errors);
        CHECK(errors.empty());
    }
}

} // namespace

TEST_CASE("the validation models read without an error, part 1 of 4", "[model][validation]")
{
    checkModels(0, 4);
}

TEST_CASE("the validation models read without an error, part 2 of 4", "[model][validation]")
{
    checkModels(1, 4);
}

TEST_CASE("the validation models read without an error, part 3 of 4", "[model][validation]")
{
    checkModels(2, 4);
}

TEST_CASE("the validation models read without an error, part 4 of 4", "[model][validation]")
{
    checkModels(3, 4);
}
