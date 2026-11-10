#include "oor/api/CardEdits.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/NumberFormatter.hpp"
#include "oor/base/text/NumberParser.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/deck/records/ModelFileLexer.hpp"
#include "oor/deck/records/RecordText.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/model/core/CardTable.hpp"
#include "oor/model/core/ModelFileWriter.hpp"
#include "oor/model/format/CardLayouts.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/LayoutSlots.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/operators/ExecutionContext.hpp"
#include "oor/query/providers/CardColumns.hpp"
#include "oor/query/providers/StoredColumnCatalogue.hpp"
#include "oor/query/table/PlainColumn.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <span>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace oor::api
{

namespace
{

using model::CardLocation;
using model::CardRegistry;
using model::CardTable;

constexpr const char* EDIT_SOURCE = "(edited card)";

// The ID space of a category of cards.
std::optional<base::IdSpace> spaceOfCategory(const std::string& category)
{
    if (category == "nodes")
        return base::IdSpace::Node;
    if (category == "elements")
        return base::IdSpace::Element;
    if (category == "properties")
        return base::IdSpace::Property;
    if (category == "materials")
        return base::IdSpace::Material;
    if (category == "coordinates")
        return base::IdSpace::Coordinate;
    return std::nullopt;
}

std::uint16_t cardTypeOf(const std::string& name)
{
    const std::optional<std::uint16_t> type = CardRegistry::instance().typeIndexOf(name);
    if (!type || *type == CardRegistry::UNKNOWN_TYPE)
        throw query::QueryError("'" + name +
                                "' is neither a category (nodes, elements, properties, materials, coordinates) nor a "
                                "card type the library knows");
    return *type;
}

// Where the card of a type and ID is: through the index of its ID space when it has one, else by a walk over the live
// rows of the table (the set cards, which share their IDs).
std::optional<CardLocation> locateInType(const model::Model& model, std::uint16_t type, base::EntityId id)
{
    const CardTable* table = model.findTable(type);
    if (table == nullptr)
        return std::nullopt;
    const base::IdSpace space = table->info().idSpace;
    if (space == base::IdSpace::PerCardType)
        return model.locateInType(type, id);
    if (space != base::IdSpace::Group && space != base::IdSpace::None)
    {
        const std::optional<CardLocation> found = model.locate(space, id);
        return found && found->table == type ? found : std::nullopt;
    }
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        if (table->rows().isAlive(row) && table->idAt(row) == id)
            return CardLocation{type, row};
    return std::nullopt;
}

std::optional<CardLocation> locate(const model::Model& model, const std::string& source, base::EntityId id)
{
    if (const std::optional<base::IdSpace> space = spaceOfCategory(source))
        return model.locate(*space, id);
    return locateInType(model, cardTypeOf(source), id);
}

CardLocation locateOrThrow(const model::Model& model, const std::string& source, base::EntityId id)
{
    const std::optional<CardLocation> location = locate(model, source, id);
    if (!location)
        throw base::UsageError("there is no card " + std::to_string(id) + " in " + source);
    return *location;
}

// The value of the first row of a column.
FieldValue valueOf(const query::QueryColumn& column)
{
    const query::PlainColumn plain = query::plainColumn(column, "");
    FieldValue value;
    if (column.size() == 0 || (!plain.valid.empty() && plain.valid[0] == 0))
        return value; // a null: Missing
    value.integer = plain.integer;
    switch (plain.kind)
    {
    case query::PlainKind::Number:
        value.kind = FieldValue::Kind::Number;
        value.number = plain.numbers[0];
        break;
    case query::PlainKind::Text:
        value.kind = FieldValue::Kind::Text;
        value.text = plain.texts[0];
        break;
    case query::PlainKind::NumberList:
        value.kind = FieldValue::Kind::Numbers;
        value.numbers.assign(plain.numbers.begin() + plain.offsets[0], plain.numbers.begin() + plain.offsets[1]);
        break;
    case query::PlainKind::TextList:
        value.kind = FieldValue::Kind::Texts;
        value.texts.assign(plain.texts.begin() + plain.offsets[0], plain.texts.begin() + plain.offsets[1]);
        break;
    }
    return value;
}

// The card records of an edit. Editing accepts records and their comments, not sections or Include directives.
std::vector<deck::ModelItem> recordItems(const std::string& text)
{
    std::vector<deck::ModelItem> records;
    std::istringstream input(text);
    std::string line;
    std::string comments;
    std::uint32_t number = 0;
    while (std::getline(input, line))
    {
        ++number;
        const std::string_view content = base::trimAscii(line);
        if (content.empty())
            continue;
        if (content.front() == '#')
        {
            if (!comments.empty())
                comments.push_back('\n');
            comments += content.substr(1);
            continue;
        }
        deck::ModelItem record;
        record.line = number;
        record.comments = std::exchange(comments, {});
        std::string problem;
        if (content.front() == '[' || !deck::RecordText::split(content, record.values, problem) ||
            record.values.empty() || record.values.front() == deck::ModelFileLexer::INCLUDE_RECORD)
            throw base::UsageError("an edit contains card records only, line " + std::to_string(number) +
                                   (problem.empty() ? std::string() : ": " + problem));
        records.push_back(std::move(record));
    }
    return records;
}

std::vector<std::string> fieldsOfText(const std::string& text)
{
    const std::vector<deck::ModelItem> records = recordItems(text);
    if (records.size() != 1)
        throw base::UsageError("the text holds " + std::to_string(records.size()) + " records where one is expected");
    return {records.front().values.begin() + 1, records.front().values.end()};
}

std::string cardText(const model::Model& model, CardLocation location)
{
    return model::ModelFileWriter::cardToRecord(model, location);
}

std::string recordText(const std::string& name, const std::vector<std::string>& fields)
{
    std::vector<std::string> values{name};
    values.insert(values.end(), fields.begin(), fields.end());
    for (const std::string& value : values)
        if (value.find_first_of("\r\n") != std::string::npos)
            throw base::UsageError("a model record has no line break inside a value");
    return deck::RecordText::format(values) + "\n";
}

// The messages of the errors of a read, one a line.
std::string errorsOf(const base::DiagnosticSink& sink)
{
    std::string text;
    for (const base::Diagnostic& diagnostic : sink.diagnostics())
        if (diagnostic.severity == base::Severity::Error)
            text += "\n  " + diagnostic.message;
    return text;
}

// Temporary rows always get fresh keys, including replacements. A failed replacement must leave the row book
// sorted; a successful store keeps the target's old key and tombstones the temporary row.
class NewKeys final : public model::KeySource
{
public:
    explicit NewKeys(model::Model& model) : m_model(model) {}
    base::CardKey next() override { return m_model.newKey(); }

private:
    model::Model& m_model;
};

// The card already in the model that has the ID of a new row in the same ID space, when there is one.
std::optional<CardLocation> takenBy(const model::Model& model, std::uint16_t type, base::EntityId id)
{
    const base::IdSpace space = CardRegistry::instance().entry(type).info.idSpace;
    if (space == base::IdSpace::Group || space == base::IdSpace::None)
        return std::nullopt;
    return space == base::IdSpace::PerCardType ? model.locateInType(type, id) : model.locate(space, id);
}

// Map a compact record to native slots. Fixed layouts keep numeric values typed; complex layouts use the
// catalogue's record mapper. There is no model-file lexer, temporary file or whole-model copy on either path.
std::vector<model::FieldValue> inputSlots(const model::CardLayout& layout, const CardInput& input)
{
    using Kind = model::FieldValue::Kind;
    for (const auto& value : input.fields)
    {
        if ((value.kind == Kind::Real && !std::isfinite(value.real)) ||
            value.text.find_first_of("\r\n") != std::string::npos)
            throw base::UsageError("a card field must be finite and contain no line break");
        if (value.kind != Kind::Blank && value.kind != Kind::Integer && value.kind != Kind::Real &&
            value.kind != Kind::Text)
            throw base::UsageError("unsupported card input value kind");
    }
    if (!input.form.empty() && layout.forms.empty())
        throw base::UsageError("this card has no forms");
    if (layout.forms.empty() && !layout.group && !layout.sections)
    {
        if (input.fields.size() > layout.fields.size())
            throw base::UsageError(input.type + " has too many fields");
        std::vector<model::FieldValue> slots;
        for (std::size_t i = 0; i < input.fields.size(); ++i)
        {
            auto value = input.fields[i];
            const auto& field = layout.fields[i];
            if (field.kind == model::LayoutKind::Integer &&
                (value.kind == Kind::Real ||
                 (value.kind == Kind::Text && !value.text.empty() && !base::NumberParser::parseInteger(value.text))))
                throw base::UsageError(input.type + "." + field.name + " takes an integer");
            if (field.kind == model::LayoutKind::Real && value.kind == Kind::Text &&
                base::NumberParser::parseInteger(value.text))
                value.text = std::string(base::trimAscii(value.text)) + ".";
            if (field.kind == model::LayoutKind::Real && value.kind == Kind::Integer)
            {
                value.kind = Kind::Real;
                value.real = static_cast<double>(value.integer);
            }
            const auto slot = static_cast<std::size_t>(field.slot - 1);
            if (slots.size() <= slot)
                slots.resize(slot + 1);
            if (value.kind != Kind::Blank && !(value.kind == Kind::Text && value.text.empty()))
            {
                if (slots[slot].kind != Kind::Blank)
                    throw base::UsageError(input.type + ": two alternatives fill the same slot");
                slots[slot] = std::move(value);
            }
        }
        return slots;
    }
    std::vector<std::string> values;
    values.reserve(input.fields.size());
    for (const auto& field : input.fields)
        values.push_back(field.kind == Kind::Integer ? base::NumberFormatter::formatInteger(field.integer)
                         : field.kind == Kind::Real  ? base::NumberFormatter::formatRealFree(field.real)
                                                     : field.text);
    if (!input.form.empty())
    {
        const auto* selected = model::recordLayout(layout, values);
        const auto requested = std::find_if(layout.forms.begin(), layout.forms.end(),
                                            [&](const auto& form) { return form.name == input.form; });
        if (requested == layout.forms.end() || selected != &requested->layout)
            throw base::UsageError("the supplied values do not select form '" + input.form + "'");
    }
    std::vector<std::string> texts;
    std::string problem;
    if (!layout.toSlots(values, texts, problem))
        throw base::UsageError("the record " + input.type + " cannot be read: " + problem);
    std::vector<model::FieldValue> slots;
    slots.reserve(texts.size());
    for (auto& text : texts)
        slots.push_back(model::FieldValue{Kind::Text, 0, 0, std::move(text)});
    return slots;
}

// All rows are checked before any ID is registered. Failed rows become tombstones; keys are never reused.
std::vector<AddedCard> readInputs(model::Model& model, std::size_t count,
                                  const std::function<CardInput(std::size_t)>& get,
                                  std::optional<CardLocation> replacing, bool singular = false)
{
    const CardRegistry& registry = CardRegistry::instance();
    if (replacing && count != 1)
        throw base::UsageError("a card is replaced by one card");
    std::optional<base::CardKey> key;
    if (replacing)
        key = model.findTable(replacing->table)->rows().keyAt(replacing->row);

    base::DiagnosticSink sink;
    std::vector<CardLocation> added;
    added.reserve(count);
    std::vector<std::pair<base::CardKey, std::string>> comments;
    std::vector<std::pair<CardLocation, std::vector<std::uint8_t>>> sourceDigits;
    const auto undo = [&]
    {
        for (const CardLocation& location : added)
            model.findTable(location.table)->rows().kill(location.row);
    };
    std::unordered_set<std::uint64_t> idsOfBatch;
    idsOfBatch.reserve(count);
    try
    {
        for (std::size_t index = 0; index < count; ++index)
        {
            const CardInput input = get(index);
            const std::string& cardName = input.type;
            const std::optional<std::uint16_t> type = registry.typeIndexOf(cardName);
            if (!type || *type == CardRegistry::UNKNOWN_TYPE)
                throw base::UsageError("'" + cardName + "' is not a card the library knows");
            if (replacing && *type != replacing->table)
                throw base::UsageError("a replacement must keep the card type");
            const model::CardLayout* layout = model::CardLayouts::standard()->find(cardName);
            if (layout == nullptr)
                throw base::UsageError("no layout for " + cardName);
            const auto slots = inputSlots(*layout, input);
            deck::RawCard card;
            card.name = *deck::CardName::fromText(cardName);
            card.line = static_cast<std::uint32_t>(index + 1);
            card.fieldCount = static_cast<std::uint32_t>(slots.size());
            CardTable& table = model.ensureTable(*type);
            model::FieldReader reader(slots, card, sink, EDIT_SOURCE);
            std::vector<std::uint8_t> digits;
            reader.recordDigits(digits);
            NewKeys keys(model);
            std::vector<std::uint32_t> rows;
            const bool read = table.readCards(reader, keys, rows);
            for (const std::uint32_t row : rows)
                added.push_back(CardLocation{*type, row});
            if (!read || reader.failed())
                throw base::UsageError("the card " + cardName + " cannot be read:" + errorsOf(sink));
            if ((replacing || singular) && rows.size() != 1)
                throw base::UsageError(
                    "this record creates multiple entities: use addCards; a replacement must produce one card");
            for (const std::uint32_t row : rows)
            {
                const CardLocation location{*type, row};
                if (!digits.empty())
                    sourceDigits.emplace_back(location, digits);
                if (!replacing && input.comment && !input.comment->empty())
                    comments.emplace_back(table.rows().keyAt(row), *input.comment);
                const base::EntityId id = table.idAt(row);
                const base::IdSpace space = table.info().idSpace;
                const std::optional<CardLocation> other = takenBy(model, *type, id);
                const bool takenInModel = other && !(replacing && *other == *replacing);
                const auto spaceKey = static_cast<std::uint64_t>(
                    space == base::IdSpace::PerCardType ? 1000 + *type : static_cast<int>(space));
                const std::uint64_t batchKey = (spaceKey << 32) | static_cast<std::uint32_t>(id);
                const bool indexed = space != base::IdSpace::Group && space != base::IdSpace::None;
                if (indexed && (takenInModel || !idsOfBatch.insert(batchKey).second))
                    throw base::DuplicateIdError("ID " + std::to_string(id) + " is already used in the " +
                                                 std::string(base::idSpaceName(space)) + " ID space");
            }
        }
    }
    catch (...)
    {
        undo();
        throw;
    }
    for (const auto& [location, digits] : sourceDigits)
    {
        auto& rows = model.findTable(location.table)->rows();
        rows.forgetSource(location.row);
        rows.setSource(location.row, digits);
    }
    if (replacing)
    {
        CardTable& table = *model.findTable(replacing->table);
        const base::EntityId id = table.idAt(added.front().row);
        const bool renumbered = id != table.idAt(replacing->row);
        if (renumbered)
            model.setId(*replacing, id);
        table.moveLastRowTo(replacing->row);
        if (table.info().idSpace == base::IdSpace::Element ||
            (renumbered && table.info().idSpace == base::IdSpace::Node))
            model.noteStructureChange();
        else
            model.noteContentChange();
        model.diagnostics().append(std::move(sink));
        return {AddedCard{std::string(table.info().name), id, key->value}};
    }
    std::vector<AddedCard> result;
    result.reserve(added.size());
    for (const CardLocation& location : added)
    {
        const CardTable& table = *model.findTable(location.table);
        model.registerRead(location, table.idAt(location.row), sink, EDIT_SOURCE, 0);
        result.push_back(AddedCard{std::string(table.info().name), table.idAt(location.row),
                                   table.rows().keyAt(location.row).value});
    }
    for (const auto& [cardKey, comment] : comments)
        model.setComment(cardKey, comment);
    model.diagnostics().append(std::move(sink));
    return result;
}

std::vector<AddedCard> readCards(model::Model& model, const std::string& text, std::optional<CardLocation> replacing)
{
    const auto records = recordItems(text);
    if (records.empty())
        throw base::UsageError("the text holds no card");
    return readInputs(
        model, records.size(),
        [&](std::size_t i)
        {
            CardInput input;
            input.type = records[i].values.front();
            input.comment = records[i].comments;
            for (std::size_t j = 1; j < records[i].values.size(); ++j)
                input.fields.push_back(model::FieldValue{model::FieldValue::Kind::Text, 0, 0, records[i].values[j]});
            return input;
        },
        replacing);
}

} // namespace

namespace
{
CardView viewAt(const Deck& deck, CardLocation place, const std::vector<std::string>& computed)
{
    const auto& model = deck.model();
    const auto location = std::optional<CardLocation>(place);
    const CardTable& table = *model->findTable(location->table);

    CardView view;
    view.type = std::string(table.info().name);
    view.id = table.idAt(location->row);
    view.key = table.rows().keyAt(location->row).value;
    // the named values: the columns of this one row
    query::CardColumns columns(model, {location->table});
    const std::vector<std::uint32_t> rows = {location->row};
    query::CardRows cardRows;
    cardRows.rows = rows;
    cardRows.single = location->table;
    query::ExecutionContext context;
    std::vector<std::string> names = query::StoredColumnCatalogue::instance().names(location->table);
    names.insert(names.end(), computed.begin(), computed.end());
    for (const std::string& name : names)
        for (const query::NamedColumn& column : columns.fetch(name, cardRows, context))
            view.values.emplace_back(column.name, valueOf(column.column));

    view.fields = fieldsOfText(cardText(*model, *location));
    view.text = cardText(*model, *location);
    view.comment = model->commentOf(table.rows().keyAt(location->row));
    return view;
}
} // namespace

std::optional<CardView> findCard(const Deck& deck, const std::string& source, base::EntityId id,
                                 const std::vector<std::string>& computed)
{
    const std::shared_ptr<model::Model>& model = deck.model();
    const std::optional<CardLocation> location = locate(*model, source, id);
    if (!location)
        return std::nullopt;
    return viewAt(deck, *location, computed);
}

std::optional<CardView> findCardByKey(const Deck& deck, const std::string& type, std::uint32_t key,
                                      const std::vector<std::string>& computed)
{
    const auto location = deck.model()->locateKey(cardTypeOf(type), base::CardKey{key});
    return location ? std::optional<CardView>(viewAt(deck, *location, computed)) : std::nullopt;
}

AddedCard insertCard(const Deck& deck, const CardInput& card)
{
    deck.model()->reserveIds(cardTypeOf(card.type), 1);
    return readInputs(*deck.model(), 1, [&](std::size_t) { return card; }, std::nullopt, true).front();
}

std::vector<AddedCard> insertCards(const Deck& deck, std::span<const CardInput> cards)
{
    std::unordered_map<std::uint16_t, std::size_t> counts;
    for (const auto& card : cards)
        ++counts[cardTypeOf(card.type)];
    for (const auto& [type, count] : counts)
        deck.model()->reserveIds(type, count);
    return readInputs(*deck.model(), cards.size(), [&](std::size_t i) { return cards[i]; }, std::nullopt);
}

std::vector<AddedCard> insertCardBatch(const Deck& deck, const std::string& type, std::size_t count,
                                       const std::function<CardInput(std::size_t)>& row)
{
    model::ColumnGrowth::Batch growth(count);
    deck.model()->reserveIds(cardTypeOf(type), count);
    return readInputs(
        *deck.model(), count,
        [&](std::size_t i)
        {
            CardInput input = row(i);
            if (input.type != type)
                throw base::UsageError("every row of a batch must have the declared type");
            return input;
        },
        std::nullopt);
}

void updateCardByKey(const Deck& deck, const std::string& type, std::uint32_t key, const CardInput& card)
{
    const auto location = deck.model()->locateKey(cardTypeOf(type), base::CardKey{key});
    if (!location)
        throw base::UsageError("the card key no longer exists");
    (void)readInputs(*deck.model(), 1, [&](std::size_t) { return card; }, location);
    if (card.comment)
        deck.model()->setComment(base::CardKey{key}, *card.comment);
}

bool removeCardByKey(const Deck& deck, const std::string& type, std::uint32_t key)
{
    const auto location = deck.model()->locateKey(cardTypeOf(type), base::CardKey{key});
    if (!location)
        return false;
    deck.model()->remove(*location);
    return true;
}

void setCardCommentByKey(const Deck& deck, const std::string& type, std::uint32_t key, const std::string& comment)
{
    if (!deck.model()->locateKey(cardTypeOf(type), base::CardKey{key}))
        throw base::UsageError("the card key no longer exists");
    deck.model()->setComment(base::CardKey{key}, comment);
}

std::vector<AddedCard> appendInclude(const Deck& deck, const std::string& path, std::span<const CardInput> cards)
{
    auto& root = deck.model()->root();
    if (path.empty() || path.find_first_of("\r\n") != std::string::npos)
        throw base::UsageError("an Include needs a path with no line break");
    std::string portablePath = path;
    std::replace(portablePath.begin(), portablePath.end(), '\\', '/');
    const auto filePath = base::pathFromUtf8(portablePath).lexically_normal();
    if (filePath.has_root_path() || portablePath.find(':') != std::string::npos || filePath.extension() != ".oos" ||
        (!filePath.empty() && *filePath.begin() == ".."))
        throw base::UsageError("a new Include path must be relative, end in .oos and stay inside the model folder");
    const auto sourcePath =
        ((root.files().empty() ? std::filesystem::path{} : root.files()[0].path.parent_path()) / filePath)
            .lexically_normal();
    for (const auto& file : root.files())
        if (file.path.lexically_normal() == sourcePath)
            throw base::UsageError("the Include path already exists");
    if (filePath == "model.oos")
        throw base::UsageError("model.oos is reserved for the main source");
    const auto firstKey = deck.model()->nextKeyValue();
    auto result = insertCards(deck, cards);
    if (root.files().empty())
        root.files().push_back({base::pathFromUtf8("model.oos"), {}, -1, 0});
    const auto fileIndex = static_cast<std::uint32_t>(root.files().size());
    root.files().push_back({sourcePath, portablePath, 0, 0});
    deck.model()->markers().push_back({firstKey, model::SourceMarkerKind::IncludeBegin, portablePath, fileIndex});
    deck.model()->markers().push_back({deck.model()->nextKeyValue(), model::SourceMarkerKind::IncludeEnd, {}, 0});
    return result;
}

std::vector<base::EntityId> cardIds(const Deck& deck, const std::string& source)
{
    const model::Model& model = *deck.model();
    const CardRegistry& registry = CardRegistry::instance();
    std::vector<std::uint16_t> types;
    if (const std::optional<base::IdSpace> space = spaceOfCategory(source))
    {
        for (std::uint16_t type = 1; type < registry.size(); ++type)
            if (registry.entry(type).info.idSpace == *space)
                types.push_back(type);
    }
    else
    {
        types.push_back(cardTypeOf(source));
    }
    std::vector<base::EntityId> ids;
    for (const std::uint16_t type : types)
    {
        const CardTable* table = model.findTable(type);
        if (table == nullptr)
            continue;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            if (table->rows().isAlive(row))
                ids.push_back(table->idAt(row));
    }
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
    return ids;
}

std::vector<std::string> cardFields(const Deck& deck, const std::string& source, base::EntityId id)
{
    const model::Model& model = *deck.model();
    return fieldsOfText(cardText(model, locateOrThrow(model, source, id)));
}

void setCardField(const Deck& deck, const std::string& source, base::EntityId id, int field, const std::string& value)
{
    if (field < 1)
        throw base::UsageError("fields are numbered from 1, not " + std::to_string(field));
    std::vector<std::string> fields = cardFields(deck, source, id);
    if (static_cast<std::size_t>(field) > fields.size())
        fields.resize(static_cast<std::size_t>(field));
    fields[static_cast<std::size_t>(field) - 1] = value;
    setCardFields(deck, source, id, fields);
}

namespace
{
std::vector<std::pair<int, std::string>> fieldNamesAt(const model::Model& model, CardLocation location)
{
    const CardTable& table = *model.findTable(location.table);
    const model::CardLayout* layout = model::CardLayouts::standard()->find(table.info().name);
    if (layout == nullptr)
        return {};
    std::vector<std::string> values = fieldsOfText(cardText(model, location));
    if (const model::CardLayout* form = model::recordLayout(*layout, values))
        values.resize(std::max(values.size(), form->fields.size()));
    const std::vector<std::string> fieldNames = model::recordFieldNames(*layout, values);
    std::vector<std::pair<int, std::string>> names;
    for (std::size_t index = 0; index < fieldNames.size(); ++index)
    {
        const std::string& name = fieldNames[index];
        if (!name.empty() &&
            std::none_of(names.begin(), names.end(), [&](const auto& known) { return known.second == name; }))
            names.emplace_back(static_cast<int>(index) + 1, name);
    }
    return names;
}
} // namespace

std::vector<std::pair<int, std::string>> cardFieldNames(const Deck& deck, const std::string& source, base::EntityId id)
{
    return fieldNamesAt(*deck.model(), locateOrThrow(*deck.model(), source, id));
}

void setCardValueByKey(const Deck& deck, const std::string& type, std::uint32_t key, const std::string& name,
                       const std::string& value)
{
    const auto location = deck.model()->locateKey(cardTypeOf(type), base::CardKey{key});
    if (!location)
        throw base::UsageError("the card key no longer exists");
    const auto names = fieldNamesAt(*deck.model(), *location);
    for (const auto& [field, fieldName] : names)
        if (fieldName == name)
        {
            auto fields = fieldsOfText(cardText(*deck.model(), *location));
            fields.resize(std::max(fields.size(), static_cast<std::size_t>(field)));
            fields[static_cast<std::size_t>(field) - 1] = value;
            (void)readCards(*deck.model(), recordText(type, fields), location);
            return;
        }
    std::string known;
    for (const auto& [field, fieldName] : names)
        known += (known.empty() ? "" : ", ") + fieldName;
    throw base::UsageError("the card has no field named '" + name + "'; its named fields are " + known);
}

void setCardValuesByKey(const Deck& deck, const std::string& type, std::uint32_t key,
                        std::span<const std::pair<std::string, model::FieldValue>> values)
{
    const auto location = deck.model()->locateKey(cardTypeOf(type), base::CardKey{key});
    if (!location)
        throw base::UsageError("the card key no longer exists");
    const auto names = fieldNamesAt(*deck.model(), *location);
    const auto current = fieldsOfText(cardText(*deck.model(), *location));
    CardInput input;
    input.type = type;
    for (const auto& field : current)
        input.fields.push_back({model::FieldValue::Kind::Text, 0, 0, field});
    for (const auto& [name, value] : values)
    {
        const auto found =
            std::find_if(names.begin(), names.end(), [&](const auto& entry) { return entry.second == name; });
        if (found == names.end())
            throw base::UsageError("the card has no field named '" + name + "'");
        const auto index = static_cast<std::size_t>(found->first - 1);
        if (input.fields.size() <= index)
            input.fields.resize(index + 1);
        input.fields[index] = value;
    }
    updateCardByKey(deck, type, key, input);
}

void setCardValue(const Deck& deck, const std::string& source, base::EntityId id, const std::string& name,
                  const std::string& value)
{
    const std::vector<std::pair<int, std::string>> names = cardFieldNames(deck, source, id);
    for (const auto& [field, fieldName] : names)
        if (fieldName == name)
        {
            setCardField(deck, source, id, field, value);
            return;
        }
    std::string known;
    for (const auto& [field, fieldName] : names)
        known += (known.empty() ? "" : ", ") + fieldName;
    throw base::UsageError("the card has no field named '" + name + "'; its named fields are " + known +
                           " (or set a field by its number with setCardField)");
}

void setCardFields(const Deck& deck, const std::string& source, base::EntityId id,
                   const std::vector<std::string>& fields)
{
    model::Model& model = *deck.model();
    const CardLocation location = locateOrThrow(model, source, id);
    const std::string name(model.findTable(location.table)->info().name);
    (void)readCards(model, recordText(name, fields), location);
}

void replaceCard(const Deck& deck, const std::string& source, base::EntityId id, const std::string& text)
{
    model::Model& model = *deck.model();
    (void)readCards(model, text, locateOrThrow(model, source, id));
}

std::vector<AddedCard> addCards(const Deck& deck, const std::string& text)
{
    return readCards(*deck.model(), text, std::nullopt);
}

bool removeCard(const Deck& deck, const std::string& source, base::EntityId id)
{
    model::Model& model = *deck.model();
    const std::optional<CardLocation> location = locate(model, source, id);
    if (!location)
        return false;
    model.remove(*location);
    return true;
}

} // namespace oor::api
