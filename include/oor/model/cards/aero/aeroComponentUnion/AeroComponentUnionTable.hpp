#pragma once

#include "oor/model/cards/aero/aeroComponentUnion/AeroComponentUnionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct AeroComponentUnionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroComponentUnionCard& card);
    [[nodiscard]] static std::optional<AeroComponentUnionCard> read(FieldReader& reader);
    static void write(const AeroComponentUnionCard& card, FieldWriter& writer);
};

// All the AECOMPL cards, in the category AeroComponent that AECOMP shares. A component has no number
// (IdSpace::None): it is found by its NAME (`model.aecompl["WgAR"]`, matched without regard to the case). The labels
// are names of other components, not IDs, so the card refers to no card by number. The cards are kept whole next to a
// column of their names.
class AeroComponentUnionTable final : public TypedTable<AeroComponentUnionTable, AeroComponentUnionCard>
{
public:
    static constexpr std::string_view NAME = "AeroComponentUnion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroComponent, base::IdSpace::None};
    using Format = AeroComponentUnionFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroComponentUnionCard&) noexcept { return 0; }
    std::uint32_t append(const AeroComponentUnionCard& card, base::CardKey key);
    [[nodiscard]] AeroComponentUnionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroComponentUnionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the labels are names, not IDs

    // The name of a row, as written, without copying the card.
    [[nodiscard]] std::string nameAt(std::uint32_t row) const { return m_name.get(row); }
    [[nodiscard]] Column<std::string>& nameColumn() noexcept { return m_name; }

private:
    Column<std::string> m_name;
    CardRows<AeroComponentUnionCard> m_cards;
};

} // namespace oor::model
