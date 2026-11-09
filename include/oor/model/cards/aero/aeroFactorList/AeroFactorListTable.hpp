#pragma once

#include "oor/model/cards/aero/aeroFactorList/AeroFactorListCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oor::model
{

struct AeroFactorListFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroFactorListCard& card);
    [[nodiscard]] static std::optional<AeroFactorListCard> read(FieldReader& reader);
    static void write(const AeroFactorListCard& card, FieldWriter& writer);
};

// All the AEFACT cards, in the category AeroFactor. The SID is unique among the AEFACT cards (IdSpace::PerCardType: an
// FLFACT may have the same number); a CAERO, a PAERO, a CSSCHD names a list by it (ReferenceTarget::AeroFactor). The
// numbers of all the cards are one column of lists.
class AeroFactorListTable final : public TypedTable<AeroFactorListTable, AeroFactorListCard>
{
public:
    static constexpr std::string_view NAME = "AeroFactorList";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroFactor, base::IdSpace::PerCardType};
    using Format = AeroFactorListFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroFactorListCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroFactorListCard& card, base::CardKey key);
    [[nodiscard]] AeroFactorListCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroFactorListCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    // The numbers of a card without copying them (the geometry of a CAERO reads its divisions from here).
    [[nodiscard]] std::span<const double> valuesAt(std::uint32_t row) const { return m_values.get(row); }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<double>& valuesColumn() noexcept { return m_values; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<double> m_values;
};

} // namespace oor::model
