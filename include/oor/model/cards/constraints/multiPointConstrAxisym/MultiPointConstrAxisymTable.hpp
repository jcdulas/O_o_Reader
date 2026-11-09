#pragma once

#include "oor/model/cards/constraints/multiPointConstrAxisym/MultiPointConstrAxisymCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct MultiPointConstrAxisymFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MultiPointConstrAxisymCard& card);
    [[nodiscard]] static std::optional<MultiPointConstrAxisymCard> read(FieldReader& reader);
    static void write(const MultiPointConstrAxisymCard& card, FieldWriter& writer);
};

// All the MPCAX cards. The ID is a constraint set ID shared by many cards (IdSpace::Group). The rings are RINGAX
// points, which are not cards of the model yet: they are ReferenceTarget::Other (not resolved, not renumbered). The
// four values of the terms of every card are four lists in CSR form.
class MultiPointConstrAxisymTable final : public TypedTable<MultiPointConstrAxisymTable, MultiPointConstrAxisymCard>
{
public:
    static constexpr std::string_view NAME = "MultiPointConstrAxisym";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Constraint, base::IdSpace::Group};
    using Format = MultiPointConstrAxisymFormat;

    [[nodiscard]] static base::EntityId cardId(const MultiPointConstrAxisymCard& card) noexcept { return card.id; }
    std::uint32_t append(const MultiPointConstrAxisymCard& card, base::CardKey key);
    [[nodiscard]] MultiPointConstrAxisymCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const MultiPointConstrAxisymCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& ringIdColumn() noexcept { return m_ringId; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& harmonicColumn() noexcept { return m_harmonic; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& componentColumn() noexcept { return m_component; }
    [[nodiscard]] VariableLengthColumn<double>& coefficientColumn() noexcept { return m_coefficient; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<base::EntityId> m_ringId;
    VariableLengthColumn<std::int32_t> m_harmonic;
    VariableLengthColumn<std::int32_t> m_component;
    VariableLengthColumn<double> m_coefficient;
};

} // namespace oor::model
