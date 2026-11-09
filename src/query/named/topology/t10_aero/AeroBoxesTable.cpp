#include "oor/analysis/geometry/AeroPanels.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t10_aero/AeroQueries.hpp"

#include <algorithm>
#include <array>

namespace oor::query
{

namespace
{

// The x, y and z columns of a point of each box.
struct PointColumns
{
    std::array<std::vector<double>, 3> axis;

    void reserve(std::size_t count)
    {
        for (std::vector<double>& values : axis)
            values.reserve(count);
    }
    void append(const std::vector<base::Vec3>& points)
    {
        for (const base::Vec3& point : points)
        {
            axis[0].push_back(point.x);
            axis[1].push_back(point.y);
            axis[2].push_back(point.z);
        }
    }
    // Adds the three columns `<prefix>X`, `<prefix>Y` and `<prefix>Z` to the table.
    void addTo(TableBuilder& builder, const std::string& prefix)
    {
        builder.real(prefix + "X", std::move(axis[0]));
        builder.real(prefix + "Y", std::move(axis[1]));
        builder.real(prefix + "Z", std::move(axis[2]));
    }
};

// aeroBoxesTable: the boxes of the CAERO1, CAERO4 and CAERO5 panels with their geometry in the basic system (the same
// numbers as `Caero1.panels()`). A CAERO2 body and a CAERO3
// Mach box surface have no boxes here, and a panel whose divisions cannot be cut (an AEFACT that is missing, a list
// with fewer than two points) has none either: `aeroPanelsTable` shows its `boxCount` as null.
class AeroBoxesTableQuery final : public QueryBase
{
public:
    AeroBoxesTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::optional<std::vector<std::int64_t>> wanted = sortedIds(run.arguments.ids("elementIds"));
        std::vector<analysis::AeroPanelBoxes> panels = analysis::allAeroPanelBoxes(*run.model);
        if (wanted)
        {
            std::erase_if(panels, [&](const analysis::AeroPanelBoxes& panel)
                          { return !std::binary_search(wanted->begin(), wanted->end(), std::int64_t{panel.eid}); });
        }
        std::size_t count = 0;
        for (const analysis::AeroPanelBoxes& panel : panels)
            count += panel.boxCount();
        if (count == 0)
            return emptyTable(info().defaultColumns());

        std::vector<std::int32_t> boxId, panelId, spanIndex, chordIndex;
        std::vector<double> area, chord;
        boxId.reserve(count);
        panelId.reserve(count);
        spanIndex.reserve(count);
        chordIndex.reserve(count);
        area.reserve(count);
        chord.reserve(count);
        std::array<PointColumns, 4> corners;
        PointColumns center, control, normal;
        for (PointColumns* points : {&corners[0], &corners[1], &corners[2], &corners[3], &center, &control, &normal})
            points->reserve(count);
        for (const analysis::AeroPanelBoxes& panel : panels)
        {
            boxId.insert(boxId.end(), panel.boxId.begin(), panel.boxId.end());
            panelId.insert(panelId.end(), panel.boxCount(), panel.eid);
            spanIndex.insert(spanIndex.end(), panel.spanIndex.begin(), panel.spanIndex.end());
            chordIndex.insert(chordIndex.end(), panel.chordIndex.begin(), panel.chordIndex.end());
            corners[0].append(panel.corner1);
            corners[1].append(panel.corner2);
            corners[2].append(panel.corner3);
            corners[3].append(panel.corner4);
            center.append(panel.center);
            control.append(panel.controlPoint);
            normal.append(panel.normal);
            area.insert(area.end(), panel.area.begin(), panel.area.end());
            chord.insert(chord.end(), panel.chord.begin(), panel.chord.end());
        }
        TableBuilder builder;
        builder.int32("boxId", std::move(boxId));
        builder.int32("elementId", std::move(panelId));
        builder.int32("spanIndex", std::move(spanIndex));
        builder.int32("chordIndex", std::move(chordIndex));
        for (std::size_t corner = 0; corner < corners.size(); ++corner)
            corners[corner].addTo(builder, "corner" + std::to_string(corner + 1));
        center.addTo(builder, "center");
        control.addTo(builder, "control");
        builder.real("area", std::move(area));
        builder.real("chord", std::move(chord));
        normal.addTo(builder, "normal");
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "aeroBoxesTable";
        info.family = "T10";
        info.description =
            "One row for each aerodynamic box of the CAERO1, CAERO4 and CAERO5 panels, in the basic system. The "
            "corners are numbered as the panel is (1 and 2 on the inboard side, 3 and 4 on the outboard side), "
            "`center` is "
            "the average of the corners and `control` the control point at three quarters of the chord at mid span "
            "(the "
            "Doublet-Lattice theory puts the boundary condition there). `chord` is the length of the box along the "
            "flow "
            "at mid span and `normal` the unit normal of the box. A CAERO2 body and a CAERO3 Mach box surface have no "
            "boxes here, and neither has a panel whose divisions cannot be cut.";
        info.parameters = {parameter("elementIds", ParameterKind::IdList, "None", "the panels to list")};
        info.defaultSort = "boxId";
        ColumnListBuilder columns;
        columns.int32("boxId").int32("elementId").int32("spanIndex").int32("chordIndex");
        for (const char* corner : {"corner1", "corner2", "corner3", "corner4"})
            columns.vector3(corner, "XYZ");
        columns.vector3("center", "XYZ").vector3("control", "XYZ").real("area").real("chord").vector3("normal", "XYZ");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addAeroBoxesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<AeroBoxesTableQuery>());
}

} // namespace oor::query
