#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/base/math/Vec3.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace oor::analysis
{

// Which card the boxes come from, by its card type. A CAERO1 is a lattice of chord boxes; a CAERO4 and a CAERO5 are
// made of strips, which are boxes that span the whole chord (one chordwise box).
enum class AeroPanelKind : std::uint8_t
{
    AeroLiftingPanel1,
    AeroStripPanel4,
    AeroPistonPanel5
};

// The card type of a kind of panel ("AeroLiftingPanel1"); a message names it by its keyword (model::cardLabel).
[[nodiscard]] std::string_view aeroPanelKindName(AeroPanelKind kind) noexcept;

// The boxes of one macro element in the basic system, one entry in each array per box, in the order of the box IDs: the
// box with the span index s and the chord index c (counted from the leading edge and from the inboard edge) has the ID
// `eid + s * chordBoxes + c`, which is how the format reference numbers them.
//
// The four corners of a box are numbered as the panel is: 1 and 2 on the inboard side (leading edge, trailing edge), 3
// and 4 on the outboard side (trailing edge, leading edge), so that the right-hand normal of 1-2-3-4 points up for a
// panel whose outboard edge is on the +y side. They come from the bilinear interpolation of the four corners of the
// panel at the division points of the span and of the chord.
//
//   center        the average of the four corners (the middle of the box in the interpolation of the panel)
//   controlPoint  the three-quarter chord point at mid span: where the Doublet-Lattice theory satisfies the boundary
//                 condition (the classical choice of the method; the input format does not state it)
//   loadPoint     the quarter chord point at mid span: where the theory puts the doublet line and the force of the box
//   area          the area of the (planar) quadrilateral
//   chord         the length of the box along the aerodynamic x axis at mid span
//   normal        the unit normal, from the cross product of the diagonals 1-3 and 2-4
struct AeroPanelBoxes
{
    base::EntityId eid = 0; // the macro element that owns the boxes
    AeroPanelKind kind = AeroPanelKind::AeroLiftingPanel1;
    std::int32_t chordBoxes = 0;
    std::int32_t spanBoxes = 0;

    std::vector<base::EntityId> boxId;
    std::vector<std::int32_t> chordIndex;
    std::vector<std::int32_t> spanIndex;
    std::vector<base::Vec3> corner1;
    std::vector<base::Vec3> corner2;
    std::vector<base::Vec3> corner3;
    std::vector<base::Vec3> corner4;
    std::vector<base::Vec3> center;
    std::vector<base::Vec3> controlPoint;
    std::vector<base::Vec3> loadPoint;
    std::vector<double> area;
    std::vector<double> chord;
    std::vector<base::Vec3> normal;

    [[nodiscard]] std::size_t boxCount() const noexcept { return boxId.size(); }
};

// The boxes of the panel with this EID (a CAERO1, CAERO4 or CAERO5). The leading edge points are taken from their
// system CP to the basic system; point 2 is X12 behind point 1 and point 3 X43 behind point 4 along the x axis of the
// aerodynamic coordinate system (the ACSID of the AERO card, else of the AEROS card, else the basic system). The
// division points are the equal fractions of NSPAN and NCHORD, or the values of the AEFACT LSPAN and LCHORD, used as
// they are (the first and last need not be 0 and 1). Throws GeometryError when there is no such
// panel or a division list is missing or has fewer than two points, and CoordinateSystemError when a coordinate system
// cannot be resolved.
[[nodiscard]] AeroPanelBoxes aeroPanelBoxes(const model::Model& model, base::EntityId eid);

// The boxes of every CAERO1, CAERO4 and CAERO5 panel of the model, ordered by EID; a panel whose boxes cannot be made
// (the problem the function above throws for) is left out and its EID is put in `skipped` when it is given.
[[nodiscard]] std::vector<AeroPanelBoxes> allAeroPanelBoxes(const model::Model& model,
                                                            std::vector<base::EntityId>* skipped = nullptr);

// What the outline of a panel card needs from the model, for the cards that have no box geometry (CAERO2 and CAERO3)
// and for the table of panels: the points of the card in the basic system and the direction of the flow. The chords of
// a panel run along that direction, so the span of a CAERO1, CAERO3, CAERO4 or CAERO5 is the distance between its two
// chords (the part of the vector from point 1 to point 4 that is perpendicular to the flow), and its area the
// trapezoid (x12 + x43) / 2 times that span, which is the sum of the areas of its boxes.
class PanelFrame
{
public:
    explicit PanelFrame(const model::Model& model);
    ~PanelFrame();
    PanelFrame(const PanelFrame&) = delete;
    PanelFrame& operator=(const PanelFrame&) = delete;

    // A point given in the system `cp` of its card, in the basic system. Throws CoordinateSystemError when the system
    // cannot be resolved.
    [[nodiscard]] base::Vec3 toBasic(base::EntityId cp, const base::Vec3& point);
    // The unit vector of the flow: the x axis of the aerodynamic coordinate system (the ACSID of the AERO card, else of
    // the AEROS card, else the basic system), in the basic system. Throws CoordinateSystemError.
    [[nodiscard]] base::Vec3 flowDirection();
    // The distance between two chords of a panel whose leading edge points are given in the basic system.
    [[nodiscard]] double spanBetween(const base::Vec3& inboard, const base::Vec3& outboard);

private:
    struct Implementation;
    std::unique_ptr<Implementation> m_implementation;
};

} // namespace oor::analysis
