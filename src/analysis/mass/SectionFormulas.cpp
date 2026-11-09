#include "oor/analysis/mass/SectionFormulas.hpp"

#include "oor/analysis/mass/SectionCache.hpp"
#include "oor/model/cards/properties/barProp/BarPropTable.hpp"
#include "oor/model/cards/properties/barShapeProp/BarShapePropTable.hpp"
#include "oor/model/cards/properties/beamLumpedAreaProp/BeamLumpedAreaPropTable.hpp"
#include "oor/model/cards/properties/beamProp/BeamPropTable.hpp"
#include "oor/model/cards/properties/beamProp3/BeamProp3Table.hpp"
#include "oor/model/cards/properties/beamShapeProp/BeamShapePropTable.hpp"
#include "oor/model/cards/properties/curvedBeamProp/CurvedBeamPropTable.hpp"
#include "oor/model/cards/properties/rodProp/RodPropTable.hpp"
#include "oor/model/cards/properties/tubeProp/TubePropTable.hpp"
#include "oor/model/cards/solution_control/modelParams/ModelParamsTable.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace oor::analysis
{

namespace
{

constexpr double NAN_VALUE = SectionProperties::UNDEFINED;

// The values at the stations with the end points added when the first is after 0 or the last before 1, the value of the
// nearest station held: the beam does not extrapolate.
void withEnds(std::span<const double> positions, std::span<const double> values, std::vector<double>& xs,
              std::vector<double>& ys)
{
    xs.assign(positions.begin(), positions.end());
    ys.assign(values.begin(), values.end());
    if (xs.empty())
        return;
    if (xs.front() > 0.0)
    {
        xs.insert(xs.begin(), 0.0);
        ys.insert(ys.begin(), ys.front());
    }
    if (xs.back() < 1.0)
    {
        xs.push_back(1.0);
        ys.push_back(ys.back());
    }
}

SectionProperties barPropSection(const model::BarPropCard& card)
{
    SectionProperties section;
    section.area = card.area;
    section.i1 = card.bendingInertia1;
    section.i2 = card.bendingInertia2;
    section.i12 = card.productInertia12;
    section.j = card.torsionConstant;
    section.k1 = card.shearFactor1 == model::BarPropCard::NO_SHEAR_FACTOR ? NAN_VALUE : card.shearFactor1;
    section.k2 = card.shearFactor2 == model::BarPropCard::NO_SHEAR_FACTOR ? NAN_VALUE : card.shearFactor2;
    section.nsm = card.nonStructMass;
    section.recovery = {{card.stressPointC, card.stressPointD, card.stressPointE, card.stressPointF}};
    return section;
}

// A library section, from the cache when there is one.
SectionProperties librarySection(std::string_view type, std::span<const double> dimensions, ShearFactorRule rule,
                                 SectionCache* cache)
{
    return cache != nullptr ? cache->standard(type, dimensions, rule)
                            : SectionFormulas::standard(type, dimensions, rule);
}

BeamSection barShapePropSection(const model::BarShapePropCard& card, ShearFactorRule rule, SectionCache* cache)
{
    BeamSection section;
    section.propertyType = "BarShapeProp";
    section.sectionType = card.kind;
    section.materialId = card.matId;
    SectionProperties station = librarySection(card.kind, card.dims, rule, cache);
    station.nsm = card.nonStructMass;
    section.stations.push_back(station);
    return section;
}

BeamSection beamPropSection(const model::BeamPropCard& card)
{
    BeamSection section;
    section.propertyType = "BeamProp";
    section.materialId = card.matId;
    section.nsmCenterA = {card.nsmCenterYA, card.nsmCenterZA};
    section.nsmCenterB = {card.nsmCenterYB, card.nsmCenterZB};
    for (const model::BeamPropStation& source : card.stations)
    {
        SectionProperties station;
        station.xxb = source.stationRatio;
        station.area = source.area;
        station.i1 = source.bendingInertia1;
        station.i2 = source.bendingInertia2;
        station.i12 = source.productInertia12;
        station.j = source.torsionConstant;
        station.k1 = card.shearFactor1;
        station.k2 = card.shearFactor2;
        station.nsm = source.nonStructMass;
        station.recovery = {{source.stressPointC, source.stressPointD, source.stressPointE, source.stressPointF}};
        section.stations.push_back(station);
    }
    return section;
}

BeamSection beamShapePropSection(const model::BeamShapePropCard& card, ShearFactorRule rule, SectionCache* cache)
{
    BeamSection section;
    section.propertyType = "BeamShapeProp";
    section.sectionType = card.kind;
    section.materialId = card.matId;
    for (const model::BeamShapePropStation& source : card.stations)
    {
        SectionProperties station = librarySection(card.kind, source.dims, rule, cache);
        station.xxb = source.stationRatio;
        station.nsm = source.nonStructMass;
        section.stations.push_back(station);
    }
    return section;
}

BeamSection beamProp3Section(const model::BeamProp3Card& card)
{
    BeamSection section;
    section.propertyType = "BeamProp3";
    section.materialId = card.matId;
    section.nsmCenterA = card.stations[0].nsmCenter;
    section.nsmCenterB = card.stations[1].nsmCenter;
    // The three stations are end A, end B and the middle node C, at X/XB 0, 1 and one half.
    constexpr double POSITIONS[3] = {0.0, 1.0, 0.5};
    for (std::size_t index = 0; index < 3; ++index)
    {
        const model::BeamProp3Station& source = card.stations[index];
        SectionProperties station;
        station.xxb = POSITIONS[index];
        station.area = source.area;
        station.i1 = source.bendingInertiaZ;
        station.i2 = source.bendingInertiaY;
        station.i12 = source.productInertiaYZ;
        station.j = source.torsionConstant;
        station.k1 = card.shearFactorY;
        station.k2 = card.shearFactorZ;
        station.nsm = source.nonStructMass;
        for (std::size_t point = 0; point < 4; ++point)
            station.recovery[point] = {source.stressPoints[2 * point], source.stressPoints[2 * point + 1]};
        section.stations.push_back(station);
    }
    return section;
}

BeamSection beamLumpedAreaPropSection(const model::BeamLumpedAreaPropCard& card)
{
    BeamSection section;
    section.propertyType = "BeamLumpedAreaProp";
    section.materialId = card.matId;
    section.nsmCenterA = {card.nsmCenterY, card.nsmCenterZ};
    section.nsmCenterB = {card.nsmCenterY, card.nsmCenterZ};
    SectionProperties station;
    station.area = card.area;
    station.i1 = card.bendingInertia1;
    station.i2 = card.bendingInertia2;
    station.i12 = card.productInertia12;
    station.j = card.torsionConstant;
    station.k1 = card.shearFactor1;
    station.k2 = card.shearFactor2;
    station.nsm = card.nonStructMass;
    section.stations.push_back(station);
    return section;
}

BeamSection rodPropSection(const model::RodPropCard& card)
{
    BeamSection section;
    section.propertyType = "RodProp";
    section.materialId = card.matId;
    SectionProperties station;
    station.area = card.area;
    station.j = card.torsion;
    station.nsm = card.nonStructMass;
    section.stations.push_back(station);
    return section;
}

BeamSection tubePropSection(const model::TubePropCard& card)
{
    BeamSection section;
    section.propertyType = "TubeProp";
    section.materialId = card.matId;
    const double radius = card.outerDiameter / 2.0;
    // A zero thickness is a solid rod. A CTUBE carries no shear: the tube has no shear factors.
    SectionProperties station;
    if (card.thickness > 0.0 && card.thickness < radius)
    {
        const double dimensions[2] = {radius, radius - card.thickness};
        station = SectionFormulas::standard("Tube", dimensions, ShearFactorRule::None);
    }
    else
    {
        const double dimensions[1] = {radius};
        station = SectionFormulas::standard("Rod", dimensions, ShearFactorRule::None);
    }
    station.nsm = card.nonStructMass;
    section.stations.push_back(station);
    return section;
}

BeamSection curvedBeamPropSection(const model::CurvedBeamPropCard& card)
{
    BeamSection section;
    section.propertyType = "CurvedBeamProp";
    section.materialId = card.matId;
    SectionProperties station;
    station.nsm = card.nonStructMass;
    if (card.form == model::CurvedBeamPropForm::Section)
    {
        station.area = card.area;
        station.i1 = card.bendingInertia1;
        station.i2 = card.bendingInertia2;
        station.j = card.torsionConstant;
        station.k1 = card.shearFactor1 > 0.0 ? card.shearFactor1 : NAN_VALUE;
        station.k2 = card.shearFactor2 > 0.0 ? card.shearFactor2 : NAN_VALUE;
        for (std::size_t point = 0; point < 4; ++point)
            station.recovery[point] = {card.stressPoints[2 * point], card.stressPoints[2 * point + 1]};
    }
    else
    {
        // A thin pipe of mean radius RM and wall T: the area 2 pi RM T, the moment pi RM^3 T, twice that in torsion.
        station.area = 2.0 * std::numbers::pi * card.meanRadius * card.thickness;
        station.i1 = std::numbers::pi * card.meanRadius * card.meanRadius * card.meanRadius * card.thickness;
        station.i2 = station.i1;
        station.i12 = 0.0;
        station.j = 2.0 * station.i1;
    }
    section.stations.push_back(station);
    return section;
}

template <typename TableT, typename Builder>
std::optional<BeamSection> fromTable(const model::Model& model, model::CardLocation location, Builder&& build)
{
    const auto& table = static_cast<const TableT&>(*model.findTable(location.table));
    return build(table.card(location.row));
}

} // namespace

double BeamSection::meanArea() const
{
    std::vector<double> positions;
    std::vector<double> values;
    for (const SectionProperties& station : stations)
    {
        if (!station.hasArea())
            return NAN_VALUE;
        positions.push_back(station.xxb);
        values.push_back(station.area);
    }
    return integrateAlongBeam(positions, values).integral;
}

double BeamSection::meanNsm() const
{
    std::vector<double> positions;
    std::vector<double> values;
    for (const SectionProperties& station : stations)
    {
        positions.push_back(station.xxb);
        values.push_back(station.nsm);
    }
    return integrateAlongBeam(positions, values).integral;
}

LinearIntegral integrateAlongBeam(std::span<const double> positions, std::span<const double> values)
{
    std::vector<double> xs;
    std::vector<double> ys;
    withEnds(positions, values, xs, ys);
    LinearIntegral result;
    if (xs.empty())
        return result;
    double moment = 0.0;
    for (std::size_t index = 1; index < xs.size(); ++index)
    {
        const double start = xs[index - 1];
        const double end = xs[index];
        const double length = end - start;
        if (length <= 0.0)
            continue;
        result.integral += 0.5 * (ys[index - 1] + ys[index]) * length;
        // The integral of x times a value that is linear from ys[index - 1] at start to ys[index] at end.
        moment += length * (ys[index - 1] * (2.0 * start + end) + ys[index] * (start + 2.0 * end)) / 6.0;
    }
    result.centroid = result.integral != 0.0 ? moment / result.integral : 0.5;
    return result;
}

bool SectionFormulas::supports(std::uint16_t propertyType) noexcept
{
    return propertyType == model::typeIndexOf<model::BarPropTable>() ||
           propertyType == model::typeIndexOf<model::BarShapePropTable>() ||
           propertyType == model::typeIndexOf<model::BeamPropTable>() ||
           propertyType == model::typeIndexOf<model::BeamShapePropTable>() ||
           propertyType == model::typeIndexOf<model::BeamProp3Table>() ||
           propertyType == model::typeIndexOf<model::BeamLumpedAreaPropTable>() ||
           propertyType == model::typeIndexOf<model::RodPropTable>() ||
           propertyType == model::typeIndexOf<model::TubePropTable>() ||
           propertyType == model::typeIndexOf<model::CurvedBeamPropTable>();
}

namespace
{

// The section of a property row with its stations in the order of the card.
std::optional<BeamSection> unsortedSection(const model::Model& model, model::CardLocation location,
                                           ShearFactorRule rule, SectionCache* cache)
{
    const std::uint16_t type = location.table;
    if (model.findTable(type) == nullptr)
        return std::nullopt;
    if (type == model::typeIndexOf<model::BarPropTable>())
    {
        return fromTable<model::BarPropTable>(model, location,
                                              [](const model::BarPropCard& card)
                                              {
                                                  BeamSection section;
                                                  section.propertyType = "BarProp";
                                                  section.materialId = card.matId;
                                                  section.stations.push_back(barPropSection(card));
                                                  return std::optional<BeamSection>(section);
                                              });
    }
    if (type == model::typeIndexOf<model::BarShapePropTable>())
        return fromTable<model::BarShapePropTable>(
            model, location, [rule, cache](const model::BarShapePropCard& card)
            { return std::optional<BeamSection>(barShapePropSection(card, rule, cache)); });
    if (type == model::typeIndexOf<model::BeamPropTable>())
        return fromTable<model::BeamPropTable>(model, location, [](const model::BeamPropCard& card)
                                               { return std::optional<BeamSection>(beamPropSection(card)); });
    if (type == model::typeIndexOf<model::BeamShapePropTable>())
        return fromTable<model::BeamShapePropTable>(
            model, location, [rule, cache](const model::BeamShapePropCard& card)
            { return std::optional<BeamSection>(beamShapePropSection(card, rule, cache)); });
    if (type == model::typeIndexOf<model::BeamProp3Table>())
        return fromTable<model::BeamProp3Table>(model, location, [](const model::BeamProp3Card& card)
                                                { return std::optional<BeamSection>(beamProp3Section(card)); });
    if (type == model::typeIndexOf<model::BeamLumpedAreaPropTable>())
        return fromTable<model::BeamLumpedAreaPropTable>(
            model, location, [](const model::BeamLumpedAreaPropCard& card)
            { return std::optional<BeamSection>(beamLumpedAreaPropSection(card)); });
    if (type == model::typeIndexOf<model::RodPropTable>())
        return fromTable<model::RodPropTable>(model, location, [](const model::RodPropCard& card)
                                              { return std::optional<BeamSection>(rodPropSection(card)); });
    if (type == model::typeIndexOf<model::TubePropTable>())
        return fromTable<model::TubePropTable>(model, location, [](const model::TubePropCard& card)
                                               { return std::optional<BeamSection>(tubePropSection(card)); });
    if (type == model::typeIndexOf<model::CurvedBeamPropTable>())
        return fromTable<model::CurvedBeamPropTable>(
            model, location, [](const model::CurvedBeamPropCard& card)
            { return std::optional<BeamSection>(curvedBeamPropSection(card)); });
    return std::nullopt;
}

} // namespace

ShearFactorRule SectionFormulas::ruleOf(const model::Model& model)
{
    const model::ModelParamsTable* table = model.findTable<model::ModelParamsTable>();
    if (table == nullptr)
        return ShearFactorRule::Solver;
    // The last MDLPRM that gives TWBRBML (BeamLibraryShearFactor) decides (a model has one MDLPRM in practice).
    ShearFactorRule rule = ShearFactorRule::Solver;
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (!table->rows().isAlive(row))
            continue;
        for (const model::ModelParameter& parameter : table->card(row).parameters)
        {
            if (parameter.name == "BeamLibraryShearFactor" && parameter.kind == model::ModelParameterKind::Integer)
                rule = parameter.integerValue == 1 ? ShearFactorRule::BeamLibrary : ShearFactorRule::Solver;
        }
    }
    return rule;
}

std::optional<BeamSection> SectionFormulas::of(const model::Model& model, model::CardLocation location)
{
    return of(model, location, ruleOf(model));
}

std::optional<BeamSection> SectionFormulas::of(const model::Model& model, model::CardLocation location,
                                               ShearFactorRule rule, SectionCache* cache)
{
    std::optional<BeamSection> section = unsortedSection(model, location, rule, cache);
    if (section)
    {
        std::stable_sort(section->stations.begin(), section->stations.end(),
                         [](const SectionProperties& left, const SectionProperties& right)
                         { return left.xxb < right.xxb; });
    }
    return section;
}

std::optional<BeamSection> SectionFormulas::of(const model::Model& model, base::EntityId pid)
{
    const auto location = model.locate(base::IdSpace::Property, pid);
    if (!location)
        return std::nullopt;
    return of(model, *location);
}

} // namespace oor::analysis
