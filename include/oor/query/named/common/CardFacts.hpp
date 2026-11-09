#pragma once

#include "oor/model/core/Model.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::query
{

// What a query needs to know about single cards that no analysis function gives: which cards a card refers to, and the
// value of a stored field of a card. The references are read by visiting the card in a copy of the model (visiting
// writes the IDs back where they were, which must not touch the model other threads read); the copy is made once and
// costs one pointer per column.
class CardFacts
{
public:
    explicit CardFacts(const model::Model& model);

    // The IDs a card refers to with a given kind of reference, in the order of its fields; blank fields are left out.
    [[nodiscard]] std::vector<base::EntityId> references(model::CardLocation card, model::ReferenceTarget target);
    // The first such ID, or 0.
    [[nodiscard]] base::EntityId firstReference(model::CardLocation card, model::ReferenceTarget target);

    // The value of the stored field `name` of a card as a real (NaN when the card type has no such field, or the value
    // is not a number). Reads the column registered for the field (src/query/providers/columns/), which is how the query layer knows
    // the fields of a card.
    [[nodiscard]] double realField(model::Model& model, model::CardLocation card, std::string_view name) const;
    [[nodiscard]] bool hasField(std::uint16_t type, std::string_view name) const;

private:
    model::Model m_scratch;
};

} // namespace oor::query
