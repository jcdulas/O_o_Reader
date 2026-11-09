#pragma once

#include "oor/model/cards/tables/tableGustSpectrum/TableGustSpectrumCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TableGustSpectrumFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TableGustSpectrumCard& card);
    [[nodiscard]] static std::optional<TableGustSpectrumCard> read(FieldReader& reader);
    static void write(const TableGustSpectrumCard& card, FieldWriter& writer);
};

// All the TABRNDG cards. The ID is a random table ID, in the ID space that TABRND1 shares (IdSpace::RandomTable).
class TableGustSpectrumTable final : public TypedTable<TableGustSpectrumTable, TableGustSpectrumCard>
{
public:
    static constexpr std::string_view NAME = "TableGustSpectrum";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Table, base::IdSpace::RandomTable};
    using Format = TableGustSpectrumFormat;

    [[nodiscard]] static base::EntityId cardId(const TableGustSpectrumCard& card) noexcept { return card.id; }
    std::uint32_t append(const TableGustSpectrumCard& card, base::CardKey key);
    [[nodiscard]] TableGustSpectrumCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const TableGustSpectrumCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a table refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::int32_t>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] Column<double>& turbulenceScaleRatioColumn() noexcept { return m_turbulenceScaleRatio; }
    [[nodiscard]] Column<double>& gustVelocityColumn() noexcept { return m_gustVelocity; }

private:
    Column<base::EntityId> m_id;
    Column<std::int32_t> m_kind{TableGustSpectrumCard::VON_KARMAN};
    Column<double> m_turbulenceScaleRatio;
    Column<double> m_gustVelocity;
};

} // namespace oor::model
