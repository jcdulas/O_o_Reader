#include "oor/model/control/case_control/commands/CommandFamilies.hpp"

namespace oor::model
{

// format reference, the structure plot commands and the grid point stress commands of OUTPUT(PLOT) / OUTPUT(POST)
// blocks. The arguments of a plot command are a language of their own (SET, ORIGIN, SYMBOL, ...): each command keeps
// them as text.
void definePlotCommands(CommandCatalogue& commands)
{
    commands.text("PlotDraw").inBlock();
    commands.text("PlotFind").inBlock();
    commands.text("PlotAxes").inBlock();
    commands.text("PlotView").inBlock();
    commands.text("PlotScale").inBlock();
    commands.text("PlotCharacterScale").inBlock();
    commands.text("PlotOrigin").inBlock();
    commands.text("PlotMaximumDeformation"); // MAXIMUM DEFORMATION x
    commands.text("PlotDistortion").inBlock();
    commands.text("PlotCamera").inBlock();
    commands.text("PlotContour").inBlock();
    commands.text("PlotOcularSeparation").inBlock();
    commands.text("PlotOrthographic").inBlock();
    commands.text("PlotPerspective").inBlock();
    commands.text("PlotStereoscopic").inBlock();
    commands.text("PlotProjectionPlane").inBlock();
    commands.text("PlotVantagePoint").inBlock();
    commands.text("PlotPaperSize").inBlock();
    commands.text("PlotPen").inBlock();
    commands.text("PlotDevice").inBlock();
    commands.text("PlotTitle").inBlock();
    commands.text("PlotSuperelements").inBlock();
    commands.text("PlotUpstreamSuperelements").inBlock();
    commands.text("PlotSurface").inBlock();
    commands.text("PlotVolume").inBlock();
}

} // namespace oor::model
