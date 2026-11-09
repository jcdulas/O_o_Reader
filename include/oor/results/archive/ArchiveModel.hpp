#pragma once

#include "oor/model/core/Model.hpp"
#include "oor/model/core/ModelReadOptions.hpp"

#include <filesystem>
#include <memory>

namespace oor::results::archive
{

// Appends the native model to an archive's temporary file. Only the archive writer should call this function.
void writeModel(const std::filesystem::path& path, const model::Model& model, int compressionLevel, bool shuffle);
// Loads the model independently of result values; missing, invalid or unsupported model data raises IoError.
[[nodiscard]] std::shared_ptr<model::Model> readModel(const std::filesystem::path& path,
                                                    const model::ModelReadOptions& options = {});

} // namespace oor::results::archive
