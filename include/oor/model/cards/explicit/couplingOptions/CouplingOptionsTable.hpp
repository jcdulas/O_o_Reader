#pragma once

#include "oor/model/cards/explicit/couplingOptions/CouplingOptionsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CouplingOptionsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CouplingOptionsCard& card);
    [[nodiscard]] static std::optional<CouplingOptionsCard> read(FieldReader& reader);
    static void write(const CouplingOptionsCard& card, FieldWriter& writer);
};

// All the COUOPT cards. The cards of a set share their OPTID, which is the ID of the table (IdSpace::Group, category
// CouplingOptions): a COUPLE names the set, not a card. The CID is a number of the card only. SUBID names a surface,
// the pressure a table of time.
class CouplingOptionsTable final : public TypedTable<CouplingOptionsTable, CouplingOptionsCard>
{
public:
    static constexpr std::string_view NAME = "CouplingOptions";
    static inline const CardTypeInfo INFO{NAME, CardCategory::CouplingOptions, base::IdSpace::Group};
    using Format = CouplingOptionsFormat;

    [[nodiscard]] static base::EntityId cardId(const CouplingOptionsCard& card) noexcept { return card.setId; }
    std::uint32_t append(const CouplingOptionsCard& card, base::CardKey key);
    [[nodiscard]] CouplingOptionsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CouplingOptionsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_setId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& setIdColumn() noexcept { return m_setId; }

private:
    Column<base::EntityId> m_setId;
    CardRows<CouplingOptionsCard> m_cards;
};

} // namespace oor::model
