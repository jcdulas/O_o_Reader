#pragma once

#include "oor/model/cards/superelements/superelementMirrorPlane/SuperelementMirrorPlaneCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementMirrorPlaneFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementMirrorPlaneCard& card);
    [[nodiscard]] static std::optional<SuperelementMirrorPlaneCard> read(FieldReader& reader);
    static void write(const SuperelementMirrorPlaneCard& card, FieldWriter& writer);
};

// All the SEMPLN cards, category Superelement, no ID of their own (IdSpace::None). One column for each field.
class SuperelementMirrorPlaneTable final : public TypedTable<SuperelementMirrorPlaneTable, SuperelementMirrorPlaneCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementMirrorPlane";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementMirrorPlaneFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementMirrorPlaneCard&) noexcept { return 0; }
    std::uint32_t append(const SuperelementMirrorPlaneCard& card, base::CardKey key);
    [[nodiscard]] SuperelementMirrorPlaneCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SuperelementMirrorPlaneCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // SEID is a superelement; the three points are grid points or POINT cards of the main Bulk Data (this model).
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }
    [[nodiscard]] Column<std::array<base::EntityId, 3>>& pointsColumn() noexcept { return m_points; }

private:
    Column<base::EntityId> m_superelementId;
    Column<std::array<base::EntityId, 3>> m_points;
};

} // namespace oor::model
