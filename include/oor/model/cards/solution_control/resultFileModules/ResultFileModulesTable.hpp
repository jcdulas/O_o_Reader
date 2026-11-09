#pragma once

#include "oor/model/cards/solution_control/resultFileModules/ResultFileModulesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ResultFileModulesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ResultFileModulesCard& card);
    [[nodiscard]] static std::optional<ResultFileModulesCard> read(FieldReader& reader);
    static void write(const ResultFileModulesCard& card, FieldWriter& writer);
};

// All the HDF5DA cards (IdSpace::PerCardType, category Other: the Case Control HDF5DA command names one). The CONFIG
// and the UDIDi are UserDefinedName references. The cards are kept whole (CardRows) next to their ID: the list of
// modules has any length, and the query engine reads the ID.
class ResultFileModulesTable final : public TypedTable<ResultFileModulesTable, ResultFileModulesCard>
{
public:
    static constexpr std::string_view NAME = "ResultFileModules";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ResultFileModulesFormat;

    [[nodiscard]] static base::EntityId cardId(const ResultFileModulesCard& card) noexcept { return card.id; }
    std::uint32_t append(const ResultFileModulesCard& card, base::CardKey key);
    [[nodiscard]] ResultFileModulesCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ResultFileModulesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ResultFileModulesCard> m_cards;
};

} // namespace oor::model
