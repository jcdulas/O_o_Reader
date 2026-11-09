#pragma once

#include "oor/base/diagnostics/DiagnosticSink.hpp"
#include "oor/base/threading/CancellationToken.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace oor::deck
{

// Source metadata is kept once per inclusion.
struct SourceFile
{
    std::filesystem::path path;
    std::string includeTextAsWritten;
    std::int32_t parent = -1;
    std::uint32_t includeLine = 0;
};

struct ReadOptions
{
    std::filesystem::path workingDirectory;
    std::size_t maxIncludeDepth = 32;
};

// The extension of a model file.
inline constexpr std::string_view MODEL_FILE_EXTENSION = ".oos";
// True when `path` names a model file (its extension is .oos, in any case).
[[nodiscard]] bool isModelFile(const std::filesystem::path& path);

enum class ModelItemKind : std::uint8_t
{
    Section,      // a section line: [executive], [case], [model], [model;SuperelementPart;5]
    Record,       // a record: a card, a command, a statement
    IncludeBegin, // an Include record: the items of the included file follow
    IncludeEnd,   // the end of the included file
    Comments      // comment lines that no record follows in their file (before a section line, at the end of a file)
};

// One item of a model file, in file order.
struct ModelItem
{
    ModelItemKind kind = ModelItemKind::Record;
    // Record: the values, the name first. Section: the values between the brackets ("model", "SuperelementPart",
    // "5"). IncludeBegin: the path as written.
    std::vector<std::string> values;
    // The comment lines before the item (Comments: the lines themselves), joined with '\n', without the '#'.
    std::string comments;
    std::uint32_t fileIndex = 0;         // index in ModelText::files
    std::uint32_t line = 0;              // 1-based
    std::uint32_t includedFileIndex = 0; // IncludeBegin: the file that is opened
};

// A model file and the files it includes, cut into items.
struct ModelText
{
    std::vector<SourceFile> files; // the main file first; an included file once per inclusion (no mapping kept)
    std::vector<ModelItem> items;
};

// Reads a model file (.oos) and every file an Include record names, into items (docs/model-format.md):
//
//   - one record per line, values separated by ';' (RecordText);
//   - '#' first on a line (after spaces) makes it a comment, which belongs to the item after it;
//   - empty lines are ignored;
//   - a line in brackets opens a section;
//   - `Include;path` inserts the items of another model file, its path relative to the including file (or absolute).
//
// A problem in the text is a diagnostic (the line is skipped); a main file that cannot be read throws IoError.
struct ModelFileLexer
{
    // The name of the record that includes a file.
    static constexpr std::string_view INCLUDE_RECORD = "Include";

    [[nodiscard]] static ModelText read(const std::filesystem::path& mainFile, const ReadOptions& options,
                                        base::DiagnosticSink& sink, const base::CancellationToken* token = nullptr);
    // The items of a text that is not in a file (an Include in it is looked up from `folder`).
    [[nodiscard]] static ModelText readText(std::string_view text, const std::filesystem::path& folder,
                                            base::DiagnosticSink& sink);
};

} // namespace oor::deck
