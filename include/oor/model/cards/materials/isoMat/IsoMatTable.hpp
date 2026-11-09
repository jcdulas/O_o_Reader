#pragma once

#include "oor/model/cards/materials/isoMat/IsoMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct IsoMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const IsoMatCard& card);
    [[nodiscard]] static std::optional<IsoMatCard> read(FieldReader& reader);
    static void write(const IsoMatCard& card, FieldWriter& writer);

    // Fills in the blank one of E, G, NU from the other two, as the format says; nullopt when E and G are both blank.
    // The fields left blank are noted in `blankFields`.
    [[nodiscard]] static bool resolve(std::optional<double> youngsModulus, std::optional<double> shearModulus,
                                      std::optional<double> poissonRatio, IsoMatCard& card);
};

class IsoMatTable final : public TypedTable<IsoMatTable, IsoMatCard>
{
public:
    static constexpr std::string_view NAME = "IsoMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = IsoMatFormat;

    [[nodiscard]] static base::EntityId cardId(const IsoMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const IsoMatCard& card, base::CardKey key);
    [[nodiscard]] IsoMatCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const IsoMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] double youngsModulus(std::uint32_t row) const { return m_youngsModulus.get(row); }
    [[nodiscard]] double shearModulus(std::uint32_t row) const { return m_shearModulus.get(row); }
    [[nodiscard]] double poissonRatio(std::uint32_t row) const { return m_poissonRatio.get(row); }
    [[nodiscard]] double density(std::uint32_t row) const { return m_density.get(row); }
    [[nodiscard]] double thermalExpansion(std::uint32_t row) const { return m_thermalExpansion.get(row); }
    [[nodiscard]] double referenceTemperature(std::uint32_t row) const { return m_referenceTemperature.get(row); }
    [[nodiscard]] double structuralDamping(std::uint32_t row) const { return m_structuralDamping.get(row); }
    [[nodiscard]] double tensionAllowable(std::uint32_t row) const { return m_tensionAllowable.get(row); }
    [[nodiscard]] double compressionAllowable(std::uint32_t row) const { return m_compressionAllowable.get(row); }
    [[nodiscard]] double shearAllowable(std::uint32_t row) const { return m_shearAllowable.get(row); }
    [[nodiscard]] base::EntityId stressFrameId(std::uint32_t row) const { return m_stressFrameId.get(row); }
    [[nodiscard]] std::uint8_t blankFields(std::uint32_t row) const { return m_blankFields.get(row); }
    void setYoungsModulus(std::uint32_t row, double value) { m_youngsModulus.set(row, value); }
    void setShearModulus(std::uint32_t row, double value) { m_shearModulus.set(row, value); }
    void setPoissonRatio(std::uint32_t row, double value) { m_poissonRatio.set(row, value); }
    void setDensity(std::uint32_t row, double value) { m_density.set(row, value); }
    void setThermalExpansion(std::uint32_t row, double value) { m_thermalExpansion.set(row, value); }
    void setReferenceTemperature(std::uint32_t row, double value) { m_referenceTemperature.set(row, value); }
    void setStructuralDamping(std::uint32_t row, double value) { m_structuralDamping.set(row, value); }
    void setTensionAllowable(std::uint32_t row, double value) { m_tensionAllowable.set(row, value); }
    void setCompressionAllowable(std::uint32_t row, double value) { m_compressionAllowable.set(row, value); }
    void setShearAllowable(std::uint32_t row, double value) { m_shearAllowable.set(row, value); }
    void setStressFrameId(std::uint32_t row, base::EntityId value) { m_stressFrameId.set(row, value); }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& youngsModulusColumn() noexcept { return m_youngsModulus; }
    [[nodiscard]] Column<double>& shearModulusColumn() noexcept { return m_shearModulus; }
    [[nodiscard]] Column<double>& poissonRatioColumn() noexcept { return m_poissonRatio; }
    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }
    [[nodiscard]] Column<double>& thermalExpansionColumn() noexcept { return m_thermalExpansion; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }
    [[nodiscard]] Column<double>& structuralDampingColumn() noexcept { return m_structuralDamping; }
    [[nodiscard]] Column<double>& tensionAllowableColumn() noexcept { return m_tensionAllowable; }
    [[nodiscard]] Column<double>& compressionAllowableColumn() noexcept { return m_compressionAllowable; }
    [[nodiscard]] Column<double>& shearAllowableColumn() noexcept { return m_shearAllowable; }
    [[nodiscard]] Column<base::EntityId>& stressFrameIdColumn() noexcept { return m_stressFrameId; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_youngsModulus;
    Column<double> m_shearModulus;
    Column<double> m_poissonRatio;
    Column<double> m_density;
    Column<double> m_thermalExpansion;
    Column<double> m_referenceTemperature;
    Column<double> m_structuralDamping;
    Column<double> m_tensionAllowable;
    Column<double> m_compressionAllowable;
    Column<double> m_shearAllowable;
    Column<base::EntityId> m_stressFrameId;
    Column<std::uint8_t> m_blankFields; // IsoMatCard::BLANK_* bits; nothing stored while every card gives E, G and NU
};

} // namespace oor::model
