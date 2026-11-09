#pragma once

#include "oor/model/cards/aero/aeroUnsteadyParams/AeroUnsteadyParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct AeroUnsteadyParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroUnsteadyParamsCard& card);
    [[nodiscard]] static std::optional<AeroUnsteadyParamsCard> read(FieldReader& reader);
    static void write(const AeroUnsteadyParamsCard& card, FieldWriter& writer);
};

// All the AERO cards. The card has no ID (IdSpace::None, category Aero) and a deck may hold one: `finishRead` reports a
// second one, as it does for the ACMODL and FLSYM cards. Its ACSID is a coordinate system
// (ReferenceTarget::Coordinate). A blank VELOCITY is a NaN in its column.
class AeroUnsteadyParamsTable final : public TypedTable<AeroUnsteadyParamsTable, AeroUnsteadyParamsCard>
{
public:
    static constexpr std::string_view NAME = "AeroUnsteadyParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Aero, base::IdSpace::None};
    using Format = AeroUnsteadyParamsFormat;

    AeroUnsteadyParamsTable();

    [[nodiscard]] static base::EntityId cardId(const AeroUnsteadyParamsCard&) noexcept { return 0; }
    std::uint32_t append(const AeroUnsteadyParamsCard& card, base::CardKey key);
    [[nodiscard]] AeroUnsteadyParamsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroUnsteadyParamsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<base::EntityId>& aeroFrameIdColumn() noexcept { return m_aeroFrameId; }
    [[nodiscard]] Column<double>& velocityColumn() noexcept { return m_velocity; }
    [[nodiscard]] Column<double>& referenceChordColumn() noexcept { return m_referenceChord; }
    [[nodiscard]] Column<double>& referenceDensityColumn() noexcept { return m_referenceDensity; }
    [[nodiscard]] Column<std::int32_t>& symmetricXzColumn() noexcept { return m_symmetricXz; }
    [[nodiscard]] Column<std::int32_t>& symmetricXyColumn() noexcept { return m_symmetricXy; }

private:
    Column<base::EntityId> m_aeroFrameId;
    Column<double> m_velocity;
    Column<double> m_referenceChord;
    Column<double> m_referenceDensity;
    Column<std::int32_t> m_symmetricXz;
    Column<std::int32_t> m_symmetricXy;
};

} // namespace oor::model
