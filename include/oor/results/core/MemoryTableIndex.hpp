#pragma once

#include "oor/results/core/ResultKind.hpp"
#include "oor/results/core/ResultLayout.hpp"
#include "oor/results/core/ResultTable.hpp"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <set>
#include <string>

namespace oor::results
{

// What tells two tables of a set made in memory apart: two tables with the
// same key would be the same table given twice. Besides the kind, the variant, the container (element type or variant
// of the grid point stresses), the subcase, the analysis, the design cycle and the superelement, the key holds:
// - the row layout, so that the centre and the corner layouts of a CQUAD4 are two tables;
// - the load set, so that the shapes of a buckling analysis (a table a shape, the mode number in the load set) are one
//   table each;
// - the name of the table, so that the same results in two frames or sorts are two tables.
struct MemoryTableKey
{
    ResultKind kind = ResultKind::Displacements;
    ResultVariant variant = ResultVariant::Plain;
    std::string container;
    std::string layout;
    std::int32_t subcase = 0;
    std::int32_t analysisCode = 0;
    std::int32_t designCycle = -1;
    std::int32_t superelement = -1;
    std::int32_t loadSet = 0;
    std::string tableName;

    [[nodiscard]] auto operator<=>(const MemoryTableKey&) const = default;
    [[nodiscard]] bool operator==(const MemoryTableKey&) const = default;
};

// The keys of the tables of one set made in memory. A new table is checked against the keys, not against every table
// the set holds, so adding n tables costs O(n log n) (it was O(n^2): prerequisite P0.25 of a companion package).
class MemoryTableIndex
{
public:
    [[nodiscard]] static MemoryTableKey keyOf(const ResultTableMeta& meta);
    // The key as the words of a message: "stress of ShellQuad4 (corner layout), subcase 1, static, table S1".
    [[nodiscard]] static std::string describe(const MemoryTableKey& key);

    [[nodiscard]] bool contains(const MemoryTableKey& key) const { return m_keys.contains(key); }
    // Adds the key; false when the set has it already.
    bool insert(const MemoryTableKey& key) { return m_keys.insert(key).second; }
    [[nodiscard]] std::size_t size() const noexcept { return m_keys.size(); }

private:
    std::set<MemoryTableKey> m_keys;
};

} // namespace oor::results
