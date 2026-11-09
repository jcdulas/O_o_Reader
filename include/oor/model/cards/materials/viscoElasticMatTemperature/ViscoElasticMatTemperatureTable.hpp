#pragma once

#include "oor/model/cards/materials/viscoElasticMatTemperature/ViscoElasticMatTemperatureCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct ViscoElasticMatTemperatureFormat
{
    // The word that names a shift function on the card, and the function a word names (case does not matter).
    [[nodiscard]] static const char* shiftWord(ViscoElasticMatTemperatureShift shift) noexcept;
    [[nodiscard]] static std::optional<ViscoElasticMatTemperatureShift> parseShift(std::string_view text) noexcept;

    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ViscoElasticMatTemperatureCard& card);
    [[nodiscard]] static std::optional<ViscoElasticMatTemperatureCard> read(FieldReader& reader);
    static void write(const ViscoElasticMatTemperatureCard& card, FieldWriter& writer);
};

// All the MATTVE cards. The ID space is the card type's own: the ID is the one of the MATVE that the card extends, so
// it is also a reference to a material (renumbering the materials follows into it). The cards are kept whole (CardRows:
// the series have a length of their own); the query engine reads the ID only.
class ViscoElasticMatTemperatureTable final
    : public TypedTable<ViscoElasticMatTemperatureTable, ViscoElasticMatTemperatureCard>
{
public:
    static constexpr std::string_view NAME = "ViscoElasticMatTemperature";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = ViscoElasticMatTemperatureFormat;

    [[nodiscard]] static base::EntityId cardId(const ViscoElasticMatTemperatureCard& card) noexcept { return card.id; }
    std::uint32_t append(const ViscoElasticMatTemperatureCard& card, base::CardKey key);
    [[nodiscard]] ViscoElasticMatTemperatureCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ViscoElasticMatTemperatureCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ViscoElasticMatTemperatureCard> m_cards;
};

} // namespace oor::model
