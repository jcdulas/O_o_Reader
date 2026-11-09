#pragma once

#include "oor/model/cards/materials/elastoPlasticMatTemperature/ElastoPlasticMatTemperatureCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ElastoPlasticMatTemperatureFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ElastoPlasticMatTemperatureCard& card);
    [[nodiscard]] static std::optional<ElastoPlasticMatTemperatureCard> read(FieldReader& reader);
    static void write(const ElastoPlasticMatTemperatureCard& card, FieldWriter& writer);
};

// All the MATTEP cards. The ID space is the card type's own: the ID is the one of the MATEP that the card extends, so
// it is also a reference to a material (renumbering the materials follows into it). T(Y0), T(H) and the constants of
// the Chaboche group are references to TABLEMi cards (ReferenceTarget::MaterialTable) and T(FID) to a TABLEST
// (StressTable). The cards are kept whole (CardRows); the query engine reads the ID only.
class ElastoPlasticMatTemperatureTable final
    : public TypedTable<ElastoPlasticMatTemperatureTable, ElastoPlasticMatTemperatureCard>
{
public:
    static constexpr std::string_view NAME = "ElastoPlasticMatTemperature";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = ElastoPlasticMatTemperatureFormat;

    [[nodiscard]] static base::EntityId cardId(const ElastoPlasticMatTemperatureCard& card) noexcept { return card.id; }
    std::uint32_t append(const ElastoPlasticMatTemperatureCard& card, base::CardKey key);
    [[nodiscard]] ElastoPlasticMatTemperatureCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ElastoPlasticMatTemperatureCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ElastoPlasticMatTemperatureCard> m_cards;
};

} // namespace oor::model
