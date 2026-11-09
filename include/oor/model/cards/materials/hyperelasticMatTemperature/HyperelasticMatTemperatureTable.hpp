#pragma once

#include "oor/model/cards/materials/hyperelasticMatTemperature/HyperelasticMatTemperatureCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct HyperelasticMatTemperatureFormat
{
    // The word that names a model on the card, and the model a word names (case does not matter).
    [[nodiscard]] static const char* modelWord(HyperelasticMatTemperatureModel model) noexcept;
    [[nodiscard]] static std::optional<HyperelasticMatTemperatureModel> parseModel(std::string_view text) noexcept;

    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HyperelasticMatTemperatureCard& card);
    [[nodiscard]] static std::optional<HyperelasticMatTemperatureCard> read(FieldReader& reader);
    static void write(const HyperelasticMatTemperatureCard& card, FieldWriter& writer);
};

// All the MATTHE cards. The ID space is the card type's own: the ID is the one of the MATHE that the card extends, so
// it is also a reference to a material (renumbering the materials follows into it), and the tables are references to
// TABLEMi cards (ReferenceTarget::MaterialTable). The cards are kept whole (CardRows); the query engine reads the ID
// only.
class HyperelasticMatTemperatureTable final
    : public TypedTable<HyperelasticMatTemperatureTable, HyperelasticMatTemperatureCard>
{
public:
    static constexpr std::string_view NAME = "HyperelasticMatTemperature";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = HyperelasticMatTemperatureFormat;

    [[nodiscard]] static base::EntityId cardId(const HyperelasticMatTemperatureCard& card) noexcept { return card.id; }
    std::uint32_t append(const HyperelasticMatTemperatureCard& card, base::CardKey key);
    [[nodiscard]] HyperelasticMatTemperatureCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const HyperelasticMatTemperatureCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<HyperelasticMatTemperatureCard> m_cards;
};

} // namespace oor::model
