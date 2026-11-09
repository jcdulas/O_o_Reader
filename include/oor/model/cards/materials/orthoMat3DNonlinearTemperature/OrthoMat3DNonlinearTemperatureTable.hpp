#pragma once

#include "oor/model/cards/materials/orthoMat3DNonlinearTemperature/OrthoMat3DNonlinearTemperatureCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct OrthoMat3DNonlinearTemperatureFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const OrthoMat3DNonlinearTemperatureCard& card);
    [[nodiscard]] static std::optional<OrthoMat3DNonlinearTemperatureCard> read(FieldReader& reader);
    static void write(const OrthoMat3DNonlinearTemperatureCard& card, FieldWriter& writer);
};

// All the MATTORT cards. The ID space is the card type's own: the ID is the one of the MATORT that the card extends, so
// it is also a reference to a material (renumbering the materials follows into it), and the tables are references to
// TABLEMi cards (ReferenceTarget::MaterialTable).
class OrthoMat3DNonlinearTemperatureTable final
    : public TypedTable<OrthoMat3DNonlinearTemperatureTable, OrthoMat3DNonlinearTemperatureCard>
{
public:
    static constexpr std::string_view NAME = "OrthoMat3DNonlinearTemperature";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = OrthoMat3DNonlinearTemperatureFormat;

    [[nodiscard]] static base::EntityId cardId(const OrthoMat3DNonlinearTemperatureCard& card) noexcept
    {
        return card.id;
    }
    std::uint32_t append(const OrthoMat3DNonlinearTemperatureCard& card, base::CardKey key);
    [[nodiscard]] OrthoMat3DNonlinearTemperatureCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const OrthoMat3DNonlinearTemperatureCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& youngsModulus1TableIdColumn() noexcept { return m_youngsModulus1TableId; }
    [[nodiscard]] Column<base::EntityId>& youngsModulus2TableIdColumn() noexcept { return m_youngsModulus2TableId; }
    [[nodiscard]] Column<base::EntityId>& youngsModulus3TableIdColumn() noexcept { return m_youngsModulus3TableId; }
    [[nodiscard]] Column<base::EntityId>& poissonRatio12TableIdColumn() noexcept { return m_poissonRatio12TableId; }
    [[nodiscard]] Column<base::EntityId>& poissonRatio23TableIdColumn() noexcept { return m_poissonRatio23TableId; }
    [[nodiscard]] Column<base::EntityId>& poissonRatio31TableIdColumn() noexcept { return m_poissonRatio31TableId; }
    [[nodiscard]] Column<base::EntityId>& shearModulus12TableIdColumn() noexcept { return m_shearModulus12TableId; }
    [[nodiscard]] Column<base::EntityId>& shearModulus23TableIdColumn() noexcept { return m_shearModulus23TableId; }
    [[nodiscard]] Column<base::EntityId>& shearModulus31TableIdColumn() noexcept { return m_shearModulus31TableId; }
    [[nodiscard]] Column<base::EntityId>& thermalExpansion1TableIdColumn() noexcept
    {
        return m_thermalExpansion1TableId;
    }
    [[nodiscard]] Column<base::EntityId>& thermalExpansion2TableIdColumn() noexcept
    {
        return m_thermalExpansion2TableId;
    }
    [[nodiscard]] Column<base::EntityId>& thermalExpansion3TableIdColumn() noexcept
    {
        return m_thermalExpansion3TableId;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_youngsModulus1TableId;
    Column<base::EntityId> m_youngsModulus2TableId;
    Column<base::EntityId> m_youngsModulus3TableId;
    Column<base::EntityId> m_poissonRatio12TableId;
    Column<base::EntityId> m_poissonRatio23TableId;
    Column<base::EntityId> m_poissonRatio31TableId;
    Column<base::EntityId> m_shearModulus12TableId;
    Column<base::EntityId> m_shearModulus23TableId;
    Column<base::EntityId> m_shearModulus31TableId;
    Column<base::EntityId> m_thermalExpansion1TableId;
    Column<base::EntityId> m_thermalExpansion2TableId;
    Column<base::EntityId> m_thermalExpansion3TableId;
};

} // namespace oor::model
