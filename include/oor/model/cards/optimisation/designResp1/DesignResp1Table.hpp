#pragma once

#include "oor/model/cards/optimisation/designResp1/DesignResp1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignResp1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignResp1Card& card);
    [[nodiscard]] static std::optional<DesignResp1Card> read(FieldReader& reader);
    static void write(const DesignResp1Card& card, FieldWriter& writer);
};

// All the DRESP1 cards. The ID is in the number space the three response cards share (category DesignResponse,
// IdSpace::PerCardType: the check `design.response-id-repeated` reports a number two of them have); what the numbers of
// the attributes name depends on the type of response (`DesignResp1References.hpp`).
class DesignResp1Table final : public TypedTable<DesignResp1Table, DesignResp1Card>
{
public:
    static constexpr std::string_view NAME = "DesignResp1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DesignResponse, base::IdSpace::PerCardType};
    using Format = DesignResp1Format;

    [[nodiscard]] static base::EntityId cardId(const DesignResp1Card& card) noexcept { return card.id; }
    std::uint32_t append(const DesignResp1Card& card, base::CardKey key);
    [[nodiscard]] DesignResp1Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignResp1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<DesignResp1Card> m_cards;
};

} // namespace oor::model
