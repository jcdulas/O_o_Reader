#include "oor/api/CardEdits.hpp"

#include "oor/base/errors/Errors.hpp"
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
#include <memory>
#include <set>
#include <span>
#include <sstream>
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

// The card keys of a read: the given key first (the card that replaces another keeps its key), then new ones.
class ReplacingKeys final : public model::KeySource
{
public:
    ReplacingKeys(model::Model& model, std::optional<base::CardKey> first) : m_model(model), m_first(first) {}
    base::CardKey next() override
    {
        if (m_first)
            return *std::exchange(m_first, std::nullopt);
        return m_model.newKey();
    }

private:
    model::Model& m_model;
    std::optional<base::CardKey> m_first;
};

// The card already in the model that has the ID of a new row in the same ID space, when there is one.
std::optional<CardLocation> takenBy(const model::Model& model, std::uint16_t type, base::EntityId id)
{
    const base::IdSpace space = CardRegistry::instance().entry(type).info.idSpace;
    if (space == base::IdSpace::Group || space == base::IdSpace::None)
        return std::nullopt;
    return space == base::IdSpace::PerCardType ? model.locateInType(type, id) : model.locate(space, id);
}

// Reads the cards of `text` into the model, all or none: the rows are checked (readable, IDs free) before any is
// registered. `replacing`: the card the (single) card of the text replaces, which gives it its key.
std::vector<AddedCard> readCards(model::Model& model, const std::string& text, std::optional<CardLocation> replacing)
{
    const CardRegistry& registry = CardRegistry::instance();
    const std::vector<deck::ModelItem> records = recordItems(text);
    if (records.empty())
        throw base::UsageError("the text holds no card");
    if (replacing && records.size() != 1)
        throw base::UsageError("a card is replaced by one card, not " + std::to_string(records.size()));
    std::optional<base::CardKey> key;
    if (replacing)
        key = model.findTable(replacing->table)->rows().keyAt(replacing->row);

    base::DiagnosticSink sink;
    std::vector<CardLocation> added;
    std::vector<std::pair<base::CardKey, std::string>> comments;
    std::vector<std::pair<CardLocation, std::vector<std::uint8_t>>> sourceDigits;
    const auto undo = [&]
    {
        for (const CardLocation& location : added)
            model.findTable(location.table)->rows().kill(location.row);
    };
    std::set<std::pair<int, base::EntityId>> idsOfBatch; // (ID space or type, ID): two new cards with one ID
    for (const deck::ModelItem& record : records)
    {
        const std::string& cardName = record.values.front();
        const std::optional<std::uint16_t> type = registry.typeIndexOf(cardName);
        if (!type || *type == CardRegistry::UNKNOWN_TYPE)
        {
            undo();
            throw base::UsageError("'" + cardName + "' is not a card the library knows");
        }
        if (replacing && *type != replacing->table)
        {
            undo();
            const std::string replaced(registry.entry(replacing->table).info.name);
            throw base::UsageError("a " + replaced + " is replaced by a card of its type, not a " + cardName +
                                   ": remove it and add the other card");
        }
        const model::CardLayout* layout = model::CardLayouts::standard()->find(cardName);
        std::vector<std::string> slots;
        std::string problem;
        if (layout == nullptr || !layout->toSlots(std::span(record.values).subspan(1), slots, problem))
        {
            undo();
            throw base::UsageError("the record " + cardName + " cannot be read: " + problem);
        }
        deck::RawCardBlock block;
        deck::RawCard card;
        card.name = *deck::CardName::fromText(cardName);
        card.line = record.line;
        card.fieldCount = static_cast<std::uint32_t>(slots.size());
        for (std::string& slot : slots)
            block.fields.push_back(block.own(std::move(slot)));
        CardTable& table = model.ensureTable(*type);
        model::FieldReader reader(block, card, sink, EDIT_SOURCE);
        std::vector<std::uint8_t> digits;
        reader.recordDigits(digits);
        ReplacingKeys keys(model, key);
        std::vector<std::uint32_t> rows;
        if (!table.readCards(reader, keys, rows) || reader.failed())
        {
            for (const std::uint32_t row : rows)
                table.rows().kill(row);
            undo();
            throw base::UsageError("the card " + cardName + " cannot be read:" + errorsOf(sink));
        }
        if (replacing && rows.size() != 1)
        {
            for (const std::uint32_t row : rows)
                table.rows().kill(row);
            throw base::UsageError("the card would give " + std::to_string(rows.size()) +
                                   " objects where it replaces one: remove it and add the cards");
        }
        for (const std::uint32_t row : rows)
        {
            added.push_back(CardLocation{*type, row});
            sourceDigits.emplace_back(CardLocation{*type, row}, digits);
            if (!replacing && !record.comments.empty())
                comments.emplace_back(table.rows().keyAt(row), record.comments);
            const base::EntityId id = table.idAt(row);
            const base::IdSpace space = table.info().idSpace;
            const std::optional<CardLocation> other = takenBy(model, *type, id);
            const bool takenInModel = other && !(replacing && *other == *replacing);
            const int spaceKey = space == base::IdSpace::PerCardType ? 1000 + *type : static_cast<int>(space);
            const bool indexed = space != base::IdSpace::Group && space != base::IdSpace::None;
            if (indexed && (takenInModel || !idsOfBatch.insert({spaceKey, id}).second))
            {
                undo();
                throw base::DuplicateIdError("ID " + std::to_string(id) + " is already used in the " +
                                             std::string(base::idSpaceName(space)) + " ID space");
            }
        }
    }
    // every card is readable and every ID free: the cards take their places
    for (const auto& [location, digits] : sourceDigits)
    {
        auto& rows = model.findTable(location.table)->rows();
        rows.forgetSource(location.row);
        rows.setSource(location.row, digits);
    }
    if (replacing)
    {
        // the new card (the last row of the table) takes the row, the key and the place of the card it replaces
        CardTable& table = *model.findTable(replacing->table);
        const base::EntityId id = table.idAt(added.front().row);
        if (id != table.idAt(replacing->row))
            model.setId(*replacing, id); // moves the card in the ID index (the ID was checked free above)
        table.moveLastRowTo(replacing->row);
        if (table.info().idSpace == base::IdSpace::Element)
            model.noteStructureChange(); // the connectivity may have changed
        else
            model.noteContentChange();
        return {AddedCard{std::string(table.info().name), id}};
    }
    std::vector<AddedCard> result;
    for (const CardLocation& location : added)
    {
        const CardTable& table = *model.findTable(location.table);
        model.registerRead(location, table.idAt(location.row), sink, EDIT_SOURCE, 0);
        result.push_back(AddedCard{std::string(table.info().name), table.idAt(location.row)});
    }
    for (const auto& [cardKey, comment] : comments)
        model.setComment(cardKey, comment);
    return result;
}

} // namespace

std::optional<CardView> findCard(const Deck& deck, const std::string& source, base::EntityId id,
                                 const std::vector<std::string>& computed)
{
    const std::shared_ptr<model::Model>& model = deck.model();
    const std::optional<CardLocation> location = locate(*model, source, id);
    if (!location)
        return std::nullopt;
    const CardTable& table = *model->findTable(location->table);

    CardView view;
    view.type = std::string(table.info().name);
    view.id = table.idAt(location->row);
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

std::vector<std::pair<int, std::string>> cardFieldNames(const Deck& deck, const std::string& source, base::EntityId id)
{
    const model::Model& model = *deck.model();
    const CardLocation location = locateOrThrow(model, source, id);
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
