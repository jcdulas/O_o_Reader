#pragma once

#include "oor/model/cards/materials/failureMatTemperature/FailureMatTemperatureCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FailureMatTemperatureFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FailureMatTemperatureCard& card);
    [[nodiscard]] static std::optional<FailureMatTemperatureCard> read(FieldReader& reader);
    static void write(const FailureMatTemperatureCard& card, FieldWriter& writer);
};

// All the MATTF cards. The ID space is the card type's own: the ID is the one of the MATF that the card extends (the ID
// of its material), so it is also a reference to a material (renumbering the materials follows into it), and the tables
// are references to TABLEMi cards (ReferenceTarget::MaterialTable). The cards are kept whole (CardRows: the blocks have
// a count of their own); the query engine reads the ID only.
class FailureMatTemperatureTable final : public TypedTable<FailureMatTemperatureTable, FailureMatTemperatureCard>
{
public:
    static constexpr std::string_view NAME = "FailureMatTemperature";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = FailureMatTemperatureFormat;

    [[nodiscard]] static base::EntityId cardId(const FailureMatTemperatureCard& card) noexcept { return card.id; }
    std::uint32_t append(const FailureMatTemperatureCard& card, base::CardKey key);
    [[nodiscard]] FailureMatTemperatureCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const FailureMatTemperatureCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<FailureMatTemperatureCard> m_cards;
};

} // namespace oor::model
