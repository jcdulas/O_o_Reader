#include "oor/base/errors/Errors.hpp"
#include "oor/results/attach/ResultAttachment.hpp"
#include "support/ModelTestSupport.hpp"
#include "support/ResultTestTables.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <memory>
#include <string>

using namespace oor;
using namespace oor::results;
using oor::testing::makeTable;
using oor::testing::makeTables;
using oor::testing::MemoryResultFile;
using oor::testing::readModelText;
using oor::testing::TableSpec;

namespace
{

const char* const PLATE =
    "IsoMat;1;2.1E+5;;0.3\n"
    "ShellProp;1;1;0.1\n"
    "Node;1;;0;0;0\n"
    "Node;2;;1;0;0\n"
    "Node;3;;1;1;0\n"
    "Node;4;;0;1;0\n"
    "Node;5;;2;0;0\n"
    "Node;6;;2;1;0\n"
    "ShellQuad4;10;1;1;2;3;4\n"
    "ShellQuad4;20;1;2;5;6;3\n"
    "ShellTria3;30;1;1;2;3\n";

std::shared_ptr<model::Model> plate()
{
    return std::make_shared<model::Model>(readModelText(PLATE));
}

TableSpec displacements(std::vector<std::int32_t> ids)
{
    TableSpec spec;
    spec.kind = ResultKind::Displacements;
    spec.ids = std::move(ids);
    spec.components = {"t1", "t2", "t3", "r1", "r2", "r3"};
    spec.values.assign(spec.ids.size() * 6, 1.0);
    return spec;
}

TableSpec shellStress(std::vector<std::int32_t> elements, const std::string& name = "ShellQuad4")
{
    TableSpec spec;
    spec.kind = ResultKind::Stress;
    spec.elementName = name;
    for (const std::int32_t element : elements)
        for (const std::int32_t layer : {1, 2})
        {
            spec.ids.push_back(element);
            spec.layers.push_back(layer);
        }
    spec.components = {"oxx", "oyy", "txy"};
    spec.values.assign(spec.ids.size() * 3, 1.0);
    return spec;
}

struct Attached
{
    std::shared_ptr<model::Model> model;
    std::shared_ptr<ResultTables> tables;
    std::shared_ptr<ResultAttachment> attachment;
};

Attached attach(const std::vector<TableSpec>& specs, AttachOptions options = {})
{
    Attached result;
    result.model = plate();
    std::vector<std::shared_ptr<ResultTable>> tables;
    for (const TableSpec& spec : specs)
        tables.push_back(makeTable(spec));
    result.tables = makeTables(tables);
    result.attachment =
        ResultAttachment::attach(result.model, std::make_shared<MemoryResultFile>(), result.tables, options);
    return result;
}

bool has(const ResultAttachment& attachment, const char* code)
{
    return std::any_of(attachment.diagnostics().begin(), attachment.diagnostics().end(),
                       [&](const base::Diagnostic& diagnostic) { return diagnostic.code == code; });
}

} // namespace

TEST_CASE("results are linked to the cards of the model by ID, nodes and elements apart", "[results][attach]")
{
    const Attached attached = attach({displacements({1, 2, 3, 4, 5, 6}), shellStress({10, 20})});
    const ResultAttachment& attachment = *attached.attachment;
    REQUIRE(attachment.nodeLinks().size() == 6);
    REQUIRE(attachment.elementLinks().size() == 2);
    for (std::int32_t node = 1; node <= 6; ++node)
    {
        REQUIRE(attachment.nodeLocation(*attached.model, node).has_value());
        REQUIRE(attachment.modelNodeId(*attached.model, node) == node);
    }
    REQUIRE(attachment.modelElementId(*attached.model, 20) == 20);
    REQUIRE(!attachment.nodeLocation(*attached.model, 99).has_value());
    REQUIRE(attachment.diagnostics().empty());
    REQUIRE(attachment.consistent(*attached.model));
    REQUIRE(attachment.state(*attached.model) == AttachmentState::Current);
    REQUIRE(attachment.analysis().empty());
    // the tables know their attachment
    REQUIRE(attached.tables->container(ResultKind::Displacements).tables().front()->attachment() ==
            attached.attachment);
    // and the model lists it
    REQUIRE(ResultAttachment::of(*attached.model).size() == 1);
}

TEST_CASE("the check reports missing nodes, missing elements and elements of another type", "[results][attach]")
{
    const Attached attached = attach({displacements({1, 2, 77}), shellStress({10, 99}), shellStress({30})});
    const ResultAttachment& attachment = *attached.attachment;
    REQUIRE(has(attachment, "attach.node-missing"));
    REQUIRE(has(attachment, "attach.element-missing"));
    REQUIRE(has(attachment, "attach.element-type")); // element 30 is a CTRIA3, the table says CQUAD4
    REQUIRE(!attachment.consistent(*attached.model));
    const auto missing = std::find_if(attachment.diagnostics().begin(), attachment.diagnostics().end(),
                                      [](const base::Diagnostic& d) { return d.code == "attach.node-missing"; });
    REQUIRE(missing->message.find("77") != std::string::npos);
    REQUIRE(missing->severity == base::Severity::Warning);
    // strict: the first error is raised
    AttachOptions strict;
    strict.strict = true;
    REQUIRE_THROWS_AS(attach({shellStress({30})}, strict), base::DeckError);
}

TEST_CASE("a table that covers part of the model is information, one that covers more is a warning",
          "[results][attach]")
{
    const Attached part = attach({shellStress({10})});
    REQUIRE(has(*part.attachment, "attach.element-count"));
    REQUIRE(part.attachment->consistent(*part.model));
    const Attached all = attach({shellStress({10, 20})});
    REQUIRE(!has(*all.attachment, "attach.element-count"));
}

TEST_CASE("a link follows its card through a renumbering and the attachment goes stale", "[results][attach]")
{
    Attached attached = attach({displacements({1, 2, 3}), shellStress({10, 20})});
    ResultAttachment& attachment = *attached.attachment;
    const auto location = attachment.nodeLocation(*attached.model, 2);
    REQUIRE(location.has_value());
    attached.model->setId(*location, 502);
    attached.model->noteStructureChange();
    REQUIRE(attachment.modelNodeId(*attached.model, 2) ==
            502); // the result ID is the file's, the card's ID is the model's
    REQUIRE(attachment.nodeLocation(*attached.model, 2).has_value());
    REQUIRE(attachment.state(*attached.model) == AttachmentState::Stale);
    attachment.refresh(*attached.model);
    REQUIRE(attachment.state(*attached.model) == AttachmentState::Current);
    REQUIRE(attachment.modelNodeId(*attached.model, 2) == 502);
    REQUIRE(attachment.nodeLinks().size() == 3);
}

TEST_CASE("a deleted card loses its link and a card added later does not take its place", "[results][attach]")
{
    Attached attached = attach({displacements({1, 2, 3, 4, 5, 6})});
    ResultAttachment& attachment = *attached.attachment;
    const auto location = attachment.nodeLocation(*attached.model, 6);
    REQUIRE(location.has_value());
    attached.model->remove(*location);
    REQUIRE(attachment.state(*attached.model) == AttachmentState::Edited); // a grid point is not connectivity
    attachment.refresh(*attached.model);
    REQUIRE(attachment.nodeLinks().size() == 5);
    REQUIRE(has(attachment, "attach.node-missing"));
    REQUIRE(!attachment.consistent(*attached.model));
    // the same ID used again by a new grid point: the result ID stays unlinked after a refresh
    REQUIRE(!attachment.nodeLocation(*attached.model, 6).has_value());
}

TEST_CASE("attaching the same tables again replaces the attachment, detach removes it", "[results][attach]")
{
    Attached attached = attach({displacements({1, 2, 3})});
    AttachOptions options;
    options.analysis = "second";
    const std::shared_ptr<ResultAttachment> again =
        ResultAttachment::attach(attached.model, std::make_shared<MemoryResultFile>(), attached.tables, options);
    REQUIRE(ResultAttachment::of(*attached.model).size() == 1);
    REQUIRE(again->analysis() == "second");
    REQUIRE(ResultAttachment::of(*attached.model).front() == again);
    // another set of tables is another attachment
    const auto other = makeTables({makeTable(displacements({1, 2}))});
    const std::shared_ptr<ResultAttachment> second =
        ResultAttachment::attach(attached.model, std::make_shared<MemoryResultFile>(), other);
    REQUIRE(ResultAttachment::of(*attached.model).size() == 2);
    REQUIRE(ResultAttachment::detach(*attached.model, again));
    REQUIRE(!ResultAttachment::detach(*attached.model, again));
    REQUIRE(ResultAttachment::of(*attached.model).size() == 1);
    REQUIRE(attached.tables->container(ResultKind::Displacements).tables().front()->attachment() == nullptr);
    REQUIRE(second->model() == attached.model);
}

TEST_CASE("result sets of the same IDs share their links while the model does not change", "[results][attach]")
{
    // a companion package: one file per load case holds one list of links for every file, not one a file
    Attached attached = attach({displacements({1, 2, 3}), shellStress({10, 20})});
    const auto second = makeTables({makeTable(displacements({1, 2, 3})), makeTable(shellStress({10, 20}))});
    const std::shared_ptr<ResultAttachment> other =
        ResultAttachment::attach(attached.model, std::make_shared<MemoryResultFile>(), second);
    REQUIRE(&other->elementLinks() == &attached.attachment->elementLinks());
    REQUIRE(&other->nodeLinks() == &attached.attachment->nodeLinks());
    REQUIRE(other->elementLocation(*attached.model, 20) == attached.attachment->elementLocation(*attached.model, 20));
    // other IDs: links of their own
    const auto third = makeTables({makeTable(shellStress({10}))});
    const std::shared_ptr<ResultAttachment> fewer =
        ResultAttachment::attach(attached.model, std::make_shared<MemoryResultFile>(), third);
    REQUIRE(&fewer->elementLinks() != &attached.attachment->elementLinks());
    REQUIRE(fewer->elementLinks().size() == 1);
    // after an edit of the model the links are made again (the others are not current)
    const auto location = attached.attachment->nodeLocation(*attached.model, 3);
    REQUIRE(location.has_value());
    attached.model->remove(*location);
    const auto fourth = makeTables({makeTable(displacements({1, 2}))});
    const auto fifth = makeTables({makeTable(displacements({1, 2}))});
    const std::shared_ptr<ResultAttachment> before =
        ResultAttachment::attach(attached.model, std::make_shared<MemoryResultFile>(), fourth);
    REQUIRE(&before->nodeLinks() != &attached.attachment->nodeLinks());
    REQUIRE(before->nodeLinks().size() == 2);
    // and shared again between two sets linked to the edited model
    const std::shared_ptr<ResultAttachment> after =
        ResultAttachment::attach(attached.model, std::make_shared<MemoryResultFile>(), fifth);
    REQUIRE(&after->nodeLinks() == &before->nodeLinks());
}
TEST_CASE("a copy of the model has its own attachments over the same result tables", "[results][attach]")
{
    Attached attached = attach({displacements({1, 2, 3}), shellStress({10})});
    const model::Model copy = attached.model->copy();
    const auto list = ResultAttachment::of(copy);
    REQUIRE(list.size() == 1);
    REQUIRE(list.front() != attached.attachment);
    REQUIRE(list.front()->tables() == attached.tables); // shared, not copied
    REQUIRE(list.front()->nodeLinks().size() == 3);
    REQUIRE(list.front()->model() == nullptr);
    REQUIRE(list.front()->state(copy) == AttachmentState::Current);
    // each side evolves alone
    attached.model->noteStructureChange();
    REQUIRE(attached.attachment->state(*attached.model) == AttachmentState::Stale);
    REQUIRE(list.front()->state(copy) == AttachmentState::Current);
}

TEST_CASE("results of a card are found by its key: the result IDs a card answers to", "[results][attach]")
{
    const Attached attached = attach({displacements({1, 2, 3}), shellStress({10})});
    const auto location = attached.model->locate(base::IdSpace::Element, 10);
    REQUIRE(location.has_value());
    const std::uint32_t key = attached.model->findTable(location->table)->rows().keyAt(location->row).value;
    REQUIRE(attached.attachment->resultIdsOf(location->table, key) == std::vector<std::int32_t>{10});
    REQUIRE(attached.attachment->resultIdsOf(location->table, key + 1000).empty());
}
