#pragma once

#include "oor/model/cards/aero/aeroControlVector/AeroControlVectorCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroControlVectorFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroControlVectorCard& card);
    [[nodiscard]] static std::optional<AeroControlVectorCard> read(FieldReader& reader);
    static void write(const AeroControlVectorCard& card, FieldWriter& writer);
};

// All the UXVEC cards, in the category ControlVector. The ID is unique among the UXVEC cards (IdSpace::PerCardType) and
// is what the Case Control AEUXREF and the AEDW, AEFORCE and AEPRESS cards name (ReferenceTarget::ControlVector). The
// labels are names, not references. The card is kept whole next to a column of its ID.
class AeroControlVectorTable final : public TypedTable<AeroControlVectorTable, AeroControlVectorCard>
{
public:
    static constexpr std::string_view NAME = "AeroControlVector";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ControlVector, base::IdSpace::PerCardType};
    using Format = AeroControlVectorFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroControlVectorCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroControlVectorCard& card, base::CardKey key);
    [[nodiscard]] AeroControlVectorCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroControlVectorCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the variables are named by label

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroControlVectorCard> m_cards;
};

} // namespace oor::model
