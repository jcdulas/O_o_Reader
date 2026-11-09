#pragma once

#include "oor/model/cards/superelements/superelementBulkOptions/SuperelementBulkOptionsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementBulkOptionsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementBulkOptionsCard& card);
    [[nodiscard]] static std::optional<SuperelementBulkOptionsCard> read(FieldReader& reader);
    static void write(const SuperelementBulkOptionsCard& card, FieldWriter& writer);
};

// All the SEBULK cards, one column for each field. The card is filed under its SEID (IdSpace::Group, category
// Superelement): a superelement has one, which a check (`superelement.sebulk-repeated`) looks for.
class SuperelementBulkOptionsTable final : public TypedTable<SuperelementBulkOptionsTable, SuperelementBulkOptionsCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementBulkOptions";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::Group};
    using Format = SuperelementBulkOptionsFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementBulkOptionsCard& card) noexcept
    {
        return card.superelementId;
    }
    std::uint32_t append(const SuperelementBulkOptionsCard& card, base::CardKey key);
    [[nodiscard]] SuperelementBulkOptionsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SuperelementBulkOptionsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_superelementId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_superelementId.set(row, id); }
    // RSEID is the number of a superelement.
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& superelementIdColumn() noexcept { return m_superelementId; }
    [[nodiscard]] Column<std::uint8_t>& partTypeColumn() noexcept { return m_partType; }
    [[nodiscard]] Column<base::EntityId>& referenceSuperelementIdColumn() noexcept { return m_referenceSuperelementId; }
    [[nodiscard]] Column<std::uint8_t>& methodColumn() noexcept { return m_method; }
    [[nodiscard]] Column<double>& toleranceColumn() noexcept { return m_tolerance; }
    [[nodiscard]] Column<std::uint8_t>& locationColumn() noexcept { return m_location; }
    [[nodiscard]] Column<base::EntityId>& unitNumberColumn() noexcept { return m_unitNumber; }

private:
    Column<base::EntityId> m_superelementId;
    Column<std::uint8_t> m_partType;
    Column<base::EntityId> m_referenceSuperelementId;
    Column<std::uint8_t> m_method;
    Column<double> m_tolerance;
    Column<std::uint8_t> m_location;
    Column<base::EntityId> m_unitNumber;
};

} // namespace oor::model
