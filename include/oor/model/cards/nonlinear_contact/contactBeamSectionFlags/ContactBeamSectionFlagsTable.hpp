#pragma once

#include "oor/model/cards/nonlinear_contact/contactBeamSectionFlags/ContactBeamSectionFlagsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct ContactBeamSectionFlagsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactBeamSectionFlagsCard& card);
    [[nodiscard]] static std::optional<ContactBeamSectionFlagsCard> read(FieldReader& reader);
    static void write(const ContactBeamSectionFlagsCard& card, FieldWriter& writer);
};

// All the BCPFLG cards. The card is keyed by the property of its beam section: unique among them
// (IdSpace::PerCardType, category Other); the property is a reference.
class ContactBeamSectionFlagsTable final : public TypedTable<ContactBeamSectionFlagsTable, ContactBeamSectionFlagsCard>
{
public:
    static constexpr std::string_view NAME = "ContactBeamSectionFlags";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ContactBeamSectionFlagsFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactBeamSectionFlagsCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactBeamSectionFlagsCard& card, base::CardKey key);
    [[nodiscard]] ContactBeamSectionFlagsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactBeamSectionFlagsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<std::array<std::int32_t, 2>>& segmentsColumn() noexcept { return m_segments; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<std::array<std::int32_t, 2>> m_segments;
};

} // namespace oor::model
