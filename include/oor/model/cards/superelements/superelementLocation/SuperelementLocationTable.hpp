#pragma once

#include "oor/model/cards/superelements/superelementLocation/SuperelementLocationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementLocationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementLocationCard& card);
    [[nodiscard]] static std::optional<SuperelementLocationCard> read(FieldReader& reader);
    static void write(const SuperelementLocationCard& card, FieldWriter& writer);
};

// All the SELOC cards, category Superelement, no ID of their own (IdSpace::None). One column for each field.
class SuperelementLocationTable final : public TypedTable<SuperelementLocationTable, SuperelementLocationCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementLocation";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementLocationFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementLocationCard&) noexcept { return 0; }
    std::uint32_t append(const SuperelementLocationCard& card, base::CardKey key);
    [[nodiscard]] SuperelementLocationCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SuperelementLocationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // SEID is a superelement, PA1 to PA3 are points of that superelement, PB1 to PB3 points of the main Bulk Data.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }
    [[nodiscard]] Column<std::array<base::EntityId, 3>>& pointsAColumn() noexcept { return m_pointsA; }
    [[nodiscard]] Column<std::array<base::EntityId, 3>>& pointsBColumn() noexcept { return m_pointsB; }

private:
    Column<base::EntityId> m_superelementId;
    Column<std::array<base::EntityId, 3>> m_pointsA;
    Column<std::array<base::EntityId, 3>> m_pointsB;
};

} // namespace oor::model
