#include "oor/model/cards/properties/beamProp/BeamPropDesignFields.hpp"

#include "oor/model/cards/properties/beamProp/BeamPropTable.hpp"
#include "oor/model/design/DesignFieldNames.hpp"
#include "oor/model/design/TableDesignFields.hpp"

#include <algorithm>
#include <array>
#include <numeric>

namespace oor::model
{

namespace
{

// The fields of a station are kind StationField with `index` 0 to 13 in the order A, I1, I2, I12, J, NSM, C1, C2, D1,
// D2, E1, E2, F1, F2 and `station` the position in the stations of the card. The fields of the whole beam have a kind
// each.
enum BeamPropKind : std::int32_t
{
    StationField,
    ShearFactor1,
    ShearFactor2,
    ShearRelief1,
    ShearRelief2,
    NonstructuralInertiaA,
    NonstructuralInertiaB,
    WarpingA,
    WarpingB,
    NonstructuralCenter1A,
    NonstructuralCenter2A,
    NonstructuralCenter1B,
    NonstructuralCenter2B,
    NeutralAxis1A,
    NeutralAxis2A,
    NeutralAxis1B,
    NeutralAxis2B,
};

// The neutral field descriptors a relation can drive.
constexpr const char* DESIGNABLE =
    "area, bendingInertia1, bendingInertia2, productInertia12, "
    "torsionConstant, nonStructMass, stressPointCy to stressPointFz with a station "
    "(EndA), (EndB) or (i), and shearFactor1, shearFactor2, shearRelief1, shearRelief2, "
    "nsmInertia(EndA), nsmInertia(EndB), warpingCoefficient(EndA), warpingCoefficient(EndB), "
    "nsmCenterY(EndA), nsmCenterZ(EndA), nsmCenterY(EndB), nsmCenterZ(EndB), "
    "neutralAxisY(EndA), neutralAxisZ(EndA), neutralAxisY(EndB), neutralAxisZ(EndB)";
constexpr const char* STATION_NAMES[14] = {
    "area",          "bendingInertia1", "bendingInertia2", "productInertia12", "torsionConstant",
    "nonStructMass", "stressPointCy",   "stressPointCz",   "stressPointDy",    "stressPointDz",
    "stressPointEy", "stressPointEz",   "stressPointFy",   "stressPointFz"};

DesignFieldFacts stationFacts(int index, int station, const std::string& stationText)
{
    using enum DesignLowerLimit;
    DesignLowerLimit lower = StressRecovery; // C1 to F2
    if (index == 3)
        lower = Signed; // I12
    else if (index <= 5)
        lower = Positive; // A, I1, I2, J, NSM
    const DesignMassEffect mass =
        index == 0 || index == 5 ? DesignMassEffect::Linear : DesignMassEffect::None; // A, NSM
    return designFacts(StationField, std::string(STATION_NAMES[index]) + "(" + stationText + ")", lower, mass, index,
                       station);
}

// The end B of a card is the station at stationRatio = 1.0 other than end A.
std::optional<std::size_t> endBStation(const BeamPropCard& card)
{
    for (std::size_t index = 1; index < card.stations.size(); ++index)
    {
        if (card.stations[index].stationRatio == 1.0)
            return index;
    }
    return std::nullopt;
}

std::string stationRefusal(const BeamPropCard& card, const StationName& station)
{
    if (station.atEndB)
        return "this BeamProp has no station at X/XB = 1.0, so there is no end B to design (it has " +
               std::to_string(card.stations.size()) + (card.stations.size() == 1 ? " station)" : " stations)");
    return "this BeamProp has no station " + std::to_string(station.intermediate) + " in record order (it has " +
           std::to_string(card.stations.size()) + " stations)";
}

// EndA is the first station; EndB is the station at ratio 1. Numeric positions include EndA as position 1.
std::optional<std::size_t> stationOf(const BeamPropCard& card, const StationName& station)
{
    if (!station.hasStation || station.atEndA)
        return 0;
    if (station.atEndB)
        return endBStation(card);
    const std::size_t index = static_cast<std::size_t>(station.intermediate - 1);
    return index < card.stations.size() ? std::optional<std::size_t>(index) : std::nullopt;
}

std::string stationText(const StationName& station)
{
    if (!station.hasStation || station.atEndA)
        return "EndA";
    return station.atEndB ? "EndB" : std::to_string(station.intermediate);
}

DesignFieldLookup findByName(const BeamPropCard& card, const DesignField& field)
{
    using enum DesignLowerLimit;
    const std::optional<StationName> parsed = splitStation(field.name);
    if (!parsed)
        return DesignFieldLookup::refused(refusalOfField("BeamProp", field, DESIGNABLE));
    const StationName& station = *parsed;
    for (int index = 0; index < 14; ++index)
    {
        if (station.word != STATION_NAMES[index])
            continue;
        const std::optional<std::size_t> found = stationOf(card, station);
        if (!found)
            return DesignFieldLookup::refused(field.name + ": " + stationRefusal(card, station));
        return DesignFieldLookup::found(stationFacts(index, static_cast<int>(*found), stationText(station)));
    }
    // The fields of the whole beam. K1, K2, S1 and S2 have no station; the others name end A or end B.
    if (!station.hasStation)
    {
        if (station.word == "shearFactor1")
            return DesignFieldLookup::found(designFacts(ShearFactor1, "shearFactor1", Positive));
        if (station.word == "shearFactor2")
            return DesignFieldLookup::found(designFacts(ShearFactor2, "shearFactor2", Positive));
        if (station.word == "shearRelief1")
            return DesignFieldLookup::found(designFacts(ShearRelief1, "shearRelief1", Signed));
        if (station.word == "shearRelief2")
            return DesignFieldLookup::found(designFacts(ShearRelief2, "shearRelief2", Signed));
        return DesignFieldLookup::refused(refusalOfField("BeamProp", field, DESIGNABLE));
    }
    if (!station.atEndA && !station.atEndB)
        return DesignFieldLookup::refused(refusalOfField("BeamProp", field, DESIGNABLE));
    const bool endB = station.atEndB;
    const std::string name = station.word + (endB ? "(EndB)" : "(EndA)");
    const auto global = [&](std::int32_t kindA, std::int32_t kindB)
    { return DesignFieldLookup::found(designFacts(endB ? kindB : kindA, name, Signed)); };
    if (station.word == "nsmInertia")
        return global(NonstructuralInertiaA, NonstructuralInertiaB);
    if (station.word == "warpingCoefficient")
        return global(WarpingA, WarpingB);
    if (station.word == "nsmCenterY")
        return global(NonstructuralCenter1A, NonstructuralCenter1B);
    if (station.word == "nsmCenterZ")
        return global(NonstructuralCenter2A, NonstructuralCenter2B);
    if (station.word == "neutralAxisY")
        return global(NeutralAxis1A, NeutralAxis1B);
    if (station.word == "neutralAxisZ")
        return global(NeutralAxis2A, NeutralAxis2B);
    return DesignFieldLookup::refused(refusalOfField("BeamProp", field, DESIGNABLE));
}

// The fields of end A by position: PID is 2, MID 3, A 4, I1 5, I2 6, I12 7, J 8, NSM 9; the line of stress points of
// the station is 12 to 19.

// A negative FID is the word of the element property table: the stations come in order of X/XB, sixteen words each from
// word 6 (SO, X/XB, A, I1, I2, I12, J, NSM, C1 to F2), so -8 is the area of the first station.

DesignFieldLookup findField(const BeamPropCard& card, const DesignField& field)
{
    return findByName(card, field);
}

double stationValue(const BeamPropStation& station, int index)
{
    switch (index)
    {
    case 0:
        return station.area;
    case 1:
        return station.bendingInertia1;
    case 2:
        return station.bendingInertia2;
    case 3:
        return station.productInertia12;
    case 4:
        return station.torsionConstant;
    case 5:
        return station.nonStructMass;
    default:
    {
        const std::array<double, 2>* const pairs[4] = {&station.stressPointC, &station.stressPointD,
                                                       &station.stressPointE, &station.stressPointF};
        const int stress = index - 6;
        return (*pairs[stress / 2])[static_cast<std::size_t>(stress % 2)];
    }
    }
}

void setStationValue(BeamPropStation& station, int index, double value)
{
    switch (index)
    {
    case 0:
        station.area = value;
        break;
    case 1:
        station.bendingInertia1 = value;
        break;
    case 2:
        station.bendingInertia2 = value;
        break;
    case 3:
        station.productInertia12 = value;
        break;
    case 4:
        station.torsionConstant = value;
        break;
    case 5:
        station.nonStructMass = value;
        break;
    default:
    {
        std::array<double, 2>* const pairs[4] = {&station.stressPointC, &station.stressPointD, &station.stressPointE,
                                                 &station.stressPointF};
        const int stress = index - 6;
        (*pairs[stress / 2])[static_cast<std::size_t>(stress % 2)] = value;
        break;
    }
    }
}

// The place of a field of the whole beam in the card.
template <typename CardT>
auto& globalField(CardT& card, std::int32_t kind)
{
    switch (kind)
    {
    case ShearFactor1:
        return card.shearFactor1;
    case ShearFactor2:
        return card.shearFactor2;
    case ShearRelief1:
        return card.shearRelief1;
    case ShearRelief2:
        return card.shearRelief2;
    case NonstructuralInertiaA:
        return card.nsmInertiaA;
    case NonstructuralInertiaB:
        return card.nsmInertiaB;
    case WarpingA:
        return card.warpingCoefficientA;
    case WarpingB:
        return card.warpingCoefficientB;
    case NonstructuralCenter1A:
        return card.nsmCenterYA;
    case NonstructuralCenter2A:
        return card.nsmCenterZA;
    case NonstructuralCenter1B:
        return card.nsmCenterYB;
    case NonstructuralCenter2B:
        return card.nsmCenterZB;
    case NeutralAxis1A:
        return card.neutralAxisYA;
    case NeutralAxis2A:
        return card.neutralAxisZA;
    case NeutralAxis1B:
        return card.neutralAxisYB;
    default:
        return card.neutralAxisZB;
    }
}

double readField(const BeamPropCard& card, const DesignFieldFacts& facts)
{
    if (facts.kind == StationField)
        return stationValue(card.stations[static_cast<std::size_t>(facts.station)], facts.index);
    return globalField(card, facts.kind);
}

void writeFields(BeamPropCard& card, std::span<const DesignFieldValue> values)
{
    for (const DesignFieldValue& item : values)
    {
        if (item.facts->kind == StationField)
            setStationValue(card.stations[static_cast<std::size_t>(item.facts->station)], item.facts->index,
                            item.value);
        else
            globalField(card, item.facts->kind) = item.value;
    }
}

// The contract columns: the station lists station_section (A, I1, I2, I12, J, NSM of each station) and station_stress
// (C1, C2, D1, D2, E1, E2, F1, F2 of each station), in the order of the stations of the card, and the pairs and
// quadruples of the whole beam: k (K1 K2), s (S1 S2), nsi and cw (end A, end B), m (M1A M2A M1B M2B), n (N1A N2A N1B
// N2B).
DesignColumn columnOf(const BeamPropCard&, const DesignFieldFacts& facts)
{
    constexpr std::int32_t SECTION_WIDTH = 6;
    constexpr std::int32_t STRESS_WIDTH = 8;
    switch (facts.kind)
    {
    case StationField:
        if (facts.index < SECTION_WIDTH)
            return {"stationSection", facts.station * SECTION_WIDTH + facts.index};
        return {"stationStress", facts.station * STRESS_WIDTH + facts.index - SECTION_WIDTH};
    case ShearFactor1:
        return {"shearFactor", 0};
    case ShearFactor2:
        return {"shearFactor", 1};
    case ShearRelief1:
        return {"shearRelief", 0};
    case ShearRelief2:
        return {"shearRelief", 1};
    case NonstructuralInertiaA:
        return {"nsmInertia", 0};
    case NonstructuralInertiaB:
        return {"nsmInertia", 1};
    case WarpingA:
        return {"warpingCoefficient", 0};
    case WarpingB:
        return {"warpingCoefficient", 1};
    case NonstructuralCenter1A:
        return {"nsmCenter", 0};
    case NonstructuralCenter2A:
        return {"nsmCenter", 1};
    case NonstructuralCenter1B:
        return {"nsmCenter", 2};
    case NonstructuralCenter2B:
        return {"nsmCenter", 3};
    case NeutralAxis1A:
        return {"neutralAxis", 0};
    case NeutralAxis2A:
        return {"neutralAxis", 1};
    case NeutralAxis1B:
        return {"neutralAxis", 2};
    default:
        return {"neutralAxis", 3};
    }
}

} // namespace

const DesignableCard& beamPropDesignFields()
{
    static const TableDesignFields<BeamPropTable> FIELDS("BeamProp", {&findField, &readField, &writeFields, &columnOf});
    return FIELDS;
}

} // namespace oor::model
