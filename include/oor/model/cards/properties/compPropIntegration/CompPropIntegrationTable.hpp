#pragma once

#include "oor/model/cards/properties/compPropIntegration/CompPropIntegrationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CompPropIntegrationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CompPropIntegrationCard& card);
    [[nodiscard]] static std::optional<CompPropIntegrationCard> read(FieldReader& reader);
    static void write(const CompPropIntegrationCard& card, FieldWriter& writer);
};

// All the PCOMPF cards. A PCOMPF has no ID of its own (IdSpace::None, category Property): it is a list of properties,
// and a deck has as many as it has integration procedures. The PIDs name PCOMP and PCOMPG cards: a single one must
// exist (ReferenceTarget::Property), the ends of a THRU range may name nothing (PropertyInRange). The cards are kept
// whole next to a column of their INT values, which is what the query engine and the Arrow export see.
class CompPropIntegrationTable final : public TypedTable<CompPropIntegrationTable, CompPropIntegrationCard>
{
public:
    static constexpr std::string_view NAME = "CompPropIntegration";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::None};
    using Format = CompPropIntegrationFormat;

    [[nodiscard]] static base::EntityId cardId(const CompPropIntegrationCard&) noexcept { return 0; }
    std::uint32_t append(const CompPropIntegrationCard& card, base::CardKey key);
    [[nodiscard]] CompPropIntegrationCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CompPropIntegrationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<std::int32_t>& integrationColumn() noexcept { return m_integration; }

private:
    Column<std::int32_t> m_integration;
    CardRows<CompPropIntegrationCard> m_cards;
};

} // namespace oor::model
