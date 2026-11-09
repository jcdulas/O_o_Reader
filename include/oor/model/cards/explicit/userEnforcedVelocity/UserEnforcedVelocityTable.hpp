#pragma once

#include "oor/model/cards/explicit/userEnforcedVelocity/UserEnforcedVelocityCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct UserEnforcedVelocityFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const UserEnforcedVelocityCard& card);
    [[nodiscard]] static std::optional<UserEnforcedVelocityCard> read(FieldReader& reader);
    static void write(const UserEnforcedVelocityCard& card, FieldWriter& writer);
};

// All the FORCUDS cards. The LID is a set ID shared by the cards of the set (IdSpace::Group, category Other: it is not
// selected by a LOAD command). It is the excitation set that a TLOAD1 names in its EXCITEID, so the table says it
// defines excitation IDs (CardTable::definesExcitationIds). The grid points are nodes (a node inside a THRU range may
// be missing). The cards are kept whole (CardRows) next to their ID, because the list of runs has a length of its own.
class UserEnforcedVelocityTable final : public TypedTable<UserEnforcedVelocityTable, UserEnforcedVelocityCard>
{
public:
    static constexpr std::string_view NAME = "UserEnforcedVelocity";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = UserEnforcedVelocityFormat;

    [[nodiscard]] static base::EntityId cardId(const UserEnforcedVelocityCard& card) noexcept { return card.setId; }
    std::uint32_t append(const UserEnforcedVelocityCard& card, base::CardKey key);
    [[nodiscard]] UserEnforcedVelocityCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const UserEnforcedVelocityCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_setId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] bool definesExcitationIds() const noexcept override { return true; }
    [[nodiscard]] base::EntityId definedExcitationId(std::uint32_t row) const override { return m_setId.get(row); }

    [[nodiscard]] Column<base::EntityId>& setIdColumn() noexcept { return m_setId; }

private:
    Column<base::EntityId> m_setId;
    CardRows<UserEnforcedVelocityCard> m_cards;
};

} // namespace oor::model
