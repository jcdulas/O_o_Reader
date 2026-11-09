#include "oor/model/control/case_control/commands/CommandFamilies.hpp"

namespace oor::model
{

// format reference "Output Control" and "Set Definition" (the titles, the echo, the format layout, the output filters).
void defineOutputControlCommands(CommandCatalogue& commands)
{
    // Titles: the text runs to the end of the line, whatever it ends with.
    commands.text("TitleText");
    commands.text("SubtitleText");
    commands.text("LabelText");
    commands.text("LoadLabelText");

    commands.keyword("BulkEcho");
    commands.integer("LinesPerPage");
    commands.integer("MaxOutputLines");
    commands.flag("PageEject");
    commands.text("PlotIdentifier");
    commands.text("PostprocessingOutput");
    commands.text("SkipLines");
    commands.text("MaxMinSummary");

    // Output filters and the sets of the result vectors they keep: ALL, NONE or a SET.
    commands.integerOrKeyword("OutputFreqSelection");
    commands.integerOrKeyword("OutputModeSelection");
    commands.integerOrKeyword("OutputTimeSelection");
    commands.integerOrKeyword("OutputPartitionSelection");
    commands.integerOrKeyword("NodeResponseSorting");
}

} // namespace oor::model
