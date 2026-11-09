#pragma once

#include "oor/model/cards/nonlinear_contact/contactBeamCap/ContactBeamCapCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ContactBeamCapFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactBeamCapCard& card);
    [[nodiscard]] static std::optional<ContactBeamCapCard> read(FieldReader& reader);
    static void write(const ContactBeamCapCard& card, FieldWriter& writer);
};

// All the BCSCAP cards. The card is keyed by its beam element: unique among them (IdSpace::PerCardType, category
// Other); the element is a reference.
class ContactBeamCapTable final : public TypedTable<ContactBeamCapTable, ContactBeamCapCard>
{
public:
    static constexpr std::string_view NAME = "ContactBeamCap";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ContactBeamCapFormat;

    ContactBeamCapTable() : m_segmentCount(32) {}

    [[nodiscard]] static base::EntityId cardId(const ContactBeamCapCard& card) noexcept { return card.id; }
    std::uint32_t append(const ContactBeamCapCard& card, base::CardKey key);
    [[nodiscard]] ContactBeamCapCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactBeamCapCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::int32_t>& capKindColumn() noexcept { return m_capKind; }
    [[nodiscard]] Column<std::int32_t>& segmentCountColumn() noexcept { return m_segmentCount; }

private:
    Column<base::EntityId> m_id;
    Column<std::int32_t> m_capKind;
    Column<std::int32_t> m_segmentCount;
};

} // namespace oor::model
