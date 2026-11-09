#pragma once

#include "oor/model/cards/dynamics/randomTimeLags/RandomTimeLagsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RandomTimeLagsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RandomTimeLagsCard& card);
    [[nodiscard]] static std::optional<RandomTimeLagsCard> read(FieldReader& reader);
    static void write(const RandomTimeLagsCard& card, FieldWriter& writer);
};

// All the RANDT1 cards. The ID is a set ID shared by the RANDPS and RANDT1 cards (IdSpace::Group, category Random: the
// RANDOM command selects the set).
class RandomTimeLagsTable final : public TypedTable<RandomTimeLagsTable, RandomTimeLagsCard>
{
public:
    static constexpr std::string_view NAME = "RandomTimeLags";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Random, base::IdSpace::Group};
    using Format = RandomTimeLagsFormat;

    [[nodiscard]] static base::EntityId cardId(const RandomTimeLagsCard& card) noexcept { return card.id; }
    std::uint32_t append(const RandomTimeLagsCard& card, base::CardKey key);
    [[nodiscard]] RandomTimeLagsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const RandomTimeLagsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a RANDT1 refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::int32_t>& countColumn() noexcept { return m_count; }
    [[nodiscard]] Column<double>& startTimeColumn() noexcept { return m_startTime; }
    [[nodiscard]] Column<double>& maximumTimeColumn() noexcept { return m_maximumTime; }

private:
    Column<base::EntityId> m_id;
    Column<std::int32_t> m_count{1};
    Column<double> m_startTime;
    Column<double> m_maximumTime;
};

} // namespace oor::model
