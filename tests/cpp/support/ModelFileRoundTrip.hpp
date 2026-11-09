#pragma once

#include "oor/base/errors/Errors.hpp"
#include "oor/deck/records/ModelFileLexer.hpp"
#include "oor/model/core/ModelFileReader.hpp"
#include "oor/model/core/ModelFileWriter.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace oor::testing
{

// The cards of a model and of its parts as text, card type by card type, every card in the free format with its reals
// exact, and the comments of the cards: what two models with the same card tables have in common, whatever the files
// they were read from.
inline std::string cardTablesText(const model::Model& model, bool withComments = true)
{
    std::string text;
    const auto addModel = [&](const model::Model& one)
    {
        for (std::uint16_t type = 0; type < model::CardRegistry::instance().size(); ++type)
        {
            const model::CardTable* table = one.findTable(type);
            if (table == nullptr)
                continue;
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (!table->rows().isAlive(row))
                    continue;
                text += model::ModelFileWriter::cardToRecord(one, model::CardLocation{type, row}) + "\n";
                if (const std::string comment = one.commentOf(table->rows().keyAt(row));
                    withComments && !comment.empty())
                    text += "comment: " + comment + "\n";
            }
        }
    };
    addModel(model);
    for (const std::unique_ptr<model::Model>& part : model.parts())
    {
        const model::PartInfo& info = *part->partInfo();
        text += "part " + info.kind + " " + std::to_string(info.id) + " " + std::to_string(info.massId) +
                (info.appends ? " append" : "") + "\n";
        addModel(*part);
    }
    return text;
}

// What a round trip through a model file found: empty when the model file holds the same card tables and writing
// the model read from it gives the same text again.
struct ModelFileRoundTrip
{
    std::string problem;
    std::string firstText;
    std::string secondText;
};

// Writes `model` as a model file (one text), reads it back, compares the card tables, and writes the model read back
// to check that the text is a fixed point. The comments of the cards are compared too unless `withComments` is false:
// one text puts the comment lines before an Include and those at the top of the included file together.
inline ModelFileRoundTrip modelFileRoundTrip(const model::Model& model, bool withComments = true)
{
    ModelFileRoundTrip result;
    try
    {
        result.firstText = model::ModelFileWriter::toString(model);
        base::DiagnosticSink lexing;
        deck::ModelText items =
            deck::ModelFileLexer::readText(result.firstText, std::filesystem::temp_directory_path(), lexing);
        model::ModelReadOptions options;
        const model::Model back = model::ModelFileReader::fromItems(std::move(items), options);
        for (const base::Diagnostic& diagnostic : lexing.diagnostics())
            result.problem += "lexing: " + diagnostic.toString() + "\n";
        for (const base::Diagnostic& diagnostic : back.diagnostics().diagnostics())
        {
            if (diagnostic.code.starts_with("model-file."))
                result.problem += "reading: " + diagnostic.toString() + "\n";
        }
        const std::string before = cardTablesText(model, withComments);
        const std::string after = cardTablesText(back, withComments);
        if (before != after)
        {
            // The first card that differs.
            std::size_t at = 0;
            while (at < before.size() && at < after.size() && before[at] == after[at])
                ++at;
            const std::size_t lineStart = before.rfind('\n', at == 0 ? 0 : at - 1);
            const std::size_t from = lineStart == std::string::npos ? 0 : lineStart + 1;
            result.problem += "card tables differ:\n  before: " + before.substr(from, before.find('\n', from) - from) +
                              "\n  after:  " + after.substr(from, after.find('\n', from) - from) + "\n";
        }
        result.secondText = model::ModelFileWriter::toString(back);
        if (result.secondText != result.firstText)
            result.problem += "the model file is not a fixed point\n";
    }
    catch (const std::exception& error)
    {
        result.problem += std::string("exception: ") + error.what() + "\n";
    }
    return result;
}

// When the environment variable OOR_MODEL_FILE_ROUND_TRIP_LOG names a file, every model a test reads through
// readModelText is written as a model file and read back, and what does not come back the same is appended to that
// file (tools/oos_layouts/round_trip_tests.sh).
inline void logModelFileRoundTrip(const model::Model& model, const std::string& modelText)
{
    const char* log = std::getenv("OOR_MODEL_FILE_ROUND_TRIP_LOG");
    if (log == nullptr)
        return;
    const ModelFileRoundTrip result = modelFileRoundTrip(model);
    if (result.problem.empty())
        return;
    std::ofstream file(log, std::ios::app | std::ios::binary);
    file << "=== model:\n"
         << modelText << "\n--- problem:\n"
         << result.problem << "--- model file:\n"
         << result.firstText << "\n";
}

} // namespace oor::testing
