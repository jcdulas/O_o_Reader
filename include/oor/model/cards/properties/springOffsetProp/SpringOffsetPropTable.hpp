#pragma once

#include "oor/model/cards/aero/BlankReal.hpp"
#include "oor/model/cards/properties/springOffsetProp/SpringOffsetPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SpringOffsetPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SpringOffsetPropCard& card);
    [[nodiscard]] static std::optional<SpringOffsetPropCard> read(FieldReader& reader);
    static void write(const SpringOffsetPropCard& card, FieldWriter& writer);
};

// All the PSPRMAT properties, kept in columns (a NaN where a real is blank). The ID is in the space every property type
// shares (IdSpace::Property), so the PID of a CSPR finds it. The two curves are tables of the dynamic table space.
class SpringOffsetPropTable final : public TypedTable<SpringOffsetPropTable, SpringOffsetPropCard>
{
public:
    static constexpr std::string_view NAME = "SpringOffsetProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = SpringOffsetPropFormat;

    SpringOffsetPropTable()
        : m_stiffness(BLANK_REAL), m_tensileFailureForce(BLANK_REAL), m_compressiveFailureForce(BLANK_REAL)
    {
    }

    [[nodiscard]] static base::EntityId cardId(const SpringOffsetPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const SpringOffsetPropCard& card, base::CardKey key);
    [[nodiscard]] SpringOffsetPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const SpringOffsetPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& stiffnessColumn() noexcept { return m_stiffness; }
    [[nodiscard]] Column<double>& tensileFailureForceColumn() noexcept { return m_tensileFailureForce; }
    [[nodiscard]] Column<double>& compressiveFailureForceColumn() noexcept { return m_compressiveFailureForce; }
    [[nodiscard]] Column<base::EntityId>& loadingTableIdColumn() noexcept { return m_loadingTableId; }
    [[nodiscard]] Column<base::EntityId>& unloadingTableIdColumn() noexcept { return m_unloadingTableId; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_stiffness;
    Column<double> m_tensileFailureForce;
    Column<double> m_compressiveFailureForce;
    Column<base::EntityId> m_loadingTableId;
    Column<base::EntityId> m_unloadingTableId;
};

} // namespace oor::model
