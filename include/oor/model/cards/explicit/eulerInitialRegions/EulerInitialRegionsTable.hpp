#pragma once

#include "oor/model/cards/explicit/eulerInitialRegions/EulerInitialRegionsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct EulerInitialRegionsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const EulerInitialRegionsCard& card);
    [[nodiscard]] static std::optional<EulerInitialRegionsCard> read(FieldReader& reader);
    static void write(const EulerInitialRegionsCard& card, FieldWriter& writer);
};

// All the TICEUL1 cards. The SID is the ID of the table (IdSpace::PerCardType, category EulerInitialization): a PEULER1
// names it. The TSID names the group of TICREG cards that share it (EulerInitialRegions).
class EulerInitialRegionsTable final : public TypedTable<EulerInitialRegionsTable, EulerInitialRegionsCard>
{
public:
    static constexpr std::string_view NAME = "EulerInitialRegions";
    static inline const CardTypeInfo INFO{NAME, CardCategory::EulerInitialization, base::IdSpace::PerCardType};
    using Format = EulerInitialRegionsFormat;

    [[nodiscard]] static base::EntityId cardId(const EulerInitialRegionsCard& card) noexcept { return card.id; }
    std::uint32_t append(const EulerInitialRegionsCard& card, base::CardKey key);
    [[nodiscard]] EulerInitialRegionsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const EulerInitialRegionsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<EulerInitialRegionsCard> m_cards;
};

} // namespace oor::model
