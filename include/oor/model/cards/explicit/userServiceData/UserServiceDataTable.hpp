#pragma once

#include "oor/model/cards/explicit/userServiceData/UserServiceDataCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct UserServiceDataFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const UserServiceDataCard& card);
    [[nodiscard]] static std::optional<UserServiceDataCard> read(FieldReader& reader);
    static void write(const UserServiceDataCard& card, FieldWriter& writer);
};

// All the GENUDS cards. A card is named by its SRV_ID, not by a number: IdSpace::None, category Other, and a text
// column for the query engine. The card refers to no other card. The cards are kept whole (CardRows): the data blocks
// have a length of their own.
class UserServiceDataTable final : public TypedTable<UserServiceDataTable, UserServiceDataCard>
{
public:
    static constexpr std::string_view NAME = "UserServiceData";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = UserServiceDataFormat;

    [[nodiscard]] static base::EntityId cardId(const UserServiceDataCard&) noexcept { return 0; }
    std::uint32_t append(const UserServiceDataCard& card, base::CardKey key);
    [[nodiscard]] UserServiceDataCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const UserServiceDataCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a GENUDS refers to no other card

    [[nodiscard]] Column<std::string>& serviceIdColumn() noexcept { return m_serviceId; }

private:
    Column<std::string> m_serviceId;
    CardRows<UserServiceDataCard> m_cards;
};

} // namespace oor::model
