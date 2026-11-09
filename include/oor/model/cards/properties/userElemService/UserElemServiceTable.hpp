#pragma once

#include "oor/model/cards/properties/userElemService/UserElemServiceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct UserElemServiceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const UserElemServiceCard& card);
    [[nodiscard]] static std::optional<UserElemServiceCard> read(FieldReader& reader);
    static void write(const UserElemServiceCard& card, FieldWriter& writer);
};

// All the ELEMUDS cards. An ELEMUDS extends the PBUSH2D that has the same number (the PID is a reference to a property:
// a renumbering of the properties follows into it) and has an ID space of its own (IdSpace::PerCardType), as a PBUSHT
// does: the category is Property. The cards are kept whole next to a column of their PIDs.
class UserElemServiceTable final : public TypedTable<UserElemServiceTable, UserElemServiceCard>
{
public:
    static constexpr std::string_view NAME = "UserElemService";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::PerCardType};
    using Format = UserElemServiceFormat;

    [[nodiscard]] static base::EntityId cardId(const UserElemServiceCard& card) noexcept { return card.id; }
    std::uint32_t append(const UserElemServiceCard& card, base::CardKey key);
    [[nodiscard]] UserElemServiceCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const UserElemServiceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<UserElemServiceCard> m_cards;
};

} // namespace oor::model
