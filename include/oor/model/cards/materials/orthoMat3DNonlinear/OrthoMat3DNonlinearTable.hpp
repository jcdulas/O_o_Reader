#pragma once

#include "oor/model/cards/materials/orthoMat3DNonlinear/OrthoMat3DNonlinearCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct OrthoMat3DNonlinearFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const OrthoMat3DNonlinearCard& card);
    [[nodiscard]] static std::optional<OrthoMat3DNonlinearCard> read(FieldReader& reader);
    static void write(const OrthoMat3DNonlinearCard& card, FieldWriter& writer);
};

// All the MATORT cards. The ID is a material ID, in the space every material type shares (IdSpace::Material). The cards
// are kept whole (CardRows: a long record of mostly optional fields); the query engine reads the ID only. A MATORT
// refers to no card.
class OrthoMat3DNonlinearTable final : public TypedTable<OrthoMat3DNonlinearTable, OrthoMat3DNonlinearCard>
{
public:
    static constexpr std::string_view NAME = "OrthoMat3DNonlinear";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = OrthoMat3DNonlinearFormat;

    [[nodiscard]] static base::EntityId cardId(const OrthoMat3DNonlinearCard& card) noexcept { return card.id; }
    std::uint32_t append(const OrthoMat3DNonlinearCard& card, base::CardKey key);
    [[nodiscard]] OrthoMat3DNonlinearCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const OrthoMat3DNonlinearCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a MATORT refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<OrthoMat3DNonlinearCard> m_cards;
};

} // namespace oor::model
