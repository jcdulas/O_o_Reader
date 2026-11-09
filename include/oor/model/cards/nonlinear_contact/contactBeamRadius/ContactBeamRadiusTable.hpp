#pragma once

#include "oor/model/cards/nonlinear_contact/contactBeamRadius/ContactBeamRadiusCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct ContactBeamRadiusFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ContactBeamRadiusCard& card);
    [[nodiscard]] static std::optional<ContactBeamRadiusCard> read(FieldReader& reader);
    static void write(const ContactBeamRadiusCard& card, FieldWriter& writer);
};

// All the BCBMRAD cards. A card has no ID (IdSpace::None, category Other); several may be in a deck. The IDs are
// references to beam elements (TYPE EID) or contact bodies (TYPE BODY).
class ContactBeamRadiusTable final : public TypedTable<ContactBeamRadiusTable, ContactBeamRadiusCard>
{
public:
    static constexpr std::string_view NAME = "ContactBeamRadius";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ContactBeamRadiusFormat;

    ContactBeamRadiusTable() : m_idKind("ElemId") {}

    [[nodiscard]] static base::EntityId cardId(const ContactBeamRadiusCard&) noexcept { return 0; }
    std::uint32_t append(const ContactBeamRadiusCard& card, base::CardKey key);
    [[nodiscard]] ContactBeamRadiusCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ContactBeamRadiusCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<double>& radiusColumn() noexcept { return m_radius; }
    [[nodiscard]] Column<std::string>& idKindColumn() noexcept { return m_idKind; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 3>>& runsColumn() noexcept { return m_runs; }

private:
    Column<double> m_radius;
    Column<std::string> m_idKind;
    VariableLengthColumn<std::array<base::EntityId, 3>> m_runs;
};

} // namespace oor::model
