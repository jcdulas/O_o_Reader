#pragma once

#include "oor/model/cards/elements/userDefined6Attributes/UserDefined6AttributesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct UserDefined6AttributesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const UserDefined6AttributesCard& card);
    [[nodiscard]] static std::optional<UserDefined6AttributesCard> read(FieldReader& reader);
    static void write(const UserDefined6AttributesCard& card, FieldWriter& writer);
};

// All the ADUM6 cards. A card has no ID (IdSpace::None, category Other) and names no other card.
class UserDefined6AttributesTable final : public TypedTable<UserDefined6AttributesTable, UserDefined6AttributesCard>
{
public:
    static constexpr std::string_view NAME = "UserDefined6Attributes";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = UserDefined6AttributesFormat;

    [[nodiscard]] static base::EntityId cardId(const UserDefined6AttributesCard&) noexcept { return 0; }
    std::uint32_t append(const UserDefined6AttributesCard& card, base::CardKey key);
    [[nodiscard]] UserDefined6AttributesCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const UserDefined6AttributesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // ELNM is a name, not a reference

    [[nodiscard]] Column<std::int32_t>& nodeCountColumn() noexcept { return m_nodeCount; }
    [[nodiscard]] Column<std::int32_t>& connectionFieldCountColumn() noexcept { return m_connectionFieldCount; }
    [[nodiscard]] Column<std::int32_t>& propertyFieldCountColumn() noexcept { return m_propertyFieldCount; }
    [[nodiscard]] Column<std::int32_t>& displacementComponentCountColumn() noexcept
    {
        return m_displacementComponentCount;
    }
    [[nodiscard]] Column<std::string>& elementNameColumn() noexcept { return m_elementName; }

private:
    Column<std::int32_t> m_nodeCount;
    Column<std::int32_t> m_connectionFieldCount;
    Column<std::int32_t> m_propertyFieldCount;
    Column<std::int32_t> m_displacementComponentCount;
    Column<std::string> m_elementName;
};

} // namespace oor::model
