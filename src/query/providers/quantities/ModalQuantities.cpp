#include "oor/query/providers/quantities/ModalQuantities.hpp"

#include "oor/analysis/mass/MassCalculator.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/model/cards/elements/pointMassInertia/PointMassInertiaTable.hpp"
#include "oor/query/named/common/NodePositions.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/results/NamedArrays.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <set>
#include <tuple>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// The analysis codes of the buckling modes and of the complex modes (results::analysisTypeName).
constexpr std::int32_t BUCKLING_MODES_CODE = 8;
constexpr std::int32_t COMPLEX_MODES_CODE = 9;

// True when an eigenvalue table belongs to the analysis a query reads.
bool isOfAnalysis(const results::ResultTableMeta& meta, ModeAnalysis analysis)
{
    if (analysis == ModeAnalysis::Buckling)
        return meta.analysisCode == BUCKLING_MODES_CODE;
    return meta.analysisCode != BUCKLING_MODES_CODE && meta.analysisCode != COMPLEX_MODES_CODE;
}

} // namespace

std::vector<ModeRow> modeRowsOf(const ResultQueryContext& context, ModeAnalysis analysis)
{
    std::vector<ModeRow> rows;
    std::vector<TableRef> tables = context.tables(results::ResultKind::Eigenvalues, false);
    sortTables(tables);
    // the subcase of the eigenvectors of each set, when there is one
    std::map<std::size_t, std::set<std::int32_t>> vectorSubcases;
    for (const TableRef& ref : context.tables(results::ResultKind::Eigenvectors, false))
        vectorSubcases[ref.set].insert(ref.table->meta().subcase);
    // the eigenvalue tables carry no subcase: with as many tables as eigenvector subcases, the k-th table is the k-th
    // subcase
    std::map<std::size_t, std::size_t> tableCount;
    std::map<std::size_t, std::size_t> tablePosition;
    for (const TableRef& ref : tables)
        if (isOfAnalysis(ref.table->meta(), analysis))
            ++tableCount[ref.set];
    for (const TableRef& ref : tables)
    {
        const results::ResultTableMeta& meta = ref.table->meta();
        if (!isOfAnalysis(meta, analysis))
            continue;
        const std::size_t position = tablePosition[ref.set]++;
        const std::shared_ptr<const results::ResultData> decoded = ref.table->data(context.decode());
        const TableValues values(*decoded);
        if (values.isComplex())
            continue;
        const int eigenvalue = values.component("eigenvalue");
        const int radians = values.component("radians");
        const int cycles = values.component("cycles");
        const int generalizedMass = values.component("generalizedMass");
        const int generalizedStiffness = values.component("generalizedStiffness");
        std::int32_t subcase = meta.subcase;
        if (subcase <= 0 && vectorSubcases[ref.set].size() == 1)
            subcase = *vectorSubcases[ref.set].begin();
        else if (subcase <= 0 && vectorSubcases[ref.set].size() == tableCount[ref.set] &&
                 position < vectorSubcases[ref.set].size())
            subcase = *std::next(vectorSubcases[ref.set].begin(), static_cast<std::ptrdiff_t>(position));
        for (std::size_t row = 0; row < values.rows(); ++row)
        {
            ModeRow mode;
            mode.set = ref.set;
            mode.analysis = context.sets()[ref.set].analysis;
            mode.subcase = subcase;
            mode.designCycle = meta.designCycle;
            mode.mode = decoded->ids[row];
            const auto read = [&](int component)
            { return component < 0 ? NOT_A_NUMBER : values.real(0, row, static_cast<std::size_t>(component)); };
            mode.eigenvalue = read(eigenvalue);
            mode.radians = read(radians);
            mode.cycles = read(cycles);
            mode.generalizedMass = read(generalizedMass);
            mode.generalizedStiffness = read(generalizedStiffness);
            mode.precision = decoded->type == results::ValueType::Float32 ? Precision::Single : Precision::Double;
            mode.frequencyWay = "stored cycles";
            if (std::isnan(mode.cycles))
            {
                // the frequency is derived from the eigenvalue: its square root is the circular frequency
                mode.frequencyWay = "derived from the eigenvalue";
                if (!std::isnan(mode.radians))
                    mode.cycles = mode.radians / (2.0 * std::numbers::pi);
                else if (!std::isnan(mode.eigenvalue))
                    mode.cycles = std::sqrt(std::max(mode.eigenvalue, 0.0)) / (2.0 * std::numbers::pi);
            }
            if (std::isnan(mode.radians) && !std::isnan(mode.eigenvalue))
                mode.radians = std::sqrt(std::max(mode.eigenvalue, 0.0));
            rows.push_back(std::move(mode));
        }
    }
    // a file with the eigenvectors and no eigenvalue table (the output of the modes was not requested): the eigenvalue
    // and the cycles each eigenvector table carries
    if (analysis == ModeAnalysis::RealModes)
    {
        std::set<std::pair<std::size_t, std::int32_t>> covered;
        for (const ModeRow& row : rows)
            covered.insert({row.set, row.subcase});
        // a set made in memory with the modal effective mass and no eigenvalue table: its modes and frequencies
        for (const ResultQueryContext::NamedRef& named : context.named("modalEffectiveMass"))
        {
            const results::NamedResultMeta& meta = named.result->meta();
            if (covered.contains({named.set, meta.subcase}))
                continue;
            const std::shared_ptr<const results::NamedResultData> data = named.result->data(context.decode());
            const results::ResultArray* modeArray = data->find("mode");
            const results::ResultArray* frequencyArray = data->find("frequency");
            if (modeArray == nullptr || frequencyArray == nullptr)
                continue;
            const std::vector<std::int32_t> modes = arrayInts(*modeArray);
            const std::vector<double> frequencies = arrayDoubles(*frequencyArray);
            for (std::size_t index = 0; index < modes.size(); ++index)
            {
                ModeRow mode;
                mode.set = named.set;
                mode.analysis = context.sets()[named.set].analysis;
                mode.subcase = meta.subcase;
                mode.designCycle = meta.designCycle;
                mode.mode = modes[index];
                mode.cycles = frequencies[index];
                mode.radians = 2.0 * std::numbers::pi * mode.cycles;
                mode.eigenvalue = mode.radians * mode.radians;
                mode.generalizedMass = NOT_A_NUMBER;
                mode.generalizedStiffness = NOT_A_NUMBER;
                mode.frequencyWay = "stored cycles";
                mode.precision =
                    frequencyArray->type == results::ArrayType::Float32 ? Precision::Single : Precision::Double;
                rows.push_back(std::move(mode));
            }
            covered.insert({named.set, meta.subcase});
        }
        for (const TableRef& ref : context.tables(results::ResultKind::Eigenvectors, false))
        {
            const results::ResultTableMeta& meta = ref.table->meta();
            if (meta.analysisCode != 2 || covered.contains({ref.set, meta.subcase}))
                continue;
            const std::shared_ptr<const results::ResultData> decoded = ref.table->data(context.decode());
            for (std::size_t step = 0; step < decoded->stepCount; ++step)
            {
                ModeRow mode;
                mode.set = ref.set;
                mode.analysis = context.sets()[ref.set].analysis;
                mode.subcase = meta.subcase;
                mode.designCycle = meta.designCycle;
                mode.mode = static_cast<std::int32_t>(std::lround(decoded->steps[step]));
                mode.eigenvalue = step < decoded->eigenvalues.size() ? decoded->eigenvalues[step] : NOT_A_NUMBER;
                mode.radians = std::isnan(mode.eigenvalue) ? NOT_A_NUMBER : std::sqrt(std::max(mode.eigenvalue, 0.0));
                mode.generalizedMass = NOT_A_NUMBER;
                mode.generalizedStiffness = NOT_A_NUMBER;
                const double stored = step < decoded->cycles.size() ? decoded->cycles[step] : NOT_A_NUMBER;
                const double derived =
                    std::isnan(mode.radians) ? NOT_A_NUMBER : mode.radians / (2.0 * std::numbers::pi);
                // some files write the circular frequency where the cycles belong: the stored value is kept when it
                // agrees
                const bool agree =
                    !std::isnan(stored) && (std::isnan(derived) || std::fabs(stored - derived) <= 1.0e-3 * derived);
                mode.cycles = agree || std::isnan(derived) ? stored : derived;
                mode.frequencyWay = agree ? "stored cycles" : "derived from the eigenvalue";
                mode.precision = decoded->type == results::ValueType::Float32 ? Precision::Single : Precision::Double;
                rows.push_back(std::move(mode));
            }
        }
    }
    return rows;
}

void ModalFractionSet::beginProvider(std::int16_t index, const ProviderInfo& info)
{
    m_current = index;
    if (m_providers.size() <= static_cast<std::size_t>(index))
        m_providers.resize(static_cast<std::size_t>(index) + 1);
    m_providers[static_cast<std::size_t>(index)] = info;
}

bool ModalFractionSet::add(ModalFractions&& row)
{
    const auto key = std::make_tuple(row.set, row.subcase, row.designCycle, row.mode);
    if (m_index.contains(key))
        return false;
    row.provider = m_current;
    m_index[key] = m_rows.size();
    m_rows.push_back(std::move(row));
    return true;
}

bool ModalFractionSet::covers(std::size_t set, std::int32_t subcase, std::int32_t designCycle, std::int32_t mode) const
{
    return m_index.contains(std::make_tuple(set, subcase, designCycle, mode));
}

namespace
{

// ---- the stored tables
// ------------------------------------------------------------------------------------------------------

// Effective mass, first way: the modal effective mass the results hold (the named result `modalEffectiveMass`).
class StoredEffectiveMassProvider final : public ModalProvider
{
public:
    [[nodiscard]] std::string name() const override { return "stored modal effective mass"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ModalRequest& request,
                                  ModalFractionSet& sink) const override
    {
        Outcome outcome;
        bool anyTable = false;
        // the modal effective mass a solver gives in memory (the named result `modalEffectiveMass`): a row a mode,
        // the fraction of each direction (else the effective mass over the rigid body mass of the direction)
        for (const ResultQueryContext::NamedRef& named : context.named("modalEffectiveMass"))
        {
            anyTable = true;
            const results::NamedResultMeta& meta = named.result->meta();
            const std::shared_ptr<const results::NamedResultData> data = named.result->data(context.decode());
            const results::ResultArray* modeArray = data->find("mode");
            const results::ResultArray* fractionArray = data->find("fraction");
            const results::ResultArray* effectiveArray = data->find("effectiveMass");
            const results::ResultArray* rigidArray = data->find("rigidBodyMass");
            if (modeArray == nullptr || fractionArray == nullptr)
                continue;
            const std::vector<std::int32_t> modes = arrayInts(*modeArray);
            const std::vector<double> stored = arrayDoubles(*fractionArray);
            const std::vector<double> effective =
                effectiveArray != nullptr ? arrayDoubles(*effectiveArray) : std::vector<double>();
            const std::vector<double> rigid = rigidArray != nullptr ? arrayDoubles(*rigidArray) : std::vector<double>();
            for (std::size_t index = 0; index < modes.size(); ++index)
            {
                const ModeRow* match = nullptr;
                for (const ModeRow& mode : request.modes)
                    if (mode.set == named.set && mode.mode == modes[index] &&
                        (mode.subcase == meta.subcase || mode.subcase <= 0) && mode.designCycle == meta.designCycle)
                        match = &mode;
                if (match == nullptr)
                    continue;
                ModalFractions row;
                row.set = match->set;
                row.subcase = match->subcase;
                row.designCycle = match->designCycle;
                row.mode = match->mode;
                for (std::size_t direction = 0; direction < 6; ++direction)
                {
                    double value = stored[index * 6 + direction];
                    if (std::isnan(value) && effective.size() == stored.size() && rigid.size() == 36 &&
                        rigid[direction * 6 + direction] > 0.0)
                        value = effective[index * 6 + direction] / rigid[direction * 6 + direction];
                    row.fraction[direction] = value;
                }
                if (sink.add(std::move(row)))
                    ++outcome.rows;
            }
        }
        if (outcome.rows == 0)
            outcome.reason = anyTable ? "the modal effective mass has no row for the modes asked for"
                                      : "the results have no modal effective mass (output of MEFFMASS was not "
                                        "requested)";
        return outcome;
    }
};

// ---- computed from the eigenvectors and the lumped masses
// --------------------------------------------------------------------

// Effective mass, second way: from the eigenvectors and the mass matrix of the model when that is made of concentrated
// masses (CONM2 with no offset and no inertia, in the basic system): the participation of a mode in a rigid body motion
// about the origin of the basic system, squared and over the generalized mass, as a fraction of the mass of that
// motion. Not given for a model with other mass (a density, an NSM, an offset): the consistent mass matrix is not
// assembled here.
class LumpedEffectiveMassProvider final : public ModalProvider
{
public:
    [[nodiscard]] std::string name() const override
    {
        return "computed from the eigenvectors and the lumped mass matrix";
    }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ModalRequest& request,
                                  ModalFractionSet& sink) const override
    {
        if (!context.hasModel())
            return {0, "no model is attached to the results (the mass matrix is made from the model)"};
        const model::Model& model = *context.modelPointer();
        const model::PointMassInertiaTable* table = model.findTable<model::PointMassInertiaTable>();
        if (table == nullptr)
            return {0,
                    "the model has no concentrated mass: only a model whose mass is concentrated (CONM2) is handled"};
        std::map<base::EntityId, double> mass;
        double lumped = 0.0;
        for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
        {
            if (!table->rows().isAlive(row))
                continue;
            const model::PointMassInertiaCard card = table->card(row);
            const bool plain =
                (card.frameId == 0 || card.frameId == model::PointMassInertiaCard::BASIC_COORDINATES) &&
                card.offset[0] == 0.0 && card.offset[1] == 0.0 && card.offset[2] == 0.0 &&
                std::all_of(card.inertia.begin(), card.inertia.end(), [](double value) { return value == 0.0; });
            if (card.frameId == model::PointMassInertiaCard::BASIC_COORDINATES || !plain)
                return {0, "the CONM2 " + std::to_string(card.id) +
                               " has an offset, an inertia or a coordinate system: the lumped mass matrix is not made"};
            mass[card.node] += card.mass;
            lumped += card.mass;
        }
        const analysis::MassTotals totals = analysis::MassCalculator::total(
            model, analysis::MassCalculator::allElements(model), analysis::MassOptions{}, context.exec().pool());
        if (std::fabs(totals.sums.mass() - lumped) > 1.0e-9 * std::max(1.0, std::fabs(totals.sums.mass())))
            return {0,
                    "the model has mass that is not concentrated (density, non-structural mass): the mass matrix is "
                    "not lumped"};
        std::vector<TableRef> vectors = context.resultantTables(results::ResultKind::Eigenvectors);
        if (vectors.empty())
            return {0, "the result file has no eigenvector table"};
        const NodePositions positions(model, context.exec().pool());
        Outcome outcome;
        for (const TableRef& ref : vectors)
        {
            const results::ResultTableMeta& meta = ref.table->meta();
            const std::shared_ptr<const results::ResultData> decoded = ref.table->data(context.decode());
            const TableValues values(*decoded);
            const int t1 = values.component("t1");
            if (values.isComplex() || t1 < 0)
                continue;
            // the lumped masses at the rows of the table, with their positions
            std::vector<std::size_t> rows;
            std::vector<double> masses;
            std::vector<base::Vec3> where;
            for (std::size_t row = 0; row < values.rows(); ++row)
            {
                const std::int32_t modelId = context.modelNodeId(ref.set, decoded->ids[row]);
                const auto found = mass.find(modelId);
                if (found == mass.end() || !positions.has(modelId))
                    continue;
                rows.push_back(row);
                masses.push_back(found->second);
                where.push_back(positions.position(modelId));
            }
            if (rows.empty())
                continue;
            double rigid[6] = {0, 0, 0, 0, 0, 0};
            for (std::size_t index = 0; index < rows.size(); ++index)
            {
                const base::Vec3& x = where[index];
                const double m = masses[index];
                rigid[0] += m;
                rigid[1] += m;
                rigid[2] += m;
                rigid[3] += m * (x.y * x.y + x.z * x.z);
                rigid[4] += m * (x.x * x.x + x.z * x.z);
                rigid[5] += m * (x.x * x.x + x.y * x.y);
            }
            for (std::size_t step = 0; step < values.steps(); ++step)
            {
                const auto modeNumber = static_cast<std::int32_t>(std::lround(decoded->steps[step]));
                const ModeRow* match = nullptr;
                for (const ModeRow& mode : request.modes)
                    if (mode.set == ref.set && mode.mode == modeNumber &&
                        (mode.subcase == meta.subcase || mode.subcase <= 0))
                        match = &mode;
                if (match == nullptr || sink.covers(match->set, match->subcase, match->designCycle, match->mode))
                    continue;
                double participation[6] = {0, 0, 0, 0, 0, 0};
                double generalized = 0.0;
                for (std::size_t index = 0; index < rows.size(); ++index)
                {
                    const std::size_t row = rows[index];
                    const double u = values.real(step, row, static_cast<std::size_t>(t1));
                    const double v = values.real(step, row, static_cast<std::size_t>(t1) + 1);
                    const double w = values.real(step, row, static_cast<std::size_t>(t1) + 2);
                    const double m = masses[index];
                    generalized += m * (u * u + v * v + w * w);
                    participation[0] += m * u;
                    participation[1] += m * v;
                    participation[2] += m * w;
                }
                // a rotation about an axis moves a point by e x r: its participation is the mode dotted with that
                participation[3] = participation[4] = participation[5] = 0.0;
                for (std::size_t index = 0; index < rows.size(); ++index)
                {
                    const std::size_t row = rows[index];
                    const base::Vec3 phi{values.real(step, row, static_cast<std::size_t>(t1)),
                                         values.real(step, row, static_cast<std::size_t>(t1) + 1),
                                         values.real(step, row, static_cast<std::size_t>(t1) + 2)};
                    const double m = masses[index];
                    participation[3] += m * base::dot(phi, base::cross(base::Vec3{1, 0, 0}, where[index]));
                    participation[4] += m * base::dot(phi, base::cross(base::Vec3{0, 1, 0}, where[index]));
                    participation[5] += m * base::dot(phi, base::cross(base::Vec3{0, 0, 1}, where[index]));
                }
                ModalFractions row;
                row.set = match->set;
                row.subcase = match->subcase;
                row.designCycle = match->designCycle;
                row.mode = match->mode;
                for (std::size_t direction = 0; direction < 6; ++direction)
                    row.fraction[direction] =
                        generalized > 0.0 && rigid[direction] > 0.0
                            ? participation[direction] * participation[direction] / generalized / rigid[direction]
                            : NOT_A_NUMBER;
                if (sink.add(std::move(row)))
                    ++outcome.rows;
            }
        }
        if (outcome.rows == 0)
            outcome.reason =
                "no eigenvector table of the modes asked for, or none has the concentrated masses of the model";
        return outcome;
    }
};

} // namespace

const ModalChain& effectiveMassChain()
{
    static const ModalChain CHAIN = []
    {
        ModalChain chain("effective mass, participation factors");
        chain.add(std::make_unique<StoredEffectiveMassProvider>());
        chain.add(std::make_unique<LumpedEffectiveMassProvider>());
        return chain;
    }();
    return CHAIN;
}

QuantityInfo modeFrequencyInfo()
{
    QuantityInfo info;
    info.quantity = "frequency of a mode";
    info.providers = {ProviderInfo{"stored cycles", false}, ProviderInfo{"derived from the eigenvalue", false}};
    return info;
}

QuantityInfo massSourceInfo()
{
    QuantityInfo info;
    info.quantity = "mass and centre of gravity";
    info.providers = {ProviderInfo{"model", false}, ProviderInfo{"grid point weight table of the result file", false}};
    return info;
}

} // namespace oor::query
