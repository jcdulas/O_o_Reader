#pragma once

#include "oor/analysis/adjacency/ElementTopology.hpp"
#include "oor/base/threading/ThreadPool.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>

namespace oor::analysis
{

// A set of keys, each with the elements that have it, in compressed sparse row form: the keys sorted, one offset per
// key, and the elements of all keys one after the other (each key's elements in the order of the cards: card type, then
// row). What the three indexes below share.
template <typename Key>
class GroupedIndex
{
public:
    // The number of distinct keys.
    [[nodiscard]] std::size_t size() const noexcept { return m_keys.size(); }
    // The number of (key, element) entries: an element is counted once for each of its keys.
    [[nodiscard]] std::size_t entryCount() const noexcept { return m_elements.size(); }
    [[nodiscard]] std::span<const Key> keys() const noexcept { return m_keys; }
    [[nodiscard]] const Key& key(std::size_t position) const noexcept { return m_keys[position]; }
    // The elements of the key at `position` in keys().
    [[nodiscard]] std::span<const ElementRef> elements(std::size_t position) const noexcept
    {
        return std::span<const ElementRef>(m_elements)
            .subspan(m_offsets[position], m_offsets[position + 1] - m_offsets[position]);
    }
    // The position of a key, nullopt when no element has it.
    [[nodiscard]] std::optional<std::size_t> find(const Key& key) const noexcept
    {
        const auto found = std::lower_bound(m_keys.begin(), m_keys.end(), key);
        if (found == m_keys.end() || !(*found == key))
            return std::nullopt;
        return static_cast<std::size_t>(found - m_keys.begin());
    }
    // The elements that have the key (an empty span when none does).
    [[nodiscard]] std::span<const ElementRef> elementsOf(const Key& key) const noexcept
    {
        const auto position = find(key);
        return position ? elements(*position) : std::span<const ElementRef>();
    }

    // Builds the index from entries sorted by key and then by element; an entry that appears twice counts once (an
    // element that names a node twice). `Entry` has a `key` and a `ref`, the element packed as (type << 32) | row.
    template <typename Entry>
    [[nodiscard]] static GroupedIndex fromSorted(const std::vector<Entry>& entries)
    {
        GroupedIndex index;
        index.m_elements.reserve(entries.size());
        index.m_offsets.push_back(0);
        for (std::size_t position = 0; position < entries.size(); ++position)
        {
            const Entry& entry = entries[position];
            const bool sameKey = position > 0 && entries[position - 1].key == entry.key;
            if (sameKey && entries[position - 1].ref == entry.ref)
                continue;
            if (!sameKey)
            {
                if (!index.m_keys.empty())
                    index.closeKey();
                index.m_keys.push_back(entry.key);
            }
            index.m_elements.push_back(ElementRef{static_cast<std::uint32_t>(entry.ref & 0xFFFFFFFFU),
                                                  static_cast<std::uint16_t>(entry.ref >> 32U)});
        }
        if (!index.m_keys.empty())
            index.closeKey();
        index.m_keys.shrink_to_fit();
        index.m_offsets.shrink_to_fit();
        index.m_elements.shrink_to_fit();
        return index;
    }

private:
    void closeKey()
    {
        if (m_elements.size() > std::numeric_limits<std::uint32_t>::max())
            throw std::length_error("too many connectivity entries for one index");
        m_offsets.push_back(static_cast<std::uint32_t>(m_elements.size()));
    }

    std::vector<Key> m_keys;
    std::vector<std::uint32_t> m_offsets;
    std::vector<ElementRef> m_elements;
};

// For every node ID that an element uses, the elements that use it (corners and mid-side nodes, scalar points too).
class NodeToElementIndex
{
public:
    // The elements that use a node (empty when none does, or the node does not exist).
    [[nodiscard]] std::span<const ElementRef> elementsOf(base::EntityId node) const noexcept;
    // The number of nodes that some element uses.
    [[nodiscard]] std::size_t nodeCount() const noexcept { return m_groups.size(); }
    [[nodiscard]] std::size_t entryCount() const noexcept { return m_groups.entryCount(); }
    [[nodiscard]] std::span<const base::EntityId> nodes() const noexcept { return m_groups.keys(); }
    [[nodiscard]] std::span<const ElementRef> elements(std::size_t position) const noexcept
    {
        return m_groups.elements(position);
    }

    // Built by the adjacency cache: every element of the model, over the pool.
    [[nodiscard]] static NodeToElementIndex build(const model::Model& model, base::ThreadPool* pool);

private:
    GroupedIndex<base::EntityId> m_groups;
    std::vector<std::int32_t> m_direct; // the position of node `id` when the IDs are dense, else empty
};

// For every edge, the elements that have it: the sides of the shells, the edges of the solids, the lines. An edge is
// its two end nodes by ID, smaller first.
class EdgeTable
{
public:
    [[nodiscard]] std::span<const ElementRef> elementsOf(const EdgeKey& edge) const noexcept
    {
        return m_groups.elementsOf(edge);
    }
    [[nodiscard]] std::size_t edgeCount() const noexcept { return m_groups.size(); }
    [[nodiscard]] std::size_t entryCount() const noexcept { return m_groups.entryCount(); }
    [[nodiscard]] std::span<const EdgeKey> edges() const noexcept { return m_groups.keys(); }
    [[nodiscard]] std::span<const ElementRef> elements(std::size_t position) const noexcept
    {
        return m_groups.elements(position);
    }

    [[nodiscard]] static EdgeTable build(const model::Model& model, base::ThreadPool* pool);

private:
    GroupedIndex<EdgeKey> m_groups;
};

// For every face, the elements that have it: a shell is its own face, a solid has its faces. A face is its corner nodes
// by ID in increasing order (a triangle has 0 last).
class FaceTable
{
public:
    [[nodiscard]] std::span<const ElementRef> elementsOf(const FaceKey& face) const noexcept
    {
        return m_groups.elementsOf(face);
    }
    [[nodiscard]] std::size_t faceCount() const noexcept { return m_groups.size(); }
    [[nodiscard]] std::size_t entryCount() const noexcept { return m_groups.entryCount(); }
    [[nodiscard]] std::span<const FaceKey> faces() const noexcept { return m_groups.keys(); }
    [[nodiscard]] std::span<const ElementRef> elements(std::size_t position) const noexcept
    {
        return m_groups.elements(position);
    }

    [[nodiscard]] static FaceTable build(const model::Model& model, base::ThreadPool* pool);

private:
    GroupedIndex<FaceKey> m_groups;
};

} // namespace oor::analysis
