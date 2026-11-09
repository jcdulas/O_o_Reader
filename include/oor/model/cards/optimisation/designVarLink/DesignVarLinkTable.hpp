#pragma once

#include "oor/model/cards/optimisation/designVarLink/DesignVarLinkCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignVarLinkFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignVarLinkCard& card);
    [[nodiscard]] static std::optional<DesignVarLinkCard> read(FieldReader& reader);
    static void write(const DesignVarLinkCard& card, FieldWriter& writer);
};

// All the DLINK cards. The ID is unique among them (IdSpace::PerCardType); the dependent variable and the independent
// ones are DESVAR IDs.
class DesignVarLinkTable final : public TypedTable<DesignVarLinkTable, DesignVarLinkCard>
{
public:
    static constexpr std::string_view NAME = "DesignVarLink";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = DesignVarLinkFormat;

    [[nodiscard]] static base::EntityId cardId(const DesignVarLinkCard& card) noexcept { return card.id; }
    std::uint32_t append(const DesignVarLinkCard& card, base::CardKey key);
    [[nodiscard]] DesignVarLinkCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignVarLinkCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<DesignVarLinkCard> m_cards;
};

} // namespace oor::model
