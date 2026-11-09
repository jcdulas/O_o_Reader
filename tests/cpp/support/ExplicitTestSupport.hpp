#pragma once

#include "oor/base/diagnostics/DiagnosticSink.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/model/references/ReferenceChecker.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"
#include "oor/model/references/Renumbering.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <string_view>
#include <vector>

// What the tests of the explicit solver cards share: reading card records, the card of a row, the references a card
// lists, the refused values and the round trip through a model file.
namespace oor::testing
{

// Reads card records (the lines of [model]) strictly.
inline model::Model readBulk(const std::string& cards)
{
    return readModelText(cards, true);
}

// The values of the card at `row` of its table.
template <typename TableT>
typename TableT::CardType cardAt(const model::Model& model, std::uint32_t row = 0)
{
    return model.findTable<TableT>()->card(row);
}

template <typename TableT>
std::size_t rowCount(const model::Model& model)
{
    const TableT* table = model.findTable<TableT>();
    return table == nullptr ? 0 : table->rows().rowCount();
}

// The number of references of the model that name nothing.
inline std::size_t unresolvedIn(model::Model& model)
{
    base::DiagnosticSink sink(base::ReadMode::Tolerant);
    return model::ReferenceChecker::check(model, sink);
}

// The card record is an error in strict mode and in tolerant mode, where the first error names the field (when
// given).
inline void expectInvalid(const std::string& card, const std::string& field = "")
{
    INFO(card);
    REQUIRE_THROWS_AS(readModelText(card, true), base::DeckError);
    const model::Model tolerant = readModelText(card, false);
    REQUIRE(tolerant.diagnostics().hasErrors());
    if (!field.empty())
    {
        const auto error = tolerant.diagnostics().firstError();
        REQUIRE(error.has_value());
        INFO(error->message);
        REQUIRE(error->location.fieldName == field);
    }
}

// Records the references a card lists: the field names and what each points to.
struct RecordingVisitor final : model::ReferenceVisitor
{
    std::vector<std::string> fields;
    std::vector<model::ReferenceTarget> targets;
    base::EntityId visit(std::string_view field, model::ReferenceTarget target, base::EntityId id) override
    {
        fields.emplace_back(field);
        targets.push_back(target);
        return id;
    }
};

// Writes the model as a model file and reads that again: the same cards.
inline void requireRoundTrip(const model::Model& model)
{
    requireModelFileRoundTrip(model);
}

} // namespace oor::testing
