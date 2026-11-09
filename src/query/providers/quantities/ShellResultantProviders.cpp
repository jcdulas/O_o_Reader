#include "oor/base/threading/ParallelFor.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/providers/quantities/ElementModelFacts.hpp"
#include "oor/query/providers/quantities/ElementQuantities.hpp"
#include "oor/query/providers/quantities/ShearPanelRods.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <tuple>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

const std::vector<std::string>& resultantFields()
{
    static const std::vector<std::string> FIELDS = {"nx", "ny", "nxy", "mx", "my", "mxy", "qx", "qy"};
    return FIELDS;
}

// ---- the force table of the result file
// --------------------------------------------------------------------------------

class StoredShellForceProvider final : public ElementProvider
{
public:
    [[nodiscard]] std::string name() const override { return "element force table of the result file"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ElementRequest& request,
                                  ElementFlatSet& sink) const override
    {
        Outcome outcome;
        std::vector<TableRef> tables;
        for (const std::string& name : context.elementNames(results::ResultKind::ElementForces))
        {
            if (familyOfContainer(name) != ElementFamily::Shell || !request.wantsType(cardOfContainer(name)))
                continue;
            for (const TableRef& ref :
                 request.tableFrame.empty()
                     ? context.elementTables(results::ResultKind::ElementForces, name, true)
                     : context.elementTablesIn(results::ResultKind::ElementForces, name, request.tableFrame))
                tables.push_back(ref);
        }
        const bool anyTable = !tables.empty();
        makeInOrder<std::optional<ElementFlat>>(
            context, tables.size(), [&](std::size_t index) { return indexedRows(tables[index]); },
            [&](std::size_t index) { return flatten(context, tables[index], request); },
            [&](std::size_t, std::optional<ElementFlat>&& flat)
            {
                if (flat)
                    outcome.rows += sink.add(std::move(*flat));
            });
        if (outcome.rows == 0)
            outcome.reason = anyTable
                                 ? "the table has no row of the elements, subcases and steps asked for"
                                 : "the result file has no force table of shells (output of FORCE was not requested)";
        return outcome;
    }

private:
    static std::optional<ElementFlat> flatten(const ResultQueryContext& context, const TableRef& ref,
                                              const ElementRequest& request)
    {
        const results::ResultTableMeta& meta = ref.table->meta();
        const std::shared_ptr<const results::ResultData> decoded = context.decodeRows(ref, request.elementIds);
        if (!decoded)
            return std::nullopt;
        const TableValues values(*decoded);
        std::vector<int> component;
        for (const std::string& field : resultantFields())
            component.push_back(values.component(field));
        if (component[0] < 0)
            return std::nullopt;
        ElementFlat flat;
        const bool stepped = meta.stepKind == results::StepKind::Time ||
                             meta.stepKind == results::StepKind::Frequency ||
                             meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
        // the rows kept, the same in every step: the read kept the elements asked for already
        std::vector<std::size_t> kept;
        kept.reserve(values.rows());
        const bool centreOnly = request.location == "center";
        const bool cornersOnly = request.location == "corners";
        for (std::size_t row = 0; row < values.rows(); ++row)
        {
            const std::int32_t element = decoded->ids[row];
            if (!request.elementIds.has_value() && !request.wantsElement(element))
                continue;
            const std::int32_t node = row < decoded->nodeIds.size() ? decoded->nodeIds[row] : 0;
            if ((centreOnly && node != 0) || (cornersOnly && node == 0))
                continue;
            if (request.propertyIds && !request.wantsProperty(context.elementCard(ref.set, element).pid))
                continue;
            kept.push_back(row);
        }
        const std::size_t total = kept.size() * values.steps();
        static const std::string CENTRE = "center";
        static const std::string CORNER = "corner";
        flat.eid.reserve(total);
        flat.node.reserve(total);
        flat.location.reserve(total);
        for (std::size_t stepIndex = 0; stepIndex < values.steps(); ++stepIndex)
            for (const std::size_t row : kept)
            {
                const std::int32_t node = row < decoded->nodeIds.size() ? decoded->nodeIds[row] : 0;
                flat.eid.push_back(decoded->ids[row]);
                flat.node.push_back(node);
                flat.location.push_back(node == 0 ? CENTRE : CORNER);
            }
        flat.layer.assign(total, 0);
        flat.position.assign(total, NOT_A_NUMBER);
        flat.step.assign(total, NOT_A_NUMBER);
        if (stepped)
            for (std::size_t stepIndex = 0; stepIndex < values.steps(); ++stepIndex)
                std::fill_n(flat.step.begin() + static_cast<std::ptrdiff_t>(stepIndex * kept.size()), kept.size(),
                            decoded->steps[stepIndex]);
        if (!decoded->eigenvalues.empty())
        {
            flat.eigenvalue.resize(total);
            for (std::size_t stepIndex = 0; stepIndex < values.steps(); ++stepIndex)
                std::fill_n(flat.eigenvalue.begin() + static_cast<std::ptrdiff_t>(stepIndex * kept.size()), kept.size(),
                            decoded->eigenvalues[stepIndex]);
        }
        std::vector<std::vector<double>> real(resultantFields().size());
        std::vector<std::vector<double>> imaginary(resultantFields().size());
        for (std::size_t field = 0; field < component.size(); ++field)
        {
            real[field].assign(total, NOT_A_NUMBER);
            if (values.isComplex())
                imaginary[field].assign(total, NOT_A_NUMBER);
            if (component[field] < 0)
                continue;
            const auto column = static_cast<std::size_t>(component[field]);
            std::size_t at = 0;
            for (std::size_t stepIndex = 0; stepIndex < values.steps(); ++stepIndex)
                for (const std::size_t row : kept)
                {
                    real[field][at] = values.real(stepIndex, row, column);
                    if (values.isComplex())
                        imaginary[field][at] = values.imag(stepIndex, row, column);
                    ++at;
                }
        }
        if (flat.eid.empty())
            return std::nullopt;
        flat.set = ref.set;
        flat.analysis = context.sets()[ref.set].analysis;
        flat.status = std::string(results::resultStatusName(meta.status));
        flat.subcase = meta.subcase;
        flat.designCycle = meta.designCycle;
        flat.stepKind = stepped ? meta.stepKind : results::StepKind::None;
        flat.elementName = meta.elementName;
        flat.card = cardOfContainer(meta.elementName);
        flat.frame = context.frameNameOf(ref);
        flat.precision = precisionOf(*decoded);
        flat.magnitudePhase = meta.storedFormat == results::ResultFormat::MagnitudePhase;
        for (std::size_t field = 0; field < component.size(); ++field)
        {
            flat.values[resultantFields()[field]] = std::move(real[field]);
            if (values.isComplex())
                flat.imaginary[resultantFields()[field]] = std::move(imaginary[field]);
        }
        return flat;
    }
};

// ---- the shear flows of the shear panels
// ----------------------------------------------------------------------------------------

// A CSHEAR carries its in-plane shear as four edge shear flows (the force table, format reference CSHEAR Figure 9-66),
// each positive as a positive shear of the element frame of format reference Figure 9-65 (x from G1 to G2, z the normal
// of the diagonals): their mean is the panel's shear flow. Its extensional stiffness is that of rods on its edges
// (PSHEAR F1 along 1-2 and 3-4, F2 along 2-3 and 1-4), whose axial forces the table does not give: they are
// recovered from the translations of the grid points (ShearPanelRods, a companion package). No bending and no
// transverse shear. In the element frame only (a shear panel has no material axes).
//
// Two forms of the rows. The resultants (`shellForces`): `nxy` the mean shear flow, plus the rods' share of the
// in-plane resultants N = sum of P L d d^T / PA (the mean over the panel of the stress its rods carry: the rods of a
// rectangle add to `nx` or `ny` only); `nx`, `ny` zero without rods, null where the panel has rods and the result set
// has no translation of its grid points or the model does not describe them; `mx my mxy qx qy` zero. The panel's
// forces (`shearPanelForces`): `shearFlow` the mean shear flow, `rod12 rod23 rod34 rod41` the axial force of the rod
// of each edge (tension positive, the same at both its ends; zero for an edge without a rod, null where it cannot be
// recovered).
class ShearPanelFlowProvider final : public ElementProvider
{
public:
    explicit ShearPanelFlowProvider(bool panelForces) : m_panelForces(panelForces) {}
    [[nodiscard]] std::string name() const override
    {
        return m_panelForces ? "shear flows of the CSHEAR force table, its rods from the displacements"
                             : "shear flows of the CSHEAR force table";
    }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ElementRequest& request,
                                  ElementFlatSet& sink) const override
    {
        Outcome outcome;
        std::vector<TableRef> tables;
        for (const std::string& name : context.elementNames(results::ResultKind::ElementForces))
        {
            if (familyOfContainer(name) != ElementFamily::Shear || !request.wantsType(cardOfContainer(name)))
                continue;
            for (const TableRef& ref : context.elementTables(results::ResultKind::ElementForces, name, false))
                tables.push_back(ref);
        }
        if (tables.empty())
        {
            outcome.reason = "the result file has no force table of shear panels (CSHEAR)";
            return outcome;
        }
        const std::string& asked = request.tableFrame.empty() ? context.selection().frameName : request.tableFrame;
        if (context.selection().frameSystem != 0 || (!asked.empty() && asked != "element"))
        {
            outcome.reason =
                "the resultants of a shear panel are in its element frame only (it has no material axes), "
                "not in '" +
                (asked.empty() ? "coordinate system " + std::to_string(context.selection().frameSystem) : asked) + "'";
            return outcome;
        }
        if (request.location == "corners")
        {
            outcome.reason = "a shear panel has its resultants at its centre only";
            return outcome;
        }
        Translations translations{context, std::nullopt};
        std::map<std::size_t, std::unique_ptr<ShearPanelRodFacts>> facts;
        for (const TableRef& ref : tables)
        {
            if (!facts.contains(ref.set))
                facts[ref.set] = std::make_unique<ShearPanelRodFacts>(context, ref.set);
            if (auto flat = flatten(context, ref, request, *facts[ref.set], translations, m_panelForces))
                outcome.rows += sink.add(std::move(*flat));
        }
        if (outcome.rows == 0)
            outcome.reason = "the table has no row of the elements, subcases and steps asked for";
        return outcome;
    }

private:
    bool m_panelForces;

    // The displacement and eigenvector tables of the call in the basic system, found once and only when a panel has
    // rods.
    struct Translations
    {
        const ResultQueryContext& context;
        std::optional<std::vector<TableRef>> tables;
        const std::vector<TableRef>& get()
        {
            if (!tables)
            {
                tables.emplace();
                for (const results::ResultKind kind :
                     {results::ResultKind::Displacements, results::ResultKind::Eigenvectors})
                    for (const TableRef& ref : context.tablesIn(kind, "basic"))
                        tables->push_back(ref);
            }
            return *tables;
        }
    };

    static std::optional<ElementFlat> flatten(const ResultQueryContext& context, const TableRef& ref,
                                              const ElementRequest& request, ShearPanelRodFacts& facts,
                                              Translations& translations, bool panelForces)
    {
        const results::ResultTableMeta& meta = ref.table->meta();
        const std::shared_ptr<const results::ResultData> decoded = context.decodeRows(ref, request.elementIds);
        if (!decoded)
            return std::nullopt;
        const TableValues values(*decoded);
        const std::array<int, 4> flows{values.component("shear12"), values.component("shear23"),
                                       values.component("shear34"), values.component("shear41")};
        if (std::any_of(flows.begin(), flows.end(), [](int column) { return column < 0; }))
            return std::nullopt;
        const bool stepped = meta.stepKind == results::StepKind::Time ||
                             meta.stepKind == results::StepKind::Frequency ||
                             meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
        std::vector<std::size_t> kept;
        std::vector<const PanelRods*> panels;
        std::vector<std::int32_t> nodes;
        for (std::size_t row = 0; row < values.rows(); ++row)
        {
            const std::int32_t element = decoded->ids[row];
            if (!request.elementIds.has_value() && !request.wantsElement(element))
                continue;
            if (request.propertyIds && !request.wantsProperty(context.elementCard(ref.set, element).pid))
                continue;
            kept.push_back(row);
            const PanelRods& rods = facts.panel(element);
            panels.push_back(&rods);
            if (rods.ok && rods.any)
                nodes.insert(nodes.end(), rods.nodes.begin(), rods.nodes.end());
        }
        if (kept.empty())
            return std::nullopt;
        std::sort(nodes.begin(), nodes.end());
        nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
        std::optional<PanelTranslations> moved;
        if (!nodes.empty())
            moved.emplace(context, translations.get(), ref, *decoded, nodes);
        const std::size_t total = kept.size() * values.steps();
        ElementFlat flat;
        flat.eid.reserve(total);
        for (std::size_t stepIndex = 0; stepIndex < values.steps(); ++stepIndex)
            for (const std::size_t row : kept)
                flat.eid.push_back(decoded->ids[row]);
        flat.node.assign(total, 0);
        flat.location.assign(total, "center");
        flat.layer.assign(total, 0);
        flat.position.assign(total, NOT_A_NUMBER);
        flat.step.assign(total, NOT_A_NUMBER);
        if (stepped)
            for (std::size_t stepIndex = 0; stepIndex < values.steps(); ++stepIndex)
                std::fill_n(flat.step.begin() + static_cast<std::ptrdiff_t>(stepIndex * kept.size()), kept.size(),
                            decoded->steps[stepIndex]);
        if (!decoded->eigenvalues.empty())
        {
            flat.eigenvalue.resize(total);
            for (std::size_t stepIndex = 0; stepIndex < values.steps(); ++stepIndex)
                std::fill_n(flat.eigenvalue.begin() + static_cast<std::ptrdiff_t>(stepIndex * kept.size()), kept.size(),
                            decoded->eigenvalues[stepIndex]);
        }
        const bool complex = values.isComplex();
        const std::vector<std::string>& fields = panelForces ? shearPanelForceFields() : resultantFields();
        std::vector<std::vector<double>> real(fields.size(), std::vector<double>(total, 0.0));
        std::vector<std::vector<double>> imaginary(fields.size(), std::vector<double>(complex ? total : 0, 0.0));
        constexpr std::array<double, 4> UNKNOWN{NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER, NOT_A_NUMBER};
        std::size_t at = 0;
        for (std::size_t stepIndex = 0; stepIndex < values.steps(); ++stepIndex)
            for (std::size_t index = 0; index < kept.size(); ++index, ++at)
            {
                const std::size_t row = kept[index];
                const PanelRods& rods = *panels[index];
                for (std::size_t part = 0; part < (complex ? 2U : 1U); ++part)
                {
                    const bool imaginaryPart = part == 1;
                    std::vector<std::vector<double>>& out = imaginaryPart ? imaginary : real;
                    double sum = 0.0;
                    for (const int column : flows)
                        sum += imaginaryPart ? values.imag(stepIndex, row, static_cast<std::size_t>(column))
                                             : values.real(stepIndex, row, static_cast<std::size_t>(column));
                    // the rods' forces: zero without rods, null where they cannot be recovered
                    std::array<double, 4> forces{};
                    if (!rods.ok)
                        forces = UNKNOWN;
                    else if (rods.any)
                        forces = moved ? rodForces(rods, *moved, stepIndex, imaginaryPart) : UNKNOWN;
                    if (panelForces)
                    {
                        out[0][at] = 0.25 * sum;
                        for (std::size_t e = 0; e < 4; ++e)
                            out[1 + e][at] = forces[e];
                        continue;
                    }
                    out[2][at] = 0.25 * sum;
                    if (!rods.ok)
                    {
                        // a panel the model does not describe: whether it has rods is unknown
                        out[0][at] = NOT_A_NUMBER;
                        out[1][at] = NOT_A_NUMBER;
                        continue;
                    }
                    // a rod adds only to the resultants its direction has a share of (an unknown force along X
                    // leaves ny and nxy known)
                    for (std::size_t e = 0; e < 4; ++e)
                        for (std::size_t field = 0; field < 3; ++field)
                            if (rods.weights[e][field] != 0.0)
                                out[field][at] += forces[e] * rods.weights[e][field];
                }
            }
        flat.set = ref.set;
        flat.analysis = context.sets()[ref.set].analysis;
        flat.status = std::string(results::resultStatusName(meta.status));
        flat.subcase = meta.subcase;
        flat.designCycle = meta.designCycle;
        flat.stepKind = stepped ? meta.stepKind : results::StepKind::None;
        flat.elementName = meta.elementName;
        flat.card = cardOfContainer(meta.elementName);
        flat.frame = "element";
        flat.precision = precisionOf(*decoded);
        flat.magnitudePhase = meta.storedFormat == results::ResultFormat::MagnitudePhase;
        for (std::size_t field = 0; field < fields.size(); ++field)
        {
            flat.values[fields[field]] = std::move(real[field]);
            if (complex)
                flat.imaginary[fields[field]] = std::move(imaginary[field]);
        }
        return flat;
    }
};

// ---- the integration of the stresses across the thickness
// ---------------------------------------------------------------

// The stress is taken to vary linearly between the two fibres: its mean over the thickness gives the membrane force,
// its first moment the bending moment. The transverse shear forces are not recovered (null). An estimate, marked
// approximate: the stresses are those of two fibres, the thickness is the property's, and nothing is known of a
// laminate or of a through-thickness shear.
class IntegratedStressProvider final : public ElementProvider
{
public:
    [[nodiscard]] std::string name() const override
    {
        return "integrated from the Z1 and Z2 stresses and the thickness";
    }
    [[nodiscard]] bool approximate() const override { return true; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ElementRequest& request,
                                  ElementFlatSet& sink) const override
    {
        Outcome outcome;
        // A resultant is a force per unit length of an edge of the element: it has an in-plane turn between the element
        // and the material systems only, as the stored forces. The
        // stresses turned into a system of space are the whole tensor, and integrating its xx, yy, xy would give no
        // resultant of the shell (a shell out of the xy plane would lose its own components).
        const std::string& asked = request.tableFrame.empty() ? context.selection().frameName : request.tableFrame;
        const bool spaceFrame =
            context.selection().frameSystem != 0 || (!asked.empty() && asked != "element" && asked != "material");
        if (request.tableFrame.empty() && spaceFrame)
        {
            outcome.reason =
                "the resultants of a shell are in its element or material system only (forces per unit "
                "length of an edge), not in '" +
                (asked.empty() ? "coordinate system " + std::to_string(context.selection().frameSystem) : asked) + "'";
            return outcome;
        }
        bool anyTable = false;
        std::map<std::size_t, std::unique_ptr<ElementModelFacts>> facts;
        for (const std::string& name : context.elementNames(results::ResultKind::Stress))
        {
            if (familyOfContainer(name) != ElementFamily::Shell || !request.wantsType(cardOfContainer(name)))
                continue;
            const std::vector<TableRef> refs =
                request.tableFrame.empty()
                    ? context.elementTables(results::ResultKind::Stress, name, true)
                    : context.elementTablesIn(results::ResultKind::Stress, name, request.tableFrame);
            // the same tables as stored (not turned), for their rows
            const std::vector<TableRef> stored = context.elementTables(results::ResultKind::Stress, name, false);
            for (std::size_t index = 0; index < refs.size(); ++index)
            {
                const TableRef& ref = refs[index];
                anyTable = true;
                if (index < stored.size() && coveredBefore(stored[index], ref, request, sink))
                    continue;
                ElementRequest everything = request;
                everything.layers.reset();
                auto stress = flattenStressTable(context, ref, false, everything);
                if (!stress || stress->isComplex())
                    continue;
                if (!facts.contains(ref.set))
                    facts[ref.set] = std::make_unique<ElementModelFacts>(context, ref.set);
                if (auto flat = integrate(*stress, *facts[ref.set]))
                    outcome.rows += sink.add(std::move(*flat));
            }
        }
        if (outcome.rows == 0)
            outcome.reason = anyTable ? "no shell has both fibres (Z1 and Z2) in the table for the rows asked for"
                                      : "the result file has no stress table of shells to integrate";
        return outcome;
    }

private:
    // PERFORMANCE: the stresses are integrated only for the elements no earlier way gave a row of
    // (ElementFlatSet::add drops the others). A table whose every element asked for already has a stored force (the
    // usual case: the force and the stress tables of the same subcase) gives nothing: it is not read. Only a static
    // table already in memory is looked at this way (its rows are known without decoding or turning it); the rows it
    // would give are a subset of those of its elements asked for.
    static bool coveredBefore(const TableRef& stored, const TableRef& ref, const ElementRequest& request,
                              const ElementFlatSet& sink)
    {
        const results::ResultTableMeta& meta = stored.table->meta();
        const results::ResultTableMeta& turned = ref.table->meta();
        const bool stepped = meta.stepKind == results::StepKind::Time ||
                             meta.stepKind == results::StepKind::Frequency ||
                             meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
        if (stepped || stored.set != ref.set || meta.subcase != turned.subcase ||
            meta.designCycle != turned.designCycle || meta.elementName != turned.elementName ||
            !stored.table->isDecoded())
            return false;
        const std::shared_ptr<const results::ResultData> data = stored.table->data();
        std::vector<std::int32_t> asked;
        asked.reserve(data->rowCount);
        for (std::size_t row = 0; row < data->rowCount; ++row)
            if ((row == 0 || data->ids[row] != data->ids[row - 1]) && request.wantsElement(data->ids[row]))
                asked.push_back(data->ids[row]);
        return sink.coversStatic(stored.set, meta.subcase, meta.designCycle, meta.elementName, asked);
    }

    static std::optional<ElementFlat> integrate(const ElementFlat& stress, ElementModelFacts& facts)
    {
        // the two fibres of each place (step, element, corner)
        std::map<std::tuple<double, std::int32_t, std::int32_t>, std::array<std::size_t, 2>> places;
        std::map<std::tuple<double, std::int32_t, std::int32_t>, std::array<bool, 2>> found;
        const auto stepOf = [&](std::size_t row) { return stress.step.empty() ? 0.0 : stress.step[row]; };
        std::vector<std::tuple<double, std::int32_t, std::int32_t>> order;
        for (std::size_t row = 0; row < stress.rows(); ++row)
        {
            if (stress.layer[row] != 1 && stress.layer[row] != 2)
                continue;
            const auto key =
                std::make_tuple(std::isnan(stepOf(row)) ? -1.0e300 : stepOf(row), stress.eid[row], stress.node[row]);
            if (!places.contains(key))
            {
                order.push_back(key);
                found[key] = {false, false};
            }
            places[key][static_cast<std::size_t>(stress.layer[row] - 1)] = row;
            found[key][static_cast<std::size_t>(stress.layer[row] - 1)] = true;
        }
        ElementFlat out;
        std::vector<std::vector<double>> values(resultantFields().size());
        const std::vector<double>& xx = stress.values.at("oxx");
        const std::vector<double>& yy = stress.values.at("oyy");
        const std::vector<double>& xy = stress.values.at("txy");
        const std::vector<double>& fibre = stress.values.at("fiberDistance");
        for (const auto& key : order)
        {
            if (!found[key][0] || !found[key][1])
                continue;
            const std::size_t one = places[key][0];
            const std::size_t two = places[key][1];
            const double z1 = fibre[one];
            const double z2 = fibre[two];
            if (std::isnan(z1) || std::isnan(z2) || z1 == z2)
                continue;
            const ShellFacts& shell = facts.shell(stress.eid[one]);
            const double t = shell.ok ? shell.thickness : std::fabs(z2 - z1);
            const auto integrate = [&](const std::vector<double>& s, double& membrane, double& moment)
            {
                const double slope = (s[two] - s[one]) / (z2 - z1);
                const double middle = s[one] - slope * z1;
                membrane = middle * t;
                moment = -slope * t * t * t / 12.0; // the sign of the format: sigma = n / t - 12 m z / t^3
            };
            double nx, mx, ny, my, nxy, mxy;
            integrate(xx, nx, mx);
            integrate(yy, ny, my);
            integrate(xy, nxy, mxy);
            out.eid.push_back(stress.eid[one]);
            out.node.push_back(stress.node[one]);
            out.layer.push_back(0);
            out.step.push_back(stress.step.empty() ? NOT_A_NUMBER : stress.step[one]);
            if (!stress.eigenvalue.empty())
                out.eigenvalue.push_back(stress.eigenvalue[one]);
            out.position.push_back(NOT_A_NUMBER);
            out.location.push_back(stress.node[one] == 0 ? "center" : "corner");
            const double row[8] = {nx, ny, nxy, mx, my, mxy, NOT_A_NUMBER, NOT_A_NUMBER};
            for (std::size_t field = 0; field < 8; ++field)
                values[field].push_back(row[field]);
        }
        if (out.eid.empty())
            return std::nullopt;
        out.set = stress.set;
        out.analysis = stress.analysis;
        out.subcase = stress.subcase;
        out.designCycle = stress.designCycle;
        out.stepKind = stress.stepKind;
        out.elementName = stress.elementName;
        out.card = stress.card;
        out.frame = stress.frame;
        out.status = stress.status;
        out.precision = stress.precision;
        for (std::size_t field = 0; field < values.size(); ++field)
            out.values[resultantFields()[field]] = std::move(values[field]);
        return out;
    }
};

bool steppedKind(results::StepKind kind) noexcept
{
    return kind == results::StepKind::Time || kind == results::StepKind::Frequency || kind == results::StepKind::Mode ||
           kind == results::StepKind::LoadFactor;
}

// True when the rows of `ids` (by increasing element) have every element of `wanted` (increasing, each once).
template <typename Ids>
bool hasEvery(const Ids& ids, const std::vector<std::int32_t>& wanted)
{
    if (!std::is_sorted(ids.begin(), ids.end()))
        return false;
    auto at = wanted.begin();
    for (std::size_t row = 0; row < ids.size() && at != wanted.end(); ++row)
    {
        if (*at < ids[row])
            return false; // an element asked for that the rows passed
        if (at != wanted.end() && *at == ids[row])
            ++at;
    }
    return at == wanted.end();
}

// The place of each row of a shell force table (DirectForceTable): the centre (code 0) where its grid point is 0,
// else a corner (code 1), as StoredShellForceProvider names them.
void shellLabels(const results::ResultData& decoded, std::vector<std::uint8_t>& location,
                 std::vector<std::int32_t>& node)
{
    const std::size_t rows = decoded.rowCount;
    location.assign(rows, 0);
    node.assign(rows, 0);
    for (std::size_t row = 0; row < rows && row < decoded.nodeIds.size(); ++row)
    {
        node[row] = decoded.nodeIds[row];
        location[row] = node[row] == 0 ? 0 : 1;
    }
}

} // namespace

const std::vector<std::string>& shellPlaceNames()
{
    static const std::vector<std::string> NAMES = {"center", "corner"};
    return NAMES;
}

std::optional<std::vector<DirectForceTable>> shellForceTables(const ResultQueryContext& context,
                                                              const ElementRequest& request, std::string& source)
{
    if (ProviderSwitches::isDisabled(SHELL_FORCE_SWITCH))
        return std::nullopt;
    const QuantityInfo info = shellResultantChain().info();
    for (const ProviderInfo& provider : info.providers)
        if (ProviderSwitches::isDisabled(provider.name))
            return std::nullopt;
    if (request.propertyIds || !request.location.empty() || request.layers || !request.axisName.empty() ||
        request.axisVector || !request.tableFrame.empty())
        return std::nullopt;
    // the stored frame, or a frame the stored tables are turned to as the provider turns them
    const std::string& frame = context.selection().frameName;
    if (context.selection().frameSystem != 0 || (!frame.empty() && frame != "element" && frame != "material"))
        return std::nullopt;
    // a shear panel's rows come from its own provider
    for (const std::string& name : context.elementNames(results::ResultKind::ElementForces))
        if (familyOfContainer(name) == ElementFamily::Shear && request.wantsType(cardOfContainer(name)))
            return std::nullopt;
    source = sourceText(info.providers.front().name, info.providers.front().approximate);
    std::vector<TableRef> tables;
    for (const std::string& name : context.elementNames(results::ResultKind::ElementForces))
    {
        if (familyOfContainer(name) != ElementFamily::Shell || !request.wantsType(cardOfContainer(name)))
            continue;
        for (const TableRef& ref : context.elementTables(results::ResultKind::ElementForces, name, true))
            tables.push_back(ref);
    }
    if (tables.empty())
        return std::nullopt;
    // the elements asked for, sorted, each once
    std::vector<std::int32_t> wanted;
    if (request.elementIds)
    {
        wanted.assign(request.elementIds->begin(), request.elementIds->end());
        if (!std::is_sorted(wanted.begin(), wanted.end()))
            std::sort(wanted.begin(), wanted.end());
        wanted.erase(std::unique(wanted.begin(), wanted.end()), wanted.end());
    }
    std::vector<DirectForceTable> made(tables.size());
    std::atomic<bool> other{false};
    const auto make = [&](std::size_t at)
    {
        const TableRef& ref = tables[at];
        const results::ResultTableMeta& meta = ref.table->meta();
        DirectForceTable& table = made[at];
        // a table in memory with no more rows than twice the elements asked for is taken whole (as stationForceTables)
        const bool whole =
            !request.elementIds || (ref.table->isDecoded() && meta.rowCountKnown && meta.rowCount <= 2 * wanted.size());
        table.decoded = context.decodeRows(ref, whole ? std::nullopt : request.elementIds);
        if (!table.decoded)
            return;
        const TableValues values(*table.decoded);
        if (values.isComplex())
        {
            other.store(true);
            return;
        }
        if (values.component("nx") < 0)
        {
            table.decoded.reset(); // no row, as the provider gives none
            return;
        }
        table.set = ref.set;
        table.analysis = context.sets()[ref.set].analysis;
        table.subcase = meta.subcase;
        table.designCycle = meta.designCycle;
        table.elementName = meta.elementName;
        table.stepped = steppedKind(meta.stepKind);
        table.source[0].fill(-1);
        table.source[1].fill(-1);
        for (std::size_t field = 0; field < resultantFields().size(); ++field)
            table.source[0][field] = values.component(resultantFields()[field]);
    };
    const results::DecodeContext decode = context.decode();
    if (decode.pool != nullptr && tables.size() > 1)
        base::parallelFor(*decode.pool, tables.size(), make, decode.token);
    else
        for (std::size_t at = 0; at < tables.size(); ++at)
            make(at);
    if (other.load())
        return std::nullopt;
    std::vector<DirectForceTable> kept;
    kept.reserve(made.size());
    for (DirectForceTable& table : made)
        if (table.decoded && table.decoded->rowCount > 0 && table.decoded->stepCount > 0)
            kept.push_back(std::move(table));
    if (kept.empty())
        return std::nullopt;
    // every element asked for in each table of a subcase (the tables of one element type mostly have the same
    // elements: looked at once)
    std::vector<std::uint8_t> complete(kept.size(), 0);
    if (request.elementIds)
        for (std::size_t index = 0; index < kept.size(); ++index)
        {
            const auto& ids = kept[index].decoded->ids;
            const auto& before = index > 0 ? kept[index - 1].decoded->ids : ids;
            complete[index] =
                index > 0 && before.size() == ids.size() && std::equal(ids.begin(), ids.end(), before.begin())
                    ? complete[index - 1]
                    : (hasEvery(ids, wanted) ? 1 : 0);
        }
    // a stress table of shells would give the integrated rows of the elements no force table of its subcase has: the
    // providers are asked unless the force tables of that subcase and element type have every element asked for
    for (const std::string& name : context.elementNames(results::ResultKind::Stress))
    {
        if (familyOfContainer(name) != ElementFamily::Shell || !request.wantsType(cardOfContainer(name)))
            continue;
        for (const TableRef& ref : context.elementTables(results::ResultKind::Stress, name, false))
        {
            const results::ResultTableMeta& meta = ref.table->meta();
            if (!request.elementIds || steppedKind(meta.stepKind))
                return std::nullopt;
            bool covered = false;
            for (std::size_t index = 0; !covered && index < kept.size(); ++index)
            {
                const DirectForceTable& table = kept[index];
                covered = complete[index] != 0 && !table.stepped && table.set == ref.set &&
                          table.subcase == meta.subcase && table.designCycle == meta.designCycle &&
                          table.elementName == meta.elementName;
            }
            if (!covered)
                return std::nullopt;
        }
    }
    // the labels of the rows, shared by a table with those of the one before
    for (std::size_t index = 0; index < kept.size(); ++index)
    {
        DirectForceTable& table = kept[index];
        if (index > 0)
        {
            const results::ResultData& one = *kept[index - 1].decoded;
            const results::ResultData& two = *table.decoded;
            if (one.ids == two.ids && one.nodeIds == two.nodeIds)
            {
                table.location = kept[index - 1].location;
                table.node = kept[index - 1].node;
                continue;
            }
        }
        auto location = std::make_shared<std::vector<std::uint8_t>>();
        auto node = std::make_shared<std::vector<std::int32_t>>();
        shellLabels(*table.decoded, *location, *node);
        table.location = std::move(location);
        table.node = std::move(node);
    }
    if (!request.elementIds)
        return kept;
    // every element asked for has a row: else the chain's other ways and onMissing decide
    std::vector<std::uint8_t> present(wanted.size(), 0);
    std::size_t found = 0;
    for (std::size_t index = 0; index < kept.size() && found < wanted.size(); ++index)
    {
        if (complete[index] != 0)
            return kept;
        const auto& ids = kept[index].decoded->ids;
        if (index > 0 && kept[index - 1].decoded->ids == ids)
            continue;
        for (std::size_t row = 0; row < ids.size(); ++row)
        {
            if (row > 0 && ids[row] == ids[row - 1])
                continue;
            const auto at = std::lower_bound(wanted.begin(), wanted.end(), ids[row]);
            if (at == wanted.end() || *at != ids[row])
                continue;
            std::uint8_t& seen = present[static_cast<std::size_t>(at - wanted.begin())];
            if (seen == 0)
            {
                seen = 1;
                ++found;
            }
        }
    }
    if (found != wanted.size())
        return std::nullopt;
    return kept;
}

const std::vector<std::string>& shearPanelForceFields()
{
    static const std::vector<std::string> FIELDS = {"shearFlow", "rod12", "rod23", "rod34", "rod41"};
    return FIELDS;
}

const ElementChain& shearPanelForceChain()
{
    static const ElementChain CHAIN = []
    {
        ElementChain chain("forces of the shear panels");
        chain.add(std::make_unique<ShearPanelFlowProvider>(true));
        return chain;
    }();
    return CHAIN;
}

const ElementChain& shellResultantChain()
{
    static const ElementChain CHAIN = []
    {
        ElementChain chain("shell force resultants");
        chain.add(std::make_unique<StoredShellForceProvider>());
        chain.add(std::make_unique<ShearPanelFlowProvider>(false));
        chain.add(std::make_unique<IntegratedStressProvider>());
        return chain;
    }();
    return CHAIN;
}

} // namespace oor::query
