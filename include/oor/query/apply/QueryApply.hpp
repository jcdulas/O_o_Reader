#pragma once

#include "oor/query/apply/CellValue.hpp"
#include "oor/query/apply/EditableTable.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace oor::model
{
class Model;
}

namespace oor::query
{

// One field of one card to write.
struct FieldWrite
{
    std::string field; // the field of the card: the name of the setter
    CellValue value;
    std::string column; // the column of the table the value came from, for messages
};

// The writes of one card, found by its card key.
struct CardWrite
{
    std::uint16_t type = 0;
    std::uint32_t key = 0;
    std::uint32_t row = 0;   // the row of the card in its table of the model now
    std::int64_t id = 0;     // its ID now
    std::uint32_t first = 0; // the first row of the edited table that stands for the card (the order of the writes)
    std::vector<FieldWrite> fields;
};

// What `apply` will write: every changed cell turned into a field of a card, the cards found in the model by card key,
// positions in the basic system converted to the CP system of their nodes.
struct ApplyPlan
{
    std::vector<CardWrite> cards;
    std::size_t cells = 0;       // the changed cells of the table
    std::size_t fieldWrites = 0; // the fields to write: fewer than the cells when several rows are one card
    std::size_t positions = 0;   // grid points whose basic position is converted to their CP system
};

// `QueryApply` builds the plan of an `apply`. The model is read, not changed: the writes themselves go through the
// setters of the card classes (the layer that has them runs the plan). The only things decided here are the ones that
// need no setter:
//   * every card of the table must still exist (looked up by card key, so a renumbered model is found); the cards that
//     do not are all listed in an ApplyError, which says it is stale, and nothing is planned;
//   * a card that several rows stand for (a row repeated by a join) must be given one value for a field, not two;
//   * edits of the basic position of a grid point are turned into an edit of its `xyz` in its CP system: the axes that
//     were not changed keep the position the model has now (not the one the table was made with), so an edit made on
//     the model meanwhile is kept; a position and the coordinates `xyz` or the `positionFrameId` of the same node
//     cannot both be edited.
// Throws ApplyError listing every problem found.
class QueryApply
{
public:
    [[nodiscard]] static ApplyPlan plan(const EditableTable& table, const model::Model& model);
};

} // namespace oor::query
