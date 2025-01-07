#ifndef ZIPARCHIVER_HPP
#define ZIPARCHIVER_HPP

#include <string>
#include <vector>
#include <filesystem>

class ZipArchiver
{
public:
    ZipArchiver();
    ~ZipArchiver();

    bool CompressDirectoryToZip(const std::string& archivePath, const std::string& directoryPath, const bool & deleteDirectory = true);
    bool AddEntryToArchive(struct archive* a, const std::filesystem::path& entryPath, const std::filesystem::path& basePath);

};

#endif //ZIPARCHIVER_HPP