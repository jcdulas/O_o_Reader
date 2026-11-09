#pragma once

#include "oor/model/cards/nonlinear_contact/contactSlideline/ContactSlidelineCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ContactSlidelineFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactSlidelineCard& card);
    [[nodiscard]] static std::optional<ContactSlidelineCard> read(FieldReader& reader);
    static void write(const ContactSlidelineCard& card, FieldWriter& writer);
};

// All the BCONP cards. The ID is unique among them (IdSpace::PerCardType, category SlidelineContact). The two lines are
// references to BLSEG cards, the friction one to a BFRIC and the system one to a coordinate system.
class ContactSlidelineTable final : public TypedTable<ContactSlidelineTable, ContactSlidelineCard>
{
public:
    static constexpr std::string_view NAME = "ContactSlideline";
    static inline const CardTypeInfo INFO{NAME, CardCategory::SlidelineContact, base::IdSpace::PerCardType};
    using Format = ContactSlidelineFormat;

    [[nodiscard]] static base::EntityId cardId(const ContactSlidelineCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactSlidelineCard& card, base::CardKey key);
    [[nodiscard]] ContactSlidelineCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactSlidelineCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& secondaryLineIdColumn() noexcept { return m_secondaryLineId; }
    [[nodiscard]] Column<base::EntityId>& primaryLineIdColumn() noexcept { return m_primaryLineId; }
    [[nodiscard]] Column<double>& penaltyScaleColumn() noexcept { return m_penaltyScale; }
    [[nodiscard]] Column<base::EntityId>& frictionIdColumn() noexcept { return m_frictionId; }
    [[nodiscard]] Column<std::int32_t>& penetrationKindColumn() noexcept { return m_penetrationKind; }
    [[nodiscard]] Column<base::EntityId>& frameIdColumn() noexcept { return m_frameId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_secondaryLineId;
    Column<base::EntityId> m_primaryLineId;
    Column<double> m_penaltyScale{1.0};
    Column<base::EntityId> m_frictionId;
    Column<std::int32_t> m_penetrationKind{1};
    Column<base::EntityId> m_frameId;
};

} // namespace oor::model
