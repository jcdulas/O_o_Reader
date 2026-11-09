#pragma once

#include "oor/model/cards/optimisation/designConstr/DesignConstrCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignConstrFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignConstrCard& card);
    [[nodiscard]] static std::optional<DesignConstrCard> read(FieldReader& reader);
    static void write(const DesignConstrCard& card, FieldWriter& writer);
};

// All the DCONSTR cards. The DCID is a set ID (IdSpace::Group) shared with the DCONADD cards, which a DESSUB or DESGLB
// selects; the RID is a response, and LID and UID are TABLEDi tables.
class DesignConstrTable final : public TypedTable<DesignConstrTable, DesignConstrCard>
{
public:
    static constexpr std::string_view NAME = "DesignConstr";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DesignConstraint, base::IdSpace::Group};
    using Format = DesignConstrFormat;

    DesignConstrTable()
        : m_lowerAllowable(DesignConstrCard::NO_LOWER_BOUND),
          m_upperAllowable(DesignConstrCard::NO_UPPER_BOUND),
          m_highFrequency(DesignConstrCard::NO_HIGH_FREQUENCY)
    {
    }

    [[nodiscard]] static base::EntityId cardId(const DesignConstrCard& card) noexcept { return card.constraintSetId; }
    std::uint32_t append(const DesignConstrCard& card, base::CardKey key);
    [[nodiscard]] DesignConstrCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DesignConstrCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_constraintSetId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_constraintSetId.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& constraintSetIdColumn() noexcept { return m_constraintSetId; }
    [[nodiscard]] Column<base::EntityId>& responseIdColumn() noexcept { return m_responseId; }
    [[nodiscard]] Column<double>& lowerAllowableColumn() noexcept { return m_lowerAllowable; }
    [[nodiscard]] Column<base::EntityId>& lowerAllowableTableIdColumn() noexcept { return m_lowerAllowableTableId; }
    [[nodiscard]] Column<double>& upperAllowableColumn() noexcept { return m_upperAllowable; }
    [[nodiscard]] Column<base::EntityId>& upperAllowableTableIdColumn() noexcept { return m_upperAllowableTableId; }
    [[nodiscard]] Column<double>& lowFrequencyColumn() noexcept { return m_lowFrequency; }
    [[nodiscard]] Column<double>& highFrequencyColumn() noexcept { return m_highFrequency; }

private:
    Column<base::EntityId> m_constraintSetId;
    Column<base::EntityId> m_responseId;
    Column<double> m_lowerAllowable;
    Column<base::EntityId> m_lowerAllowableTableId;
    Column<double> m_upperAllowable;
    Column<base::EntityId> m_upperAllowableTableId;
    Column<double> m_lowFrequency;
    Column<double> m_highFrequency;
};

} // namespace oor::model
