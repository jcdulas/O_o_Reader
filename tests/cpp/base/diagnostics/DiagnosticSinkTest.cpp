#include "oor/base/diagnostics/DiagnosticSink.hpp"
#include "oor/base/errors/Errors.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace oor::base;

TEST_CASE("a location prints the parts it has", "[base][diagnostics]")
{
    REQUIRE(SourceLocation{"plate.oos", 12, "CQUAD4", "PID"}.toString() == "plate.oos:12 (CQUAD4, PID)");
    REQUIRE(SourceLocation{"plate.oos", 12, "", ""}.toString() == "plate.oos:12");
    REQUIRE(SourceLocation{"plate.oos", 0, "", ""}.toString() == "plate.oos");
    REQUIRE(SourceLocation{"", 0, "GRID", ""}.toString() == "(GRID)");
    REQUIRE(SourceLocation{}.toString().empty());
}

TEST_CASE("a diagnostic reads as one line", "[base][diagnostics]")
{
    const Diagnostic diagnostic{Severity::Error, "deck.duplicate-id", "element 7 is defined twice",
                                SourceLocation{"a.oos", 3, "CQUAD4", ""}};
    REQUIRE(diagnostic.toString() == "error[deck.duplicate-id] a.oos:3 (CQUAD4): element 7 is defined twice");
}

TEST_CASE("tolerant mode collects errors and goes on", "[base][diagnostics]")
{
    DiagnosticSink sink(ReadMode::Tolerant);
    sink.warning("w", "a warning");
    sink.error("e1", "first");
    sink.error("e2", "second");
    REQUIRE(sink.hasErrors());
    REQUIRE(sink.errorCount() == 2);
    REQUIRE_FALSE(sink.shouldStop());
    REQUIRE(sink.diagnostics().size() == 3);
    REQUIRE(sink.firstError()->code == "e1");
}

TEST_CASE("strict mode asks to stop at the first error", "[base][diagnostics]")
{
    DiagnosticSink sink(ReadMode::Strict);
    sink.warning("w", "a warning");
    REQUIRE_FALSE(sink.shouldStop());
    sink.error("e", "boom");
    REQUIRE(sink.shouldStop());
}

TEST_CASE("merging chunk sinks keeps file order", "[base][diagnostics]")
{
    DiagnosticSink all;
    DiagnosticSink first;
    DiagnosticSink second;
    first.error("a", "chunk 0");
    second.warning("b", "chunk 1");
    second.error("c", "chunk 1");
    all.append(std::move(first));
    all.append(std::move(second));
    REQUIRE(all.diagnostics().size() == 3);
    REQUIRE(all.diagnostics()[0].code == "a");
    REQUIRE(all.diagnostics()[2].code == "c");
    REQUIRE(all.errorCount() == 2);
}

TEST_CASE("a DeckError carries its diagnostic", "[base][diagnostics]")
{
    const Diagnostic diagnostic{Severity::Error, "x", "message", SourceLocation{"f.oos", 1, "", ""}};
    try
    {
        throw DeckError(diagnostic);
    }
    catch (const OorError& error)
    {
        REQUIRE(std::string(error.what()) == "error[x] f.oos:1: message");
        REQUIRE(dynamic_cast<const DeckError&>(error).diagnostic().code == "x");
    }
}
