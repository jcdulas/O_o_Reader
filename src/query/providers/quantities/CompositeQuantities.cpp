#include "oor/query/providers/quantities/CompositeQuantities.hpp"

#include "oor/base/threading/ParallelFor.hpp"
#include "oor/model/cards/properties/compProp/CompPropTable.hpp"
#include "oor/model/cards/properties/compPropGlobal/CompPropGlobalTable.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/common/LaminateCards.hpp"
#include "oor/query/named/results/ResultQuery.hpp"
#include "oor/query/providers/quantities/MaterialAllowables.hpp"
#include "oor/query/results/ResultModelIndex.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
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

bool endsWith(const std::string& text, const std::string& tail)
{
    return text.size() >= tail.size() && text.compare(text.size() - tail.size(), tail.size(), tail) == 0;
}

// The camelCase name of a stored theory name: "TSAI-WU", "tsai wu" and "tsaiWu" give "tsaiWu".
std::string normalised(std::string text)
{
    // a name written in capitals only is read as lower case words
    if (std::none_of(text.begin(), text.end(), [](unsigned char character) { return std::islower(character) != 0; }))
        for (char& character : text)
            character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    std::string result;
    bool capital = false;
    for (const char character : text)
    {
        if (character == '-' || character == ' ' || character == '_')
        {
            capital = !result.empty();
            continue;
        }
        const auto letter = static_cast<unsigned char>(character);
        result += static_cast<char>(capital ? std::toupper(letter) : (result.empty() ? std::tolower(letter) : letter));
        capital = false;
    }
    return result;
}

} // namespace

const std::vector<std::string>& plyStressFields()
{
    static const std::vector<std::string> FIELDS = {"o11", "o22", "t12", "t1z", "t2z", "omax", "omin", "maxShear"};
    return FIELDS;
}

const std::vector<std::string>& plyStrainFields()
{
    static const std::vector<std::string> FIELDS = {"e11", "e22", "e12", "e1z", "e2z", "emax", "emin", "maxShear"};
    return FIELDS;
}

std::optional<ElementFlat> flattenPlyTable(const ResultQueryContext& context, const TableRef& ref, bool strain,
                                           const ElementRequest& request)
{
    const results::ResultTableMeta& meta = ref.table->meta();
    if (!endsWith(meta.elementName, "Composite"))
        return std::nullopt;
    const std::string card = cardOfContainer(meta.elementName);
    if (!request.wantsType(card))
        return std::nullopt;
    const std::shared_ptr<const results::ResultData> decoded = context.decodeRows(ref, request.elementIds);
    if (!decoded)
        return std::nullopt;
    const TableValues values(*decoded);
    const std::vector<std::string>& fields = strain ? plyStrainFields() : plyStressFields();
    const char* const stressNames[8] = {"o11", "o22", "t12", "t1z", "t2z", "major", "minor", "maxShear"};
    const char* const strainNames[8] = {"e11", "e22", "e12", "e1z", "e2z", "major", "minor", "maxShear"};
    int component[8];
    bool any = false;
    for (std::size_t field = 0; field < 8; ++field)
    {
        component[field] = values.component(strain ? strainNames[field] : stressNames[field]);
        any = any || component[field] >= 0;
    }
    if (!any)
        return std::nullopt;
    const bool complex = values.isComplex();
    ElementFlat flat;
    std::vector<std::vector<double>> real(8);
    std::vector<std::vector<double>> imaginary(8);
    const bool stepped = meta.stepKind == results::StepKind::Time || meta.stepKind == results::StepKind::Frequency ||
                         meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
    // the rows the table keeps, the same at each step: the decoding selected the elements the call names already, and
    // the property of an element is looked up once for its run of plies
    std::vector<std::size_t> kept;
    kept.reserve(values.rows());
    bool elementWanted = true;
    for (std::size_t row = 0; row < values.rows(); ++row)
    {
        const std::int32_t element = decoded->ids[row];
        if (row == 0 || element != decoded->ids[row - 1])
            elementWanted = (request.elementIds.has_value() || request.wantsElement(element)) &&
                            (!request.propertyIds || request.wantsProperty(context.elementCard(ref.set, element).pid));
        if (elementWanted && request.wantsLayer(decoded->layers[row]))
            kept.push_back(row);
    }
    const std::size_t expected = kept.size() * values.steps();
    for (auto* column : {&flat.eid, &flat.node, &flat.layer})
        column->reserve(expected);
    flat.step.reserve(expected);
    for (std::size_t field = 0; field < 8; ++field)
    {
        real[field].reserve(expected);
        if (complex)
            imaginary[field].reserve(expected);
    }
    for (std::size_t step = 0; step < values.steps(); ++step)
    {
        for (const std::size_t row : kept)
        {
            flat.eid.push_back(decoded->ids[row]);
            flat.node.push_back(row < decoded->nodeIds.size() ? decoded->nodeIds[row] : 0);
            flat.layer.push_back(decoded->layers[row]);
        }
        flat.step.insert(flat.step.end(), kept.size(), stepped ? decoded->steps[step] : NOT_A_NUMBER);
        if (!decoded->eigenvalues.empty())
            flat.eigenvalue.insert(flat.eigenvalue.end(), kept.size(), decoded->eigenvalues[step]);
        for (std::size_t field = 0; field < 8; ++field)
        {
            if (component[field] < 0)
            {
                real[field].insert(real[field].end(), kept.size(), NOT_A_NUMBER);
                if (complex)
                    imaginary[field].insert(imaginary[field].end(), kept.size(), NOT_A_NUMBER);
                continue;
            }
            const auto column = static_cast<std::size_t>(component[field]);
            for (const std::size_t row : kept)
                real[field].push_back(values.real(step, row, column));
            if (complex)
                for (const std::size_t row : kept)
                    imaginary[field].push_back(values.imag(step, row, column));
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
    flat.card = card;
    flat.frame = context.frameNameOf(ref);
    flat.precision = decoded->type == results::ValueType::Float32 || decoded->type == results::ValueType::Complex64
                         ? Precision::Single
                         : Precision::Double;
    flat.magnitudePhase = meta.storedFormat == results::ResultFormat::MagnitudePhase;
    for (std::size_t field = 0; field < 8; ++field)
    {
        flat.values[fields[field]] = std::move(real[field]);
        if (complex)
            flat.imaginary[fields[field]] = std::move(imaginary[field]);
    }
    return flat;
}

std::vector<TableRef> plyTables(const ResultQueryContext& context, bool strain, const ElementRequest& request)
{
    const results::ResultKind kind = strain ? results::ResultKind::Strain : results::ResultKind::Stress;
    std::vector<TableRef> tables;
    for (const std::string& name : context.elementNames(kind))
    {
        if (!endsWith(name, "Composite") || !request.wantsType(cardOfContainer(name)))
            continue;
        for (const TableRef& ref : context.elementTables(kind, name, false))
            tables.push_back(ref);
    }
    return tables;
}

std::vector<ElementFlat> flattenPlyTables(const ResultQueryContext& context, const std::vector<TableRef>& tables,
                                          bool strain, const ElementRequest& request)
{
    std::vector<ElementFlat> flats;
    makeInOrder<std::optional<ElementFlat>>(
        context, tables.size(), [&](std::size_t index) { return indexedRows(tables[index]); },
        [&](std::size_t index) { return flattenPlyTable(context, tables[index], strain, request); },
        [&](std::size_t, std::optional<ElementFlat>&& flat)
        {
            if (flat)
                flats.push_back(std::move(*flat));
        });
    return flats;
}

std::optional<std::vector<DirectForceTable>> storedPlyTables(const ResultQueryContext& context,
                                                             const ElementRequest& request, bool strain)
{
    if (ProviderSwitches::isDisabled(PLY_TABLE_SWITCH))
        return std::nullopt;
    if (request.propertyIds || request.layers || !request.location.empty())
        return std::nullopt;
    const std::vector<TableRef> refs = plyTables(context, strain, request);
    if (refs.empty())
        return std::nullopt;
    const char* const stressNames[8] = {"o11", "o22", "t12", "t1z", "t2z", "major", "minor", "maxShear"};
    const char* const strainNames[8] = {"e11", "e22", "e12", "e1z", "e2z", "major", "minor", "maxShear"};
    std::vector<DirectForceTable> made(refs.size());
    std::atomic<bool> complex{false};
    const auto make = [&](std::size_t at)
    {
        const TableRef& ref = refs[at];
        const results::ResultTableMeta& meta = ref.table->meta();
        DirectForceTable& table = made[at];
        table.decoded = context.decodeRows(ref, request.elementIds);
        if (!table.decoded)
            return;
        const TableValues values(*table.decoded);
        if (values.isComplex())
        {
            complex.store(true); // refused by the block, as the flats' rows are
            return;
        }
        for (auto& end : table.source)
            end.fill(-1);
        bool any = false;
        for (std::size_t field = 0; field < 8; ++field)
        {
            table.source[0][field] = values.component(strain ? strainNames[field] : stressNames[field]);
            any = any || table.source[0][field] >= 0;
        }
        if (!any || table.decoded->layers.size() != table.decoded->rowCount)
        {
            table.decoded.reset(); // no row, as flattenPlyTable gives none
            return;
        }
        table.set = ref.set;
        table.analysis = context.sets()[ref.set].analysis;
        table.subcase = meta.subcase;
        table.designCycle = meta.designCycle;
        table.elementName = meta.elementName;
        table.stepped = meta.stepKind == results::StepKind::Time || meta.stepKind == results::StepKind::Frequency ||
                        meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
        table.strain = strain;
    };
    const results::DecodeContext decode = context.decode();
    if (decode.pool != nullptr && refs.size() > 1)
        base::parallelFor(*decode.pool, refs.size(), make, decode.token);
    else
        for (std::size_t at = 0; at < refs.size(); ++at)
            make(at);
    if (complex.load())
        return std::nullopt;
    std::vector<DirectForceTable> kept;
    kept.reserve(made.size());
    for (DirectForceTable& table : made)
        if (table.decoded && table.decoded->rowCount > 0 && table.decoded->stepCount > 0)
            kept.push_back(std::move(table));
    if (kept.empty())
        return std::nullopt;
    // the labels of the rows: the ply of each, one place; shared by a table with the rows of the one before
    for (std::size_t index = 0; index < kept.size(); ++index)
    {
        DirectForceTable& table = kept[index];
        const results::ResultData& decoded = *table.decoded;
        if (index > 0)
        {
            const results::ResultData& before = *kept[index - 1].decoded;
            if (before.ids == decoded.ids && before.layers == decoded.layers)
            {
                table.location = kept[index - 1].location;
                table.layer = kept[index - 1].layer;
                continue;
            }
        }
        table.location = std::make_shared<std::vector<std::uint8_t>>(decoded.rowCount, 0);
        table.layer = std::make_shared<std::vector<std::int32_t>>(decoded.layers.begin(), decoded.layers.end());
    }
    return kept;
}

std::string storedTheoryName(const std::string& stored)
{
    return normalised(stored);
}

std::string theoryName(int failureTheory)
{
    switch (static_cast<model::FailureTheory>(failureTheory))
    {
    case model::FailureTheory::Hill:
        return "hill";
    case model::FailureTheory::Hoffman:
        return "hoffman";
    case model::FailureTheory::TsaiWu:
        return "tsaiWu";
    case model::FailureTheory::MaxStrain:
        return "maxStrain";
    case model::FailureTheory::HashinFailure:
        return "hashin"; // HFAIL: the Hashin criteria of a unidirectional ply (MAT8 HFAIL)
    default:
        return {};
    }
}

PlyTheory plyTheoryOf(const std::string& theory) noexcept
{
    static const std::pair<const char*, PlyTheory> NAMES[] = {
        {"tsaiWu", PlyTheory::TsaiWu},       {"hill", PlyTheory::Hill},           {"hoffman", PlyTheory::Hoffman},
        {"maxStress", PlyTheory::MaxStress}, {"maxStrain", PlyTheory::MaxStrain}, {"hashin", PlyTheory::Hashin},
        {"puck", PlyTheory::Puck},           {"larc03", PlyTheory::Larc03}};
    for (const auto& [name, value] : NAMES)
        if (theory == name)
            return value;
    return PlyTheory::None;
}

PlyIndex plyFailureIndex(const std::string& theory, double s1, double s2, double t12, const PlyMaterial& m)
{
    return plyFailureIndex(plyTheoryOf(theory), s1, s2, t12, m);
}

PlyIndex plyFailureIndex(PlyTheory theory, double s1, double s2, double t12, const PlyMaterial& m)
{
    PlyIndex result;
    result.index = result.quadratic = result.linear = NOT_A_NUMBER;
    // the theories of failure modes: the index of the governing mode, not a polynomial of the stresses
    if (theory == PlyTheory::Hashin || theory == PlyTheory::Puck || theory == PlyTheory::Larc03)
    {
        static const std::string HASHIN = "hashin";
        static const std::string PUCK = "puck";
        static const std::string LARC03 = "larc03";
        const std::string& name = theory == PlyTheory::Hashin ? HASHIN : theory == PlyTheory::Puck ? PUCK : LARC03;
        result.index = plyModeIndices(name, s1, s2, t12, m).index;
        return result;
    }
    if (theory == PlyTheory::MaxStress || theory == PlyTheory::MaxStrain)
    {
        double a = s1;
        double b = s2;
        double c = t12;
        double xt = m.xt, xc = m.xc, yt = m.yt, yc = m.yc, ss = m.s;
        if (theory == PlyTheory::MaxStrain)
        {
            // the strains of the lamina, and the allowables as strains
            const double nu21 = m.e1 > 0.0 ? m.nu12 * m.e2 / m.e1 : 0.0;
            if (!(m.e1 > 0.0) || !(m.e2 > 0.0) || !(m.g12 > 0.0))
                return result;
            a = (s1 - m.nu12 * s2) / m.e1;
            b = (s2 - nu21 * s1) / m.e2;
            c = t12 / m.g12;
            if (!m.strainAllowables)
            {
                xt /= m.e1;
                xc /= m.e1;
                yt /= m.e2;
                yc /= m.e2;
                ss /= m.g12;
            }
        }
        const double ratios[3] = {a >= 0.0 ? (xt > 0.0 ? a / xt : NOT_A_NUMBER) : (xc > 0.0 ? -a / xc : NOT_A_NUMBER),
                                  b >= 0.0 ? (yt > 0.0 ? b / yt : NOT_A_NUMBER) : (yc > 0.0 ? -b / yc : NOT_A_NUMBER),
                                  ss > 0.0 ? std::fabs(c) / ss : NOT_A_NUMBER};
        double index = 0.0;
        for (const double ratio : ratios)
        {
            if (std::isnan(ratio))
                return result;
            index = std::max(index, ratio);
        }
        result.index = index;
        result.quadratic = 0.0;
        result.linear = index; // the index scales with the stresses
        return result;
    }
    if (!(m.xt > 0.0) || !(m.yt > 0.0) || !(m.s > 0.0))
        return result;
    const double xc = m.xc > 0.0 ? m.xc : m.xt;
    const double yc = m.yc > 0.0 ? m.yc : m.yt;
    if (theory == PlyTheory::Hill)
    {
        const double x = s1 >= 0.0 ? m.xt : xc;
        const double y = s2 >= 0.0 ? m.yt : yc;
        result.quadratic = (s1 / x) * (s1 / x) - s1 * s2 / (x * x) + (s2 / y) * (s2 / y) + (t12 / m.s) * (t12 / m.s);
        result.linear = 0.0;
    }
    else if (theory == PlyTheory::Hoffman)
    {
        result.quadratic = (s1 * s1 - s1 * s2) / (m.xt * xc) + s2 * s2 / (m.yt * yc) + t12 * t12 / (m.s * m.s);
        result.linear = (xc - m.xt) / (m.xt * xc) * s1 + (yc - m.yt) / (m.yt * yc) * s2;
    }
    else if (theory == PlyTheory::TsaiWu)
    {
        const double f11 = 1.0 / (m.xt * xc);
        const double f22 = 1.0 / (m.yt * yc);
        const double f66 = 1.0 / (m.s * m.s);
        const double f12 = m.f12 * std::sqrt(f11 * f22);
        result.quadratic = f11 * s1 * s1 + f22 * s2 * s2 + f66 * t12 * t12 + 2.0 * f12 * s1 * s2;
        result.linear = (1.0 / m.xt - 1.0 / xc) * s1 + (1.0 / m.yt - 1.0 / yc) * s2;
    }
    else
    {
        return result;
    }
    result.index = result.quadratic + result.linear;
    return result;
}

double scaleToLimit(const PlyIndex& index, double limit)
{
    const double q = index.quadratic;
    const double l = index.linear;
    if (std::isnan(q) || std::isnan(l))
        return NOT_A_NUMBER;
    if (std::fabs(q) < 1.0e-300)
        return std::fabs(l) < 1.0e-300 ? NOT_A_NUMBER : limit / l;
    const double discriminant = l * l + 4.0 * q * limit;
    if (discriminant < 0.0)
        return NOT_A_NUMBER;
    return (-l + std::sqrt(discriminant)) / (2.0 * q);
}

LaminateLookup::LaminateLookup(const ResultQueryContext& context) : m_context(context), m_index(context) {}

namespace
{

std::uint64_t laminateKey(std::size_t set, std::int32_t resultEid) noexcept
{
    return (static_cast<std::uint64_t>(set) << 32U) | static_cast<std::uint32_t>(resultEid);
}

} // namespace

const LaminateFacts& LaminateLookup::facts(std::size_t set, std::int32_t resultEid)
{
    const std::uint64_t key = laminateKey(set, resultEid);
    const auto found = m_entries.find(key);
    if (found != m_entries.end())
        return *found->second;
    static const LaminateFacts NONE;
    const LaminateFacts* entry = &NONE;
    const model::Model* model = m_context.modelOf(set);
    if (model != nullptr)
    {
        if (const auto location = m_index.propertyLocation(set, resultEid))
        {
            // the laminate of the property, read once for its elements
            const auto propertyKey = std::make_tuple(model, location->table, location->row);
            const auto known = m_properties.find(propertyKey);
            entry = known != m_properties.end()
                        ? &known->second
                        : &m_properties.emplace(propertyKey, readFacts(*model, *location)).first->second;
        }
    }
    return *m_entries.emplace(key, entry).first->second;
}

LaminateFacts LaminateLookup::readFacts(const model::Model& model, const model::CardLocation& location)
{
    LaminateFacts result;
    const auto laminate = readLaminate(model, location);
    if (!laminate)
        return result;
    if (laminate->bondAllowable > 0.0)
        result.bondAllowable = laminate->bondAllowable;
    const model::CardTable* table = model.findTable(location.table);
    if (location.table == model::typeIndexOf<model::CompPropTable>())
        result.theory = theoryName(
            static_cast<int>(static_cast<const model::CompPropTable&>(*table).card(location.row).failureTheory));
    else if (location.table == model::typeIndexOf<model::CompPropGlobalTable>())
        result.theory = theoryName(
            static_cast<int>(static_cast<const model::CompPropGlobalTable&>(*table).card(location.row).failureTheory));
    for (const PlyInfo& ply : laminate->plies)
    {
        PlyFacts facts;
        facts.globalId = ply.globalId;
        facts.material = ply.material;
        facts.angle = ply.angle;
        facts.thickness = ply.thickness;
        facts.allowables = orthoMat2DAllowables(model, ply.material);
        result.plies.push_back(std::move(facts));
    }
    return result;
}

void LaminateLookup::prepare(const ElementFlat& flat, const ElementFlat* before)
{
    if (before != nullptr && before->set == flat.set && before->eid == flat.eid)
        return;
    for (std::size_t row = 0; row < flat.rows(); ++row)
        if (row == 0 || flat.eid[row] != flat.eid[row - 1])
            (void)facts(flat.set, flat.eid[row]);
}

const LaminateFacts* LaminateLookup::found(std::size_t set, std::int32_t resultEid) const
{
    const auto entry = m_entries.find(laminateKey(set, resultEid));
    return entry != m_entries.end() ? entry->second : nullptr;
}

double bondIndex(double t1z, double t2z, double sb, bool resultant) noexcept
{
    if (!(sb > 0.0))
        return NOT_A_NUMBER;
    const double shear = resultant ? std::hypot(t1z, t2z) : std::max(std::fabs(t1z), std::fabs(t2z));
    return shear / sb;
}

namespace
{

// ---- the stored index
// -------------------------------------------------------------------------------------------------------

class StoredFailureIndexProvider final : public ElementProvider
{
public:
    [[nodiscard]] std::string name() const override { return "failure index table of the result file"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ElementRequest& request,
                                  ElementFlatSet& sink) const override
    {
        if (!request.theory.empty())
            return {0, "the theory '" + request.theory + "' was asked for: the stored index is that of the property"};
        Outcome outcome;
        bool anyTable = false;
        for (const std::string& name : context.elementNames(results::ResultKind::FailureIndices))
        {
            if (!endsWith(name, "Composite") || !request.wantsType(cardOfContainer(name)))
                continue;
            for (const TableRef& ref : context.elementTables(results::ResultKind::FailureIndices, name, false))
            {
                anyTable = true;
                const results::ResultTableMeta& meta = ref.table->meta();
                const std::shared_ptr<const results::ResultData> decoded = context.decodeRows(ref, request.elementIds);
                if (!decoded)
                    continue;
                const TableValues values(*decoded);
                const int index = values.component("failureIndex");
                if (index < 0 || values.isComplex())
                    continue;
                const int bondColumn = values.component("bondFailureIndex"); // the solver writes it with the PCOMP SB
                const results::ResultText* theories = nullptr;
                for (const results::ResultText& text : decoded->texts)
                    if (text.name == "failureTheory")
                        theories = &text;
                // the theory is written on the first rows of an element: it holds for the rows that follow
                std::vector<std::string> theoryOf(values.rows());
                std::string current;
                std::int32_t currentElement = -1;
                for (std::size_t row = 0; row < values.rows(); ++row)
                {
                    if (decoded->ids[row] != currentElement)
                    {
                        currentElement = decoded->ids[row];
                        current.clear();
                    }
                    if (theories != nullptr && !theories->at(row).empty())
                        current = normalised(theories->at(row));
                    theoryOf[row] = current;
                }
                ElementFlat flat;
                std::vector<double> real;
                std::vector<double> bond;
                std::vector<std::string> theory;
                const bool stepped =
                    meta.stepKind == results::StepKind::Time || meta.stepKind == results::StepKind::Frequency ||
                    meta.stepKind == results::StepKind::Mode || meta.stepKind == results::StepKind::LoadFactor;
                for (std::size_t step = 0; step < values.steps(); ++step)
                    for (std::size_t row = 0; row < values.rows(); ++row)
                    {
                        const std::int32_t element = decoded->ids[row];
                        if (!request.wantsElement(element) || !request.wantsLayer(decoded->layers[row]))
                            continue;
                        if (request.propertyIds && !request.wantsProperty(context.elementCard(ref.set, element).pid))
                            continue;
                        flat.eid.push_back(element);
                        flat.node.push_back(0);
                        flat.layer.push_back(decoded->layers[row]);
                        flat.step.push_back(stepped ? decoded->steps[step] : NOT_A_NUMBER);
                        if (!decoded->eigenvalues.empty())
                            flat.eigenvalue.push_back(decoded->eigenvalues[step]);
                        flat.position.push_back(NOT_A_NUMBER);
                        flat.location.push_back("center");
                        real.push_back(values.real(step, row, static_cast<std::size_t>(index)));
                        bond.push_back(bondColumn >= 0 ? values.real(step, row, static_cast<std::size_t>(bondColumn))
                                                       : NOT_A_NUMBER);
                        theory.push_back(theoryOf[row]);
                    }
                if (flat.eid.empty())
                    continue;
                flat.set = ref.set;
                flat.analysis = context.sets()[ref.set].analysis;
                flat.status = std::string(results::resultStatusName(meta.status));
                flat.subcase = meta.subcase;
                flat.designCycle = meta.designCycle;
                flat.stepKind = stepped ? meta.stepKind : results::StepKind::None;
                flat.elementName = meta.elementName;
                flat.card = cardOfContainer(meta.elementName);
                flat.frame = "ply";
                flat.precision = decoded->type == results::ValueType::Float32 ? Precision::Single : Precision::Double;
                flat.values["failureIndex"] = std::move(real);
                flat.values["bondIndex"] = std::move(bond);
                flat.texts["failureTheory"] = std::move(theory);
                outcome.rows += sink.add(std::move(flat));
            }
        }
        if (outcome.rows == 0)
            outcome.reason = anyTable
                                 ? "the table has no row of the elements, plies and subcases asked for"
                                 : "the result file has no failure index table of laminates (output of FAILURE INDICES "
                                   "was not requested)";
        return outcome;
    }
};

// ---- recomputed from the ply stresses and the allowables
// ----------------------------------------------------------------------

class RecomputedFailureIndexProvider final : public ElementProvider
{
public:
    [[nodiscard]] std::string name() const override { return "recomputed from the ply stresses and the allowables"; }
    [[nodiscard]] Outcome provide(const ResultQueryContext& context, const ElementRequest& request,
                                  ElementFlatSet& sink) const override
    {
        if (!context.hasModel())
            return {0, "no model is attached to the results (the plies and the allowables are in the model)"};
        const std::string asked = request.theory;
        if (!asked.empty() && asked != "maxStress" && asked != "maxStrain" && asked != "hill" && asked != "hoffman" &&
            asked != "tsaiWu" && !isModeTheory(asked))
            throw QueryError(
                "theory is None, \"maxStress\", \"maxStrain\", \"hill\", \"hoffman\", \"tsaiWu\", "
                "\"hashin\", \"puck\" or \"larc03\", not '" +
                asked + "'");
        Outcome outcome;
        std::map<std::string, std::int64_t> reasons;
        const std::vector<TableRef> tables = plyTables(context, false, request);
        const bool anyTable = !tables.empty();
        // the stresses of the plies: those the caller made of the same tables, else made here
        std::vector<ElementFlat> made;
        if (request.plyStresses == nullptr)
            made = flattenPlyTables(context, tables, false, request);
        const std::vector<ElementFlat>& stresses = request.plyStresses != nullptr ? *request.plyStresses : made;
        LaminateLookup laminates(context);
        for (std::size_t index = 0; index < stresses.size(); ++index)
            laminates.prepare(stresses[index], index > 0 ? &stresses[index - 1] : nullptr);
        // the indices of a flat on any thread (the laminates are read), added to the sink in the order of the flats
        struct Made
        {
            std::optional<ElementFlat> flat;
            std::map<std::string, std::int64_t> reasons;
        };
        const auto make = [&](std::size_t index)
        {
            Made result;
            const ElementFlat& stress = stresses[index];
            if (stress.isComplex())
                return result;
            const std::vector<double>& o11 = stress.values.at("o11");
            const std::vector<double>& o22 = stress.values.at("o22");
            const std::vector<double>& t12 = stress.values.at("t12");
            std::vector<double> indices;
            std::vector<std::string> theories;
            std::vector<std::size_t> keep;
            indices.reserve(stress.rows());
            theories.reserve(stress.rows());
            keep.reserve(stress.rows());
            std::set<std::int32_t> told;
            static const LaminateFacts NONE;
            const LaminateFacts* laminate = &NONE;
            PlyTheory parsed = PlyTheory::None;
            for (std::size_t row = 0; row < stress.rows(); ++row)
            {
                if (row == 0 || stress.eid[row] != stress.eid[row - 1])
                {
                    const LaminateFacts* found = laminates.found(stress.set, stress.eid[row]);
                    laminate = found != nullptr ? found : &NONE;
                    parsed = plyTheoryOf(asked.empty() ? laminate->theory : asked);
                }
                const std::string& theory = asked.empty() ? laminate->theory : asked;
                const auto note = [&](const std::string& why)
                {
                    if (told.insert(stress.eid[row]).second)
                        ++result.reasons[why];
                };
                if (theory.empty())
                {
                    note("the property has no failure theory (PCOMP FT) and none was asked for");
                    continue;
                }
                const auto ply = static_cast<std::size_t>(stress.layer[row] - 1);
                if (ply >= laminate->plies.size() || !laminate->plies[ply].allowables)
                {
                    note("a ply of the laminate has no MAT8 material (or the ply is not in the laminate)");
                    continue;
                }
                const PlyIndex value =
                    plyFailureIndex(parsed, o11[row], o22[row], t12[row], *laminate->plies[ply].allowables);
                if (std::isnan(value.index))
                {
                    note("the MAT8 lacks an allowable the theory '" + theory + "' needs");
                    continue;
                }
                indices.push_back(value.index);
                theories.push_back(theory);
                keep.push_back(row);
            }
            if (keep.empty())
                return result;
            // the labels of the rows kept, the values of the index
            ElementFlat flat = stress.rowLabels(keep);
            flat.values["failureIndex"] = std::move(indices);
            flat.texts["failureTheory"] = std::move(theories);
            flat.frame = "ply";
            result.flat = std::move(flat);
            return result;
        };
        makeInOrder<Made>(
            context, stresses.size(), [&](std::size_t index) { return stresses[index].rows(); }, make,
            [&](std::size_t, Made&& result)
            {
                for (const auto& [why, count] : result.reasons)
                    reasons[why] += count;
                if (result.flat)
                    outcome.rows += sink.add(std::move(*result.flat));
            });
        if (outcome.rows == 0)
            outcome.reason = !reasons.empty() ? joinReasons(reasons)
                             : anyTable ? "no stress row of the plies asked for"
                                        : "the result file has no stress table of laminates (output of STRESS with "
                                          "SOUT=YES was not requested)";
        return outcome;
    }
};

} // namespace

const ElementChain& failureIndexChain()
{
    static const ElementChain CHAIN = []
    {
        ElementChain chain("composite failure index, strength ratio");
        chain.add(std::make_unique<StoredFailureIndexProvider>());
        chain.add(std::make_unique<RecomputedFailureIndexProvider>());
        return chain;
    }();
    return CHAIN;
}

} // namespace oor::query
