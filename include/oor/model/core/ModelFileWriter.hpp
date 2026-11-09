#pragma once

#include "oor/base/threading/CancellationToken.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/model/format/CardLayouts.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace oor::model
{

struct ModelFileWriteOptions
{
    // Keep the Include structure: each included file is written to a model file of its own (its path under the folder
    // of the main file, with the extension .oos) and named by an Include record. Otherwise one file.
    bool splitIncludes = false;
    bool omitDefaults = true;
    // An included file outside the folder of the main source file may be written outside the output folder.
    bool includeOutside = false;
    // Comment lines written at the top of the file (each line after a '#').
    std::string headerComment;
    // How the slots of each card are written as the fields of its record. Null: the standard catalogue.
    std::shared_ptr<const CardLayouts> layouts;
};

// Content omitted from model output, such as unsupported program statements or raw control entries.
// Each item describes one omission.
struct ModelFileWriteReport
{
    std::vector<std::string> dropped;
};

// Writes a Model as a model file (.oos, docs/model-format.md): the [executive] and [case] sections (ControlRecords),
// then the cards of [model] in key (= file) order with their comments, then each part in a [model;Kind;id] section. A
// card is written by its card writer (FieldWriter slots, exact reals, defaults left blank), then turned into a record
// by the layout of its type. Deterministic.
class ModelFileWriter
{
public:
    // The record of one card (its name and fields, no comment).
    [[nodiscard]] static std::string cardToRecord(const Model& model, CardLocation location,
                                                  const CardLayouts& layouts = *CardLayouts::standard());

    // Throws UsageError when a card writer fills a slot that the layout of its type has no field for (a gap in the
    // card catalogue).
    [[nodiscard]] static std::string toString(const Model& model, const ModelFileWriteOptions& options = {},
                                              ModelFileWriteReport* report = nullptr);
    // Throws IoError when a file cannot be written (no file is changed then), and as toString.
    static void write(const Model& model, const std::filesystem::path& mainFile,
                      const ModelFileWriteOptions& options = {}, ModelFileWriteReport* report = nullptr,
                      const base::CancellationToken* token = nullptr);
};

} // namespace oor::model
