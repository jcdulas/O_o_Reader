#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace oor::deck
{

// The files that are open at this point of the reading, from the main file to the innermost INCLUDE. Detects an
// INCLUDE that names a file already open (a cycle) and limits the nesting depth.
class IncludeTree
{
public:
    explicit IncludeTree(std::size_t maxDepth) noexcept : m_maxDepth(maxDepth) {}

    enum class EnterResult
    {
        Entered,
        Cycle,
        TooDeep
    };

    // Opens `path` (the main file or an included file). On Cycle or TooDeep nothing is opened.
    EnterResult enter(const std::filesystem::path& path);
    // Closes the innermost open file.
    void leave() noexcept
    {
        m_open.pop_back();
        m_names.pop_back();
    }

    [[nodiscard]] std::size_t depth() const noexcept { return m_open.size(); }
    // The open files joined as "a.bdf -> b.bdf -> c.bdf", for messages.
    [[nodiscard]] std::string describe() const;

private:
    std::size_t m_maxDepth;
    std::vector<std::filesystem::path> m_open; // canonical paths, outermost first
    std::vector<std::filesystem::path> m_names;
};

} // namespace oor::deck
