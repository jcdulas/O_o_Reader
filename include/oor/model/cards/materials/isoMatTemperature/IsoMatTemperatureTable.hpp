#pragma once

#include "oor/model/cards/materials/isoMatTemperature/IsoMatTemperatureCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct IsoMatTemperatureFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const IsoMatTemperatureCard& card);
    [[nodiscard]] static std::optional<IsoMatTemperatureCard> read(FieldReader& reader);
    static void write(const IsoMatTemperatureCard& card, FieldWriter& writer);
};

// All the MATT1 cards. The ID space is the card type's own: the ID is the one of the MAT1 that the card extends, so it
// is also a reference to a material (renumbering the materials follows into it), and the tables are references to
// TABLEMi cards (ReferenceTarget::MaterialTable).
class IsoMatTemperatureTable final : public TypedTable<IsoMatTemperatureTable, IsoMatTemperatureCard>
{
public:
    static constexpr std::string_view NAME = "IsoMatTemperature";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = IsoMatTemperatureFormat;

    [[nodiscard]] static base::EntityId cardId(const IsoMatTemperatureCard& card) noexcept { return card.id; }
    std::uint32_t append(const IsoMatTemperatureCard& card, base::CardKey key);
    [[nodiscard]] IsoMatTemperatureCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const IsoMatTemperatureCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& youngsModulusTableIdColumn() noexcept { return m_youngsModulusTableId; }
    [[nodiscard]] Column<base::EntityId>& shearModulusTableIdColumn() noexcept { return m_shearModulusTableId; }
    [[nodiscard]] Column<base::EntityId>& poissonRatioTableIdColumn() noexcept { return m_poissonRatioTableId; }
    [[nodiscard]] Column<base::EntityId>& densityTableIdColumn() noexcept { return m_densityTableId; }
    [[nodiscard]] Column<base::EntityId>& thermalExpansionTableIdColumn() noexcept { return m_thermalExpansionTableId; }
    [[nodiscard]] Column<base::EntityId>& structuralDampingTableIdColumn() noexcept
    {
        return m_structuralDampingTableId;
    }
    [[nodiscard]] Column<base::EntityId>& tensionAllowableTableIdColumn() noexcept { return m_tensionAllowableTableId; }
    [[nodiscard]] Column<base::EntityId>& compressionAllowableTableIdColumn() noexcept
    {
        return m_compressionAllowableTableId;
    }
    [[nodiscard]] Column<base::EntityId>& shearAllowableTableIdColumn() noexcept { return m_shearAllowableTableId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_youngsModulusTableId;
    Column<base::EntityId> m_shearModulusTableId;
    Column<base::EntityId> m_poissonRatioTableId;
    Column<base::EntityId> m_densityTableId;
    Column<base::EntityId> m_thermalExpansionTableId;
    Column<base::EntityId> m_structuralDampingTableId;
    Column<base::EntityId> m_tensionAllowableTableId;
    Column<base::EntityId> m_compressionAllowableTableId;
    Column<base::EntityId> m_shearAllowableTableId;
};

} // namespace oor::model
