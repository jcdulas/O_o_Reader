#pragma once

#include "oor/model/cards/optimisation/designResp2/DesignResp2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

// The functions the third field may name instead of a DEQATN.
inline constexpr std::array<std::string_view, 9> DESIGN_RESP2_FUNCTIONS{
    "Sum",     "SafetyFactorMax", "Average",      "SumOfSquares",    "RootSumSquare",
    "Maximum", "Minimum",         "BetaFunction", "MatchedResponses"};
// The words METHOD may hold.
inline constexpr std::array<std::string_view, 4> DESIGN_RESP2_METHODS{"Minimum", "Maximum", "LeastSquares",
                                                                      "BetaFunction"};

struct DesignResp2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignResp2Card& card);
    [[nodiscard]] static std::optional<DesignResp2Card> read(FieldReader& reader);
    static void write(const DesignResp2Card& card, FieldWriter& writer);
};

// All the DRESP2 cards. The ID is in the number space the three response cards share (category DesignResponse,
// IdSpace::PerCardType: the check `design.response-id-repeated` reports a number two of them have); the EQID is a
// DEQATN, the groups name design variables, responses, relations and grid points.
class DesignResp2Table final : public TypedTable<DesignResp2Table, DesignResp2Card>
{
public:
    static constexpr std::string_view NAME = "DesignResp2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DesignResponse, base::IdSpace::PerCardType};
    using Format = DesignResp2Format;

    [[nodiscard]] static base::EntityId cardId(const DesignResp2Card& card) noexcept { return card.id; }
    std::uint32_t append(const DesignResp2Card& card, base::CardKey key);
    [[nodiscard]] DesignResp2Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignResp2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<DesignResp2Card> m_cards;
};

} // namespace oor::model
