#pragma once

#include "oor/model/core/Model.hpp"
#include "oor/results/archive/ArchiveWriter.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// The simple entry points of the library: what the WebAssembly module (wasm/) offers to JavaScript, in plain C++ so
// that it is tested natively. Each class wraps the library objects behind a few methods with plain types.
namespace oor::api
{

struct DeckReadSettings
{
    // Strict: the first error raises DeckError. Otherwise the problems are kept in `diagnostics()`.
    bool strict = false;
};

struct DeckWriteSettings
{
    bool splitIncludes = false; // write each included file to its own file again
    std::string headerComment;  // comment lines at the top of the model file
};

// A problem found while reading, in plain text.
struct DiagnosticText
{
    std::string severity; // "info", "warning" or "error"
    std::string code;
    std::string message;
    std::string location; // "file:line (CARD, field)"; empty when unknown
};

// A model: its control sections and cards, read from .oos text or a native .ooh5 archive.
class Deck
{
public:
    // Detects a native archive by content; otherwise reads model records and their Include files.
    // Throws IoError when the file cannot be read, DeckError in strict mode.
    [[nodiscard]] static std::shared_ptr<Deck> readModel(const std::string& path,
                                                         const DeckReadSettings& settings = {});
    // Writes a model file. Returns any content that could not be carried, one description per item.
    std::vector<std::string> writeModel(const std::string& path, const DeckWriteSettings& settings = {}) const;
    // The model file text (one file, the Include structure flattened).
    [[nodiscard]] std::string toModelText(const DeckWriteSettings& settings = {}) const;
    // Writes an archive containing this model alone (no result tables).
    results::ResultWriteReport writeArchive(const std::string& path,
                                            const results::ArchiveWriteOptions& options = {}) const;

    // The number of cards of each type the deck holds, by card name, in alphabetical order.
    [[nodiscard]] std::vector<std::pair<std::string, std::size_t>> cardCounts() const;
    [[nodiscard]] std::size_t cardCount() const;
    [[nodiscard]] std::vector<DiagnosticText> diagnostics() const;
    [[nodiscard]] const std::shared_ptr<model::Model>& model() const noexcept { return m_model; }

private:
    friend class Results;
    explicit Deck(std::shared_ptr<model::Model> model) : m_model(std::move(model)) {}

    std::shared_ptr<model::Model> m_model;
};

// The text of a diagnostic of the library.
[[nodiscard]] DiagnosticText diagnosticText(const base::Diagnostic& diagnostic);

} // namespace oor::api
