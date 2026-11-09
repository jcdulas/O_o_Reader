#include "oor/analysis/derived/DerivedQuantities.hpp"
#include "oor/analysis/frames/FrameConventions.hpp"
#include "oor/base/threading/ParallelFor.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/providers/quantities/ElementModelFacts.hpp"
#include "oor/query/providers/quantities/ElementQuantities.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <functional>
#include <map>
#include <memory>
#include <set>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

// The positions of the value fields in a flat (the order of stressFields() and strainFields()).
enum Field : std::size_t
{
    FIBER = 0,
    XX,
    YY,
    ZZ,
    XY,
    YZ,
    ZX,
    P1,
    P2,
    P3,
    VonMises,
    MaxShear,
    AXIAL,
    TORSION,
    FieldCount
};

struct Mapping
{
    Field field;
    const char* component;
};

// What each family's tables call the fields (stress, strain): the components are looked up by name in the table.
std::vector<Mapping> mappingFor(ElementFamily family, bool strain)
{
    const auto name = [&](const char* stress, const char* strainName) { return strain ? strainName : stress; };
    switch (family)
    {
    case ElementFamily::Shell:
        // A shell table turned into a system of space (the basic system, a coordinate system) holds the whole tensor:
        // its zz, yz and zx components are read too (a table in the element or material system has none).
        return {{FIBER, "fiberDistance"}, {XX, name("oxx", "exx")},   {YY, name("oyy", "eyy")},
                {ZZ, name("ozz", "ezz")}, {XY, name("txy", "exy")},   {YZ, name("tyz", "eyz")},
                {ZX, name("txz", "exz")}, {P1, name("omax", "emax")}, {P2, name("omin", "emin")},
                {VonMises, "vonMises"},   {MaxShear, "maxShear"}};
    case ElementFamily::Solid:
        return {{XX, name("oxx", "exx")}, {YY, name("oyy", "eyy")}, {ZZ, name("ozz", "ezz")}, {XY, name("txy", "exy")},
                {YZ, name("tyz", "eyz")}, {ZX, name("txz", "exz")}, {P1, name("o1", "e1")},   {P2, name("o2", "e2")},
                {P3, name("o3", "e3")},   {VonMises, "vonMises"}};
    case ElementFamily::Rod:
        return {{AXIAL, "axial"}, {TORSION, "torsion"}};
    case ElementFamily::Shear:
        return {{MaxShear, "maxShear"}};
    default:
        return {};
    }
}

} // namespace

ElementFamily familyOfContainer(const std::string& elementName) noexcept
{
    static const std::set<std::string> SHELLS = {"ShellQuad4", "ShellTria3",        "ShellQuad8",
                                                 "ShellTria6", "ShellQuad4Drilling", "ShellTria3Drilling"};
    static const std::set<std::string> SOLIDS = {"SolidTetra", "SolidPenta", "SolidHexa", "SolidPyramid"};
    static const std::set<std::string> RODS = {"Rod2", "Rod2InlineSection", "Tube2"};
    if (SHELLS.contains(elementName))
        return ElementFamily::Shell;
    if (SOLIDS.contains(elementName))
        return ElementFamily::Solid;
    if (RODS.contains(elementName))
        return ElementFamily::Rod;
    if (elementName == "ShearPanel4")
        return ElementFamily::Shear;
    if (elementName == "Bar2")
        return ElementFamily::Bar;
    if (elementName == "Beam2")
        return ElementFamily::Beam;
    return ElementFamily::Other;
}

const std::vector<std::string>& stressFields()
{
    static const std::vector<std::string> FIELDS = {"fiberDistance", "oxx",      "oyy",   "ozz",    "txy",
                                                    "tyz",           "tzx",      "o1",    "o2",     "o3",
                                                    "vonMises",      "maxShear", "axial", "torsion"};
    return FIELDS;
}

const std::vector<std::string>& strainFields()
{
    static const std::vector<std::string> FIELDS = {"fiberDistance", "exx",      "eyy",   "ezz",    "exy",
                                                    "eyz",           "ezx",      "e1",    "e2",     "e3",
                                                    "vonMises",      "maxShear", "axial", "torsion"};
    return FIELDS;
}

ElementFlat newFlat(std::size_t rows, bool strain)
{
    ElementFlat flat;
    for (const std::string& field : strain ? strainFields() : stressFields())
        flat.values[field] = std::vector<double>(rows, NOT_A_NUMBER);
    return flat;
}

std::string joinReasons(const std::map<std::string, std::int64_t>& reasons)
{
    std::string text;
    std::size_t shown = 0;
    for (const auto& [reason, count] : reasons)
    {
        if (shown++ == 3)
        {
            text += "; ...";
            break;
        }
        text += (text.empty() ? "" : "; ") + reason + " (" + std::to_string(count) + " elements)";
    }
    return text;
}

std::optional<ElementFlat> flattenStressTable(const ResultQueryContext& context, const TableRef& ref, bool strain,
                                              const ElementRequest& request)
{
    const results::ResultTableMeta& meta = ref.table->meta();
    const ElementFamily family = familyOfContainer(meta.elementName);
    if (family == ElementFamily::Other)
        return std::nullopt;
    const std::string card = cardOfContainer(meta.elementName);
    if (!request.wantsType(card))
        return std::nullopt;
    // the rows of the elements the call names only (every row when it names none)
    const std::shared_ptr<const results::ResultData> decoded = context.decodeRows(ref, request.elementIds);
    if (!decoded)
        return std::nullopt;
    const bool selected = request.elementIds.has_value();
    const TableValues values(*decoded);
    const bool complex = values.isComplex();
    const bool shellOrSolid = family == ElementFamily::Shell || family == ElementFamily::Solid;
    std::set<std::int32_t> propertyOk;
    std::set<std::int32_t> propertyNo;
    const auto wantsProperty = [&](std::int32_t eid)
    {
        if (!request.propertyIds)
            return true;
        if (propertyOk.contains(eid))
            return true;
        if (propertyNo.contains(eid))
            return false;
        const bool wanted = request.wantsProperty(context.elementCard(ref.set, eid).pid);
        (wanted ? propertyOk : propertyNo).insert(eid);
        return wanted;
    };

    // the first and last station of each beam
    std::map<std::int32_t, std::pair<std::int32_t, std::int32_t>> stations;
    if (family == ElementFamily::Beam)
        for (std::size_t row = 0; row < values.rows(); ++row)
        {
            auto [position, inserted] =
                stations.emplace(decoded->ids[row], std::make_pair(decoded->layers[row], decoded->layers[row]));
            if (!inserted)
            {
                position->second.first = std::min(position->second.first, decoded->layers[row]);
                position->second.second = std::max(position->second.second, decoded->layers[row]);
            }
        }

    const std::vector<Mapping> mapping = mappingFor(family, strain);
    std::vector<std::pair<Field, int>> sources;
    for (const Mapping& entry : mapping)
    {
        const int component = values.component(entry.component);
        if (component >= 0)
            sources.emplace_back(entry.field, component);
    }
    const std::string barPrefix = strain ? "e" : "s";
    int barEnds[2][4] = {{-1, -1, -1, -1}, {-1, -1, -1, -1}};
    int beamPoints[4] = {-1, -1, -1, -1};
    const int axialComponent = values.component("axial");
    if (family == ElementFamily::Bar)
        for (int point = 0; point < 4; ++point)
        {
            barEnds[0][point] = values.component(barPrefix + std::to_string(point + 1) + "a");
            barEnds[1][point] = values.component(barPrefix + std::to_string(point + 1) + "b");
        }
    if (family == ElementFamily::Beam)
    {
        const char* const names[4] = {"sxc", "sxd", "sxe", "sxf"};
        for (int point = 0; point < 4; ++point)
            beamPoints[point] = values.component(names[point]);
    }
    if ((family == ElementFamily::Bar && barEnds[0][0] < 0) || (family == ElementFamily::Beam && beamPoints[0] < 0))
        return std::nullopt;
    if (family != ElementFamily::Bar && family != ElementFamily::Beam && sources.empty())
        return std::nullopt;

    std::array<std::vector<double>, FieldCount> real;
    std::array<std::vector<double>, FieldCount> imaginary;
    std::vector<std::int32_t> eid;
    std::vector<std::int32_t> node;
    std::vector<std::int32_t> layer;
    std::vector<double> step;
    std::vector<double> eigenvalue;
    std::vector<double> position;
    std::vector<std::string> location;
    const bool hasEigen = !decoded->eigenvalues.empty();
    const bool stepped = meta.stepKind == results::StepKind::Time || meta.stepKind == results::StepKind::Frequency ||
                         meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
    const auto push = [&](std::size_t stepIndex, std::size_t row, std::int32_t nodeId, std::int32_t layerNumber,
                          const std::string& where, const std::array<double, FieldCount>& re,
                          const std::array<double, FieldCount>& im)
    {
        eid.push_back(decoded->ids[row]);
        node.push_back(nodeId);
        layer.push_back(layerNumber);
        step.push_back(stepped && stepIndex < decoded->steps.size() ? decoded->steps[stepIndex] : NOT_A_NUMBER);
        eigenvalue.push_back(hasEigen ? decoded->eigenvalues[stepIndex] : NOT_A_NUMBER);
        position.push_back(row < decoded->positions.size() ? decoded->positions[row] : NOT_A_NUMBER);
        location.push_back(where);
        for (std::size_t field = 0; field < FieldCount; ++field)
        {
            real[field].push_back(re[field]);
            if (complex)
                imaginary[field].push_back(im[field]);
        }
    };
    // at most a row for each row of the table at each step (a bar has two, its ends)
    const std::size_t expected = values.steps() * values.rows() * (family == ElementFamily::Bar ? 2 : 1);
    for (auto* vector : {&step, &eigenvalue, &position})
        vector->reserve(expected);
    for (auto* vector : {&eid, &node, &layer})
        vector->reserve(expected);
    location.reserve(expected);
    for (std::size_t field = 0; field < FieldCount; ++field)
    {
        real[field].reserve(expected);
        if (complex)
            imaginary[field].reserve(expected);
    }
    for (std::size_t stepIndex = 0; stepIndex < values.steps(); ++stepIndex)
        for (std::size_t row = 0; row < values.rows(); ++row)
        {
            const std::int32_t element = decoded->ids[row];
            if (!selected && !request.wantsElement(element))
                continue;
            const std::int32_t nodeId = shellOrSolid && row < decoded->nodeIds.size() ? decoded->nodeIds[row] : 0;
            if (!request.location.empty())
            {
                if (request.location == "center" &&
                    (!(shellOrSolid || family == ElementFamily::Rod || family == ElementFamily::Shear) || nodeId != 0))
                    continue;
                if (request.location == "corners" && (!shellOrSolid || nodeId == 0))
                    continue;
            }
            const std::int32_t layerNumber = family == ElementFamily::Shell ? decoded->layers[row] : 0;
            if (family == ElementFamily::Shell && !request.wantsLayer(layerNumber))
                continue;
            if (!wantsProperty(element))
                continue;
            std::array<double, FieldCount> re;
            std::array<double, FieldCount> im;
            re.fill(NOT_A_NUMBER);
            im.fill(NOT_A_NUMBER);
            if (family == ElementFamily::Bar)
            {
                for (int end = 0; end < 2; ++end)
                {
                    double largest = -std::numeric_limits<double>::infinity();
                    double smallest = std::numeric_limits<double>::infinity();
                    for (int point = 0; point < 4; ++point)
                    {
                        const double value = values.real(stepIndex, row, static_cast<std::size_t>(barEnds[end][point]));
                        largest = std::max(largest, value);
                        smallest = std::min(smallest, value);
                    }
                    re.fill(NOT_A_NUMBER);
                    re[P1] = largest;
                    re[P2] = smallest;
                    if (axialComponent >= 0)
                        re[AXIAL] = values.real(stepIndex, row, static_cast<std::size_t>(axialComponent));
                    push(stepIndex, row, 0, 0, end == 0 ? "endA" : "endB", re, im);
                }
                continue;
            }
            if (family == ElementFamily::Beam)
            {
                double largest = -std::numeric_limits<double>::infinity();
                double smallest = std::numeric_limits<double>::infinity();
                for (int point = 0; point < 4; ++point)
                {
                    const double value = values.real(stepIndex, row, static_cast<std::size_t>(beamPoints[point]));
                    largest = std::max(largest, value);
                    smallest = std::min(smallest, value);
                }
                re[P1] = largest;
                re[P2] = smallest;
                const auto range = stations.at(element);
                push(stepIndex, row, 0, 0,
                     decoded->layers[row] == range.first ? "endA"
                                                         : (decoded->layers[row] == range.second ? "endB" : "station"),
                     re, im);
                continue;
            }
            for (const auto& [field, component] : sources)
            {
                re[field] = values.real(stepIndex, row, static_cast<std::size_t>(component));
                if (complex)
                    im[field] = values.imag(stepIndex, row, static_cast<std::size_t>(component));
            }
            // the solver stores the principal values of a solid as A, B, C in the order of its eigen solution, which is
            // not sorted (the largest, the smallest, then the middle one). The unified columns are o1 >= o2 >= o3, as
            // the derived quantities give them, so that o1 is the maximum and o3 the minimum.
            if (family == ElementFamily::Solid && !complex && !std::isnan(re[P1]) && !std::isnan(re[P2]) &&
                !std::isnan(re[P3]))
            {
                std::array<double, 3> principal = {re[P1], re[P2], re[P3]};
                std::sort(principal.begin(), principal.end(), std::greater<>());
                re[P1] = principal[0];
                re[P2] = principal[1];
                re[P3] = principal[2];
            }
            const char* where = family == ElementFamily::Rod || family == ElementFamily::Shear
                                    ? "center"
                                    : (nodeId == 0 ? "center" : "corner");
            push(stepIndex, row, nodeId, layerNumber, where, re, im);
        }
    if (eid.empty())
        return std::nullopt;
    ElementFlat flat;
    flat.set = ref.set;
    flat.analysis = context.sets()[ref.set].analysis;
    flat.status = std::string(results::resultStatusName(meta.status));
    flat.subcase = meta.subcase;
    flat.designCycle = meta.designCycle;
    flat.stepKind = stepped ? meta.stepKind : results::StepKind::None;
    flat.elementName = meta.elementName;
    flat.card = card;
    flat.frame = context.frameNameOf(ref);
    flat.precision = precisionOf(*decoded);
    flat.magnitudePhase = meta.storedFormat == results::ResultFormat::MagnitudePhase;
    flat.eid = std::move(eid);
    flat.node = std::move(node);
    flat.layer = std::move(layer);
    flat.step = std::move(step);
    flat.eigenvalue = hasEigen ? std::move(eigenvalue) : std::vector<double>();
    flat.position = std::move(position);
    flat.location = std::move(location);
    const std::vector<std::string>& names = strain ? strainFields() : stressFields();
    for (std::size_t field = 0; field < FieldCount; ++field)
    {
        flat.values[names[field]] = std::move(real[field]);
        if (complex)
            flat.imaginary[names[field]] = std::move(imaginary[field]);
    }
    return flat;
}

std::string ScalarCompletion::note(std::size_t row) const
{
    return noteOf(computed[row], strain);
}

std::string ScalarCompletion::noteOf(std::uint8_t bits, bool strain)
{
    if (bits == 0)
        return std::string();
    std::string text;
    const auto add = [&](const char* what) { text += (text.empty() ? "" : ", ") + std::string(what); };
    if ((bits & VON_MISES) != 0)
        add("vonMises");
    if ((bits & PRINCIPAL) != 0)
        add(strain ? "principal strains" : "principal stresses");
    if ((bits & MAX_SHEAR) != 0)
        add("maxShear");
    return text + " computed from the components";
}

std::uint8_t completeScalarRow(StressRow& row, bool strain, bool solid, bool values)
{
    static_assert(STRESS_FIELD_COUNT == FieldCount);
    const bool needMises = std::isnan(row[VonMises]);
    const bool needPrincipal = std::isnan(row[P1]) || std::isnan(row[P2]) || (solid && std::isnan(row[P3]));
    const bool needShear = std::isnan(row[MaxShear]);
    if (!needMises && !needPrincipal && !needShear)
        return 0;
    if (std::isnan(row[XX]) || std::isnan(row[YY]) || std::isnan(row[XY]) ||
        (solid && (std::isnan(row[ZZ]) || std::isnan(row[YZ]) || std::isnan(row[ZX]))))
        return 0;
    const std::uint8_t computed = (needMises ? ScalarCompletion::VON_MISES : 0) |
                                  (needPrincipal ? ScalarCompletion::PRINCIPAL : 0) |
                                  (needShear ? ScalarCompletion::MAX_SHEAR : 0);
    if (!values)
        return computed;
    // octahedral shear takes the place of von Mises in some solid tables: it is not this quantity, so von Mises is
    // computed for them too (the table never stored it)
    double components[6] = {
        row[XX], row[YY], solid ? row[ZZ] : 0.0, row[XY], solid ? row[YZ] : 0.0, solid ? row[ZX] : 0.0};
    // A shell turned into a system of space has the whole tensor of a plane stress state. Its two principal values in
    // its plane are the roots of p^2 - I1 p + I2 = 0, I1 and I2 the first two invariants (the third principal value,
    // along the normal, is 0); the scalars of the shell tables follow from them, as in its own system.
    if (!solid && (!std::isnan(row[ZZ]) || !std::isnan(row[YZ]) || !std::isnan(row[ZX])))
    {
        const double shearScale = strain ? 0.5 : 1.0; // the tensor shear of an engineering shear strain
        const double sxx = row[XX];
        const double syy = row[YY];
        const double szz = std::isnan(row[ZZ]) ? 0.0 : row[ZZ];
        const double sxy = shearScale * row[XY];
        const double syz = std::isnan(row[YZ]) ? 0.0 : shearScale * row[YZ];
        const double szx = std::isnan(row[ZX]) ? 0.0 : shearScale * row[ZX];
        const double firstInvariant = sxx + syy + szz;
        const double secondInvariant = sxx * syy + syy * szz + szz * sxx - sxy * sxy - syz * syz - szx * szx;
        const double centre = 0.5 * firstInvariant;
        const double radius = std::sqrt(std::max(0.0, centre * centre - secondInvariant));
        components[0] = centre + radius;
        components[1] = centre - radius;
        components[3] = 0.0;
    }
    const analysis::TensorScalars scalars = analysis::DerivedQuantities::scalarsOf(strain, solid, components);
    if (needMises)
        row[VonMises] = scalars.vonMises;
    if (needPrincipal)
    {
        row[P1] = scalars.first;
        row[P2] = scalars.second;
        if (solid)
            row[P3] = scalars.third;
    }
    if (needShear)
        row[MaxShear] = scalars.maxShear;
    return computed;
}

ScalarCompletion completeScalars(ElementFlat& flat, bool strain)
{
    ScalarCompletion completion;
    completion.strain = strain;
    completion.computed.assign(flat.rows(), 0);
    const ElementFamily family = familyOfContainer(flat.elementName);
    if ((family != ElementFamily::Shell && family != ElementFamily::Solid) || flat.isComplex())
        return completion;
    const bool solid = family == ElementFamily::Solid;
    const std::vector<std::string>& names = strain ? strainFields() : stressFields();
    std::array<std::vector<double>*, FieldCount> fields{};
    for (std::size_t field = 0; field < FieldCount; ++field)
        fields[field] = &flat.values[names[field]];
    for (std::size_t row = 0; row < flat.rows(); ++row)
    {
        StressRow values;
        for (std::size_t field = 0; field < FieldCount; ++field)
            values[field] = (*fields[field])[row];
        const std::uint8_t computed = completeScalarRow(values, strain, solid, true);
        if (computed == 0)
            continue;
        if ((computed & ScalarCompletion::VON_MISES) != 0)
        {
            (*fields[VonMises])[row] = values[VonMises];
            ++completion.vonMises;
        }
        if ((computed & ScalarCompletion::PRINCIPAL) != 0)
        {
            (*fields[P1])[row] = values[P1];
            (*fields[P2])[row] = values[P2];
            if (solid)
                (*fields[P3])[row] = values[P3];
            ++completion.principal;
        }
        if ((computed & ScalarCompletion::MAX_SHEAR) != 0)
        {
            (*fields[MaxShear])[row] = values[MaxShear];
            ++completion.maxShear;
        }
        completion.computed[row] = computed;
    }
    return completion;
}

namespace
{

// Which fields of a flat of this family make the tensor of the constitutive law.
bool isTensorFamily(ElementFamily family) noexcept
{
    return family == ElementFamily::Shell || family == ElementFamily::Solid || family == ElementFamily::Rod;
}

// The tables of a kind and container that rotate to the frame asked for (only the families that have a frame change).
std::vector<TableRef> tablesFor(const ResultQueryContext& context, results::ResultKind kind, const std::string& name)
{
    const ElementFamily family = familyOfContainer(name);
    return context.elementTables(kind, name, family == ElementFamily::Shell || family == ElementFamily::Solid);
}

// ---- stress and strain from their own tables
// ----------------------------------------------------------------------------

class StoredTableProvider : public ElementProvider
{
public:
    StoredTableProvider(bool strain, std::string label) : m_strain(strain), m_label(std::move(label)) {}
    [[nodiscard]] std::string name() const override { return m_label; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ElementRequest& request,
                                  ElementFlatSet& sink) const override
    {
        const results::ResultKind kind = m_strain ? results::ResultKind::Strain : results::ResultKind::Stress;
        Outcome outcome;
        std::vector<TableRef> tables;
        for (const std::string& name : context.elementNames(kind))
        {
            if (familyOfContainer(name) == ElementFamily::Other)
                continue;
            for (const TableRef& ref : tablesFor(context, kind, name))
                tables.push_back(ref);
        }
        const bool anyTable = !tables.empty();
        makeInOrder<std::optional<ElementFlat>>(
            context, tables.size(), [&](std::size_t index) { return indexedRows(tables[index]); },
            [&](std::size_t index) { return flattenStressTable(context, tables[index], m_strain, request); },
            [&](std::size_t, std::optional<ElementFlat>&& flat)
            {
                if (flat)
                    outcome.rows += sink.add(std::move(*flat));
            });
        if (outcome.rows == 0)
            outcome.reason = anyTable
                                 ? std::string("the table has no row of the elements, subcases and steps asked for")
                                 : std::string("the result file has no ") + (m_strain ? "strain" : "stress") +
                                       " table for these elements (the output was not requested)";
        return outcome;
    }

private:
    bool m_strain;
    std::string m_label;
};

// ---- the linear material: strain to stress and back
// -----------------------------------------------------------------------

class ConstitutiveProvider : public ElementProvider
{
public:
    // `toStress`: the strain tables and the material give the stress; else the stress tables give the strain.
    explicit ConstitutiveProvider(bool toStress, std::string label) : m_toStress(toStress), m_label(std::move(label)) {}
    [[nodiscard]] std::string name() const override { return m_label; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ElementRequest& request,
                                  ElementFlatSet& sink) const override
    {
        if (!context.hasModel())
            return {0, "no model is attached to the results (the material is in the model)"};
        const results::ResultKind source = m_toStress ? results::ResultKind::Strain : results::ResultKind::Stress;
        Outcome outcome;
        std::map<std::string, std::int64_t> reasons;
        bool anyTable = false;
        std::map<std::size_t, std::unique_ptr<ElementModelFacts>> facts;
        for (const std::string& name : context.elementNames(source))
        {
            const ElementFamily family = familyOfContainer(name);
            if (!isTensorFamily(family))
                continue;
            for (const TableRef& ref : tablesFor(context, source, name))
            {
                anyTable = true;
                auto flat = flattenStressTable(context, ref, m_toStress, request);
                if (!flat || flat->isComplex())
                    continue;
                if (!facts.contains(ref.set))
                    facts[ref.set] = std::make_unique<ElementModelFacts>(context, ref.set);
                ElementFlat out = newFlat(flat->rows(), !m_toStress);
                std::vector<std::size_t> keep;
                convert(*flat, out, family, *facts[ref.set], keep, reasons);
                if (keep.empty())
                    continue;
                if (keep.size() != out.rows())
                    out.keepRows(keep);
                outcome.rows += sink.add(std::move(out));
            }
        }
        if (outcome.rows == 0)
        {
            if (!anyTable)
                outcome.reason = std::string("the result file has no ") + (m_toStress ? "strain" : "stress") +
                                 " table to start from";
            else if (!reasons.empty())
                outcome.reason = joinReasons(reasons);
            else
                outcome.reason = "no row of the elements, subcases and steps asked for (or only complex values)";
        }
        return outcome;
    }

private:
    void convert(const ElementFlat& in, ElementFlat& out, ElementFamily family, ElementModelFacts& facts,
                 std::vector<std::size_t>& keep, std::map<std::string, std::int64_t>& reasons) const
    {
        // the same rows, the same keys
        out.set = in.set;
        out.analysis = in.analysis;
        out.subcase = in.subcase;
        out.designCycle = in.designCycle;
        out.stepKind = in.stepKind;
        out.elementName = in.elementName;
        out.card = in.card;
        out.frame = in.frame;
        out.status = in.status;
        out.precision = in.precision;
        out.eid = in.eid;
        out.node = in.node;
        out.layer = in.layer;
        out.step = in.step;
        out.eigenvalue = in.eigenvalue;
        out.position = in.position;
        out.location = in.location;
        const std::vector<std::string>& from = m_toStress ? strainFields() : stressFields();
        const std::vector<std::string>& to = m_toStress ? stressFields() : strainFields();
        const auto input = [&](Field field, std::size_t row) { return in.values.at(from[field])[row]; };
        std::set<std::int32_t> told;
        for (std::size_t row = 0; row < in.rows(); ++row)
        {
            const auto fail = [&](const std::string& why)
            {
                if (told.insert(in.eid[row]).second)
                    ++reasons[why];
            };
            const auto put = [&](Field field, double value) { out.values[to[field]][row] = value; };
            if (family == ElementFamily::Shell)
            {
                const ShellFacts& shell = facts.shell(in.eid[row]);
                if (!shell.ok)
                {
                    fail(shell.why);
                    continue;
                }
                put(FIBER, input(FIBER, row));
                if (m_toStress)
                {
                    const double factor = shell.e / (1.0 - shell.nu * shell.nu);
                    const double ex = input(XX, row);
                    const double ey = input(YY, row);
                    put(XX, factor * (ex + shell.nu * ey));
                    put(YY, factor * (ey + shell.nu * ex));
                    put(XY, shell.g * input(XY, row));
                }
                else
                {
                    const double sx = input(XX, row);
                    const double sy = input(YY, row);
                    put(XX, (sx - shell.nu * sy) / shell.e);
                    put(YY, (sy - shell.nu * sx) / shell.e);
                    put(XY, input(XY, row) / shell.g);
                }
            }
            else if (family == ElementFamily::Solid)
            {
                const SolidFacts& solid = facts.solid(in.eid[row]);
                if (!solid.ok)
                {
                    fail(solid.why);
                    continue;
                }
                const double a = input(XX, row);
                const double b = input(YY, row);
                const double c = input(ZZ, row);
                if (m_toStress)
                {
                    const double lambda = solid.e * solid.nu / ((1.0 + solid.nu) * (1.0 - 2.0 * solid.nu));
                    const double trace = a + b + c;
                    put(XX, lambda * trace + 2.0 * solid.g * a);
                    put(YY, lambda * trace + 2.0 * solid.g * b);
                    put(ZZ, lambda * trace + 2.0 * solid.g * c);
                    put(XY, solid.g * input(XY, row));
                    put(YZ, solid.g * input(YZ, row));
                    put(ZX, solid.g * input(ZX, row));
                }
                else
                {
                    put(XX, (a - solid.nu * (b + c)) / solid.e);
                    put(YY, (b - solid.nu * (a + c)) / solid.e);
                    put(ZZ, (c - solid.nu * (a + b)) / solid.e);
                    put(XY, input(XY, row) / solid.g);
                    put(YZ, input(YZ, row) / solid.g);
                    put(ZX, input(ZX, row) / solid.g);
                }
            }
            else
            {
                const RodFacts& rod = facts.rod(in.eid[row]);
                if (!rod.ok)
                {
                    fail(rod.why);
                    continue;
                }
                put(AXIAL, m_toStress ? rod.e * input(AXIAL, row) : input(AXIAL, row) / rod.e);
                put(TORSION, m_toStress ? rod.g * input(TORSION, row) : input(TORSION, row) / rod.g);
            }
            keep.push_back(row);
        }
    }

    bool m_toStress;
    std::string m_label;
};

// ---- the stress from the element forces and the property
// ------------------------------------------------------------------

class StressFromForcesProvider final : public ElementProvider
{
public:
    [[nodiscard]] std::string name() const override { return "element forces and property"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ElementRequest& request,
                                  ElementFlatSet& sink) const override
    {
        if (!context.hasModel())
            return {0, "no model is attached to the results (the thickness and the area are in the model)"};
        Outcome outcome;
        std::map<std::string, std::int64_t> reasons;
        bool anyTable = false;
        std::map<std::size_t, std::unique_ptr<ElementModelFacts>> facts;
        for (const std::string& name : context.elementNames(results::ResultKind::ElementForces))
        {
            const ElementFamily family = familyOfContainer(name);
            if (family != ElementFamily::Shell && family != ElementFamily::Rod)
                continue;
            if (!request.wantsType(cardOfContainer(name)))
                continue;
            for (const TableRef& ref : context.elementTables(results::ResultKind::ElementForces, name, false))
            {
                anyTable = true;
                if (!facts.contains(ref.set))
                    facts[ref.set] = std::make_unique<ElementModelFacts>(context, ref.set);
                if (auto flat = fromTable(context, ref, family, request, *facts[ref.set], reasons))
                    outcome.rows += sink.add(std::move(*flat));
            }
        }
        if (outcome.rows == 0)
        {
            if (!anyTable)
                outcome.reason = "the result file has no element force table of the rods or shells asked for";
            else if (!reasons.empty())
                outcome.reason = joinReasons(reasons);
            else
                outcome.reason = "no row of the elements, subcases and steps asked for (or only complex values)";
        }
        return outcome;
    }

private:
    static std::optional<ElementFlat> fromTable(const ResultQueryContext& context, const TableRef& ref,
                                                ElementFamily family, const ElementRequest& request,
                                                ElementModelFacts& facts, std::map<std::string, std::int64_t>& reasons)
    {
        const results::ResultTableMeta& meta = ref.table->meta();
        const std::shared_ptr<const results::ResultData> decoded = context.decodeRows(ref, request.elementIds);
        if (!decoded)
            return std::nullopt;
        const TableValues values(*decoded);
        if (values.isComplex())
            return std::nullopt;
        const bool shell = family == ElementFamily::Shell;
        const int nx = values.component(shell ? "nx" : "axial");
        const int ny = values.component(shell ? "ny" : "torsion");
        const int nxy = values.component("nxy");
        const int mx = values.component("mx");
        const int my = values.component("my");
        const int mxy = values.component("mxy");
        if (nx < 0 || ny < 0 || (shell && (nxy < 0 || mx < 0 || my < 0 || mxy < 0)))
            return std::nullopt;
        ElementFlat flat = newFlat(0, false);
        std::vector<std::int32_t> eid;
        std::vector<std::int32_t> node;
        std::vector<std::int32_t> layer;
        std::vector<double> step;
        std::vector<double> eigenvalue;
        std::vector<std::string> location;
        std::array<std::vector<double>, FieldCount> out;
        const bool stepped = meta.stepKind == results::StepKind::Time ||
                             meta.stepKind == results::StepKind::Frequency ||
                             meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
        std::set<std::int32_t> told;
        for (std::size_t stepIndex = 0; stepIndex < values.steps(); ++stepIndex)
            for (std::size_t row = 0; row < values.rows(); ++row)
            {
                const std::int32_t element = decoded->ids[row];
                if (!request.wantsElement(element))
                    continue;
                const std::int32_t nodeId = shell && row < decoded->nodeIds.size() ? decoded->nodeIds[row] : 0;
                if (!request.location.empty())
                {
                    if (request.location == "center" && nodeId != 0)
                        continue;
                    if (request.location == "corners" && (nodeId == 0 || !shell))
                        continue;
                }
                if (request.propertyIds && !request.wantsProperty(context.elementCard(ref.set, element).pid))
                    continue;
                const auto push = [&](std::int32_t layerNumber, const std::array<double, FieldCount>& re)
                {
                    eid.push_back(element);
                    node.push_back(nodeId);
                    layer.push_back(layerNumber);
                    step.push_back(stepped ? decoded->steps[stepIndex] : NOT_A_NUMBER);
                    eigenvalue.push_back(decoded->eigenvalues.empty() ? NOT_A_NUMBER : decoded->eigenvalues[stepIndex]);
                    location.push_back(shell && nodeId != 0 ? "corner" : "center");
                    for (std::size_t field = 0; field < FieldCount; ++field)
                        out[field].push_back(re[field]);
                };
                std::array<double, FieldCount> re;
                re.fill(NOT_A_NUMBER);
                if (shell)
                {
                    const ShellFacts& shellFacts = facts.shell(element);
                    if (!shellFacts.ok)
                    {
                        if (told.insert(element).second)
                            ++reasons[shellFacts.why];
                        continue;
                    }
                    const double t = shellFacts.thickness;
                    for (int fibre = 0; fibre < 2; ++fibre)
                    {
                        if (!request.wantsLayer(fibre + 1))
                            continue;
                        const double z = fibre == 0 ? shellFacts.z1 : shellFacts.z2;
                        const auto stressAt = [&](int membrane, int moment)
                        {
                            // the sign of the format: a positive moment puts the fibre at negative z in tension
                            return values.real(stepIndex, row, static_cast<std::size_t>(membrane)) / t -
                                   12.0 * values.real(stepIndex, row, static_cast<std::size_t>(moment)) * z /
                                       (t * t * t);
                        };
                        re.fill(NOT_A_NUMBER);
                        re[FIBER] = z;
                        re[XX] = stressAt(nx, mx);
                        re[YY] = stressAt(ny, my);
                        re[XY] = stressAt(nxy, mxy);
                        push(fibre + 1, re);
                    }
                }
                else
                {
                    const RodFacts& rod = facts.rod(element);
                    if (!rod.ok)
                    {
                        if (told.insert(element).second)
                            ++reasons[rod.why];
                        continue;
                    }
                    re[AXIAL] = values.real(stepIndex, row, static_cast<std::size_t>(nx)) / rod.area;
                    if (rod.torsion > 0.0 && rod.coefficient > 0.0)
                        re[TORSION] =
                            values.real(stepIndex, row, static_cast<std::size_t>(ny)) * rod.coefficient / rod.torsion;
                    push(0, re);
                }
            }
        if (eid.empty())
            return std::nullopt;
        flat.set = ref.set;
        flat.analysis = context.sets()[ref.set].analysis;
        flat.status = std::string(results::resultStatusName(meta.status));
        flat.subcase = meta.subcase;
        flat.designCycle = meta.designCycle;
        flat.stepKind = stepped ? meta.stepKind : results::StepKind::None;
        flat.elementName = meta.elementName;
        flat.card = cardOfContainer(meta.elementName);
        flat.frame = shell ? "element" : "element";
        flat.precision = precisionOf(*decoded);
        flat.eid = std::move(eid);
        flat.node = std::move(node);
        flat.layer = std::move(layer);
        flat.step = std::move(step);
        flat.eigenvalue = decoded->eigenvalues.empty() ? std::vector<double>() : std::move(eigenvalue);
        flat.position = std::vector<double>(flat.eid.size(), NOT_A_NUMBER);
        flat.location = std::move(location);
        for (std::size_t field = 0; field < FieldCount; ++field)
            flat.values[stressFields()[field]] = std::move(out[field]);
        return flat;
    }
};

} // namespace

const ElementChain& stressChain()
{
    static const ElementChain CHAIN = []
    {
        ElementChain chain("stress components");
        chain.add(std::make_unique<StoredTableProvider>(false, "stress table of the result file"));
        chain.add(std::make_unique<ConstitutiveProvider>(true, "strain table and linear material"));
        chain.add(std::make_unique<StressFromForcesProvider>());
        return chain;
    }();
    return CHAIN;
}

const ElementChain& strainChain()
{
    static const ElementChain CHAIN = []
    {
        ElementChain chain("strain components");
        chain.add(std::make_unique<StoredTableProvider>(true, "strain table of the result file"));
        chain.add(std::make_unique<ConstitutiveProvider>(false, "stress table and linear material"));
        return chain;
    }();
    return CHAIN;
}

namespace
{

bool steppedKind(results::StepKind kind) noexcept
{
    return kind == results::StepKind::Time || kind == results::StepKind::Frequency || kind == results::StepKind::Mode ||
           kind == results::StepKind::LoadFactor;
}

// True when the rows of `ids` (by increasing element) have every element of `wanted` (increasing, each once).
template <typename Ids>
bool hasEvery(const Ids& ids, const std::vector<std::int32_t>& wanted)
{
    if (wanted.empty())
        return true;
    if (!std::is_sorted(ids.begin(), ids.end()))
        return false;
    auto at = wanted.begin();
    for (std::size_t row = 0; row < ids.size() && at != wanted.end(); ++row)
    {
        if (*at < ids[row])
            return false; // an element asked for that the rows passed
        if (*at == ids[row])
            ++at;
    }
    return at == wanted.end();
}

} // namespace

std::optional<std::vector<DirectForceTable>> storedStressTables(const ResultQueryContext& context,
                                                                const ElementRequest& request, bool strain,
                                                                std::string& source)
{
    if (ProviderSwitches::isDisabled(STRESS_TABLE_SWITCH))
        return std::nullopt;
    const QuantityInfo info = (strain ? strainChain() : stressChain()).info();
    for (const ProviderInfo& provider : info.providers)
        if (ProviderSwitches::isDisabled(provider.name))
            return std::nullopt;
    if (request.propertyIds || !request.location.empty() || request.layers)
        return std::nullopt;
    const results::ResultKind kind = strain ? results::ResultKind::Strain : results::ResultKind::Stress;
    source = sourceText(info.providers.front().name, info.providers.front().approximate);
    // the tables StoredTableProvider reads, in its order (those of a type not asked for give no row)
    std::vector<std::pair<TableRef, ElementFamily>> tables;
    for (const std::string& name : context.elementNames(kind))
    {
        const ElementFamily family = familyOfContainer(name);
        if (family == ElementFamily::Other || !request.wantsType(cardOfContainer(name)))
            continue;
        // the rows of bars and beams are made of their points: the providers' (unless they give none)
        if ((family == ElementFamily::Bar || family == ElementFamily::Beam) && !request.elementIds)
            return std::nullopt;
        for (const TableRef& ref : tablesFor(context, kind, name))
            tables.emplace_back(ref, family);
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
        const auto& [ref, family] = tables[at];
        const results::ResultTableMeta& meta = ref.table->meta();
        if (family == ElementFamily::Bar || family == ElementFamily::Beam)
        {
            const auto rows = context.decodeRows(ref, request.elementIds);
            if (rows && rows->rowCount > 0 && rows->stepCount > 0)
                other.store(true);
            return;
        }
        DirectForceTable& table = made[at];
        // a table in memory with no more rows than twice the elements asked for is taken whole (as shellForceTables)
        const bool whole =
            !request.elementIds || (ref.table->isDecoded() && meta.rowCountKnown && meta.rowCount <= 2 * wanted.size());
        table.decoded = context.decodeRows(ref, whole ? std::nullopt : request.elementIds);
        if (!table.decoded)
            return;
        const TableValues values(*table.decoded);
        if (values.isComplex())
        {
            other.store(true); // refused by the block, as the providers' rows are
            return;
        }
        for (auto& end : table.source)
            end.fill(-1);
        bool any = false;
        for (const Mapping& entry : mappingFor(family, strain))
        {
            table.source[0][entry.field] = values.component(entry.component);
            any = any || table.source[0][entry.field] >= 0;
        }
        if (!any)
        {
            table.decoded.reset(); // no row, as flattenStressTable gives none
            return;
        }
        table.set = ref.set;
        table.analysis = context.sets()[ref.set].analysis;
        table.subcase = meta.subcase;
        table.designCycle = meta.designCycle;
        table.elementName = meta.elementName;
        table.stepped = steppedKind(meta.stepKind);
        table.position = table.source[0][FIBER];
        table.completion = family == ElementFamily::Shell   ? DirectForceTable::Completion::Shell
                           : family == ElementFamily::Solid ? DirectForceTable::Completion::Solid
                                                            : DirectForceTable::Completion::None;
        table.strain = strain;
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
    // the labels of the rows, as flattenStressTable gives them, shared by a table with those of the one before
    for (std::size_t index = 0; index < kept.size(); ++index)
    {
        DirectForceTable& table = kept[index];
        const results::ResultData& decoded = *table.decoded;
        if (index > 0 && kept[index - 1].elementName == table.elementName)
        {
            const results::ResultData& before = *kept[index - 1].decoded;
            if (before.ids == decoded.ids && before.nodeIds == decoded.nodeIds && before.layers == decoded.layers)
            {
                table.location = kept[index - 1].location;
                table.node = kept[index - 1].node;
                table.layer = kept[index - 1].layer;
                continue;
            }
        }
        const ElementFamily family = familyOfContainer(table.elementName);
        const bool shellOrSolid = family == ElementFamily::Shell || family == ElementFamily::Solid;
        const std::size_t rows = decoded.rowCount;
        auto location = std::make_shared<std::vector<std::uint8_t>>(rows, 0);
        auto node = std::make_shared<std::vector<std::int32_t>>(rows, 0);
        auto layer = std::make_shared<std::vector<std::int32_t>>(rows, 0);
        for (std::size_t row = 0; row < rows; ++row)
        {
            (*node)[row] = shellOrSolid && row < decoded.nodeIds.size() ? decoded.nodeIds[row] : 0;
            (*location)[row] = shellOrSolid && (*node)[row] != 0 ? 1 : 0; // center, corner
            (*layer)[row] = family == ElementFamily::Shell && row < decoded.layers.size() ? decoded.layers[row] : 0;
        }
        table.location = std::move(location);
        table.node = std::move(node);
        table.layer = std::move(layer);
    }
    // With a model the other providers of the chain may add the rows of a subcase and element type that the stored
    // tables do not have (from the strains or the stresses, from the forces of shells and rods): their tables must
    // then have only elements whose rows the stored tables of the same subcase and type already give (the providers'
    // rows of them are all dropped), else the providers are asked.
    if (context.hasModel())
    {
        std::vector<TableRef> starts;
        const results::ResultKind otherKind = strain ? results::ResultKind::Stress : results::ResultKind::Strain;
        for (const std::string& name : context.elementNames(otherKind))
        {
            const ElementFamily family = familyOfContainer(name);
            if ((family == ElementFamily::Shell || family == ElementFamily::Solid || family == ElementFamily::Rod) &&
                request.wantsType(cardOfContainer(name)))
                for (const TableRef& ref : tablesFor(context, otherKind, name))
                    starts.push_back(ref);
        }
        if (!strain)
            for (const std::string& name : context.elementNames(results::ResultKind::ElementForces))
            {
                const ElementFamily family = familyOfContainer(name);
                if ((family == ElementFamily::Shell || family == ElementFamily::Rod) &&
                    request.wantsType(cardOfContainer(name)))
                    for (const TableRef& ref : context.elementTables(results::ResultKind::ElementForces, name, false))
                        starts.push_back(ref);
            }
        if (!starts.empty())
        {
            if (!request.elementIds)
                return std::nullopt;
            // the elements asked for of each result set and element type: those its stored tables have rows of
            std::map<std::pair<std::size_t, std::string>, std::vector<std::int32_t>> wantedOf;
            for (std::size_t index = 0; index < kept.size(); ++index)
            {
                const auto& ids = kept[index].decoded->ids;
                if (index > 0 && kept[index - 1].elementName == kept[index].elementName &&
                    kept[index - 1].set == kept[index].set && kept[index - 1].decoded->ids == ids)
                    continue;
                std::vector<std::int32_t>& of = wantedOf[{kept[index].set, kept[index].elementName}];
                for (std::size_t row = 0; row < ids.size(); ++row)
                    if ((row == 0 || ids[row] != ids[row - 1]) &&
                        std::binary_search(wanted.begin(), wanted.end(), ids[row]))
                        of.push_back(ids[row]);
            }
            for (auto& [key, ids] : wantedOf)
            {
                std::sort(ids.begin(), ids.end());
                ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
            }
            // the stored tables that have every element asked for of their type
            std::vector<std::uint8_t> complete(kept.size(), 0);
            for (std::size_t index = 0; index < kept.size(); ++index)
            {
                const DirectForceTable& table = kept[index];
                const DirectForceTable* before = index > 0 ? &kept[index - 1] : nullptr;
                complete[index] =
                    before != nullptr && before->elementName == table.elementName && before->set == table.set &&
                            before->decoded->ids == table.decoded->ids
                        ? complete[index - 1]
                        : (hasEvery(table.decoded->ids, wantedOf[{table.set, table.elementName}]) ? 1 : 0);
            }
            for (const TableRef& ref : starts)
            {
                const results::ResultTableMeta& meta = ref.table->meta();
                const auto found = wantedOf.find({ref.set, meta.elementName});
                if (found == wantedOf.end() || found->second.empty())
                    continue; // no element asked for is of this type (one that has no stored row is refused below)
                if (steppedKind(meta.stepKind))
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
    }
    if (!request.elementIds)
        return kept;
    // every element asked for has a row: else the chain's other ways and onMissing decide
    std::vector<std::uint8_t> present(wanted.size(), 0);
    std::size_t found = 0;
    for (std::size_t index = 0; index < kept.size() && found < wanted.size(); ++index)
    {
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

QuantityInfo derivedScalarInfo()
{
    QuantityInfo info;
    info.quantity = "von Mises, principal values, maximum shear";
    info.providers = {ProviderInfo{"stored column", false}, ProviderInfo{"computed from the components", false}};
    return info;
}

} // namespace oor::query
