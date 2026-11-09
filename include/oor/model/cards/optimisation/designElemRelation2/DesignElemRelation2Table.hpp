#pragma once

#include "oor/model/cards/optimisation/designElemRelation2/DesignElemRelation2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignElemRelation2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignElemRelation2Card& card);
    [[nodiscard]] static std::optional<DesignElemRelation2Card> read(FieldReader& reader);
    static void write(const DesignElemRelation2Card& card, FieldWriter& writer);
};

// All the DVCREL2 cards. The ID is unique among the DVCREL2 cards (IdSpace::PerCardType) and in the category
// ConnectivityRelation with the DVCREL1 cards; the EID is an element of the type TYPE names, the EQID a DEQATN and the
// DVIDs design variables.
class DesignElemRelation2Table final : public TypedTable<DesignElemRelation2Table, DesignElemRelation2Card>
{
public:
    static constexpr std::string_view NAME = "DesignElemRelation2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ConnectivityRelation, base::IdSpace::PerCardType};
    using Format = DesignElemRelation2Format;

    [[nodiscard]] static base::EntityId cardId(const DesignElemRelation2Card& card) noexcept { return card.id; }
    std::uint32_t append(const DesignElemRelation2Card& card, base::CardKey key);
    [[nodiscard]] DesignElemRelation2Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignElemRelation2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<DesignElemRelation2Card> m_cards;
};

} // namespace oor::model
