#pragma once

#include "oor/model/cards/elements/userDefined8Prop/UserDefined8PropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct UserDefined8PropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const UserDefined8PropCard& card);
    [[nodiscard]] static std::optional<UserDefined8PropCard> read(FieldReader& reader);
    static void write(const UserDefined8PropCard& card, FieldWriter& writer);
};

// All the PDUM8 properties. The ID is a property ID, in the space every property shares (IdSpace::Property); MID names
// a material. The additional fields are kept as two columns, the kind and the number of each field.
class UserDefined8PropTable final : public TypedTable<UserDefined8PropTable, UserDefined8PropCard>
{
public:
    static constexpr std::string_view NAME = "UserDefined8Prop";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = UserDefined8PropFormat;

    [[nodiscard]] static base::EntityId cardId(const UserDefined8PropCard& card) noexcept { return card.id; }
    std::uint32_t append(const UserDefined8PropCard& card, base::CardKey key);
    [[nodiscard]] UserDefined8PropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const UserDefined8PropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] DummyFieldColumns& fieldColumns() noexcept { return m_fields; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    DummyFieldColumns m_fields;
};

} // namespace oor::model
