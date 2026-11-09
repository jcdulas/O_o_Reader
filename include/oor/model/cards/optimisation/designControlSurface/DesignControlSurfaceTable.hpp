#pragma once

#include "oor/model/cards/optimisation/designControlSurface/DesignControlSurfaceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct DesignControlSurfaceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignControlSurfaceCard& card);
    [[nodiscard]] static std::optional<DesignControlSurfaceCard> read(FieldReader& reader);
    static void write(const DesignControlSurfaceCard& card, FieldWriter& writer);
};

// All the DVPSURF cards. The ID is unique among them (IdSpace::PerCardType, category Other: no card names a DVPSURF).
// The TRIMID is a TRIM or TRIM2 (AeroTrimSet) and the DVID a design variable (DesignVariable); the AELABEL is a name.
class DesignControlSurfaceTable final : public TypedTable<DesignControlSurfaceTable, DesignControlSurfaceCard>
{
public:
    static constexpr std::string_view NAME = "DesignControlSurface";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = DesignControlSurfaceFormat;

    [[nodiscard]] static base::EntityId cardId(const DesignControlSurfaceCard& card) noexcept { return card.id; }
    std::uint32_t append(const DesignControlSurfaceCard& card, base::CardKey key);
    [[nodiscard]] DesignControlSurfaceCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DesignControlSurfaceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::string>& controlSurfaceLabelColumn() noexcept { return m_controlSurfaceLabel; }
    [[nodiscard]] Column<base::EntityId>& trimIdColumn() noexcept { return m_trimId; }
    [[nodiscard]] Column<base::EntityId>& designVarIdColumn() noexcept { return m_designVarId; }
    [[nodiscard]] Column<double>& coefficientColumn() noexcept { return m_coefficient; }

private:
    Column<base::EntityId> m_id;
    Column<std::string> m_controlSurfaceLabel;
    Column<base::EntityId> m_trimId;
    Column<base::EntityId> m_designVarId;
    Column<double> m_coefficient;
};

} // namespace oor::model
