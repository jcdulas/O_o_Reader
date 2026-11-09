#pragma once

#include "oor/model/cards/explicit/rigidEllipsoidExternal/RigidEllipsoidExternalCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct RigidEllipsoidExternalFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidEllipsoidExternalCard& card);
    [[nodiscard]] static std::optional<RigidEllipsoidExternalCard> read(FieldReader& reader);
    static void write(const RigidEllipsoidExternalCard& card, FieldWriter& writer);
};

// All the RELEX cards. A RELEX has no numeric ID (IdSpace::None, category Other): it is found by its NAME, which the
// format reference uses to tie the ellipsoid to the contact definitions and to the output, so a repeated NAME is
// reported when the deck is read. The cards are kept whole next to a column of their names.
class RigidEllipsoidExternalTable final : public TypedTable<RigidEllipsoidExternalTable, RigidEllipsoidExternalCard>
{
public:
    static constexpr std::string_view NAME = "RigidEllipsoidExternal";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = RigidEllipsoidExternalFormat;

    [[nodiscard]] static base::EntityId cardId(const RigidEllipsoidExternalCard&) noexcept { return 0; }
    std::uint32_t append(const RigidEllipsoidExternalCard& card, base::CardKey key);
    [[nodiscard]] RigidEllipsoidExternalCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const RigidEllipsoidExternalCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the card refers to no other card
    // Two RELEX cards of one NAME are reported.
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<std::string>& nameColumn() noexcept { return m_name; }

private:
    Column<std::string> m_name;
    CardRows<RigidEllipsoidExternalCard> m_cards;
};

} // namespace oor::model
