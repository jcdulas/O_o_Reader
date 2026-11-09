#pragma once

#include "oor/base/diagnostics/DiagnosticSink.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace oor::model
{

// The categories of cards a set-ID target stands for: a LOAD set ID (LoadCombinationSet) is the set ID of cards of the
// Load category, a constraint set ID (ConstraintSet) the set ID of cards of the Constraint category, a dynamic load set
// ID (DynamicLoadSet) one of the DynamicLoad category, and an excitation set (ExcitationSet, the EXCITEID of a dynamic
// load) one of the DynamicLoad or the Load category, a Bulk Data set (Set: the ID of a SET1 or SET3) one of the Set
// category, and so on for the frequency sets, the complex methods, the delays, the phase leads, the time steps, the
// nonlinear parameters, the random sets, the transfer functions and the initial conditions (the last one takes the
// temperature sets of the Load category too). Empty for the other targets.
[[nodiscard]] std::span<const CardCategory> categoriesOfSet(ReferenceTarget target) noexcept;

// The category of a set-ID target that stands for exactly one category; nothing for the other targets.
[[nodiscard]] std::optional<CardCategory> categoryOfSet(ReferenceTarget target) noexcept;

// The set IDs in use by the live cards of a category (cards whose ID space is IdSpace::Group).
[[nodiscard]] std::unordered_set<base::EntityId> setIdsOfCategory(const Model& model, CardCategory category);

// The live cards of a category that have this set ID, in table then row order.
[[nodiscard]] std::vector<CardLocation> cardsOfSet(const Model& model, CardCategory category, base::EntityId id);

// Where an ID of a reference field points to, when it points to something the model can find: a card of the right
// category, found by ID, and for a set ID (LoadCombinationSet, ConstraintSet, DynamicLoadSet, ExcitationSet, Set,
// FrequencySet,
// ...) the first card of the set. A HybridDampingRotorOptionsTable reference finds a TABDMP1, else a TABLEDi. Nothing
// for an ID that is not in use.
[[nodiscard]] std::optional<CardLocation> resolveReference(const Model& model, ReferenceTarget target,
                                                           base::EntityId id);

class AeroBoxRanges;

// resolveReference for many references of one model: the first card of each set ID of a category, the excitation sets
// and the aerodynamic boxes are indexed once, when a reference first needs them (resolveReference looks at every card
// of the category for each one). The model must not change while the resolver is in use.
class ReferenceResolver
{
public:
    explicit ReferenceResolver(const Model& model);
    ~ReferenceResolver();
    ReferenceResolver(const ReferenceResolver&) = delete;
    ReferenceResolver& operator=(const ReferenceResolver&) = delete;

    [[nodiscard]] std::optional<CardLocation> resolve(ReferenceTarget target, base::EntityId id);

private:
    using FirstCards = std::unordered_map<base::EntityId, CardLocation>;
    const FirstCards& setsOf(CardCategory category);

    const Model& m_model;
    std::unordered_map<CardCategory, FirstCards> m_sets;
    std::optional<FirstCards> m_excitations;
    std::unique_ptr<const AeroBoxRanges> m_boxes;
};

// Tells whether the target of a reference exists in `model`, by every rule the reference checker applies: the card of
// the ID space or the set, a superelement number the deck defines (a part, a card of the family, the SEID of a grid
// point), a mass increment a part carries, an exterior point that an EXTRN of the part lists, and, in a part that is
// the image of another one (SEBULK REPEAT or MIRROR), what the image holds. A blank field and the targets that may name
// nothing exist. Each call builds what it needs (use it for one answer, not in a loop over every card).
[[nodiscard]] bool referenceExists(const Model& model, ReferenceTarget target, base::EntityId id);

// True when a reference to `id` is allowed to point at nothing: a blank field (0) and the basic coordinate system.
[[nodiscard]] bool isExemptReference(ReferenceTarget target, base::EntityId id) noexcept;

// True for the targets that may name an ID no card defines: the end point of a THRU range (NodeInRange: the format
// skips the gaps) and a scalar point (ScalarOrGridPoint: a scalar element defines its scalar points by naming them, no
// SPOINT is needed). Such a reference is never reported; it is resolved when a card of the node ID space has that ID.
[[nodiscard]] bool mayPointAtNothing(ReferenceTarget target) noexcept;

// A reference field of one card that points at nothing.
struct UnresolvedReference
{
    std::string field;
    std::string message;
    [[nodiscard]] bool operator==(const UnresolvedReference&) const noexcept = default;
};

// The unresolved references of the card at `location` (editing compares the list before and after a change: only the
// references the change made are the editor's business).
[[nodiscard]] std::vector<UnresolvedReference> unresolvedAt(Model& model, CardLocation location);

// One reference field met by ReferenceChecker::scan: of a card, or of a Case Control command (`inCaseControl`, with the
// scope it is in: the kind of its section, subcaseKindName ("subcase"), and its number, 0 for the global section). The
// field of a Case Control command is the neutral name of the command.
struct ReferenceUse
{
    CardLocation location;  // the card (table index and row); unused in the Case Control
    std::string_view field; // the name of the field
    ReferenceTarget target = ReferenceTarget::Other;
    base::EntityId id = 0; // the ID the field holds (never 0: blank fields are not shown)
    bool resolved = true;  // false when the ID points at nothing that exists (and has to)
    // The ID is one of another superelement (ReferenceVisitor::visitInSuperelement), looked up in its model: the grid
    // points of a SECONCT, the design variables of an SEDLINK. `superelement` is its number (0: the residual
    // structure).
    bool scoped = false;
    base::EntityId superelement = 0;
    bool inCaseControl = false;
    std::string_view scope; // Case Control: "subcase", "global", ... (subcaseKindName)
    std::int64_t scopeNumber = 0;
};

// Looks at every reference field of every card and reports the ones that point at a card (or at a set of cards) that
// does not exist, each as an error with the card name and the field.
struct ReferenceChecker
{
    // Returns the number of unresolved references found.
    static std::size_t check(Model& model, base::DiagnosticSink& sink);

    // Calls `callback` for every non-blank reference field of every live card, in table then row order, then for the
    // commands of the Case Control, each with whether its target exists. Nothing is changed in the model (it takes the
    // model as const: the visit gives back every ID as found). A card table may still write the IDs back into its
    // columns (the same values, and a column buffer shared with a copy of the model is then cloned), so a scan must not
    // run while other threads read the model: scan a copy (`Model::copy()` is cheap and shares the buffers).
    static void scan(const Model& model, const std::function<void(const ReferenceUse&)>& callback);
};

} // namespace oor::model
