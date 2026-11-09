#pragma once

#include "oor/model/cards/properties/barShapeProp/BarShapePropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

struct BarShapePropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BarShapePropCard& card);
    [[nodiscard]] static std::optional<BarShapePropCard> read(FieldReader& reader);
    static void write(const BarShapePropCard& card, FieldWriter& writer);

    // The number of dimensions of a section of the standard library; 0 for a type that is not in it.
    [[nodiscard]] static std::size_t dimensionCount(std::string_view type) noexcept;
};

// All the PBARL cards. The dimensions of every section are one list in CSR form (the number depends on the type).
class BarShapePropTable final : public TypedTable<BarShapePropTable, BarShapePropCard>
{
public:
    static constexpr std::string_view NAME = "BarShapeProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = BarShapePropFormat;

    [[nodiscard]] static base::EntityId cardId(const BarShapePropCard& card) noexcept { return card.id; }
    std::uint32_t append(const BarShapePropCard& card, base::CardKey key);
    [[nodiscard]] BarShapePropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const BarShapePropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<std::string>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] VariableLengthColumn<double>& dimsColumn() noexcept { return m_dims; }
    [[nodiscard]] Column<double>& nonStructMassColumn() noexcept { return m_nonStructMass; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<std::string> m_kind;
    VariableLengthColumn<double> m_dims;
    Column<double> m_nonStructMass;
};

} // namespace oor::model
