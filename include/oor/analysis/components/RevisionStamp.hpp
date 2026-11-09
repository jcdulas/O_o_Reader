#pragma once

#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace oor::analysis
{

// The revision stamps of the components of a model: for each, a 64-bit hash of the content of
// its cards and of the cards they depend on.
//
// The content of a card is its complete neutral slot text, with real values written using
// the digits that give the same double back: two cards have the same hash exactly when they have the same values. The
// card key, the position in the file and the comments are not part of it, so the same content gives the same stamp in
// another session, after the model was written and read again, or after cards were added in another order. The hashes
// of the cards of a component are added (modulo 2^64): the stamp does not depend on the order of the cards, nor on the
// number of threads.
//
// The cards of a component are its elements, and what they depend on, found by following every reference field: the
// properties, the materials, the coordinate systems, the nodes (and what these refer to in turn: the grid points that
// define a coordinate system, the tables of a material), plus the companions of those cards: the cards that complete a
// property or a material with the same ID (MATT1, MATS1, PBUSHT, ...) and the non-structural mass cards that name a
// property or an element of the component. A component that contact merges ("merge" mode) also holds the contact cards
// of the pairs that join it (ContactPair::cards). Loads, constraints and Case Control are not part of it: the load
// stamp covers loads (LoadCaseGroups).
class RevisionStamp
{
public:
    // The stamps by component number (the stamp of component `id` is at position id - 1), cached with the model until
    // its content changes: the components of the default contact options, or of `contact`.
    [[nodiscard]] static std::shared_ptr<const std::vector<std::uint64_t>> of(const model::Model& model,
                                                                              base::ThreadPool* pool = nullptr);
    [[nodiscard]] static std::shared_ptr<const std::vector<std::uint64_t>> of(const model::Model& model,
                                                                              const ContactOptions& contact,
                                                                              base::ThreadPool* pool = nullptr);
    // Computes without the cache.
    [[nodiscard]] static std::vector<std::uint64_t> compute(const model::Model& model, const ComponentModel& components,
                                                            base::ThreadPool* pool = nullptr);
    // The stamps of sets of cards made the same way: each set is closed over its references and companions, and its
    // cards hashed and added. The stamp of a group, a spider or a cluster of connections is the
    // stamp of its member cards (elements, MPC cards).
    [[nodiscard]] static std::vector<std::uint64_t> computeFor(
        const model::Model& model, const std::vector<std::vector<model::CardLocation>>& members,
        base::ThreadPool* pool = nullptr);

    // The hash of one card: its exact neutral name and complete field values.
    [[nodiscard]] static std::uint64_t cardHash(const model::Model& model, model::CardLocation card);

    // The hash that goes into a sum for a card hash: spreads the bits, so that adding hashes does not cancel.
    [[nodiscard]] static std::uint64_t mix(std::uint64_t value) noexcept;
    // Hashes bytes (FNV-1a, 64 bits).
    [[nodiscard]] static std::uint64_t hashBytes(std::uint64_t seed, const void* data, std::size_t size) noexcept;
    [[nodiscard]] static std::uint64_t hashReal(std::uint64_t seed, double value) noexcept;
};

} // namespace oor::analysis
