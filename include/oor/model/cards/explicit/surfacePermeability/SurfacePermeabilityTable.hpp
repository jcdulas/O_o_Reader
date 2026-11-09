#pragma once

#include "oor/model/cards/explicit/surfacePermeability/SurfacePermeabilityCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SurfacePermeabilityFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SurfacePermeabilityCard& card);
    [[nodiscard]] static std::optional<SurfacePermeabilityCard> read(FieldReader& reader);
    static void write(const SurfacePermeabilityCard& card, FieldWriter& writer);
};

// All the PERMEAB cards: the PID is unique among the PERMEAB cards (IdSpace::PerCardType, category Porosity, shared
// with the other porosity cards only by the check of the coordinator). A LEAKAGE names it.
class SurfacePermeabilityTable final : public TypedTable<SurfacePermeabilityTable, SurfacePermeabilityCard>
{
public:
    static constexpr std::string_view NAME = "SurfacePermeability";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Porosity, base::IdSpace::PerCardType};
    using Format = SurfacePermeabilityFormat;

    [[nodiscard]] static base::EntityId cardId(const SurfacePermeabilityCard& card) noexcept { return card.id; }
    std::uint32_t append(const SurfacePermeabilityCard& card, base::CardKey key);
    [[nodiscard]] SurfacePermeabilityCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SurfacePermeabilityCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<SurfacePermeabilityCard> m_cards;
};

} // namespace oor::model
