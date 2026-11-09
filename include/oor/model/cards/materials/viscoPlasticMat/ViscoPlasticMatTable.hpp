#pragma once

#include "oor/model/cards/materials/viscoPlasticMat/ViscoPlasticMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ViscoPlasticMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ViscoPlasticMatCard& card);
    [[nodiscard]] static std::optional<ViscoPlasticMatCard> read(FieldReader& reader);
    static void write(const ViscoPlasticMatCard& card, FieldWriter& writer);
};

// All the MATVP cards. The ID space is the card type's own: the ID is the one of the material that the card extends, so
// it is also a reference to a material (renumbering the materials follows into it). IT3D names a TABL3D0, TABL3D1 or
// TABL3D2 (ReferenceTarget::MultiDimensionalTable), so a renumbering of those tables follows into it. The cards are
// kept whole (CardRows); the query engine reads the ID only.
class ViscoPlasticMatTable final : public TypedTable<ViscoPlasticMatTable, ViscoPlasticMatCard>
{
public:
    static constexpr std::string_view NAME = "ViscoPlasticMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = ViscoPlasticMatFormat;

    [[nodiscard]] static base::EntityId cardId(const ViscoPlasticMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const ViscoPlasticMatCard& card, base::CardKey key);
    [[nodiscard]] ViscoPlasticMatCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ViscoPlasticMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ViscoPlasticMatCard> m_cards;
};

} // namespace oor::model
