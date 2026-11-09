#include "oor/analysis/quality/GeometryCheckThresholds.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <string>

namespace oor::analysis
{

std::string_view qualityCheckName(QualityCheck check) noexcept
{
    switch (check)
    {
    case QualityCheck::AspectRatio:
        return "aspectRatio";
    case QualityCheck::Skew:
        return "skew";
    case QualityCheck::Warp:
        return "warp";
    case QualityCheck::Taper:
        return "taper";
    case QualityCheck::MinAngle:
        return "minAngle";
    case QualityCheck::MaxAngle:
        return "maxAngle";
    case QualityCheck::Jacobian:
        return "jacobian";
    case QualityCheck::EdgePointLength:
        return "edgePointLengthRatio";
    case QualityCheck::EdgePointAngle:
        return "edgePointAngle";
    case QualityCheck::FaceWarp:
        return "faceWarp";
    }
    return {};
}

const std::vector<QualityCheck>& allQualityChecks()
{
    static const std::vector<QualityCheck> CHECKS = {
        QualityCheck::AspectRatio,    QualityCheck::Skew,     QualityCheck::Warp,     QualityCheck::Taper,
        QualityCheck::MinAngle,       QualityCheck::MaxAngle, QualityCheck::Jacobian, QualityCheck::EdgePointLength,
        QualityCheck::EdgePointAngle, QualityCheck::FaceWarp};
    return CHECKS;
}

bool GeometryCheckThresholds::set(std::string_view keyword, double value) noexcept
{
    const std::string_view key = keyword;
    if (key == "QuadSkewLimit")
        q4Skew = value;
    else if (key == "QuadTaperLimit")
        q4Taper = value;
    else if (key == "QuadWarpLimit")
        q4Warp = value;
    else if (key == "QuadMinAngle")
        q4Iamin = value;
    else if (key == "QuadMaxAngle")
        q4Iamax = value;
    else if (key == "TriangleMinAngle")
        t3Skew = value;
    else if (key == "TriangleMaxAngle")
        t3Iamax = value;
    else if (key == "TetrahedronAspectRatioLimit")
        tetAr = value;
    else if (key == "TetrahedronEdgePointLengthRatioLimit")
        tetEplr = value;
    else if (key == "TetrahedronEdgePointAngleLimit")
        tetEpia = value;
    else if (key == "TetrahedronJacobianLimit")
        tetDetj = value;
    else if (key == "TetrahedronGlobalJacobianLimit")
        tetDetg = value;
    else if (key == "HexahedronAspectRatioLimit")
        hexAr = value;
    else if (key == "HexahedronEdgePointLengthRatioLimit")
        hexEplr = value;
    else if (key == "HexahedronEdgePointAngleLimit")
        hexEpia = value;
    else if (key == "HexahedronJacobianLimit")
        hexDetj = value;
    else if (key == "HexahedronWarpLimit")
        hexWarp = value;
    else if (key == "PentahedronAspectRatioLimit")
        penAr = value;
    else if (key == "PentahedronEdgePointLengthRatioLimit")
        penEplr = value;
    else if (key == "PentahedronEdgePointAngleLimit")
        penEpia = value;
    else if (key == "PentahedronJacobianLimit")
        penDetj = value;
    else if (key == "PentahedronWarpLimit")
        penWarp = value;
    else if (key == "PyramidAspectRatioLimit")
        pyrAr = value;
    else if (key == "PyramidEdgePointLengthRatioLimit")
        pyrEplr = value;
    else if (key == "PyramidEdgePointAngleLimit")
        pyrEpia = value;
    else if (key == "PyramidJacobianLimit")
        pyrDetj = value;
    else if (key == "PyramidWarpLimit")
        pyrWarp = value;
    else
        return false;
    return true;
}

GeometryCheckThresholds GeometryCheckThresholds::fromModel(const model::Model& model)
{
    GeometryCheckThresholds thresholds;
    for (const model::ControlStatement* statement : model.control().executive.findAll("GeometryCheck"))
    {
        for (const model::StatementParameter& parameter : statement->parameters)
        {
            if (!parameter.hasValue)
                continue;
            double value = 0.0;
            const char* begin = parameter.value.data();
            const char* end = begin + parameter.value.size();
            const auto [stop, error] = std::from_chars(begin, end, value);
            if (error != std::errc() || stop != end)
                continue; // not a number: the statement is not one of the tolerances
            thresholds.set(parameter.name, value);
        }
    }
    return thresholds;
}

namespace
{

// The checks of a solid: the aspect ratio, the position and angle of the mid-side nodes, the Jacobian, the warp of the
// quadrilateral faces.
std::uint16_t solidChecks(const QualityValues& values, double aspect, double eplr, double epia, double detj,
                          double warp) noexcept
{
    std::uint16_t failed = 0;
    if (values.aspectRatio > aspect)
        failed |= bit(QualityCheck::AspectRatio);
    if (values.edgePointLengthRatio > eplr)
        failed |= bit(QualityCheck::EdgePointLength);
    if (values.edgePointAngle < epia)
        failed |= bit(QualityCheck::EdgePointAngle);
    if (values.jacobianMin <= detj)
        failed |= bit(QualityCheck::Jacobian);
    if (values.faceWarp < warp)
        failed |= bit(QualityCheck::FaceWarp);
    return failed;
}

} // namespace

std::uint16_t GeometryCheckThresholds::failedChecks(ElementShape shape, const QualityValues& values) const noexcept
{
    std::uint16_t failed = 0;
    switch (shape)
    {
    case ElementShape::Triangle:
        if (values.minAngle < t3Skew)
            failed |= bit(QualityCheck::MinAngle);
        if (values.maxAngle > t3Iamax)
            failed |= bit(QualityCheck::MaxAngle);
        break;
    case ElementShape::Quadrilateral:
        if (values.skew > q4Skew)
            failed |= bit(QualityCheck::Skew);
        if (values.taper > q4Taper)
            failed |= bit(QualityCheck::Taper);
        if (values.warp > q4Warp)
            failed |= bit(QualityCheck::Warp);
        if (values.minAngle < q4Iamin)
            failed |= bit(QualityCheck::MinAngle);
        if (values.maxAngle > q4Iamax)
            failed |= bit(QualityCheck::MaxAngle);
        break;
    case ElementShape::Tetrahedron:
        failed = solidChecks(values, tetAr, tetEplr, tetEpia, std::max(tetDetj, tetDetg), -2.0);
        break;
    case ElementShape::Hexahedron:
        failed = solidChecks(values, hexAr, hexEplr, hexEpia, hexDetj, hexWarp);
        break;
    case ElementShape::Pentahedron:
        failed = solidChecks(values, penAr, penEplr, penEpia, penDetj, penWarp);
        break;
    case ElementShape::Pyramid:
        failed = solidChecks(values, pyrAr, pyrEplr, pyrEpia, pyrDetj, pyrWarp);
        break;
    default:
        break;
    }
    return failed;
}

} // namespace oor::analysis
