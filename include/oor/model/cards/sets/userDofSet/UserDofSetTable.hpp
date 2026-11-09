#pragma once

#include "oor/model/cards/sets/userDofSet/UserDofSetCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct UserDofSetFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const UserDofSetCard& card);
    [[nodiscard]] static std::optional<UserDofSetCard> read(FieldReader& reader);
    static void write(const UserDofSetCard& card, FieldWriter& writer);
};

// All the USET cards. A card has no ID (IdSpace::None): its set name is not an ID. The points and the components of
// every card are two lists in CSR form.
class UserDofSetTable final : public TypedTable<UserDofSetTable, UserDofSetCard>
{
public:
    static constexpr std::string_view NAME = "UserDofSet";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Set, base::IdSpace::None};
    using Format = UserDofSetFormat;

    [[nodiscard]] static base::EntityId cardId(const UserDofSetCard&) noexcept { return 0; }
    std::uint32_t append(const UserDofSetCard& card, base::CardKey key);
    [[nodiscard]] UserDofSetCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const UserDofSetCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<std::string>& setNameColumn() noexcept { return m_setName; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& componentColumn() noexcept { return m_component; }

private:
    Column<std::string> m_setName;
    VariableLengthColumn<base::EntityId> m_node;
    VariableLengthColumn<std::int32_t> m_component;
};

} // namespace oor::model
