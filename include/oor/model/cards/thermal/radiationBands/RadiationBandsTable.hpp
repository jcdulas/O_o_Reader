#pragma once

#include "oor/model/cards/thermal/radiationBands/RadiationBandsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RadiationBandsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RadiationBandsCard& card);
    [[nodiscard]] static std::optional<RadiationBandsCard> read(FieldReader& reader);
    static void write(const RadiationBandsCard& card, FieldWriter& writer);
};

// All the RADBND cards (no ID: IdSpace::None, category Other). The format reference allows one in a deck: a
// second one is kept like the first and reported when the deck is read, the first being the one in force. The card
// names no other card. The wavelengths are one list in CSR form.
class RadiationBandsTable final : public TypedTable<RadiationBandsTable, RadiationBandsCard>
{
public:
    static constexpr std::string_view NAME = "RadiationBands";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = RadiationBandsFormat;

    [[nodiscard]] static base::EntityId cardId(const RadiationBandsCard&) noexcept { return 0; }
    std::uint32_t append(const RadiationBandsCard& card, base::CardKey key);
    [[nodiscard]] RadiationBandsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RadiationBandsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}
    // Only one RADBND entry is allowed: a second one is reported.
    void finishRead(Model& model, const ReadContext& context) override;

    [[nodiscard]] Column<base::EntityId>& bandCountColumn() noexcept { return m_bandCount; }
    [[nodiscard]] Column<double>& planckSecondConstantColumn() noexcept { return m_planckSecondConstant; }
    [[nodiscard]] VariableLengthColumn<double>& wavelengthsColumn() noexcept { return m_wavelengths; }

private:
    Column<base::EntityId> m_bandCount;
    Column<double> m_planckSecondConstant;
    VariableLengthColumn<double> m_wavelengths;
};

} // namespace oor::model
