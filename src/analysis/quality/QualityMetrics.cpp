#include "oor/analysis/quality/QualityMetrics.hpp"

namespace oor::analysis
{

void QualityArrays::assign(std::size_t count)
{
    for (std::vector<double>* array :
         {&aspectRatio, &skew, &warp, &taper, &minAngle, &maxAngle, &jacobianRatio, &jacobianMin, &minEdge, &maxEdge,
          &collapse, &edgePointLengthRatio, &edgePointAngle, &faceWarp})
        array->assign(count, NOT_DEFINED);
}

void QualityArrays::store(std::size_t index, const QualityValues& values) noexcept
{
    aspectRatio[index] = values.aspectRatio;
    skew[index] = values.skew;
    warp[index] = values.warp;
    taper[index] = values.taper;
    minAngle[index] = values.minAngle;
    maxAngle[index] = values.maxAngle;
    jacobianRatio[index] = values.jacobianRatio;
    jacobianMin[index] = values.jacobianMin;
    minEdge[index] = values.minEdge;
    maxEdge[index] = values.maxEdge;
    collapse[index] = values.collapse;
    edgePointLengthRatio[index] = values.edgePointLengthRatio;
    edgePointAngle[index] = values.edgePointAngle;
    faceWarp[index] = values.faceWarp;
}

QualityValues QualityArrays::at(std::size_t index) const noexcept
{
    QualityValues values;
    values.aspectRatio = aspectRatio[index];
    values.skew = skew[index];
    values.warp = warp[index];
    values.taper = taper[index];
    values.minAngle = minAngle[index];
    values.maxAngle = maxAngle[index];
    values.jacobianRatio = jacobianRatio[index];
    values.jacobianMin = jacobianMin[index];
    values.minEdge = minEdge[index];
    values.maxEdge = maxEdge[index];
    values.collapse = collapse[index];
    values.edgePointLengthRatio = edgePointLengthRatio[index];
    values.edgePointAngle = edgePointAngle[index];
    values.faceWarp = faceWarp[index];
    return values;
}

} // namespace oor::analysis
