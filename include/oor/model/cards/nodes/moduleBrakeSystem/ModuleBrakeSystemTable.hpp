#pragma once

#include "oor/model/cards/nodes/moduleBrakeSystem/ModuleBrakeSystemCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleBrakeSystemFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleBrakeSystemCard& card);
    [[nodiscard]] static std::optional<ModuleBrakeSystemCard> read(FieldReader& reader);
    static void write(const ModuleBrakeSystemCard& card, FieldWriter& writer);
};

// All the MDBKSYS cards. The ID is unique among them (IdSpace::PerCardType); the Case Control BSQUEAL selects it, as it
// selects a BSQUEAL or a BRKSYS (format reference MDBKSYS: the ID of the BSQUEAL command), so the card is in the
// category BrakeSqueal. The bodies of the disks are looked up as BCBODY1 cards (`ContactBody`) in their modules; the
// modules, the MOTION numbers and the BRKSYS numbers are plain integers. The cards are kept whole next to their ID.
class ModuleBrakeSystemTable final : public TypedTable<ModuleBrakeSystemTable, ModuleBrakeSystemCard>
{
public:
    static constexpr std::string_view NAME = "ModuleBrakeSystem";
    static inline const CardTypeInfo INFO{NAME, CardCategory::BrakeSqueal, base::IdSpace::PerCardType};
    using Format = ModuleBrakeSystemFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleBrakeSystemCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleBrakeSystemCard& card, base::CardKey key);
    [[nodiscard]] ModuleBrakeSystemCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleBrakeSystemCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleBrakeSystemCard> m_cards;
};

} // namespace oor::model
