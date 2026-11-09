#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace oor::analysis
{

// One reference field of a card, resolved: which kind of card it names, the ID it holds, and the card that ID finds
// (the first card of a set for a set ID; nothing when it points at nothing).
struct ReferenceEdge
{
    model::ReferenceTarget target = model::ReferenceTarget::Other;
    base::EntityId id = 0;
    model::CardLocation to;
    bool resolved = false; // `to` is a card
};

// A reference field that points at nothing and has to point at something (the field of a card, or of a Case Control
// command).
struct DanglingReference
{
    model::CardLocation from; // unused for a Case Control command
    std::string field;
    model::ReferenceTarget target = model::ReferenceTarget::Other;
    base::EntityId id = 0;
    bool inCaseControl = false;
    std::string scope;            // the kind of a Case Control section: "subcase", "global", ... (subcaseKindName)
    std::int64_t scopeNumber = 0; // and the number after it
};

// Every reference of the cards of a model, looked at once (ReferenceChecker::scan) and kept by card: what a card refers
// to (the dependencies of a component), what is referred to by nothing (unused cards), what points at nothing
// (dangling references). It describes the model as it was when it was built: rows are those of that moment.
class ReferenceIndex
{
public:
    // Scans a copy of the model (visiting a card writes its IDs back where they were, which must not happen while other
    // threads read the model).
    [[nodiscard]] static ReferenceIndex build(const model::Model& model);

    // The references of the card at `location`, in the order of its fields.
    [[nodiscard]] std::span<const ReferenceEdge> from(model::CardLocation location) const noexcept;
    // True when some card refers to this one (it was the target of a resolved reference).
    [[nodiscard]] bool isReferenced(model::CardLocation location) const noexcept;
    // The references that point at nothing: the cards in table then row order, then the Case Control.
    [[nodiscard]] const std::vector<DanglingReference>& dangling() const noexcept { return m_dangling; }
    [[nodiscard]] std::size_t edgeCount() const noexcept { return m_edges.size(); }

private:
    std::vector<ReferenceEdge> m_edges;
    std::vector<std::vector<std::uint32_t>> m_start; // by card type: where the edges of each row start (rows + 1)
    std::vector<std::vector<std::uint8_t>> m_referenced;
    std::vector<DanglingReference> m_dangling;
};

} // namespace oor::analysis
