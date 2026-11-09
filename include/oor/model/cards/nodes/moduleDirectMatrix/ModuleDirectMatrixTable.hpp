#pragma once

#include "oor/model/cards/nodes/moduleDirectMatrix/ModuleDirectMatrixCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct ModuleDirectMatrixFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleDirectMatrixCard& card);
    [[nodiscard]] static std::optional<ModuleDirectMatrixCard> read(FieldReader& reader);
    static void write(const ModuleDirectMatrixCard& card, FieldWriter& writer);
};

// All the MDDMIG cards: the headers and the columns of every matrix, one row each, in the order of the deck, category
// Matrix like the DMIG. A card has no numeric ID (IdSpace::None); the matrix is found by the name. They are kept whole
// next to a column of their names. The point of a column (MODJ, GJ) and the points of its terms are looked up in this
// model, as grid, scalar or extra points, when their module is 0, the main Bulk Data; the others are plain IDs.
class ModuleDirectMatrixTable final : public TypedTable<ModuleDirectMatrixTable, ModuleDirectMatrixCard>
{
public:
    static constexpr std::string_view NAME = "ModuleDirectMatrix";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Matrix, base::IdSpace::None};
    using Format = ModuleDirectMatrixFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleDirectMatrixCard&) noexcept { return 0; }
    std::uint32_t append(const ModuleDirectMatrixCard& card, base::CardKey key);
    [[nodiscard]] ModuleDirectMatrixCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleDirectMatrixCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<std::string>& entryNameColumn() noexcept { return m_entryName; }

private:
    Column<std::string> m_entryName;
    CardRows<ModuleDirectMatrixCard> m_cards;
};

} // namespace oor::model
