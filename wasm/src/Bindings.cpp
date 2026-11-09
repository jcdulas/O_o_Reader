// The JavaScript interface of the library, with Emscripten's embind. Each binding converts plain JavaScript values
// (objects, arrays, typed arrays) to the C++ types of oor::api and back; the work itself is done by oor::api (tested
// natively in tests/cpp/api). Files are read and written in Emscripten's in-memory file system (`module.FS`).
//
// Errors: every C++ exception becomes a JavaScript Error with the message of the exception.

#include "oor/api/CardEdits.hpp"
#include "oor/api/CardTables.hpp"
#include "oor/api/Deck.hpp"
#include "oor/api/Design.hpp"
#include "oor/api/Queries.hpp"
#include "oor/api/Results.hpp"
#include "oor/base/Version.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/query/table/Hdf5Export.hpp"
#include "oor/query/table/PlainColumn.hpp"
#include "oor/query/table/TablePackage.hpp"

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <cmath>
#include <deque>
#include <exception>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace
{

using emscripten::val;
using namespace oor;

// ------------------------------------------------------------------------------------------------- helpers

// Runs `work`; a C++ exception is thrown again as a JavaScript Error.
template <typename Work>
auto guarded(Work work) -> decltype(work())
{
    try
    {
        return work();
    }
    catch (const std::exception& error)
    {
        val::global("Error").new_(std::string(error.what())).throw_();
    }
    catch (...)
    {
        val::global("Error").new_(std::string("unknown C++ error")).throw_();
    }
    __builtin_unreachable();
}

// A member of a JavaScript options object, or `fallback` when the object or the member is missing.
template <typename T>
T option(const val& options, const char* name, T fallback)
{
    if (options.isUndefined() || options.isNull())
        return fallback;
    const val member = options[name];
    if (member.isUndefined() || member.isNull())
        return fallback;
    return member.as<T>();
}

// Copies of C++ vectors as JavaScript typed arrays and arrays.
template <typename T>
val typedArray(const char* type, const T* data, std::size_t count)
{
    return val::global(type).new_(emscripten::typed_memory_view(count, data));
}
val int32Array(const std::vector<std::int32_t>& values)
{
    return typedArray("Int32Array", values.data(), values.size());
}
val float64Array(const std::vector<double>& values)
{
    return typedArray("Float64Array", values.data(), values.size());
}
val textArray(const std::vector<std::string>& texts)
{
    val array = val::array();
    for (const std::string& text : texts)
        array.call<void>("push", text);
    return array;
}

template <typename T>
std::vector<T> vectorOf(const val& array)
{
    if (array.isUndefined() || array.isNull())
        return {};
    return emscripten::vecFromJSArray<T>(array);
}

val diagnosticsArray(const std::vector<api::DiagnosticText>& diagnostics)
{
    val array = val::array();
    for (const api::DiagnosticText& diagnostic : diagnostics)
    {
        val item = val::object();
        item.set("severity", diagnostic.severity);
        item.set("code", diagnostic.code);
        item.set("message", diagnostic.message);
        item.set("location", diagnostic.location);
        array.call<void>("push", item);
    }
    return array;
}

val reportObject(const results::ResultWriteReport& report)
{
    val object = val::object();
    object.set("path", report.path);
    object.set("format", report.format);
    object.set("finished", report.finished);
    object.set("bytes", static_cast<double>(report.bytes));
    val written = val::array();
    for (const results::WrittenTable& table : report.written)
    {
        val item = val::object();
        item.set("kind", table.kind);
        item.set("container", table.container);
        item.set("subcase", table.subcase);
        item.set("location", table.fileTable);
        item.set("steps", static_cast<double>(table.steps));
        item.set("rows", static_cast<double>(table.rows));
        written.call<void>("push", item);
    }
    object.set("written", written);
    val skipped = val::array();
    for (const results::SkippedTable& table : report.skipped)
    {
        val item = val::object();
        item.set("kind", table.kind);
        item.set("container", table.container);
        item.set("subcase", table.subcase);
        item.set("reason", table.reason);
        item.set("message", table.message);
        skipped.call<void>("push", item);
    }
    object.set("skipped", skipped);
    return object;
}

// ------------------------------------------------------------------------------------------------- deck

api::DeckWriteSettings deckSettings(const val& options)
{
    api::DeckWriteSettings settings;
    settings.splitIncludes = option<bool>(options, "splitIncludes", settings.splitIncludes);
    settings.headerComment = option<std::string>(options, "headerComment", settings.headerComment);
    return settings;
}

std::shared_ptr<api::Deck> readModel(const std::string& path, const val& options)
{
    return guarded(
        [&]
        {
            api::DeckReadSettings settings;
            settings.strict = option<bool>(options, "strict", false);
            return api::Deck::readModel(path, settings);
        });
}

// What the model file could not carry, one text each.
val writeModel(api::Deck& deck, const std::string& path, const val& options)
{
    return guarded(
        [&]
        {
            val dropped = val::array();
            for (const std::string& line : deck.writeModel(path, deckSettings(options)))
                dropped.call<void>("push", line);
            return dropped;
        });
}

std::string modelText(api::Deck& deck, const val& options)
{
    return guarded([&] { return deck.toModelText(deckSettings(options)); });
}

val deckCardCounts(api::Deck& deck)
{
    return guarded(
        [&]
        {
            val counts = val::object();
            for (const auto& [card, count] : deck.cardCounts())
                counts.set(card, static_cast<double>(count));
            return counts;
        });
}

double deckCardCount(api::Deck& deck)
{
    return static_cast<double>(deck.cardCount());
}

val deckDiagnostics(api::Deck& deck)
{
    return guarded([&] { return diagnosticsArray(deck.diagnostics()); });
}

// ------------------------------------------------------------------------------------------------- cards by ID

// A named value of a card as a JavaScript value: a number, an array of numbers, a text, an array of texts or null.
val fieldValue(const api::FieldValue& value)
{
    switch (value.kind)
    {
    case api::FieldValue::Kind::Number:
        return val(value.number);
    case api::FieldValue::Kind::Numbers:
        return float64Array(value.numbers);
    case api::FieldValue::Kind::Text:
        return val(value.text);
    case api::FieldValue::Kind::Texts:
        return textArray(value.texts);
    case api::FieldValue::Kind::Missing:
        break;
    }
    return val::null();
}

// deck.card(source, id, computed): { type, id, values: { name: value }, fields: [...], text, comment }, or null.
val deckCard(api::Deck& deck, const std::string& source, int id, const val& computed)
{
    return guarded(
        [&]
        {
            const std::optional<api::CardView> card = api::findCard(deck, source, id, vectorOf<std::string>(computed));
            if (!card)
                return val::null();
            val object = val::object();
            object.set("type", card->type);
            object.set("id", card->id);
            val values = val::object();
            for (const auto& [name, value] : card->values)
                values.set(name, fieldValue(value));
            object.set("values", values);
            object.set("fields", textArray(card->fields));
            object.set("text", card->text);
            object.set("comment", card->comment);
            return object;
        });
}

val deckCardIds(api::Deck& deck, const std::string& source)
{
    return guarded([&] { return int32Array(api::cardIds(deck, source)); });
}

val deckCardFields(api::Deck& deck, const std::string& source, int id)
{
    return guarded([&] { return textArray(api::cardFields(deck, source, id)); });
}

void deckSetCardField(api::Deck& deck, const std::string& source, int id, int field, const std::string& value)
{
    guarded([&] { api::setCardField(deck, source, id, field, value); });
}

void deckSetCardFields(api::Deck& deck, const std::string& source, int id, const val& fields)
{
    guarded([&] { api::setCardFields(deck, source, id, vectorOf<std::string>(fields)); });
}

void deckReplaceCard(api::Deck& deck, const std::string& source, int id, const std::string& text)
{
    guarded([&] { api::replaceCard(deck, source, id, text); });
}

// deck.addCards(text): [{ type, id }] of the cards added.
val deckAddCards(api::Deck& deck, const std::string& text)
{
    return guarded(
        [&]
        {
            val added = val::array();
            for (const api::AddedCard& card : api::addCards(deck, text))
            {
                val item = val::object();
                item.set("type", card.type);
                item.set("id", card.id);
                added.call<void>("push", item);
            }
            return added;
        });
}

// deck.cardFieldNames(source, id): [{ field, name }] of the fields of the card that have a name.
val deckCardFieldNames(api::Deck& deck, const std::string& source, int id)
{
    return guarded(
        [&]
        {
            val names = val::array();
            for (const auto& [field, name] : api::cardFieldNames(deck, source, id))
            {
                val item = val::object();
                item.set("field", field);
                item.set("name", name);
                names.call<void>("push", item);
            }
            return names;
        });
}

void deckSetCardValue(api::Deck& deck, const std::string& source, int id, const std::string& name,
                      const std::string& value)
{
    guarded([&] { api::setCardValue(deck, source, id, name, value); });
}

bool deckRemoveCard(api::Deck& deck, const std::string& source, int id)
{
    return guarded([&] { return api::removeCard(deck, source, id); });
}

// ------------------------------------------------------------------------------------------------- results

std::shared_ptr<api::Results> openResults(const std::string& path)
{
    return guarded([&] { return api::Results::open(path); });
}

std::string resultsFormat(api::Results& results)
{
    return results.format();
}

val resultsMetadata(api::Results& results)
{
    const results::ResultFileMetadata& metadata = results.metadata();
    val object = val::object();
    object.set("format", metadata.format);
    object.set("path", metadata.path);
    object.set("fileSize", static_cast<double>(metadata.fileSize));
    object.set("producer", metadata.producer);
    object.set("createdAt", metadata.createdAt);
    object.set("title", metadata.title);
    object.set("subtitle", metadata.subtitle);
    object.set("label", metadata.label);
    object.set("tableCount", metadata.tableCount);
    object.set("namedCount", metadata.namedCount);
    object.set("complete", metadata.complete);
    return object;
}

val resultsDiagnostics(api::Results& results)
{
    return guarded([&] { return diagnosticsArray(results.diagnostics()); });
}

// Results.fromTables(label, [table, ...]): results made by a program, one object a table:
//   { kind: "displacements", elementType: "ShellQuad4", variant: "", subcase: 1, analysisCode: 1,
//     stepKind: "time" | "frequency" | "mode" | "loadFactor", steps: [...], eigenvalues: [...], cycles: [...],
//     ids: [...], pointTypes, nodeIds, elementIds, layers, positions, texts: { name: [...] },
//     components: [...], values: Float32Array | Float64Array | [...], complex: false,
//     title, subtitle, label, tableName, units }
// The values are (steps, rows, components) in C order; complex values have their real and imaginary parts one after
// the other. A Float32Array keeps single precision.
std::shared_ptr<api::Results> resultsFromTables(const std::string& label, const val& tables)
{
    return guarded(
        [&]
        {
            // the values the inputs point to, alive until the tables are made (a deque never moves its items)
            std::deque<std::vector<double>> doubles;
            std::deque<std::vector<float>> floats;
            std::vector<results::MemoryTableInput> inputs;
            const auto count = tables["length"].as<unsigned>();
            for (unsigned index = 0; index < count; ++index)
            {
                const val table = tables[index];
                const std::string where = "table " + std::to_string(index) + ": ";
                results::MemoryTableInput input;
                const std::string kindName = option<std::string>(table, "kind", "");
                const std::optional<results::ResultKind> kind = results::resultKindFromName(kindName);
                if (!kind)
                    throw base::UsageError(where + "'" + kindName + "' is not a kind of result (displacements, ...)");
                input.kind = *kind;
                input.elementName = option<std::string>(table, "elementType", "");
                const std::optional<results::ResultVariant> variant =
                    results::resultVariantFromName(option<std::string>(table, "variant", ""));
                if (!variant)
                    throw base::UsageError(where + "unknown variant");
                input.variant = *variant;
                input.subcase = option<int>(table, "subcase", 1);
                input.analysisCode = option<int>(table, "analysisCode", 0);
                const std::string stepKind = option<std::string>(table, "stepKind", "");
                if (stepKind == "time")
                    input.stepKind = results::StepKind::Time;
                else if (stepKind == "frequency")
                    input.stepKind = results::StepKind::Frequency;
                else if (stepKind == "mode")
                    input.stepKind = results::StepKind::Mode;
                else if (stepKind == "loadFactor")
                    input.stepKind = results::StepKind::LoadFactor;
                else if (!stepKind.empty() && stepKind != "none" && stepKind != "loadSet")
                    throw base::UsageError(where + "unknown step kind '" + stepKind + "'");
                input.steps = vectorOf<double>(table["steps"]);
                if (input.steps.empty())
                    input.steps = {std::numeric_limits<double>::quiet_NaN()}; // one step with no value: static
                input.eigenvalues = vectorOf<double>(table["eigenvalues"]);
                input.cycles = vectorOf<double>(table["cycles"]);
                input.ids = vectorOf<std::int32_t>(table["ids"]);
                input.pointTypes = vectorOf<std::int32_t>(table["pointTypes"]);
                input.nodeIds = vectorOf<std::int32_t>(table["nodeIds"]);
                input.elementIds = vectorOf<std::int32_t>(table["elementIds"]);
                input.layers = vectorOf<std::int32_t>(table["layers"]);
                input.positions = vectorOf<double>(table["positions"]);
                input.components = vectorOf<std::string>(table["components"]);
                const val texts = table["texts"];
                if (!texts.isUndefined() && !texts.isNull())
                {
                    const val names = val::global("Object").call<val>("keys", texts);
                    for (unsigned name = 0; name < names["length"].as<unsigned>(); ++name)
                    {
                        const std::string textName = names[name].as<std::string>();
                        input.texts.emplace_back(textName, vectorOf<std::string>(texts[textName]));
                    }
                }
                const bool complex = option<bool>(table, "complex", false);
                const val values = table["values"];
                if (!values.isUndefined() && !values.isNull() && values.instanceof(val::global("Float32Array")))
                {
                    floats.push_back(emscripten::convertJSArrayToNumberVector<float>(values));
                    input.type = complex ? results::ValueType::Complex64 : results::ValueType::Float32;
                    input.values = std::as_bytes(std::span<const float>(floats.back()));
                }
                else
                {
                    doubles.push_back(vectorOf<double>(values));
                    input.type = complex ? results::ValueType::Complex128 : results::ValueType::Float64;
                    input.values = std::as_bytes(std::span<const double>(doubles.back()));
                }
                input.title = option<std::string>(table, "title", "");
                input.subtitle = option<std::string>(table, "subtitle", "");
                input.label = option<std::string>(table, "label", "");
                input.tableName = option<std::string>(table, "tableName", "");
                input.units = option<std::string>(table, "units", "unknown");
                inputs.push_back(std::move(input));
            }
            return api::Results::fromTables(label, std::move(inputs));
        });
}

val resultsTables(api::Results& results)
{
    return guarded(
        [&]
        {
            val array = val::array();
            for (const api::TableDescription& table : results.tables())
            {
                val item = val::object();
                item.set("index", static_cast<double>(table.index));
                item.set("kind", table.kind);
                item.set("variant", table.variant);
                item.set("elementType", table.elementType);
                item.set("layout", table.layout);
                item.set("subcase", table.subcase);
                item.set("analysis", table.analysis);
                item.set("stepKind", table.stepKind);
                item.set("steps", float64Array(table.steps));
                item.set("eigenvalues", float64Array(table.eigenvalues));
                item.set("cycles", float64Array(table.cycles));
                item.set("rowCount", static_cast<double>(table.rowCount));
                item.set("components", textArray(table.components));
                item.set("valueType", table.valueType);
                item.set("tableName", table.tableName);
                item.set("title", table.title);
                item.set("subtitle", table.subtitle);
                item.set("label", table.label);
                array.call<void>("push", item);
            }
            return array;
        });
}

val resultsRead(api::Results& results, double index, const val& selection)
{
    return guarded(
        [&]
        {
            api::TableSelection chosen;
            if (!selection.isUndefined() && !selection.isNull())
            {
                for (const double step : vectorOf<double>(selection["stepIndices"]))
                    chosen.stepIndices.push_back(static_cast<std::int64_t>(step));
                chosen.components = vectorOf<std::string>(selection["components"]);
                chosen.ids = vectorOf<std::int32_t>(selection["ids"]);
                chosen.elementIds = vectorOf<std::int32_t>(selection["elementIds"]);
            }
            const api::TableValues table = results.read(static_cast<std::size_t>(index), chosen);
            val object = val::object();
            val shape = val::array();
            for (const std::size_t extent : table.shape)
                shape.call<void>("push", static_cast<double>(extent));
            object.set("shape", shape);
            object.set("valueType", table.valueType);
            object.set("components", textArray(table.components));
            object.set("steps", float64Array(table.steps));
            object.set("ids", int32Array(table.ids));
            object.set("nodeIds", int32Array(table.nodeIds));
            object.set("layers", int32Array(table.layers));
            object.set("elementIds", int32Array(table.elementIds));
            val texts = val::object();
            for (const auto& [name, values] : table.texts)
                texts.set(name, textArray(values));
            object.set("texts", texts);
            // the values: float32 (or complex64) stay single precision, the others are double precision
            const results::ResultData& data = *table.data;
            const bool single = data.type == results::ValueType::Float32 || data.type == results::ValueType::Complex64;
            if (single)
            {
                const auto values = data.values<float>();
                object.set("values", typedArray("Float32Array", values.data(), values.size()));
            }
            else
            {
                const auto values = data.values<double>();
                object.set("values", typedArray("Float64Array", values.data(), values.size()));
            }
            return object;
        });
}

void resultsAttach(api::Results& results, std::shared_ptr<api::Deck> deck, bool strict)
{
    guarded([&] { results.attach(deck, strict); });
}

results::ArchiveWriteOptions archiveSettings(const val& options)
{
    results::ArchiveWriteOptions settings;
    settings.compressionLevel = option<int>(options, "compressionLevel", settings.compressionLevel);
    settings.shuffle = option<bool>(options, "shuffle", settings.shuffle);
    settings.strict = option<bool>(options, "strict", settings.strict);
    settings.title = option<std::string>(options, "title", settings.title);
    settings.subtitle = option<std::string>(options, "subtitle", settings.subtitle);
    settings.label = option<std::string>(options, "label", settings.label);
    return settings;
}

val deckWriteArchive(api::Deck& deck, const std::string& path, const val& options)
{
    return guarded([&] { return reportObject(deck.writeArchive(path, archiveSettings(options))); });
}

bool resultsHasModel(api::Results& results)
{
    return guarded([&] { return results.hasModel(); });
}

std::shared_ptr<api::Deck> resultsModel(api::Results& results)
{
    return guarded([&] { return results.model(); });
}

val resultsWriteArchive(api::Results& results, const std::string& path, const val& options)
{
    return guarded(
        [&]
        {
            auto settings = archiveSettings(options);
            return reportObject(results.writeArchive(path, settings));
        });
}

val resultsWriteXlsx(api::Results& results, const std::string& path, const val& options)
{
    return guarded(
        [&]
        {
            results::XlsxWriteOptions settings;
            settings.title = option<std::string>(options, "title", settings.title);
            settings.subtitle = option<std::string>(options, "subtitle", settings.subtitle);
            settings.label = option<std::string>(options, "label", settings.label);
            settings.compressionLevel = option<int>(options, "compressionLevel", settings.compressionLevel);
            return reportObject(results.writeXlsx(path, settings));
        });
}

// ------------------------------------------------------------------------------------------------- queries

// A query table handed to JavaScript (the table is immutable and shared).
struct QueryTable
{
    std::shared_ptr<const query::QueryTable> table;
};

std::shared_ptr<QueryTable> runQuery(const std::string& name, const val& deck, const val& results,
                                     const std::string& argumentsJson)
{
    return guarded(
        [&]
        {
            const std::shared_ptr<api::Deck> model =
                deck.isNull() || deck.isUndefined() ? nullptr : deck.as<std::shared_ptr<api::Deck>>();
            const std::shared_ptr<api::Results> set =
                results.isNull() || results.isUndefined() ? nullptr : results.as<std::shared_ptr<api::Results>>();
            const query::QueryArguments arguments = api::argumentsFromJson(name, argumentsJson);
            return std::make_shared<QueryTable>(QueryTable{api::runQuery(name, model, set, arguments)});
        });
}

val queryColumnNames(QueryTable& table)
{
    return textArray(table.table->columnNames());
}

double queryRowCount(QueryTable& table)
{
    return static_cast<double>(table.table->rowCount());
}

// A column as {name, kind, values | texts, offsets?, valid?}.
val columnObject(const query::QueryTable& table, std::size_t index)
{
    const query::PlainColumn column = query::plainColumn(table, index);
    val object = val::object();
    object.set("name", column.name);
    switch (column.kind)
    {
    case query::PlainKind::Number:
        object.set("kind", std::string("number"));
        break;
    case query::PlainKind::Text:
        object.set("kind", std::string("text"));
        break;
    case query::PlainKind::NumberList:
        object.set("kind", std::string("numberList"));
        break;
    case query::PlainKind::TextList:
        object.set("kind", std::string("textList"));
        break;
    }
    object.set("integer", column.integer);
    if (column.kind == query::PlainKind::Number || column.kind == query::PlainKind::NumberList)
        object.set("values", float64Array(column.numbers));
    else
        object.set("values", textArray(column.texts));
    if (!column.offsets.empty())
    {
        std::vector<double> offsets(column.offsets.begin(), column.offsets.end());
        object.set("offsets", float64Array(offsets));
    }
    if (!column.valid.empty())
        object.set("valid", typedArray("Uint8Array", column.valid.data(), column.valid.size()));
    return object;
}

val queryColumn(QueryTable& table, const std::string& name)
{
    return guarded(
        [&]
        {
            const std::optional<std::size_t> index = table.table->find(name);
            if (!index)
                throw query::QueryError("the table has no column '" + name + "'");
            return columnObject(*table.table, *index);
        });
}

val queryColumns(QueryTable& table)
{
    return guarded(
        [&]
        {
            val columns = val::object();
            for (std::size_t index = 0; index < table.table->columnCount(); ++index)
                columns.set(table.table->name(index), columnObject(*table.table, index));
            return columns;
        });
}

std::string queryCsv(QueryTable& table)
{
    return guarded([&] { return api::toCsv(*table.table); });
}

// writeQueryTables(path, [{name, table}], compressionLevel): the custom outputs, in HDF5.
void writeQueryTables(const std::string& path, const val& tables, int compressionLevel)
{
    guarded(
        [&]
        {
            std::vector<query::NamedQueryTable> named;
            const auto count = tables["length"].as<unsigned>();
            for (unsigned index = 0; index < count; ++index)
            {
                const val item = tables[index];
                named.push_back(query::NamedQueryTable{item["name"].as<std::string>(),
                                                       item["table"].as<std::shared_ptr<QueryTable>>()->table});
            }
            query::writeQueryTables(path, named, compressionLevel);
        });
}

val queryCatalogue()
{
    return guarded(
        [&]
        {
            val array = val::array();
            for (const api::QueryDescription& description : api::queryCatalogue())
            {
                val item = val::object();
                item.set("name", description.name);
                item.set("family", description.family);
                item.set("description", description.description);
                item.set("resultQuery", description.resultQuery);
                item.set("needsModel", description.needsModel);
                val parameters = val::array();
                for (const query::ParameterInfo& parameter : description.parameters)
                {
                    val entry = val::object();
                    entry.set("name", parameter.name);
                    entry.set("default", parameter.defaultText);
                    entry.set("description", parameter.description);
                    parameters.call<void>("push", entry);
                }
                item.set("parameters", parameters);
                array.call<void>("push", item);
            }
            return array;
        });
}

// ------------------------------------------------------------------------------------------------- card tables

// The table as a table package (TablePackage.hpp): a Uint8Array that keeps the exact Arrow layout, for another module.
val queryPackage(QueryTable& table)
{
    return guarded(
        [&]
        {
            const std::vector<std::uint8_t> bytes = query::packTable(*table.table);
            return typedArray("Uint8Array", bytes.data(), bytes.size());
        });
}

// cardTable(deck, source, columns): the cards of one type ("ShellQuad4"), the grid points ("nodes") or the other points
// ("points"), with the columns asked for (every stored column for an empty list).
std::shared_ptr<QueryTable> cardTable(const std::shared_ptr<api::Deck>& deck, const std::string& source,
                                      const val& columns)
{
    return guarded(
        [&]
        {
            std::vector<std::string> names;
            if (!columns.isNull() && !columns.isUndefined())
            {
                const auto count = columns["length"].as<unsigned>();
                for (unsigned index = 0; index < count; ++index)
                    names.push_back(columns[index].as<std::string>());
            }
            return std::make_shared<QueryTable>(QueryTable{api::cardTable(*deck, source, names)});
        });
}

// cardTableColumns(source): {stored: [...], computed: [...]}.
val cardTableColumns(const std::string& source)
{
    return guarded(
        [&]
        {
            const api::CardTableColumns columns = api::cardTableColumns(source);
            val result = val::object();
            result.set("stored", textArray(columns.stored));
            result.set("computed", textArray(columns.computed));
            return result;
        });
}

val cardTypesOf(const std::shared_ptr<api::Deck>& deck, const std::string& category)
{
    return guarded([&] { return textArray(api::cardTypesOf(*deck, category)); });
}

val displacementRotations(const std::shared_ptr<api::Deck>& deck)
{
    return guarded([&] { return float64Array(api::displacementRotations(*deck)); });
}

// The version counters as numbers (exact up to 2^53, far above any count of edits).
val modelVersions(const std::shared_ptr<api::Deck>& deck)
{
    return guarded(
        [&]
        {
            const api::ModelVersions versions = api::modelVersions(*deck);
            val result = val::object();
            result.set("structure", static_cast<double>(versions.structure));
            result.set("content", static_cast<double>(versions.content));
            return result;
        });
}

val designVariables(const std::shared_ptr<api::Deck>& deck)
{
    return guarded(
        [&]
        {
            const auto design = api::designModel(*deck);
            val result = val::object();
            val variables = val::array();
            for (const auto& variable : design.variables())
            {
                val item = val::object();
                item.set("id", variable.id);
                item.set("label", variable.label);
                item.set("initialValue", variable.initial);
                item.set("lower", variable.lower);
                item.set("upper", variable.upper);
                item.set("moveLimit", variable.moveLimit);
                item.set("dependent", variable.link != 0);
                variables.call<void>("push", item);
            }
            result.set("variables", variables);
            result.set("problems", textArray(design.problems()));
            val unsupported = val::array();
            for (const auto& entry : design.notApplied())
            {
                val item = val::object();
                item.set("card", entry.card);
                item.set("id", entry.id);
                item.set("reason", entry.reason);
                unsupported.call<void>("push", item);
            }
            result.set("notApplied", unsupported);
            return result;
        });
}

val applyDesignPoint(const std::shared_ptr<api::Deck>& deck, const val& ids, const val& values, const val& options)
{
    return guarded(
        [&]
        {
            const auto mode = option<std::string>(options, "limits", "error");
            const auto limits = mode == "error"  ? model::DesignLimitMode::Error
                                : mode == "clip" ? model::DesignLimitMode::Clip
                                                 : model::DesignLimitMode::Ignore;
            if (mode != "error" && mode != "clip" && mode != "ignore")
                throw std::invalid_argument("design limits must be error, clip or ignore");
            // Check before narrowing: a fractional or overflowing ID must not reach another variable.
            std::vector<std::int32_t> checkedIds;
            for (const double id : vectorOf<double>(ids))
            {
                if (!std::isfinite(id) || std::floor(id) != id || id < 1 || id > 2147483647.0)
                    throw std::invalid_argument("design variable IDs must be positive 32-bit integers");
                checkedIds.push_back(static_cast<std::int32_t>(id));
            }
            const auto update = api::applyDesignPoint(*deck, checkedIds, vectorOf<double>(values), limits,
                                                      option<bool>(options, "setInitialValues", true));
            val result = val::object();
            result.set("variables", float64Array(update.variables));
            result.set("changedFields", double(update.changedFields));
            val fields = val::array();
            for (const auto& change : update.fields)
            {
                val item = val::object();
                item.set("relation", double(change.relation));
                item.set("before", change.before);
                item.set("requested", change.requested);
                item.set("after", change.after);
                item.set("limit", static_cast<int>(change.limit));
                item.set("changed", change.changed);
                fields.call<void>("push", item);
            }
            result.set("fields", fields);
            result.set("movedGrids", double(update.grids.size()));
            return result;
        });
}

std::string version()
{
    return std::string(base::LIBRARY_VERSION);
}

} // namespace

EMSCRIPTEN_BINDINGS(oor_reader)
{
    using namespace emscripten;

    function("version", &version);

    class_<api::Deck>("Deck")
        .smart_ptr<std::shared_ptr<api::Deck>>("DeckPointer")
        .class_function("readModel", &readModel)
        .function("writeModel", &writeModel)
        .function("writeArchive", &deckWriteArchive)
        .function("toModelText", &modelText)
        .function("cardCounts", &deckCardCounts)
        .function("cardCount", &deckCardCount)
        .function("diagnostics", &deckDiagnostics)
        .function("card", &deckCard)
        .function("cardIds", &deckCardIds)
        .function("cardFields", &deckCardFields)
        .function("setCardField", &deckSetCardField)
        .function("setCardFields", &deckSetCardFields)
        .function("replaceCard", &deckReplaceCard)
        .function("addCards", &deckAddCards)
        .function("removeCard", &deckRemoveCard)
        .function("cardFieldNames", &deckCardFieldNames)
        .function("setCardValue", &deckSetCardValue);

    class_<api::Results>("Results")
        .smart_ptr<std::shared_ptr<api::Results>>("ResultsPointer")
        .class_function("open", &openResults)
        .class_function("fromTables", &resultsFromTables)
        .function("format", &resultsFormat)
        .function("metadata", &resultsMetadata)
        .function("diagnostics", &resultsDiagnostics)
        .function("tables", &resultsTables)
        .function("read", &resultsRead)
        .function("attach", &resultsAttach)
        .function("hasModel", &resultsHasModel)
        .function("model", &resultsModel)
        .function("writeArchive", &resultsWriteArchive)
        .function("writeXlsx", &resultsWriteXlsx);

    class_<QueryTable>("QueryTable")
        .smart_ptr<std::shared_ptr<QueryTable>>("QueryTablePointer")
        .function("columnNames", &queryColumnNames)
        .function("rowCount", &queryRowCount)
        .function("column", &queryColumn)
        .function("columns", &queryColumns)
        .function("toCsv", &queryCsv)
        .function("toPackage", &queryPackage);

    function("runQuery", &runQuery);
    function("queryCatalogue", &queryCatalogue);
    function("writeQueryTables", &writeQueryTables);

    function("cardTable", &cardTable);
    function("cardTableColumns", &cardTableColumns);
    function("cardTypesOf", &cardTypesOf);
    function("displacementRotations", &displacementRotations);
    function("modelVersions", &modelVersions);
    function("designVariables", &designVariables);
    function("applyDesignPoint", &applyDesignPoint);
}
