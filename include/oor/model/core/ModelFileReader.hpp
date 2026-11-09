#pragma once

#include "oor/base/threading/CancellationToken.hpp"
#include "oor/deck/records/ModelFileLexer.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/model/core/ModelReadOptions.hpp"

#include <filesystem>
#include <string_view>

namespace oor::model
{

// Reads .oos records into typed cards, executive and case records, parts and include metadata.
// Unknown exact card names are preserved with their compact record values in tolerant mode.
struct ModelFileReader
{
    // Throws IoError when the main file cannot be read, DeckError in strict mode at the first error diagnostic,
    // OperationCancelled when `token` is cancelled.
    [[nodiscard]] static Model read(const std::filesystem::path& path, const ModelReadOptions& options,
                                    const base::CancellationToken* token = nullptr);

    // The same for the items of a model file already lexed (ModelFileLexer).
    [[nodiscard]] static Model fromItems(deck::ModelText text, const ModelReadOptions& options,
                                         const base::CancellationToken* token = nullptr);
};

} // namespace oor::model
