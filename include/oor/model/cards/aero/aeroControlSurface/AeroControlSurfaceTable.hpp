#pragma once

#include "oor/model/cards/aero/aeroControlSurface/AeroControlSurfaceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct AeroControlSurfaceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroControlSurfaceCard& card);
    [[nodiscard]] static std::optional<AeroControlSurfaceCard> read(FieldReader& reader);
    static void write(const AeroControlSurfaceCard& card, FieldWriter& writer);
};

// All the AESURF cards, in the category AeroControlSurface. The solver ignores the ID, but a CSSCHD names the surface
// by it (ReferenceTarget::AeroControlSurface), so it is unique among the AESURF cards (IdSpace::PerCardType). CID1 and
// CID2 are coordinate systems (ReferenceTarget::Coordinate), ALID1 and ALID2 AELIST cards (ReferenceTarget::AeroList),
// TQLLIM and TQULIM TABLED1 cards (ReferenceTarget::DynamicTable). A TRIM, an AELINK and a UXVEC find the surface by
// its label, which is not a reference. The absent limits are NaN in the columns of the hinge moments.
class AeroControlSurfaceTable final : public TypedTable<AeroControlSurfaceTable, AeroControlSurfaceCard>
{
public:
    static constexpr std::string_view NAME = "AeroControlSurface";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroControlSurface, base::IdSpace::PerCardType};
    using Format = AeroControlSurfaceFormat;

    AeroControlSurfaceTable();

    [[nodiscard]] static base::EntityId cardId(const AeroControlSurfaceCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroControlSurfaceCard& card, base::CardKey key);
    [[nodiscard]] AeroControlSurfaceCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroControlSurfaceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& labelColumn() noexcept { return m_label; }
    [[nodiscard]] Column<base::EntityId>& frameId1Column() noexcept { return m_frameId1; }
    [[nodiscard]] Column<base::EntityId>& boxListId1Column() noexcept { return m_boxListId1; }
    [[nodiscard]] Column<base::EntityId>& frameId2Column() noexcept { return m_frameId2; }
    [[nodiscard]] Column<base::EntityId>& boxListId2Column() noexcept { return m_boxListId2; }
    [[nodiscard]] Column<double>& effectivenessColumn() noexcept { return m_effectiveness; }
    [[nodiscard]] Column<std::uint8_t>& linearDownwashColumn() noexcept { return m_linearDownwash; }
    [[nodiscard]] Column<double>& referenceChordColumn() noexcept { return m_referenceChord; }
    [[nodiscard]] Column<double>& referenceAreaColumn() noexcept { return m_referenceArea; }
    [[nodiscard]] Column<double>& deflectionLowerLimitColumn() noexcept { return m_deflectionLowerLimit; }
    [[nodiscard]] Column<double>& deflectionUpperLimitColumn() noexcept { return m_deflectionUpperLimit; }
    [[nodiscard]] Column<double>& hingeMomentLowerLimitColumn() noexcept { return m_hingeMomentLowerLimit; }
    [[nodiscard]] Column<double>& hingeMomentUpperLimitColumn() noexcept { return m_hingeMomentUpperLimit; }
    [[nodiscard]] Column<base::EntityId>& lowerLimitTableIdColumn() noexcept { return m_lowerLimitTableId; }
    [[nodiscard]] Column<base::EntityId>& upperLimitTableIdColumn() noexcept { return m_upperLimitTableId; }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_label;
    Column<base::EntityId> m_frameId1;
    Column<base::EntityId> m_boxListId1;
    Column<base::EntityId> m_frameId2;
    Column<base::EntityId> m_boxListId2;
    Column<double> m_effectiveness;
    Column<std::uint8_t> m_linearDownwash;
    Column<double> m_referenceChord;
    Column<double> m_referenceArea;
    Column<double> m_deflectionLowerLimit;
    Column<double> m_deflectionUpperLimit;
    Column<double> m_hingeMomentLowerLimit;
    Column<double> m_hingeMomentUpperLimit;
    Column<base::EntityId> m_lowerLimitTableId;
    Column<base::EntityId> m_upperLimitTableId;
};

} // namespace oor::model
