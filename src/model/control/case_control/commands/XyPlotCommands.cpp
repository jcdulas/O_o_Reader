#include "oor/model/control/case_control/commands/CommandFamilies.hpp"

namespace oor::model
{

// format reference, the X-Y output commands of OUTPUT(XYPLOT) blocks: what to plot or print, titles, axes, grids,
// limits, paper.
void defineXyPlotCommands(CommandCatalogue& commands)
{
    // What to plot, print, punch or peak-pick: "XYPLOT DISP RESPONSE 20 / 1(T2)", a language of its own, kept as text.
    commands.text("XyPlotRequest").inBlock();
    commands.text("XyPrintRequest").inBlock();
    commands.text("XyPunchRequest").inBlock();
    commands.text("XyPeakSummary").inBlock();

    // Titles and curve labels.
    commands.text("XyXTitle").inBlock();
    commands.text("XyYTitle").inBlock();
    commands.text("XyYTopTitle").inBlock();
    commands.text("XyYBottomTitle").inBlock();
    commands.text("XyCurveTitle").inBlock();
    commands.integer("XyCurveSymbol").inBlock();

    // Axes, grids and scales (the upper and lower half of a two-curve frame are the T and B variants).
    commands.keyword("XyXAxisLine").inBlock();
    commands.keyword("XyYAxisLine").inBlock();
    commands.keyword("XyXLogScale").inBlock();
    commands.keyword("XyYLogScale").inBlock();
    commands.keyword("XyYTopLogScale").inBlock();
    commands.keyword("XyYBottomLogScale").inBlock();
    commands.text("XyXGridLines").inBlock();
    commands.text("XyYGridLines").inBlock();
    commands.text("XyXTopGridLines").inBlock();
    commands.text("XyXBottomGridLines").inBlock();
    commands.text("XyYTopGridLines").inBlock();
    commands.text("XyYBottomGridLines").inBlock();
    commands.real("XyXMinimum").inBlock();
    commands.real("XyXMaximum").inBlock();
    commands.real("XyYMinimum").inBlock();
    commands.real("XyYMaximum").inBlock();
    commands.real("XyYTopMinimum").inBlock();
    commands.real("XyYTopMaximum").inBlock();
    commands.real("XyYBottomMinimum").inBlock();
    commands.real("XyYBottomMaximum").inBlock();
    commands.real("XyXInterceptValue").inBlock();
    commands.real("XyYInterceptValue").inBlock();
    commands.real("XyXPaperSize").inBlock();
    commands.real("XyYPaperSize").inBlock();

    // Tick marks and margins: "UPPER TICS = -1".
    commands.text("XyFrameUpper").inBlock();
    commands.text("XyFrameLower").inBlock();
    commands.text("XyFrameLeft").inBlock();
    commands.text("XyFrameRight").inBlock();
    commands.text("XyFrameTopLeft").inBlock();
    commands.text("XyFrameTopRight").inBlock();
    commands.text("XyFrameBottomLeft").inBlock();
    commands.text("XyFrameBottomRight").inBlock();
}

} // namespace oor::model
