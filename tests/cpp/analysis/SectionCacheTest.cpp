// The cache of library sections (SectionCache): a section taken from it has the bits of one computed afresh, each key
// is solved once whatever the threads that ask for it, the batches bound what it keeps, and the cache of a model lives
// across its edits and is shared with its copies (ComponentCache::sections).

#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/analysis/mass/SectionCache.hpp"
#include "oor/analysis/mass/SectionFormulas.hpp"
#include "support/ModelTestSupport.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <bit>
#include <cstdint>
#include <limits>
#include <thread>
#include <vector>

using namespace oor;
using namespace oor::analysis;
using oor::testing::readModelText;

namespace
{

// Every value of a section as its bits (NaN compares by its bits too).
std::vector<std::uint64_t> bitsOf(const SectionProperties& section)
{
    std::vector<double> values{section.xxb,
                               section.area,
                               section.i1,
                               section.i2,
                               section.i12,
                               section.j,
                               section.k1,
                               section.k2,
                               section.nsm,
                               section.cw,
                               section.drawingCentroid[0],
                               section.drawingCentroid[1],
                               section.drawingShearCentre[0],
                               section.drawingShearCentre[1]};
    for (const auto& point : section.recovery)
        values.insert(values.end(), point.begin(), point.end());
    values.insert(values.end(), section.drawingBounds.begin(), section.drawingBounds.end());
    std::vector<std::uint64_t> bits;
    for (const double value : values)
        bits.push_back(std::bit_cast<std::uint64_t>(value));
    return bits;
}

// A Z of the stringers of a companion package' barrel: the finite element formulation of its shear factors.
constexpr std::array<double, 4> STRINGER = {15.0, 1.6, 21.8, 25.0};

} // namespace

TEST_CASE("a section from the cache has the bits of one computed afresh, each key solved once", "[analysis][section]")
{
    SectionCache cache;
    const SectionProperties first = cache.standard("ZSection", STRINGER, ShearFactorRule::Solver);
    const SectionProperties again = cache.standard("ZSection", STRINGER, ShearFactorRule::Solver);
    const SectionProperties fresh = SectionFormulas::standard("ZSection", STRINGER, ShearFactorRule::Solver);
    CHECK(bitsOf(first) == bitsOf(fresh));
    CHECK(bitsOf(again) == bitsOf(fresh));
    // Another rule is another key.
    const SectionProperties legacy = cache.standard("ZSection", STRINGER, ShearFactorRule::BeamLibrary);
    CHECK(bitsOf(legacy) == bitsOf(SectionFormulas::standard("ZSection", STRINGER, ShearFactorRule::BeamLibrary)));
    CHECK(cache.statistics().computed == 2);
    CHECK(cache.statistics().reused == 1);
    // The dimensions are keys by their bits: a NaN (an impossible section) is a key like another.
    const std::array<double, 2> impossible = {std::numeric_limits<double>::quiet_NaN(), 1.0};
    CHECK(bitsOf(cache.standard("Bar", impossible, ShearFactorRule::Solver)) ==
          bitsOf(cache.standard("Bar", impossible, ShearFactorRule::Solver)));
    CHECK(cache.statistics().computed == 3);
    CHECK(cache.size() == 3);
}

TEST_CASE("threads that ask for the same new section at once have it solved once", "[analysis][section]")
{
    SectionCache cache;
    constexpr std::size_t THREADS = 6;
    std::vector<SectionProperties> found(THREADS);
    std::vector<std::thread> threads;
    for (std::size_t index = 0; index < THREADS; ++index)
        threads.emplace_back([&, index]
                             { found[index] = cache.standard("ZSection", STRINGER, ShearFactorRule::Solver); });
    for (std::thread& thread : threads)
        thread.join();
    CHECK(cache.statistics().computed == 1);
    CHECK(cache.statistics().reused == THREADS - 1);
    const std::vector<std::uint64_t> expected =
        bitsOf(SectionFormulas::standard("ZSection", STRINGER, ShearFactorRule::Solver));
    for (const SectionProperties& section : found)
        CHECK(bitsOf(section) == expected);
}

TEST_CASE("the cache keeps the sections of its last batch and at most MAX_SECTIONS more", "[analysis][section]")
{
    SectionCache cache;
    // Rods: a closed form, cheap to compute.
    cache.nextBatch();
    for (std::size_t index = 0; index <= SectionCache::MAX_SECTIONS; ++index)
    {
        const std::array<double, 1> radius = {1.0 + static_cast<double>(index)};
        (void)cache.standard("Rod", radius, ShearFactorRule::Solver);
    }
    cache.trim();
    CHECK(cache.size() == SectionCache::MAX_SECTIONS + 1); // all of the last batch
    cache.nextBatch();
    const std::array<double, 1> kept = {2.0};
    (void)cache.standard("Rod", kept, ShearFactorRule::Solver);
    cache.trim();
    CHECK(cache.size() == 1);
    CHECK(cache.statistics().dropped == SectionCache::MAX_SECTIONS);
    // Under the bound nothing is dropped.
    cache.nextBatch();
    const std::array<double, 1> other = {3.0};
    (void)cache.standard("Rod", other, ShearFactorRule::Solver);
    cache.trim();
    CHECK(cache.size() == 2);
}

TEST_CASE("the section cache of a model lives across its edits and is shared with its copies", "[analysis][section]")
{
    model::Model model = readModelText(
        "IsoMat;1;7.E+4;;0.3;2.7E-9\n"
        "BarShapeProp;1;1;;ZSection;15.;1.6;21.8;25.\n"
        "BarShapeProp;2;1;;ZSection;15.;1.6;21.8;25.\n");
    SectionCache& cache = ComponentCache::of(model).sections();
    const auto first = SectionFormulas::of(model, 1);
    REQUIRE(first);
    REQUIRE(SectionFormulas::of(model, *model.locate(base::IdSpace::Property, 1), ShearFactorRule::Solver, &cache));
    REQUIRE(SectionFormulas::of(model, *model.locate(base::IdSpace::Property, 2), ShearFactorRule::Solver, &cache));
    CHECK(cache.statistics().computed == 1);
    model.noteContentChange();
    CHECK(&ComponentCache::of(model).sections() == &cache);
    const model::Model copy = model.copy();
    CHECK(&ComponentCache::of(copy).sections() == &cache);
    // A copy made of a model whose cache has no sections yet shares the one made then.
    model::Model other = readModelText("IsoMat;1;7.E+4;;0.3;2.7E-9\nBarShapeProp;1;1;;ZSection;15.;1.6;21.8;25.\n");
    (void)ComponentCache::of(other);
    const model::Model otherCopy = other.copy();
    CHECK(&ComponentCache::of(otherCopy).sections() == &ComponentCache::of(other).sections());
}
