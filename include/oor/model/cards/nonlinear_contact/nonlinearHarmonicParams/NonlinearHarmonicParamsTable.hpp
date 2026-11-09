#pragma once

#include "oor/model/cards/nonlinear_contact/nonlinearHarmonicParams/NonlinearHarmonicParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct NonlinearHarmonicParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonlinearHarmonicParamsCard& card);
    [[nodiscard]] static std::optional<NonlinearHarmonicParamsCard> read(FieldReader& reader);
    static void write(const NonlinearHarmonicParamsCard& card, FieldWriter& writer);
};

// All the NLHARM cards. The ID is unique among them (IdSpace::PerCardType, category NonlinearHarmonic): the NLHARM
// command of Case Control selects it. NLFREQ is a reference to the forcing frequencies (category NonlinearFrequency).
class NonlinearHarmonicParamsTable final : public TypedTable<NonlinearHarmonicParamsTable, NonlinearHarmonicParamsCard>
{
public:
    static constexpr std::string_view NAME = "NonlinearHarmonicParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::NonlinearHarmonic, base::IdSpace::PerCardType};
    using Format = NonlinearHarmonicParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const NonlinearHarmonicParamsCard& card) noexcept { return card.id; }
    std::uint32_t append(const NonlinearHarmonicParamsCard& card, base::CardKey key);
    [[nodiscard]] NonlinearHarmonicParamsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const NonlinearHarmonicParamsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::int32_t>& subharmonicFactorColumn() noexcept { return m_subharmonicFactor; }
    [[nodiscard]] Column<std::int32_t>& harmonicCountColumn() noexcept { return m_harmonicCount; }
    [[nodiscard]] Column<base::EntityId>& freqListIdColumn() noexcept { return m_freqListId; }

private:
    Column<base::EntityId> m_id;
    Column<std::int32_t> m_subharmonicFactor{1};
    Column<std::int32_t> m_harmonicCount;
    Column<base::EntityId> m_freqListId;
};

} // namespace oor::model
