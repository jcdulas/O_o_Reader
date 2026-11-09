#pragma once

#include "oor/model/cards/solution_control/initialPlasticStrain/InitialPlasticStrainCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct InitialPlasticStrainFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const InitialPlasticStrainCard& card);
    [[nodiscard]] static std::optional<InitialPlasticStrainCard> read(FieldReader& reader);
    static void write(const InitialPlasticStrainCard& card, FieldWriter& writer);
};

// All the IPSTRAIN cards. A card has no ID (IdSpace::None, category Other) and any number of them may be in a deck; the
// two ends of its range of elements are `ElementInRange` references (the elements of a range that exist are the ones
// that count). The cards are kept whole.
class InitialPlasticStrainTable final : public TypedTable<InitialPlasticStrainTable, InitialPlasticStrainCard>
{
public:
    static constexpr std::string_view NAME = "InitialPlasticStrain";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = InitialPlasticStrainFormat;

    [[nodiscard]] static base::EntityId cardId(const InitialPlasticStrainCard&) noexcept { return 0; }
    std::uint32_t append(const InitialPlasticStrainCard& card, base::CardKey key);
    [[nodiscard]] InitialPlasticStrainCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const InitialPlasticStrainCard& card) { m_cards.set(row, card); }
    void keepRows(const std::vector<std::uint32_t>& rows) { m_cards.keepRows(rows); }

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

private:
    CardRows<InitialPlasticStrainCard> m_cards;
};

} // namespace oor::model
