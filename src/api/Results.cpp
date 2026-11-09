#include "oor/api/Results.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/results/attach/ResultAttachment.hpp"
#include "oor/results/core/ReadResults.hpp"

#include <filesystem>

namespace oor::api
{

std::shared_ptr<Results> Results::open(const std::string& path)
{
    std::shared_ptr<Results> opened(new Results());
    opened->m_file = results::readResults(path);
    opened->m_tables = std::make_shared<results::ResultTables>(results::ResultTables::build(opened->m_file, {}));
    opened->m_list = opened->m_tables->all();
    return opened;
}

std::shared_ptr<Results> Results::fromTables(const std::string& label, std::vector<results::MemoryTableInput> inputs)
{
    std::shared_ptr<Results> made(new Results());
    made->m_file = std::make_shared<results::MemorySetFile>(label);
    made->m_tables = std::make_shared<results::ResultTables>();
    std::vector<std::shared_ptr<results::ResultTable>> tables;
    for (results::MemoryTableInput& input : inputs)
        tables.push_back(results::makeMemoryTable(std::move(input)));
    results::MemoryTableIndex index;
    results::addMemoryTables(*made->m_tables, index, tables); // refuses two tables with the same key
    made->m_list = made->m_tables->all();
    return made;
}

std::string Results::format() const
{
    return std::string(m_file->format());
}

const results::ResultFileMetadata& Results::metadata() const
{
    return m_file->metadata();
}

std::vector<DiagnosticText> Results::diagnostics() const
{
    std::vector<DiagnosticText> texts;
    for (const base::Diagnostic& diagnostic : m_file->diagnostics())
        texts.push_back(diagnosticText(diagnostic));
    if (m_attachment)
        for (const base::Diagnostic& diagnostic : m_attachment->diagnostics())
            texts.push_back(diagnosticText(diagnostic));
    return texts;
}

std::vector<TableDescription> Results::tables() const
{
    std::vector<TableDescription> descriptions;
    for (std::size_t index = 0; index < m_list.size(); ++index)
    {
        const results::ResultTableMeta& meta = m_list[index]->meta();
        TableDescription description;
        description.index = index;
        description.kind = std::string(results::resultKindName(meta.kind));
        description.variant = std::string(results::resultVariantName(meta.variant));
        description.elementType = meta.elementName.empty() ? meta.subName : meta.elementName;
        description.layout = meta.layout;
        description.subcase = meta.subcase;
        description.analysis = std::string(results::analysisTypeName(meta.analysisCode));
        description.stepKind = std::string(results::stepKindName(meta.stepKind));
        description.steps = meta.steps;
        description.eigenvalues = meta.eigenvalues;
        description.cycles = meta.cycles;
        description.rowCount = meta.rowCount;
        description.components = meta.components;
        description.valueType = std::string(results::valueTypeName(meta.valueType));
        description.tableName = meta.tableName;
        description.title = meta.title;
        description.subtitle = meta.subtitle;
        description.label = meta.label;
        descriptions.push_back(std::move(description));
    }
    return descriptions;
}

TableValues Results::read(std::size_t index, const TableSelection& selection) const
{
    if (index >= m_list.size())
        throw base::UsageError("there is no result table " + std::to_string(index) + " (the file has " +
                               std::to_string(m_list.size()) + ")");
    std::shared_ptr<results::ResultTable> table = m_list[index];
    results::ResultFilter filter;
    if (!selection.stepIndices.empty())
        filter.stepIndices = selection.stepIndices;
    if (!selection.components.empty())
        filter.components = selection.components;
    if (!selection.ids.empty())
        filter.ids = selection.ids;
    if (!selection.elementIds.empty())
        filter.elementIds = selection.elementIds;
    if (!filter.isEmpty())
        table = table->select(filter);

    const std::shared_ptr<const results::ResultData> data = table->data();
    TableValues values;
    values.shape = {data->stepCount, data->rowCount, data->componentCount};
    if (results::isComplexType(data->type))
        values.shape.push_back(2);
    values.valueType = std::string(results::valueTypeName(data->type));
    values.components = data->components;
    values.steps = data->steps;
    values.ids = data->ids.toVector();
    values.nodeIds = data->nodeIds.toVector();
    values.layers = data->layers.toVector();
    values.elementIds = data->elementIds.toVector();
    for (const results::ResultText& text : data->texts)
        values.texts.emplace_back(text.name, text.values());
    values.data = data;
    return values;
}

void Results::attach(const std::shared_ptr<Deck>& deck, bool strict)
{
    if (!deck)
        throw base::UsageError("attach needs a deck");
    results::AttachOptions options;
    options.strict = strict;
    m_attachment = results::ResultAttachment::attach(deck->model(), m_file, m_tables, options);
}

results::ResultWriteReport Results::writeArchive(const std::string& path,
                                                 const results::ArchiveWriteOptions& options) const
{
    return results::writeArchive(path, *m_tables, options);
}

results::ResultWriteReport Results::writeXlsx(const std::string& path, const results::XlsxWriteOptions& options) const
{
    return results::writeXlsx(path, *m_tables, options);
}

query::ResultSet Results::resultSet() const
{
    query::ResultSet set;
    set.analysis = std::filesystem::path(m_file->path()).filename().string();
    set.tables = m_tables;
    set.attachment = m_attachment;
    set.file = m_file;
    return set;
}

} // namespace oor::api
