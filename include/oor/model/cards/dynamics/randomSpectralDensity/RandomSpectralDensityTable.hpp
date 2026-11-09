#pragma once

#include "oor/model/cards/dynamics/randomSpectralDensity/RandomSpectralDensityCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RandomSpectralDensityFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RandomSpectralDensityCard& card);
    [[nodiscard]] static std::optional<RandomSpectralDensityCard> read(FieldReader& reader);
    static void write(const RandomSpectralDensityCard& card, FieldWriter& writer);
};

// All the RANDPS cards. The ID is a set ID shared by the RANDPS and RANDT1 cards (IdSpace::Group, category Random: the
// RANDOM command selects the set). TID is a random table (ReferenceTarget::RandomTable).
class RandomSpectralDensityTable final : public TypedTable<RandomSpectralDensityTable, RandomSpectralDensityCard>
{
public:
    static constexpr std::string_view NAME = "RandomSpectralDensity";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Random, base::IdSpace::Group};
    using Format = RandomSpectralDensityFormat;

    [[nodiscard]] static base::EntityId cardId(const RandomSpectralDensityCard& card) noexcept { return card.id; }
    std::uint32_t append(const RandomSpectralDensityCard& card, base::CardKey key);
    [[nodiscard]] RandomSpectralDensityCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RandomSpectralDensityCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::int32_t>& subcaseJColumn() noexcept { return m_subcaseJ; }
    [[nodiscard]] Column<std::int32_t>& subcaseKColumn() noexcept { return m_subcaseK; }
    [[nodiscard]] Column<double>& realPartColumn() noexcept { return m_realPart; }
    [[nodiscard]] Column<double>& imaginaryPartColumn() noexcept { return m_imaginaryPart; }
    [[nodiscard]] Column<base::EntityId>& tableIdColumn() noexcept { return m_tableId; }

private:
    Column<base::EntityId> m_id;
    Column<std::int32_t> m_subcaseJ;
    Column<std::int32_t> m_subcaseK;
    Column<double> m_realPart;
    Column<double> m_imaginaryPart;
    Column<base::EntityId> m_tableId;
};

} // namespace oor::model
