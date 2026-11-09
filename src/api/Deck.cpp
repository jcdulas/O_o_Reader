#include "oor/api/Deck.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/deck/records/ModelFileLexer.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/model/core/ModelFileReader.hpp"
#include "oor/model/core/ModelFileWriter.hpp"

#include <map>

namespace oor::api
{

DiagnosticText diagnosticText(const base::Diagnostic& diagnostic)
{
    DiagnosticText text;
    switch (diagnostic.severity)
    {
    case base::Severity::Info:
        text.severity = "info";
        break;
    case base::Severity::Warning:
        text.severity = "warning";
        break;
    case base::Severity::Error:
        text.severity = "error";
        break;
    }
    text.code = diagnostic.code;
    text.message = diagnostic.message;
    text.location = diagnostic.location.toString();
    return text;
}

std::shared_ptr<Deck> Deck::readModel(const std::string& path, const DeckReadSettings& settings)
{
    model::ModelReadOptions options;
    options.strict = settings.strict;
    auto model = std::make_shared<model::Model>(model::ModelFileReader::read(base::pathFromUtf8(path), options));
    return std::shared_ptr<Deck>(new Deck(std::move(model)));
}

namespace
{

model::ModelFileWriteOptions modelFileOptionsOf(const DeckWriteSettings& settings)
{
    model::ModelFileWriteOptions options;
    options.splitIncludes = settings.splitIncludes;
    options.headerComment = settings.headerComment;
    return options;
}

} // namespace

std::vector<std::string> Deck::writeModel(const std::string& path, const DeckWriteSettings& settings) const
{
    model::ModelFileWriteReport report;
    model::ModelFileWriter::write(*m_model, base::pathFromUtf8(path), modelFileOptionsOf(settings), &report);
    return report.dropped;
}

std::string Deck::toModelText(const DeckWriteSettings& settings) const
{
    return model::ModelFileWriter::toString(*m_model, modelFileOptionsOf(settings));
}

std::vector<std::pair<std::string, std::size_t>> Deck::cardCounts() const
{
    std::map<std::string, std::size_t> counts;
    const model::CardRegistry& registry = model::CardRegistry::instance();
    for (std::uint16_t type = 0; type < registry.size(); ++type)
    {
        const model::CardTable* table = m_model->findTable(type);
        if (table == nullptr || table->rows().aliveCount() == 0)
            continue;
        const std::string name = type == model::CardRegistry::UNKNOWN_TYPE
                                     ? std::string("(unknown cards)")
                                     : std::string(registry.entry(type).info.name);
        counts[name] += table->rows().aliveCount();
    }
    return {counts.begin(), counts.end()};
}

std::size_t Deck::cardCount() const
{
    return m_model->cardCount();
}

std::vector<DiagnosticText> Deck::diagnostics() const
{
    std::vector<DiagnosticText> texts;
    for (const base::Diagnostic& diagnostic : m_model->diagnostics().diagnostics())
        texts.push_back(diagnosticText(diagnostic));
    return texts;
}

} // namespace oor::api
