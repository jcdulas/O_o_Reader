#pragma once

#include "oor/model/cards/properties/fastenerProp/FastenerPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct FastenerPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FastenerPropCard& card);
    [[nodiscard]] static std::optional<FastenerPropCard> read(FieldReader& reader);
    static void write(const FastenerPropCard& card, FieldWriter& writer);
};

// All the PFAST cards. The stiffnesses are (N, 3) NumPy views without copy.
class FastenerPropTable final : public TypedTable<FastenerPropTable, FastenerPropCard>
{
public:
    static constexpr std::string_view NAME = "FastenerProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = FastenerPropFormat;

    FastenerPropTable() : m_matFrameId(FastenerPropCard::NO_FRAME) {}

    [[nodiscard]] static base::EntityId cardId(const FastenerPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const FastenerPropCard& card, base::CardKey key);
    [[nodiscard]] FastenerPropCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const FastenerPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& diameterColumn() noexcept { return m_diameter; }
    [[nodiscard]] Column<base::EntityId>& matFrameIdColumn() noexcept { return m_matFrameId; }
    [[nodiscard]] Column<std::uint8_t>& absoluteFrameColumn() noexcept { return m_absoluteFrame; }
    [[nodiscard]] Column<std::array<double, 3>>& translationalStiffnessColumn() noexcept
    {
        return m_translationalStiffness;
    }
    [[nodiscard]] Column<std::array<double, 3>>& rotationalStiffnessColumn() noexcept { return m_rotationalStiffness; }
    [[nodiscard]] Column<double>& massColumn() noexcept { return m_mass; }
    [[nodiscard]] Column<double>& structuralDampingColumn() noexcept { return m_structuralDamping; }
    [[nodiscard]] Column<double>& alphaCoefficientColumn() noexcept { return m_alphaCoefficient; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }
    [[nodiscard]] Column<double>& coincidentLengthColumn() noexcept { return m_coincidentLength; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_diameter;
    Column<base::EntityId> m_matFrameId;
    Column<std::uint8_t> m_absoluteFrame;
    Column<std::array<double, 3>> m_translationalStiffness;
    Column<std::array<double, 3>> m_rotationalStiffness;
    Column<double> m_mass;
    Column<double> m_structuralDamping;
    Column<double> m_alphaCoefficient;
    Column<double> m_referenceTemperature;
    Column<double> m_coincidentLength;
};

} // namespace oor::model
