#pragma once

#include "oor/model/cards/materials/bulkViscosityMat/BulkViscosityMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct BulkViscosityMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BulkViscosityMatCard& card);
    [[nodiscard]] static std::optional<BulkViscosityMatCard> read(FieldReader& reader);
    static void write(const BulkViscosityMatCard& card, FieldWriter& writer);
};

// All the MATBV cards. The ID space is the card type's own: the ID is the one of the material that the card extends, so
// it is also a reference to a material (renumbering the materials follows into it).
class BulkViscosityMatTable final : public TypedTable<BulkViscosityMatTable, BulkViscosityMatCard>
{
public:
    static constexpr std::string_view NAME = "BulkViscosityMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = BulkViscosityMatFormat;

    [[nodiscard]] static base::EntityId cardId(const BulkViscosityMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const BulkViscosityMatCard& card, base::CardKey key);
    [[nodiscard]] BulkViscosityMatCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const BulkViscosityMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& bulkViscosityKindColumn() noexcept { return m_bulkViscosityKind; }
    [[nodiscard]] Column<double>& quadraticBulkViscosityColumn() noexcept { return m_quadraticBulkViscosity; }
    [[nodiscard]] Column<double>& linearBulkViscosityColumn() noexcept { return m_linearBulkViscosity; }
    [[nodiscard]] Column<double>& compressionScaleColumn() noexcept { return m_compressionScale; }
    [[nodiscard]] Column<double>& membraneDampingColumn() noexcept { return m_membraneDamping; }
    [[nodiscard]] Column<double>& initialMetricRelaxationColumn() noexcept { return m_initialMetricRelaxation; }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_bulkViscosityKind;
    Column<double> m_quadraticBulkViscosity;
    Column<double> m_linearBulkViscosity;
    Column<double> m_compressionScale;
    Column<double> m_membraneDamping;
    Column<double> m_initialMetricRelaxation;
};

} // namespace oor::model
