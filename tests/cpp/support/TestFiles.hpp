#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <string>

namespace oor::testing
{

// A scratch folder with files written by a test; removed with the object. Each test gets its own folder.
class TestFiles
{
public:
    TestFiles()
    {
        // Test cases run as separate processes, at the same time: the folder name must differ between them.
        static std::atomic<int> counter{0};
        std::random_device entropy;
        m_root = std::filesystem::temp_directory_path() / ("oor_test_" + std::to_string(counter++) + "_" +
                                                           std::to_string(entropy()) + "_" + std::to_string(entropy()));
        std::filesystem::create_directories(m_root);
    }
    ~TestFiles()
    {
        std::error_code ignored;
        std::filesystem::remove_all(m_root, ignored);
    }
    TestFiles(const TestFiles&) = delete;
    TestFiles& operator=(const TestFiles&) = delete;

    std::filesystem::path write(const std::string& relativePath, const std::string& content) const
    {
        const std::filesystem::path path = m_root / relativePath;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file(path, std::ios::binary);
        file << content;
        return path;
    }

    // The content of a file under the folder.
    [[nodiscard]] std::string read(const std::string& relativePath) const
    {
        std::ifstream file(m_root / relativePath, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    }

    [[nodiscard]] bool exists(const std::string& relativePath) const
    {
        return std::filesystem::exists(m_root / relativePath);
    }

    [[nodiscard]] const std::filesystem::path& root() const { return m_root; }

private:
    std::filesystem::path m_root;
};

} // namespace oor::testing
