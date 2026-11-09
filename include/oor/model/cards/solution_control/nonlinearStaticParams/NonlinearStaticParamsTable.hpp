#pragma once

#include "oor/model/cards/solution_control/SolutionFields.hpp"
#include "oor/model/cards/solution_control/nonlinearStaticParams/NonlinearStaticParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct NonlinearStaticParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonlinearStaticParamsCard& card);
    [[nodiscard]] static std::optional<NonlinearStaticParamsCard> read(FieldReader& reader);
    static void write(const NonlinearStaticParamsCard& card, FieldWriter& writer);
};

// All the NLPARM cards. The ID is the one the NLPARM command of the Case Control selects: it is unique among the NLPARM
// cards (IdSpace::PerCardType) and shared in the category NonlinearStatic with the NLPCI card of the same ID. A field
// that is not given is NO_INTEGER in its column, a real (EPSU, EPSP, EPSW) NaN.
class NonlinearStaticParamsTable final : public TypedTable<NonlinearStaticParamsTable, NonlinearStaticParamsCard>
{
public:
    static constexpr std::string_view NAME = "NonlinearStaticParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::NonlinearStatic, base::IdSpace::PerCardType};
    using Format = NonlinearStaticParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const NonlinearStaticParamsCard& card) noexcept { return card.id; }
    std::uint32_t append(const NonlinearStaticParamsCard& card, base::CardKey key);
    [[nodiscard]] NonlinearStaticParamsCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const NonlinearStaticParamsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // an NLPARM refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::int32_t>& incrementCountColumn() noexcept { return m_incrementCount; }
    [[nodiscard]] Column<double>& timeStepColumn() noexcept { return m_timeStep; }
    [[nodiscard]] Column<std::uint8_t>& stiffnessMethodColumn() noexcept { return m_stiffnessMethod; }
    [[nodiscard]] Column<std::int32_t>& stiffnessUpdateStepColumn() noexcept { return m_stiffnessUpdateStep; }
    [[nodiscard]] Column<std::int32_t>& maxIterationsColumn() noexcept { return m_maxIterations; }
    [[nodiscard]] Column<std::string>& convergenceFlagsColumn() noexcept { return m_convergenceFlags; }
    [[nodiscard]] Column<std::string>& intermediateOutputColumn() noexcept { return m_intermediateOutput; }
    [[nodiscard]] Column<double>& epsilonUColumn() noexcept { return m_epsilonU; }
    [[nodiscard]] Column<double>& epsilonPColumn() noexcept { return m_epsilonP; }
    [[nodiscard]] Column<double>& epsilonWColumn() noexcept { return m_epsilonW; }
    [[nodiscard]] Column<std::int32_t>& maxDivergencesColumn() noexcept { return m_maxDivergences; }
    [[nodiscard]] Column<std::int32_t>& maxQuasiNewtonVectorsColumn() noexcept { return m_maxQuasiNewtonVectors; }
    [[nodiscard]] Column<std::int32_t>& maxLineSearchesColumn() noexcept { return m_maxLineSearches; }
    [[nodiscard]] Column<double>& stressSubincrementFractionColumn() noexcept { return m_stressSubincrementFraction; }
    [[nodiscard]] Column<double>& lineSearchToleranceColumn() noexcept { return m_lineSearchTolerance; }
    [[nodiscard]] Column<std::int32_t>& maxBisectionsColumn() noexcept { return m_maxBisections; }
    [[nodiscard]] Column<double>& maxRotationRatioColumn() noexcept { return m_maxRotationRatio; }
    [[nodiscard]] Column<double>& rotationToleranceColumn() noexcept { return m_rotationTolerance; }
    [[nodiscard]] Column<std::int32_t>& minIterationsColumn() noexcept { return m_minIterations; }

private:
    Column<base::EntityId> m_id;
    Column<std::int32_t> m_incrementCount{NO_INTEGER};
    Column<double> m_timeStep;
    Column<std::uint8_t> m_stiffnessMethod;
    Column<std::int32_t> m_stiffnessUpdateStep{NO_INTEGER};
    Column<std::int32_t> m_maxIterations{25};
    Column<std::string> m_convergenceFlags;
    Column<std::string> m_intermediateOutput{"No"};
    Column<double> m_epsilonU{std::numeric_limits<double>::quiet_NaN()};
    Column<double> m_epsilonP{std::numeric_limits<double>::quiet_NaN()};
    Column<double> m_epsilonW{std::numeric_limits<double>::quiet_NaN()};
    Column<std::int32_t> m_maxDivergences{3};
    Column<std::int32_t> m_maxQuasiNewtonVectors{25};
    Column<std::int32_t> m_maxLineSearches{4};
    Column<double> m_stressSubincrementFraction{0.2};
    Column<double> m_lineSearchTolerance{0.5};
    Column<std::int32_t> m_maxBisections{5};
    Column<double> m_maxRotationRatio{20.0};
    Column<double> m_rotationTolerance{20.0};
    Column<std::int32_t> m_minIterations{NO_INTEGER};
};

} // namespace oor::model
