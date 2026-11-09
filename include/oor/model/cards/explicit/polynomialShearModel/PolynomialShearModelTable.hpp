#pragma once

#include "oor/model/cards/explicit/polynomialShearModel/PolynomialShearModelCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct PolynomialShearModelFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PolynomialShearModelCard& card);
    [[nodiscard]] static std::optional<PolynomialShearModelCard> read(FieldReader& reader);
    static void write(const PolynomialShearModelCard& card, FieldWriter& writer);
};

// All the SHRPOL cards. The ID is the number of a shear model, unique among the cards of its category
// (IdSpace::PerCardType: the other SHRxxx cards share the category, and a check reports an ID two of them have). The
// cards are kept whole (CardRows) next to their ID: they are few, and the query engine reads the ID.
class PolynomialShearModelTable final : public TypedTable<PolynomialShearModelTable, PolynomialShearModelCard>
{
public:
    static constexpr std::string_view NAME = "PolynomialShearModel";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ShearModel, base::IdSpace::PerCardType};
    using Format = PolynomialShearModelFormat;

    [[nodiscard]] static base::EntityId cardId(const PolynomialShearModelCard& card) noexcept { return card.id; }
    std::uint32_t append(const PolynomialShearModelCard& card, base::CardKey key);
    [[nodiscard]] PolynomialShearModelCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const PolynomialShearModelCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a SHRPOL refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<PolynomialShearModelCard> m_cards;
};

} // namespace oor::model
