#pragma once

#include "oor/model/cards/explicit/initialSurfaceVolume/InitialSurfaceVolumeCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct InitialSurfaceVolumeFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const InitialSurfaceVolumeCard& card);
    [[nodiscard]] static std::optional<InitialSurfaceVolumeCard> read(FieldReader& reader);
    static void write(const InitialSurfaceVolumeCard& card, FieldWriter& writer);
};

// All the SURFINI cards. The VID is unique among the SURFINI cards (IdSpace::PerCardType) and is in the category
// EulerRegionShape, the one the region cards (TICREG, EULFREG) name through their VID. Its BSID names the BSURF that
// holds the segments of the surface.
class InitialSurfaceVolumeTable final : public TypedTable<InitialSurfaceVolumeTable, InitialSurfaceVolumeCard>
{
public:
    static constexpr std::string_view NAME = "InitialSurfaceVolume";
    static inline const CardTypeInfo INFO{NAME, CardCategory::EulerRegionShape, base::IdSpace::PerCardType};
    using Format = InitialSurfaceVolumeFormat;

    [[nodiscard]] static base::EntityId cardId(const InitialSurfaceVolumeCard& card) noexcept { return card.id; }
    std::uint32_t append(const InitialSurfaceVolumeCard& card, base::CardKey key);
    [[nodiscard]] InitialSurfaceVolumeCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const InitialSurfaceVolumeCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<InitialSurfaceVolumeCard> m_cards;
};

} // namespace oor::model
