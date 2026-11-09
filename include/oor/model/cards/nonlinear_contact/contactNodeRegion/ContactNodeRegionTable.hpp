#pragma once

#include "oor/model/cards/nonlinear_contact/contactNodeRegion/ContactNodeRegionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct ContactNodeRegionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactNodeRegionCard& card);
    [[nodiscard]] static std::optional<ContactNodeRegionCard> read(FieldReader& reader);
    static void write(const ContactNodeRegionCard& card, FieldWriter& writer);
};

// All the BCGRID cards. The ID is unique among them (IdSpace::PerCardType); the category is ContactBody, which is what
// a BCONECT names (the SOL 700 form is named by the BSID of a BCBODY, which resolves among the contact bodies too). The
// grid lists are one CSR column of (first, last, by) runs.
class ContactNodeRegionTable final : public TypedTable<ContactNodeRegionTable, ContactNodeRegionCard>
{
public:
    static constexpr std::string_view NAME = "ContactNodeRegion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactBody, base::IdSpace::PerCardType};
    using Format = ContactNodeRegionFormat;

    ContactNodeRegionTable() : m_dimension("ThreeDimensional") {}

    [[nodiscard]] static base::EntityId cardId(const ContactNodeRegionCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactNodeRegionCard& card, base::CardKey key);
    [[nodiscard]] ContactNodeRegionCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactNodeRegionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& bodyParamsIdColumn() noexcept { return m_bodyParamsId; }
    [[nodiscard]] Column<std::string>& dimensionColumn() noexcept { return m_dimension; }
    [[nodiscard]] Column<std::uint8_t>& sol700Column() noexcept { return m_sol700; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 3>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_bodyParamsId;
    Column<std::string> m_dimension;
    Column<std::uint8_t> m_sol700;
    VariableLengthColumn<std::array<base::EntityId, 3>> m_runs;
};

} // namespace oor::model
