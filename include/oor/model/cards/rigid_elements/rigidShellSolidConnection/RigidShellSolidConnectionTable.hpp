#pragma once

#include "oor/model/cards/rigid_elements/rigidShellSolidConnection/RigidShellSolidConnectionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct RigidShellSolidConnectionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidShellSolidConnectionCard& card);
    [[nodiscard]] static std::optional<RigidShellSolidConnectionCard> read(FieldReader& reader);
    static void write(const RigidShellSolidConnectionCard& card, FieldWriter& writer);
};

// All the RSSCON cards. A rigid element is an element: its ID is in the ID space shared by all the elements. The six
// IDs of a card are one column of six values; they are element references for TYPE = ELEM and grid point references for
// TYPE = GRID.
class RigidShellSolidConnectionTable final
    : public TypedTable<RigidShellSolidConnectionTable, RigidShellSolidConnectionCard>
{
public:
    static constexpr std::string_view NAME = "RigidShellSolidConnection";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = RigidShellSolidConnectionFormat;

    [[nodiscard]] static base::EntityId cardId(const RigidShellSolidConnectionCard& card) noexcept { return card.id; }
    std::uint32_t append(const RigidShellSolidConnectionCard& card, base::CardKey key);
    [[nodiscard]] RigidShellSolidConnectionCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RigidShellSolidConnectionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& typeColumn() noexcept { return m_type; }
    [[nodiscard]] Column<std::array<base::EntityId, RigidShellSolidConnectionCard::ID_COUNT>>& idsColumn() noexcept
    {
        return m_ids;
    }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_type;
    Column<std::array<base::EntityId, RigidShellSolidConnectionCard::ID_COUNT>> m_ids;
};

} // namespace oor::model
