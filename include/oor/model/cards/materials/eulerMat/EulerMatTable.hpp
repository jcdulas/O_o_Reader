#pragma once

#include "oor/model/cards/materials/eulerMat/EulerMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct EulerMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const EulerMatCard& card);
    [[nodiscard]] static std::optional<EulerMatCard> read(FieldReader& reader);
    static void write(const EulerMatCard& card, FieldWriter& writer);
};

// All the MATDEUL cards. The ID is a material ID, in the space every material type shares (IdSpace::Material). The IDs
// of the models are references to the cards of the equation of state, shear, yield, failure and spall categories. The
// cards are kept whole (CardRows); the query engine reads the ID only.
class EulerMatTable final : public TypedTable<EulerMatTable, EulerMatCard>
{
public:
    static constexpr std::string_view NAME = "EulerMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = EulerMatFormat;

    [[nodiscard]] static base::EntityId cardId(const EulerMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const EulerMatCard& card, base::CardKey key);
    [[nodiscard]] EulerMatCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const EulerMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<EulerMatCard> m_cards;
};

} // namespace oor::model
