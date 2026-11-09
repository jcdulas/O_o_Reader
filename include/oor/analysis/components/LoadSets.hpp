#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace oor::analysis
{

// What a load card does to the structure.
enum class LoadKind : std::uint8_t
{
    Force,              // FORCE, FORCE1, FORCE2: a concentrated force on a node
    Moment,             // MOMENT, MOMENT1, MOMENT2
    ScalarLoad,         // SLOAD: a load on a scalar point (or on T1 of a grid point)
    EnforcedMotion,     // SPCD: an enforced non-zero motion counts as a load
    Pressure,           // PLOAD (on the face of grid points), PLOAD2, PLOAD4: a pressure on elements or faces
    LineLoad,           // PLOAD1: a distributed load on a bar or a beam
    Gravity,            // GRAV: an acceleration of the whole model
    Acceleration,       // ACCEL: an acceleration of the whole model that varies along an axis
    PointAcceleration,  // ACCEL1: an acceleration of the grid points it lists
    Rotation,           // RFORCE: a rotation of the whole model
    NodeTemperature,    // TEMP: temperatures on nodes
    DefaultTemperature, // TEMPD: the temperature of every node that TEMP does not give
    ElementTemperature, // TEMPP1, TEMPRB: temperatures of elements
    Deformation,        // DEFORM: imposed deformations of line elements
    Other               // a card of the Load category that applies no load of its own (LSEQ, the cyclic loads)
};

// A run of element IDs (first == last for one element).
struct IdRun
{
    base::EntityId first = 0;
    base::EntityId last = 0;
};

// One load card of a load set once the LOAD combinations are expanded: what it is, how much of it counts (the product
// of the factors of the LOAD cards it was reached through), and where it acts.
struct ElementaryLoad
{
    model::CardLocation card;
    base::EntityId sourceSid = 0; // the set the card belongs to
    double scale = 1.0;
    LoadKind kind = LoadKind::Other;
    // the nodes it acts on (a pressure given by grid points, temperatures: theirs; the grid points of an ACCEL1 that
    // the model has)
    std::vector<base::EntityId> nodes;
    std::vector<IdRun> elements; // the elements it acts on (a pressure, a line load, an element temperature)
    // True for a load that acts on the whole model: gravity, a varying acceleration, a rotation, the default
    // temperature.
    [[nodiscard]] bool isGlobal() const noexcept
    {
        return kind == LoadKind::Gravity || kind == LoadKind::Acceleration || kind == LoadKind::Rotation ||
               kind == LoadKind::DefaultTemperature;
    }
    // True for a load that is applied on nodes one by one (what the load application nodes are made of).
    [[nodiscard]] bool isConcentrated() const noexcept
    {
        return kind == LoadKind::Force || kind == LoadKind::Moment || kind == LoadKind::ScalarLoad ||
               kind == LoadKind::EnforcedMotion;
    }
};

// A load set as the cards of the Load category define it. The set is the cards that have this ID; a LOAD card is
// replaced by the sets it combines, each with its factor times the overall factor S (the
// combination applies to the cards of the sets it names). The order is that of the cards of the set (table, then row),
// a LOAD card giving its terms in the order of the card. A set that does not exist gives nothing; a combination that
// comes back to itself stops there. The cards of each load set of a model (the Load category: the load cards and the
// LOAD combinations), found in one pass over the load tables, in the order of the types and rows: what the expansion of
// a set looks up, so that a query that expands thousands of sets reads the cards once (looking each set up in the
// tables costs every card of the model for every set). Valid while the model is not edited.
class LoadSetIndex
{
public:
    explicit LoadSetIndex(const model::Model& model);
    // The cards of the set, none when it has none.
    [[nodiscard]] const std::vector<model::CardLocation>& cardsOf(base::EntityId sid) const;
    // True when the model has a card of this type in a load set.
    [[nodiscard]] bool hasType(std::uint16_t typeIndex) const noexcept;
    // The LOAD entries of the set, in the order of their rows (none for a set of load cards): what a walk of the
    // combinations reads without going over the cards of a set (700 000 PLOAD2 of a skin panel).
    [[nodiscard]] const std::vector<model::CardLocation>& loadEntriesOf(base::EntityId sid) const;
    // True when the set holds cards other than LOAD entries.
    [[nodiscard]] bool hasOwnCards(base::EntityId sid) const;

private:
    std::unordered_map<base::EntityId, std::vector<model::CardLocation>> m_sets;
    std::unordered_map<base::EntityId, std::vector<model::CardLocation>> m_loadEntries;
    std::unordered_set<base::EntityId> m_withOwnCards;
    std::vector<std::uint16_t> m_types; // sorted
};

// A load set that a LOAD combination reaches: its cards (none of them a LOAD entry) come with the factors of the LOAD
// entries on the way.
struct ReachedSet
{
    base::EntityId sid = 0;
    // The product of the factors on the way without the overall factor S of the LOAD entry of the set expanded, as
    // load_combinations_table multiplies them: Si of its term; Si x S2 x Sj through a nested LOAD entry of overall
    // factor S2 and term Sj (multiplied in the order (Si x S2) x Sj); 1 for a set of its own.
    double factor = 1.0;
    // What the expansion multiplies the loads of the set by (ElementaryLoad::scale): ((1 x S) x Si) x ..., multiplied
    // in the order of the expansion, so equal to it to the bit; 1 for a set of its own.
    double scale = 1.0;
};

class LoadSets
{
public:
    [[nodiscard]] static std::vector<ElementaryLoad> expand(const model::Model& model, base::EntityId sid,
                                                            double scale = 1.0);
    // The same, the cards of the sets looked up in `index` (made once for the model) rather than in its tables.
    [[nodiscard]] static std::vector<ElementaryLoad> expand(const model::Model& model, const LoadSetIndex& index,
                                                            base::EntityId sid, double scale = 1.0);
    // The loads of the set's own cards, its LOAD entries left out, each with the scale 1 and the set as source: what
    // expand gives for a set that is not a combination. `types` (sorted), when not empty, keeps the cards of these
    // types only (the others are not read).
    [[nodiscard]] static std::vector<ElementaryLoad> ownLoads(const model::Model& model, const LoadSetIndex& index,
                                                              base::EntityId sid,
                                                              std::span<const std::uint16_t> types = {});
    // The sets of load cards a set brings, with their factors, as load_combinations_table has always found them: for a
    // LOAD combination, the sets the terms of its (first) LOAD entry name, in the order of the terms, a term that is
    // itself a LOAD entry followed down (nested LOAD entries are permitted); a term that is not a
    // LOAD entry is listed whatever its cards (one that names no card brings none); a set reached twice is listed
    // twice; a LOAD entry already on the way is not followed again (a cycle, which the format reference forbids and the
    // validation reports, load.combination-cycle). For a set of its own: itself (nothing for a set with no card). The
    // own loads of the sets reached times their `scale` are the expansion of the combination, to the bit, but where the
    // expansion differs from the walk: a cycle (it repeats the entries), more than eight nested entries (it stops), a
    // set that holds load cards besides its LOAD entry, or a second LOAD entry of one SID, none of which the format
    // reference foresees (the expansion loads them; the own cards of the set asked for are listed by the queries under
    // its SID).
    [[nodiscard]] static std::vector<ReachedSet> reachedSets(const model::Model& model, const LoadSetIndex& index,
                                                             base::EntityId sid);
    // The sets whose own cards expand gives, each with the scale of its cards, as expand walks them: a set that holds
    // cards other than LOAD entries, each time the walk meets it (a set reached twice is listed twice), the terms of
    // every LOAD entry followed, down to eight entries deep, without stopping a cycle (as expand). ownLoads of each,
    // their scale that `scale` (`factor` is `scale` too), are the loads expand gives, the same multiset to the bit (in
    // another order where a set holds cards besides a LOAD entry): what reads the loads of many combinations of the
    // same sets reads each set once.
    [[nodiscard]] static std::vector<ReachedSet> expansionSets(const model::Model& model, const LoadSetIndex& index,
                                                               base::EntityId sid);
    // The kind of load a card type is (Other for a card that is not a load).
    [[nodiscard]] static LoadKind kindOfType(std::uint16_t typeIndex);
};

} // namespace oor::analysis
