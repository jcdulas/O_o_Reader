#pragma once

#include "oor/model/cards/aero/aeroStaticParams/AeroStaticParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

struct AeroStaticParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroStaticParamsCard& card);
    [[nodiscard]] static std::optional<AeroStaticParamsCard> read(FieldReader& reader);
    static void write(const AeroStaticParamsCard& card, FieldWriter& writer);
};

// All the AEROS cards. The card has no ID (IdSpace::None, category Aero) and a deck may hold one: `finishRead` reports
// a second one. ACSID and RCSID are coordinate systems (ReferenceTarget::Coordinate).
class AeroStaticParamsTable final : public TypedTable<AeroStaticParamsTable, AeroStaticParamsCard>
{
public:
    static constexpr std::string_view NAME = "AeroStaticParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Aero, base::IdSpace::None};
    using Format = AeroStaticParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroStaticParamsCard&) noexcept { return 0; }
    std::uint32_t append(const AeroStaticParamsCard& card, base::CardKey key);
    [[nodiscard]] AeroStaticParamsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AeroStaticParamsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<base::EntityId>& aeroFrameIdColumn() noexcept { return m_aeroFrameId; }
    [[nodiscard]] Column<base::EntityId>& referenceFrameIdColumn() noexcept { return m_referenceFrameId; }
    [[nodiscard]] Column<double>& referenceChordColumn() noexcept { return m_referenceChord; }
    [[nodiscard]] Column<double>& referenceSpanColumn() noexcept { return m_referenceSpan; }
    [[nodiscard]] Column<double>& referenceAreaColumn() noexcept { return m_referenceArea; }
    [[nodiscard]] Column<std::int32_t>& symmetricXzColumn() noexcept { return m_symmetricXz; }
    [[nodiscard]] Column<std::int32_t>& symmetricXyColumn() noexcept { return m_symmetricXy; }

private:
    Column<base::EntityId> m_aeroFrameId;
    Column<base::EntityId> m_referenceFrameId;
    Column<double> m_referenceChord;
    Column<double> m_referenceSpan;
    Column<double> m_referenceArea;
    Column<std::int32_t> m_symmetricXz;
    Column<std::int32_t> m_symmetricXy;
};

} // namespace oor::model
