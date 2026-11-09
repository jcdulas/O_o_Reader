#include "oor/model/cards/properties/beamProp3/BeamProp3Table.hpp"

#include "oor/base/text/TextUtilities.hpp"

namespace oor::model
{

namespace
{

// The first field of what each station has, in the format reference numbering (see BeamProp3Card).
constexpr int stressField(std::size_t station)
{
    return 9 + 16 * static_cast<int>(station); // CY .. FZ
}
constexpr int stressOutputField(std::size_t station)
{
    return 17 + 16 * (static_cast<int>(station) - 1); // SO(B), SO(C)
}
constexpr int sectionField(std::size_t station)
{
    return station == 0 ? 3 : stressOutputField(station) + 2; // A IZ IY IYZ J NSM
}
constexpr int NEUTRAL_FIELD = 51;
constexpr int MASS_FIELD = 57;
constexpr int NSM_INERTIA_FIELD = 63;
constexpr int WARPING_COEFFICIENT_FIELD = 72;
constexpr int STRESS_FIELD = 75;
constexpr int WARPING_FIELD = 81;
constexpr int LAST_FIELD = 116;

// The fields from `first` to `last` must be blank.
void requireBlank(FieldReader& reader, int first, int last, const char* name)
{
    for (int field = first; field <= last; ++field)
    {
        if (!reader.isBlank(field))
        {
            reader.error("card.bad-value", "this field of a " + std::string(reader.cardName()) + " must be blank",
                         name);
            return;
        }
    }
}

} // namespace

std::optional<BeamProp3Card> BeamProp3Format::read(FieldReader& reader)
{
    static constexpr const char* STRESS_NAMES[8] = {"stressPointCy", "stressPointCz", "stressPointDy", "stressPointDz",
                                                    "stressPointEy", "stressPointEz", "stressPointFy", "stressPointFz"};
    static constexpr const char* WARPING_NAMES[12] = {"warpingAtC", "warpingGradientYAtC", "warpingGradientZAtC",
                                                      "warpingAtD", "warpingGradientYAtD", "warpingGradientZAtD",
                                                      "warpingAtE", "warpingGradientYAtE", "warpingGradientZAtE",
                                                      "warpingAtF", "warpingGradientYAtF", "warpingGradientZAtF"};

    BeamProp3Card card;
    card.id = reader.requiredInteger(1, "id");
    card.matId = reader.requiredInteger(2, "matId");

    // End A.
    BeamProp3Station& endA = card.stations[0];
    endA.area = reader.requiredReal(3, "area");
    endA.bendingInertiaZ = reader.requiredReal(4, "bendingInertiaZ");
    endA.bendingInertiaY = reader.requiredReal(5, "bendingInertiaY");
    endA.productInertiaYZ = reader.real(6, "productInertiaYZ", 0.0);
    // format reference: the sum of the two moments of inertia, stored resolved
    endA.torsionConstant = reader.real(7, "torsionConstant", endA.bendingInertiaZ + endA.bendingInertiaY);
    endA.nonStructMass = reader.real(8, "nonStructMass", 0.0);

    // Ends B and C: a blank value is the one of end A, except for the stress points.
    for (std::size_t station = 1; station < 3; ++station)
    {
        BeamProp3Station& end = card.stations[station];
        if (const std::string_view stressOutput = reader.text(stressOutputField(station)); (stressOutput == "Yes"))
            end.stressOutput = true;
        else if (!stressOutput.empty() && !(stressOutput == "YesSameAsEndA"))
            reader.error("card.bad-value",
                         std::string("stressOutput") + " must be YES, YESA or blank, found \"" +
                             std::string(stressOutput) + "\"",
                         "stressOutput");
        requireBlank(reader, stressOutputField(station) + 1, stressOutputField(station) + 1, "stressOutput");
        const int first = sectionField(station);
        end.area = reader.real(first, "area", endA.area);
        end.bendingInertiaZ = reader.real(first + 1, "bendingInertiaZ", endA.bendingInertiaZ);
        end.bendingInertiaY = reader.real(first + 2, "bendingInertiaY", endA.bendingInertiaY);
        end.productInertiaYZ = reader.real(first + 3, "productInertiaYZ", endA.productInertiaYZ);
        end.torsionConstant = reader.real(first + 4, "torsionConstant", endA.torsionConstant);
        end.nonStructMass = reader.real(first + 5, "nonStructMass", endA.nonStructMass);
    }
    for (std::size_t station = 0; station < 3; ++station)
    {
        for (int index = 0; index < 8; ++index)
            card.stations[station].stressPoints[static_cast<std::size_t>(index)] =
                reader.real(stressField(station) + index, STRESS_NAMES[index], 0.0);
    }

    card.shearFactorY = reader.real(49, "shearFactorY", 1.0);
    card.shearFactorZ = reader.real(50, "shearFactorZ", 1.0);
    for (std::size_t station = 0; station < 3; ++station)
    {
        BeamProp3Station& end = card.stations[station];
        const BeamProp3Station& endA2 = card.stations[0];
        const int offset = static_cast<int>(station);
        end.neutralAxis[0] = reader.real(NEUTRAL_FIELD + 2 * offset, "neutralAxisY", endA2.neutralAxis[0]);
        end.neutralAxis[1] = reader.real(NEUTRAL_FIELD + 2 * offset + 1, "neutralAxisZ", endA2.neutralAxis[1]);
    }
    for (std::size_t station = 0; station < 3; ++station)
    {
        BeamProp3Station& end = card.stations[station];
        const BeamProp3Station& endA2 = card.stations[0];
        const int offset = static_cast<int>(station);
        end.nsmCenter[0] = reader.real(MASS_FIELD + 2 * offset, "nsmCenterY", endA2.nsmCenter[0]);
        end.nsmCenter[1] = reader.real(MASS_FIELD + 2 * offset + 1, "nsmCenterZ", endA2.nsmCenter[1]);
    }
    for (std::size_t station = 0; station < 3; ++station)
    {
        BeamProp3Station& end = card.stations[station];
        const BeamProp3Station& endA2 = card.stations[0];
        const int offset = static_cast<int>(station);
        end.nsmInertia[0] = reader.real(NSM_INERTIA_FIELD + 3 * offset, "nsmInertiaY", endA2.nsmInertia[0]);
        end.nsmInertia[1] = reader.real(NSM_INERTIA_FIELD + 3 * offset + 1, "nsmInertiaZ", endA2.nsmInertia[1]);
        end.nsmInertia[2] = reader.real(NSM_INERTIA_FIELD + 3 * offset + 2, "nsmInertiaYZ", endA2.nsmInertia[2]);
    }
    for (std::size_t station = 0; station < 3; ++station)
        card.stations[station].warpingCoefficient =
            reader.real(WARPING_COEFFICIENT_FIELD + static_cast<int>(station), "warpingCoefficient",
                        card.stations[0].warpingCoefficient);
    if (const std::string_view stress = reader.text(STRESS_FIELD); (stress == "GaussPoints"))
        card.gaussStress = true;
    else if (!stress.empty() && !(stress == "Nodes"))
        reader.error(
            "card.bad-value",
            std::string("stressLocation") + " must be GRID, GAUSS or blank, found \"" + std::string(stress) + "\"",
            "stressLocation");
    requireBlank(reader, STRESS_FIELD + 1, WARPING_FIELD - 1, "stressLocation");
    for (int field = WARPING_FIELD; field < WARPING_FIELD + 36; ++field)
        card.warpingGiven = card.warpingGiven || !reader.isBlank(field);
    for (std::size_t station = 0; station < 3; ++station)
    {
        for (int index = 0; index < 12; ++index)
        {
            card.stations[station].warping[static_cast<std::size_t>(index)] =
                reader.real(WARPING_FIELD + 12 * static_cast<int>(station) + index, WARPING_NAMES[index],
                            card.stations[0].warping[static_cast<std::size_t>(index)]);
        }
    }
    if (reader.lastUsedField() > LAST_FIELD)
        reader.error("card.bad-value",
                     "a " + std::string(reader.cardName()) + " has no field after the warping of node C",
                     "warpingGradientZAtF");
    if (!reader.failed())
        reader.report(check(card));
    return card;
}

std::optional<CardProblem> BeamProp3Format::check(const BeamProp3Card& card)
{
    if (card.id <= 0)
        return CardProblem{"card.bad-id", "the property ID must be a positive integer", "id"};
    if (card.matId <= 0)
        return CardProblem{"card.bad-id", "the material ID must be a positive integer", "matId"};
    if (card.stations[0].stressOutput)
        return CardProblem{"card.bad-value", "end A has no " + std::string(CardProblem::FIELD) + " field",
                           "stressOutput"};
    for (const BeamProp3Station& station : card.stations)
    {
        if (!(station.area > 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0", "area"};
        if (!(station.bendingInertiaZ > 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                               "bendingInertiaZ"};
        if (!(station.bendingInertiaY > 0.0))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                               "bendingInertiaY"};
        if (station.productInertiaYZ * station.productInertiaYZ > station.bendingInertiaZ * station.bendingInertiaY)
            return CardProblem{"card.bad-value",
                               std::string(CardProblem::FIELD) + " squared must not exceed " +
                                   CardProblem::fieldNamed("bendingInertiaZ") + " times " +
                                   CardProblem::fieldNamed("bendingInertiaY"),
                               "productInertiaYZ"};
        if (station.torsionConstant < 0.0)
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must not be negative",
                               "torsionConstant"};
    }
    if (!card.warpingGiven)
    {
        for (const BeamProp3Station& station : card.stations)
        {
            for (const double value : station.warping)
            {
                if (value != 0.0)
                    return CardProblem{"card.bad-value", "a warping value needs the warping fields to be given",
                                       "warpingAtC"};
            }
        }
    }
    if (!(card.shearFactorY > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "shearFactorY"};
    if (!(card.shearFactorZ > 0.0))
        return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be greater than 0.0",
                           "shearFactorZ"};
    return std::nullopt;
}

void BeamProp3Format::write(const BeamProp3Card& card, FieldWriter& writer)
{
    writer.integer(1, card.id);
    writer.integer(2, card.matId);
    const BeamProp3Station& endA = card.stations[0];
    writer.real(3, endA.area);
    writer.real(4, endA.bendingInertiaZ);
    writer.real(5, endA.bendingInertiaY);
    writer.realUnlessDefault(6, endA.productInertiaYZ, 0.0);
    writer.real(7, endA.torsionConstant); // its default depends on IZ and IY: always written
    writer.realUnlessDefault(8, endA.nonStructMass, 0.0);
    for (std::size_t station = 1; station < 3; ++station)
    {
        const BeamProp3Station& end = card.stations[station];
        if (end.stressOutput)
            writer.text(stressOutputField(station), "Yes");
        // What a blank value of B or C takes from end A is always written, so that the station does not move with A.
        const int first = sectionField(station);
        writer.real(first, end.area);
        writer.real(first + 1, end.bendingInertiaZ);
        writer.real(first + 2, end.bendingInertiaY);
        writer.real(first + 3, end.productInertiaYZ);
        writer.real(first + 4, end.torsionConstant);
        writer.real(first + 5, end.nonStructMass);
    }
    for (std::size_t station = 0; station < 3; ++station)
    {
        for (int index = 0; index < 8; ++index)
            writer.realUnlessDefault(stressField(station) + index,
                                     card.stations[station].stressPoints[static_cast<std::size_t>(index)], 0.0);
    }
    writer.realUnlessDefault(49, card.shearFactorY, 1.0);
    writer.realUnlessDefault(50, card.shearFactorZ, 1.0);
    // The values of the three stations that share a field list: end A may leave its blank, B and C always write theirs.
    const auto put = [&](int field, std::size_t station, double value)
    {
        if (station == 0)
            writer.realUnlessDefault(field, value, 0.0);
        else
            writer.real(field, value);
    };
    for (std::size_t station = 0; station < 3; ++station)
    {
        const BeamProp3Station& end = card.stations[station];
        const int offset = static_cast<int>(station);
        put(NEUTRAL_FIELD + 2 * offset, station, end.neutralAxis[0]);
        put(NEUTRAL_FIELD + 2 * offset + 1, station, end.neutralAxis[1]);
        put(MASS_FIELD + 2 * offset, station, end.nsmCenter[0]);
        put(MASS_FIELD + 2 * offset + 1, station, end.nsmCenter[1]);
        put(NSM_INERTIA_FIELD + 3 * offset, station, end.nsmInertia[0]);
        put(NSM_INERTIA_FIELD + 3 * offset + 1, station, end.nsmInertia[1]);
        put(NSM_INERTIA_FIELD + 3 * offset + 2, station, end.nsmInertia[2]);
        put(WARPING_COEFFICIENT_FIELD + offset, station, end.warpingCoefficient);
    }
    if (card.gaussStress)
        writer.text(STRESS_FIELD, "GaussPoints");
    for (std::size_t station = 0; station < 3 && card.warpingGiven; ++station)
    {
        for (int index = 0; index < 12; ++index)
            put(WARPING_FIELD + 12 * static_cast<int>(station) + index, station,
                card.stations[station].warping[static_cast<std::size_t>(index)]);
    }
}

namespace
{

// The values of the three stations, as the columns store them.
struct StationArrays
{
    std::array<double, 18> section = {};
    std::array<double, 24> stressPoints = {};
    std::array<std::uint8_t, 2> stressOutput = {};
    std::array<double, 6> neutralAxis = {};
    std::array<double, 6> nsmCenter = {};
    std::array<double, 9> nsmInertia = {};
    std::array<double, 3> warpingCoefficient = {};
    std::array<double, 36> warping = {};
};

StationArrays arraysOf(const BeamProp3Card& card)
{
    StationArrays arrays;
    for (std::size_t station = 0; station < 3; ++station)
    {
        const BeamProp3Station& end = card.stations[station];
        const std::array<double, 6> section = {
            end.area,         end.bendingInertiaZ, end.bendingInertiaY, end.productInertiaYZ, end.torsionConstant,
            end.nonStructMass};
        for (std::size_t index = 0; index < 6; ++index)
            arrays.section[6 * station + index] = section[index];
        for (std::size_t index = 0; index < 8; ++index)
            arrays.stressPoints[8 * station + index] = end.stressPoints[index];
        if (station > 0)
            arrays.stressOutput[station - 1] = end.stressOutput ? 1 : 0;
        arrays.neutralAxis[2 * station] = end.neutralAxis[0];
        arrays.neutralAxis[2 * station + 1] = end.neutralAxis[1];
        arrays.nsmCenter[2 * station] = end.nsmCenter[0];
        arrays.nsmCenter[2 * station + 1] = end.nsmCenter[1];
        for (std::size_t index = 0; index < 3; ++index)
            arrays.nsmInertia[3 * station + index] = end.nsmInertia[index];
        arrays.warpingCoefficient[station] = end.warpingCoefficient;
        for (std::size_t index = 0; index < 12; ++index)
            arrays.warping[12 * station + index] = end.warping[index];
    }
    return arrays;
}

} // namespace

std::uint32_t BeamProp3Table::append(const BeamProp3Card& card, base::CardKey key)
{
    const std::uint32_t row = m_rows.append(key);
    m_id.push(card.id);
    m_matId.push(card.matId);
    const StationArrays arrays = arraysOf(card);
    m_section.push(arrays.section);
    m_stressPoints.push(arrays.stressPoints);
    m_stressOutput.push(arrays.stressOutput);
    m_shearFactor.push({card.shearFactorY, card.shearFactorZ});
    m_neutralAxis.push(arrays.neutralAxis);
    m_nsmCenter.push(arrays.nsmCenter);
    m_nsmInertia.push(arrays.nsmInertia);
    m_warpingCoefficient.push(arrays.warpingCoefficient);
    m_gaussStress.push(card.gaussStress ? 1 : 0);
    m_warpingGiven.push(card.warpingGiven ? 1 : 0);
    m_warping.push(arrays.warping);
    return row;
}

BeamProp3Card BeamProp3Table::card(std::uint32_t row) const
{
    BeamProp3Card card;
    card.id = m_id.get(row);
    card.matId = m_matId.get(row);
    const std::array<double, 18> section = m_section.get(row);
    const std::array<double, 24> stress = m_stressPoints.get(row);
    const std::array<std::uint8_t, 2> output = m_stressOutput.get(row);
    const std::array<double, 6> neutral = m_neutralAxis.get(row);
    const std::array<double, 6> mass = m_nsmCenter.get(row);
    const std::array<double, 9> nsmInertia = m_nsmInertia.get(row);
    const std::array<double, 3> warpingCoefficient = m_warpingCoefficient.get(row);
    const std::array<double, 36> warping = m_warping.get(row);
    for (std::size_t station = 0; station < 3; ++station)
    {
        BeamProp3Station& end = card.stations[station];
        end.area = section[6 * station];
        end.bendingInertiaZ = section[6 * station + 1];
        end.bendingInertiaY = section[6 * station + 2];
        end.productInertiaYZ = section[6 * station + 3];
        end.torsionConstant = section[6 * station + 4];
        end.nonStructMass = section[6 * station + 5];
        for (std::size_t index = 0; index < 8; ++index)
            end.stressPoints[index] = stress[8 * station + index];
        end.stressOutput = station > 0 && output[station - 1] != 0;
        end.neutralAxis = {neutral[2 * station], neutral[2 * station + 1]};
        end.nsmCenter = {mass[2 * station], mass[2 * station + 1]};
        end.nsmInertia = {nsmInertia[3 * station], nsmInertia[3 * station + 1], nsmInertia[3 * station + 2]};
        end.warpingCoefficient = warpingCoefficient[station];
        for (std::size_t index = 0; index < 12; ++index)
            end.warping[index] = warping[12 * station + index];
    }
    const std::array<double, 2> shear = m_shearFactor.get(row);
    card.shearFactorY = shear[0];
    card.shearFactorZ = shear[1];
    card.gaussStress = m_gaussStress.get(row) != 0;
    card.warpingGiven = m_warpingGiven.get(row) != 0;
    return card;
}

void BeamProp3Table::store(std::uint32_t row, const BeamProp3Card& card)
{
    m_id.set(row, card.id);
    m_matId.set(row, card.matId);
    const StationArrays arrays = arraysOf(card);
    m_section.set(row, arrays.section);
    m_stressPoints.set(row, arrays.stressPoints);
    m_stressOutput.set(row, arrays.stressOutput);
    m_shearFactor.set(row, {card.shearFactorY, card.shearFactorZ});
    m_neutralAxis.set(row, arrays.neutralAxis);
    m_nsmCenter.set(row, arrays.nsmCenter);
    m_nsmInertia.set(row, arrays.nsmInertia);
    m_warpingCoefficient.set(row, arrays.warpingCoefficient);
    m_gaussStress.set(row, card.gaussStress ? 1 : 0);
    m_warpingGiven.set(row, card.warpingGiven ? 1 : 0);
    m_warping.set(row, arrays.warping);
}

void BeamProp3Table::keepRows(const std::vector<std::uint32_t>& rows)
{
    m_id.keepRows(rows);
    m_matId.keepRows(rows);
    m_section.keepRows(rows);
    m_stressPoints.keepRows(rows);
    m_stressOutput.keepRows(rows);
    m_shearFactor.keepRows(rows);
    m_neutralAxis.keepRows(rows);
    m_nsmCenter.keepRows(rows);
    m_nsmInertia.keepRows(rows);
    m_warpingCoefficient.keepRows(rows);
    m_gaussStress.keepRows(rows);
    m_warpingGiven.keepRows(rows);
    m_warping.keepRows(rows);
}

void BeamProp3Table::visitReferences(std::uint32_t row, ReferenceVisitor& visitor)
{
    if (const base::EntityId matId = m_matId.get(row); matId > 0)
        m_matId.set(row, visitor.visit("matId", ReferenceTarget::Material, matId));
}

} // namespace oor::model
