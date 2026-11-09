#pragma once

#include "oor/model/cards/thermal/radiationExchangeMatrix/RadiationExchangeMatrixCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RadiationExchangeMatrixFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RadiationExchangeMatrixCard& card);
    [[nodiscard]] static std::optional<RadiationExchangeMatrixCard> read(FieldReader& reader);
    static void write(const RadiationExchangeMatrixCard& card, FieldWriter& writer);
};

// All the RADMTX cards. The ID is the ICAVITY, shared by the columns of the matrix (category RadiationCavity,
// IdSpace::Group); the values of a column are one list in CSR form.
class RadiationExchangeMatrixTable final : public TypedTable<RadiationExchangeMatrixTable, RadiationExchangeMatrixCard>
{
public:
    static constexpr std::string_view NAME = "RadiationExchangeMatrix";
    static inline const CardTypeInfo INFO{NAME, CardCategory::RadiationCavity, base::IdSpace::Group};
    using Format = RadiationExchangeMatrixFormat;

    [[nodiscard]] static base::EntityId cardId(const RadiationExchangeMatrixCard& card) noexcept
    {
        return card.cavityId;
    }
    std::uint32_t append(const RadiationExchangeMatrixCard& card, base::CardKey key);
    [[nodiscard]] RadiationExchangeMatrixCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RadiationExchangeMatrixCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_cavityId.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_cavityId.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // its ID is the cavity it describes

    [[nodiscard]] Column<base::EntityId>& cavityIdColumn() noexcept { return m_cavityId; }
    [[nodiscard]] Column<std::int32_t>& columnIndexColumn() noexcept { return m_columnIndex; }
    [[nodiscard]] VariableLengthColumn<double>& factorsColumn() noexcept { return m_factors; }

private:
    Column<base::EntityId> m_cavityId;
    Column<std::int32_t> m_columnIndex;
    VariableLengthColumn<double> m_factors;
};

} // namespace oor::model
