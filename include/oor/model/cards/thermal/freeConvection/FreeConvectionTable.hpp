#pragma once

#include "oor/model/cards/thermal/freeConvection/FreeConvectionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct FreeConvectionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FreeConvectionCard& card);
    [[nodiscard]] static std::optional<FreeConvectionCard> read(FieldReader& reader);
    static void write(const FreeConvectionCard& card, FieldWriter& writer);
};

// All the CONV cards. The ID is the element ID of the surface element the card applies to (one CONV per element: its
// own ID per card type, IdSpace::PerCardType); it is also a reference to that element, so renumbering the elements
// follows into it. The category is Other: a CONV is a boundary condition, not a load set.
class FreeConvectionTable final : public TypedTable<FreeConvectionTable, FreeConvectionCard>
{
public:
    static constexpr std::string_view NAME = "FreeConvection";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = FreeConvectionFormat;

    [[nodiscard]] static base::EntityId cardId(const FreeConvectionCard& card) noexcept { return card.id; }
    std::uint32_t append(const FreeConvectionCard& card, base::CardKey key);
    [[nodiscard]] FreeConvectionCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FreeConvectionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& convectionPropIdColumn() noexcept { return m_convectionPropId; }
    [[nodiscard]] Column<base::EntityId>& filmNodeColumn() noexcept { return m_filmNode; }
    [[nodiscard]] Column<base::EntityId>& controlNodeColumn() noexcept { return m_controlNode; }
    [[nodiscard]] Column<std::array<base::EntityId, FreeConvectionCard::AMBIENT_COUNT>>& ambientNodesColumn() noexcept
    {
        return m_ambientNodes;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_convectionPropId;
    Column<base::EntityId> m_filmNode;
    Column<base::EntityId> m_controlNode;
    Column<std::array<base::EntityId, FreeConvectionCard::AMBIENT_COUNT>> m_ambientNodes;
};

} // namespace oor::model
