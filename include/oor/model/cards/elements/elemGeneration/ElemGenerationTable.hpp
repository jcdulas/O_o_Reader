#pragma once

#include "oor/model/cards/elements/elemGeneration/ElemGenerationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct ElemGenerationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ElemGenerationCard& card);
    [[nodiscard]] static std::optional<ElemGenerationCard> read(FieldReader& reader);
    static void write(const ElemGenerationCard& card, FieldWriter& writer);
};

// All the CGEN cards (category Other). A card has no ID of its own (IdSpace::None): its FIELD_EID is the first of the
// element IDs it generates, which no card of the model holds, so a deck may have as many CGEN cards as it likes. The
// PID is a reference to a property (ReferenceTarget::Property); the FIELD_ID names a GRIDG card, which no step types,
// so it is a plain number. The cards are kept whole next to the columns of their four leading fields.
class ElemGenerationTable final : public TypedTable<ElemGenerationTable, ElemGenerationCard>
{
public:
    static constexpr std::string_view NAME = "ElemGeneration";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ElemGenerationFormat;

    [[nodiscard]] static base::EntityId cardId(const ElemGenerationCard&) noexcept { return 0; }
    std::uint32_t append(const ElemGenerationCard& card, base::CardKey key);
    [[nodiscard]] ElemGenerationCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ElemGenerationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<std::string>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] Column<base::EntityId>& firstElemIdColumn() noexcept { return m_firstElemId; }
    [[nodiscard]] Column<base::EntityId>& propIdColumn() noexcept { return m_propId; }
    [[nodiscard]] Column<base::EntityId>& gridFieldIdColumn() noexcept { return m_gridFieldId; }

private:
    Column<std::string> m_kind;
    Column<base::EntityId> m_firstElemId;
    Column<base::EntityId> m_propId;
    Column<base::EntityId> m_gridFieldId;
    CardRows<ElemGenerationCard> m_cards;
};

} // namespace oor::model
