#pragma once

#include "oor/model/cards/nodes/moduleContactPair/ModuleContactPairCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleContactPairFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleContactPairCard& card);
    [[nodiscard]] static std::optional<ModuleContactPairCard> read(FieldReader& reader);
    static void write(const ModuleContactPairCard& card, FieldWriter& writer);
};

// All the MDBCNCT cards. The ID is unique among them (IdSpace::PerCardType, category ContactPair, the category of the
// BCONECT: an MDBCTB1 names either, and BCONTACT may select either). BCGPID and BCPPID are references to BCONPRG and
// BCONPRP cards. The bodies are references to BCBODY1 cards (the body category) in their module: the main Bulk Data,
// or the part of another module. The cards are kept whole next to their ID.
class ModuleContactPairTable final : public TypedTable<ModuleContactPairTable, ModuleContactPairCard>
{
public:
    static constexpr std::string_view NAME = "ModuleContactPair";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactPair, base::IdSpace::PerCardType};
    using Format = ModuleContactPairFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleContactPairCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleContactPairCard& card, base::CardKey key);
    [[nodiscard]] ModuleContactPairCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleContactPairCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleContactPairCard> m_cards;
};

} // namespace oor::model
