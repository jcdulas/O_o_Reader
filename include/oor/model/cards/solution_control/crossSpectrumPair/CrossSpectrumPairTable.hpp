#pragma once

#include "oor/model/cards/solution_control/crossSpectrumPair/CrossSpectrumPairCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct CrossSpectrumPairFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CrossSpectrumPairCard& card);
    [[nodiscard]] static std::optional<CrossSpectrumPairCard> read(FieldReader& reader);
    static void write(const CrossSpectrumPairCard& card, FieldWriter& writer);
};

// All the RCROSS cards. The ID is a set ID shared by the RCROSS cards (IdSpace::Group). The ID of a response is an
// element for STRESS, STRAIN and FORCE (ReferenceTarget::Element) and a grid or scalar point (Node) for the others.
class CrossSpectrumPairTable final : public TypedTable<CrossSpectrumPairTable, CrossSpectrumPairCard>
{
public:
    static constexpr std::string_view NAME = "CrossSpectrumPair";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = CrossSpectrumPairFormat;

    [[nodiscard]] static base::EntityId cardId(const CrossSpectrumPairCard& card) noexcept { return card.id; }
    std::uint32_t append(const CrossSpectrumPairCard& card, base::CardKey key);
    [[nodiscard]] CrossSpectrumPairCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const CrossSpectrumPairCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& responseKind1Column() noexcept { return m_responseKind1; }
    [[nodiscard]] Column<std::string>& responseKind2Column() noexcept { return m_responseKind2; }
    [[nodiscard]] Column<std::array<base::EntityId, 2>>& targetIdsColumn() noexcept { return m_targetIds; }
    [[nodiscard]] Column<std::array<std::int32_t, 2>>& componentsColumn() noexcept { return m_components; }
    [[nodiscard]] Column<base::EntityId>& curveIdColumn() noexcept { return m_curveId; }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_responseKind1;
    Column<std::string> m_responseKind2;
    Column<std::array<base::EntityId, 2>> m_targetIds;
    Column<std::array<std::int32_t, 2>> m_components;
    Column<base::EntityId> m_curveId;
};

} // namespace oor::model
