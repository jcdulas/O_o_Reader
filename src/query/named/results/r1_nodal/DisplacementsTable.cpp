#include "NodalVectorTable.hpp"

namespace oor::query
{

namespace
{

// displacementsTable: the displacement of every grid point, subcase and step
class DisplacementsTableQuery final : public ResultQuery
{
public:
    DisplacementsTableQuery() : ResultQuery(makeInfo()) {}

protected:
    [[nodiscard]] std::shared_ptr<const QueryTable> runWith(const ResultQueryContext& context,
                                                            const QueryRun&) const override
    {
        return nodalVectorTable(context, info(), results::ResultKind::Displacements);
    }

private:
    static QueryInfo makeInfo()
    {
        return nodalVectorInfo(
            "displacementsTable",
            "The displacements of the grid points and scalar points: one row for each subcase, step and point, with "
            "the three translations, the three rotations, the norm of the translations and the frame the values are "
            "written in (`frame` turns them to another one first; a complex result is given as `t1_re`, `t1_im` or "
            "`t1_mag`, `t1_phase`). The values have the precision of the result file.");
    }
};

} // namespace

void addDisplacementsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DisplacementsTableQuery>());
}

} // namespace oor::query
