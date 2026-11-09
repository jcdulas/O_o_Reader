#include "oor/model/format/SlotTrace.hpp"

#ifdef OOR_TRACE_SLOTS

#include <cstdlib>
#include <fstream>
#include <mutex>
#include <random>
#include <set>

namespace oor::model::slot_trace
{

namespace
{

// The distinct lines seen by this process, written to a file of their own when the process ends.
class TraceLines
{
public:
    ~TraceLines()
    {
        const char* folder = std::getenv("OOR_SLOT_TRACE_DIR");
        if (folder == nullptr || m_lines.empty())
            return;
        std::random_device random;
        const std::string path = std::string(folder) + "/trace-" + std::to_string(random()) + ".txt";
        std::ofstream file(path, std::ios::binary);
        for (const std::string& line : m_lines)
            file << line << '\n';
    }

    void add(std::string line)
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        m_lines.insert(std::move(line));
    }

private:
    std::mutex m_mutex;
    std::set<std::string> m_lines;
};

TraceLines& lines()
{
    static TraceLines instance;
    return instance;
}

} // namespace

void noteRead(std::string_view cardType, int slot, std::string_view accessor, std::string_view name)
{
    if (cardType.empty() || slot < 1)
        return;
    lines().add("R\t" + std::string(cardType) + "\t" + std::to_string(slot) + "\t" + std::string(accessor) + "\t" +
                std::string(name));
}

void noteSpan(std::string_view cardType, int first, int last)
{
    if (!cardType.empty())
        lines().add("S\t" + std::string(cardType) + "\t" + std::to_string(first) + "\t" + std::to_string(last));
}

void noteWrite(std::string_view cardType, const std::string& slots)
{
    if (!cardType.empty())
        lines().add("W\t" + std::string(cardType) + "\t" + slots);
}

} // namespace oor::model::slot_trace

#else

namespace oor::model::slot_trace
{

void noteRead(std::string_view, int, std::string_view, std::string_view) {}
void noteSpan(std::string_view, int, int) {}
void noteWrite(std::string_view, const std::string&) {}

} // namespace oor::model::slot_trace

#endif
