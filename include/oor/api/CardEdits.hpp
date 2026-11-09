#pragma once

#include "oor/api/Deck.hpp"
#include "oor/base/ids/EntityId.hpp"

#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

// The cards of a deck one by one: found by ID, read as named values, edited, added and removed. Works the same way for
// every card type, through what every card type has: its stored columns (the names of its fields, registered in
// src/query/providers/columns/) and its format (how it is read from and written to fields).
//
// Where a card is looked for (`source`): a category of cards that share an ID space ("nodes", "elements", "properties",
// "materials", "coordinates"), or a card type ("ShellQuad4", "ShellProp", "SinglePointConstrList"). A card type whose
// cards share an ID (the set cards: several SPC1 of SID 7) gives the first card of that ID, in the order of the deck.
namespace oor::api
{

// The value of one named field of a card.
struct FieldValue
{
    enum class Kind
    {
        Missing, // the card leaves it blank and it has no default (a null)
        Number,
        Numbers, // a list or a vector (the grid points of an element, the position of a grid point)
        Text,
        Texts
    };
    Kind kind = Kind::Missing;
    bool integer = false; // the numbers are whole
    double number = std::numeric_limits<double>::quiet_NaN();
    std::vector<double> numbers;
    std::string text;
    std::vector<std::string> texts;
};

// A card, read.
struct CardView
{
    std::string type; // "Node"
    base::EntityId id = 0;
    // The named values: every stored field of the card type, then the computed columns asked for ("xyzGlobal").
    std::vector<std::pair<std::string, FieldValue>> values;
    // The fields of the card as it is written, field 1 first (the card name is not a field). A blank field is "".
    std::vector<std::string> fields;
    std::string text;    // the card as one model record
    std::string comment; // the comment lines above the card in the deck, joined with '\n'
};

// The card of `source` that has `id`; nothing when there is none. `computed` adds computed columns of the card type
// ("xyzGlobal" for a grid point, "area" or "mass" for an element, ...). Throws QueryError for an unknown source or
// column.
[[nodiscard]] std::optional<CardView> findCard(const Deck& deck, const std::string& source, base::EntityId id,
                                               const std::vector<std::string>& computed = {});

// The IDs of the live cards of `source`, increasing, each once.
[[nodiscard]] std::vector<base::EntityId> cardIds(const Deck& deck, const std::string& source);

// The fields of a card as it is written, field 1 first (as CardView::fields). Throws UsageError when there is no such
// card.
[[nodiscard]] std::vector<std::string> cardFields(const Deck& deck, const std::string& source, base::EntityId id);

// Editing. An edited card keeps its place in the deck, its comment and its link to attached results (its card key);
// its new fields are read and checked as the model reader checks them. Each function changes nothing when it throws:
// UsageError for fields the card cannot hold (with the reasons), DuplicateIdError for an ID another card has.

// Sets field `field` (from 1, the numbering of the card's fields: field 1 is the first after the name) to the text
// `value` ("" for blank), and reads the card again.
void setCardField(const Deck& deck, const std::string& source, base::EntityId id, int field, const std::string& value);
// The compact fields of the model record, in order, each name once: {3, "thickness"} of a ShellProp,
// {3, "position1"} of a Node. Section markers and list counts have no field name.
[[nodiscard]] std::vector<std::pair<int, std::string>> cardFieldNames(const Deck& deck, const std::string& source,
                                                                      base::EntityId id);
// Sets the field named `name` (one of cardFieldNames, exactly) to the text `value`. Throws
// UsageError, listing the named fields, when the card has no field of that name.
void setCardValue(const Deck& deck, const std::string& source, base::EntityId id, const std::string& name,
                  const std::string& value);
// Gives the card all its fields at once (as CardView::fields).
void setCardFields(const Deck& deck, const std::string& source, base::EntityId id,
                   const std::vector<std::string>& fields);
// Replaces the card by the model record written in `text`, of the same type.
void replaceCard(const Deck& deck, const std::string& source, base::EntityId id, const std::string& text);

// A card added: its type and ID.
struct AddedCard
{
    std::string type;
    base::EntityId id = 0;
};
// Adds the model records written in `text` (one card per line), at the end of the model,
// all or none. Throws UsageError for a card the library does not know or cannot read, DuplicateIdError for a taken ID.
std::vector<AddedCard> addCards(const Deck& deck, const std::string& text);

// Removes the card; false when there is none.
bool removeCard(const Deck& deck, const std::string& source, base::EntityId id);

} // namespace oor::api
