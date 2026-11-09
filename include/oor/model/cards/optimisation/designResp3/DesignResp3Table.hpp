#pragma once

#include "oor/model/cards/optimisation/designResp3/DesignResp3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignResp3Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignResp3Card& card);
    [[nodiscard]] static std::optional<DesignResp3Card> read(FieldReader& reader);
    static void write(const DesignResp3Card& card, FieldWriter& writer);
};

// All the DRESP3 cards. The ID is in the number space the three response cards share (category DesignResponse,
// IdSpace::PerCardType); the groups name design variables, responses, relations and grid points.
class DesignResp3Table final : public TypedTable<DesignResp3Table, DesignResp3Card>
{
public:
    static constexpr std::string_view NAME = "DesignResp3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DesignResponse, base::IdSpace::PerCardType};
    using Format = DesignResp3Format;

    [[nodiscard]] static base::EntityId cardId(const DesignResp3Card& card) noexcept { return card.id; }
    std::uint32_t append(const DesignResp3Card& card, base::CardKey key);
    [[nodiscard]] DesignResp3Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignResp3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<DesignResp3Card> m_cards;
};

} // namespace oor::model
