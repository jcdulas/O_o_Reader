#pragma once

#include "oor/model/cards/loads/imperfectionCase/ImperfectionCaseCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct ImperfectionCaseFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ImperfectionCaseCard& card);
    [[nodiscard]] static std::optional<ImperfectionCaseCard> read(FieldReader& reader);
    static void write(const ImperfectionCaseCard& card, FieldWriter& writer);
};

// All the IMPCASE cards, in the category ImperfectionCase: the ID is unique per card type (IdSpace::PerCardType) and is
// what the Case Control IMPERFECT names. The imperfections are one column of (first, last) runs; every single ID and
// each end of a THRU range is visited as ReferenceTarget::ImperfectionGeometry, the target of the IMPGEOM cards, so a
// renumbering of the IMPGEOM follows into the list.
class ImperfectionCaseTable final : public TypedTable<ImperfectionCaseTable, ImperfectionCaseCard>
{
public:
    static constexpr std::string_view NAME = "ImperfectionCase";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ImperfectionCase, base::IdSpace::PerCardType};
    using Format = ImperfectionCaseFormat;

    [[nodiscard]] static base::EntityId cardId(const ImperfectionCaseCard& card) noexcept { return card.id; }
    std::uint32_t append(const ImperfectionCaseCard& card, base::CardKey key);
    [[nodiscard]] ImperfectionCaseCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ImperfectionCaseCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<std::array<base::EntityId, 2>>& runsColumn() noexcept { return m_runs; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<std::array<base::EntityId, 2>> m_runs;
};

} // namespace oor::model
