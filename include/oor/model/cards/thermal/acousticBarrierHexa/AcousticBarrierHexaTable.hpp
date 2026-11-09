#pragma once

#include "oor/model/cards/thermal/acousticBarrierHexa/AcousticBarrierHexaCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct AcousticBarrierHexaFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AcousticBarrierHexaCard& card);
    [[nodiscard]] static std::optional<AcousticBarrierHexaCard> read(FieldReader& reader);
    static void write(const AcousticBarrierHexaCard& card, FieldWriter& writer);

    // The field of the format reference of the grid point at `index` of `nodes`: 3 to 14 for G1 to G12, 19 to 22 for
    // G17 to G20.
    [[nodiscard]] static int fieldOfNode(std::size_t index) noexcept;
};

// All the CHACBR cards. The ID is an element ID, in the space every element shares (IdSpace::Element); PID names a
// PACBAR.
class AcousticBarrierHexaTable final
    : public TypedTable<AcousticBarrierHexaTable, AcousticBarrierHexaCard>
{
public:
    static constexpr std::string_view NAME = "AcousticBarrierHexa";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = AcousticBarrierHexaFormat;

    [[nodiscard]] static base::EntityId cardId(const AcousticBarrierHexaCard& card) noexcept { return card.id; }
    std::uint32_t append(const AcousticBarrierHexaCard& card, base::CardKey key);
    [[nodiscard]] AcousticBarrierHexaCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AcousticBarrierHexaCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<std::array<base::EntityId, AcousticBarrierHexaCard::NODE_COUNT>>& nodesColumn() noexcept
    {
        return m_nodes;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_propId;
    Column<std::array<base::EntityId, AcousticBarrierHexaCard::NODE_COUNT>> m_nodes;
};

} // namespace oor::model
