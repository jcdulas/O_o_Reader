#pragma once

#include "support/ExplicitTestSupport.hpp"

#include "oor/model/references/ReferenceVisitor.hpp"

#include <string>
#include <string_view>
#include <vector>

// What the tests of the superelement cards share on top of ExplicitTestSupport: a visitor that also records the
// references that go to the model of another superelement.
namespace oor::testing
{

struct ScopedReference
{
    std::string field;
    model::ReferenceTarget target;
    base::EntityId id;
    base::EntityId superelement; // 0 for a reference of the card's own model
    bool scoped;
};

// Records every reference of a card in order, with the superelement it goes to when it goes to another model.
struct ScopedRecorder final : model::ReferenceVisitor
{
    std::vector<ScopedReference> seen;

    base::EntityId visit(std::string_view field, model::ReferenceTarget target, base::EntityId id) override
    {
        seen.push_back(ScopedReference{std::string(field), target, id, 0, false});
        return id;
    }

    base::EntityId visitInSuperelement(std::string_view field, model::ReferenceTarget target, base::EntityId id,
                                        base::EntityId superelement) override
    {
        seen.push_back(ScopedReference{std::string(field), target, id, superelement, true});
        return id;
    }

    // The references of the fields called `field`, as (id, superelement) with -1 for a reference of the own model.
    [[nodiscard]] std::vector<std::pair<base::EntityId, base::EntityId>> of(std::string_view field) const
    {
        std::vector<std::pair<base::EntityId, base::EntityId>> found;
        for (const ScopedReference& reference : seen)
        {
            if (reference.field == field)
                found.emplace_back(reference.id, reference.scoped ? reference.superelement : -1);
        }
        return found;
    }
};

// The references of the card at `row` of a table, recorded.
template <typename TableT>
ScopedRecorder referencesOf(model::Model& model, std::uint32_t row = 0)
{
    ScopedRecorder recorder;
    model.table<TableT>().visitReferences(row, recorder);
    return recorder;
}

} // namespace oor::testing
