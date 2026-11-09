#pragma once

#include "oor/model/cards/nonlinear_contact/contactPropertyRegion/ContactPropertyRegionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct ContactPropertyRegionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactPropertyRegionCard& card);
    [[nodiscard]] static std::optional<ContactPropertyRegionCard> read(FieldReader& reader);
    static void write(const ContactPropertyRegionCard& card, FieldWriter& writer);
};

// All the BCPROP cards (category ContactSurface, unique among them). The property lists are one CSR column of
// (first, last, by) runs; the ends of a range may name properties that do not exist.
class ContactPropertyRegionTable final : public TypedTable<ContactPropertyRegionTable, ContactPropertyRegionCard>
{
public:
    static constexpr std::string_view NAME = "ContactPropertyRegion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactSurface, base::IdSpace::PerCardType};
    using Format = ContactPropertyRegionFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactPropertyRegionCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactPropertyRegionCard& card, base::CardKey key);
    [[nodiscard]] ContactPropertyRegionCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactPropertyRegionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 3>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<std::array<base::EntityId, 3>> m_runs;
};

} // namespace oor::model
