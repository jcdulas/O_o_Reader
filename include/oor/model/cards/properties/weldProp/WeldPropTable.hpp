#pragma once

#include "oor/model/cards/properties/weldProp/WeldPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct WeldPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const WeldPropCard& card);
    [[nodiscard]] static std::optional<WeldPropCard> read(FieldReader& reader);
    static void write(const WeldPropCard& card, FieldWriter& writer);
};

class WeldPropTable final : public TypedTable<WeldPropTable, WeldPropCard>
{
public:
    static constexpr std::string_view NAME = "WeldProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = WeldPropFormat;

    WeldPropTable()
        : m_minLengthRatio(WeldPropCard::DEFAULT_MIN_LENGTH_RATIO),
          m_maxLengthRatio(WeldPropCard::DEFAULT_MAX_LENGTH_RATIO)
    {
    }

    [[nodiscard]] static base::EntityId cardId(const WeldPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const WeldPropCard& card, base::CardKey key);
    [[nodiscard]] WeldPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const WeldPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& matIdColumn() noexcept { return m_matId; }
    [[nodiscard]] Column<double>& diameterColumn() noexcept { return m_diameter; }
    [[nodiscard]] Column<std::uint8_t>& eliminateConstraintsColumn() noexcept { return m_eliminateConstraints; }
    [[nodiscard]] Column<std::uint8_t>& spotColumn() noexcept { return m_spot; }
    [[nodiscard]] Column<double>& minLengthRatioColumn() noexcept { return m_minLengthRatio; }
    [[nodiscard]] Column<double>& maxLengthRatioColumn() noexcept { return m_maxLengthRatio; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_matId;
    Column<double> m_diameter;
    Column<std::uint8_t> m_eliminateConstraints;
    Column<std::uint8_t> m_spot;
    Column<double> m_minLengthRatio;
    Column<double> m_maxLengthRatio;
};

} // namespace oor::model
