#pragma once

#include "oor/model/control/statement/ControlStatement.hpp"

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

// The statements of a File Management or Executive Control section in file order: what ExecutiveControl and
// FileManagement have in common. INCLUDE markers (IncludeBegin, IncludeEnd) are statements of the section they were met
// in, so that the structure is written back (`split_includes`).
//
// Identity. A statement has a key, assigned when it is created and never reused; Python handles hold keys, so they
// follow their statement when others are added or removed. Pointers into the list are valid only until its next change.
class ControlStatementList
{
public:
    [[nodiscard]] const std::vector<ControlStatement>& entries() const noexcept { return m_entries; }
    [[nodiscard]] std::vector<ControlStatement>& entries() noexcept { return m_entries; }
    // No statement at all: the model has nothing of this section to write.
    [[nodiscard]] bool isEmpty() const noexcept { return m_entries.empty(); }
    // A new key, for a statement.
    [[nodiscard]] std::uint32_t newKey() noexcept { return m_nextKey++; }
    // Becomes a copy of `other`, with new keys for every statement: a handle to what this section held is stale
    // afterwards instead of standing for another object.
    void replaceWith(const ControlStatementList& other);

    // --- lookup ------------------------------------------------------------------------------------------------------
    [[nodiscard]] std::optional<std::size_t> indexOf(std::uint32_t key) const noexcept;
    [[nodiscard]] ControlStatement* find(std::uint32_t key) noexcept;
    [[nodiscard]] const ControlStatement* find(std::uint32_t key) const noexcept;
    // The last typed statement with this format reference name (a statement repeated: the last one counts), or null.
    [[nodiscard]] const ControlStatement* findLast(std::string_view name) const noexcept;
    [[nodiscard]] ControlStatement* findLast(std::string_view name) noexcept;
    // Every typed statement with this format reference name, in file order.
    [[nodiscard]] std::vector<const ControlStatement*> findAll(std::string_view name) const;

    // --- editing -----------------------------------------------------------------------------------------------------
    // Puts a statement at `index` (the end when beyond it) and gives it its key.
    ControlStatement& insertAt(std::size_t index, ControlStatement statement);
    // Removes the statement with this key. False (and nothing changes) when there is none or when it is an INCLUDE
    // marker (the structure of the files is not edited here).
    bool remove(std::uint32_t key);
    // Removes every typed statement of this name; whether there was any.
    bool removeAll(std::string_view name);
    // The position after the last statement that is not a Comment (the comments at the end of a section stay last).
    [[nodiscard]] std::size_t endIndex() const noexcept;

private:
    std::vector<ControlStatement> m_entries;
    std::uint32_t m_nextKey = 1;
};

} // namespace oor::model
