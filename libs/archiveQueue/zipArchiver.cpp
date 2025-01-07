#include "zipArchiver.hpp"
#include <archive.h>
#include <archive_entry.h>
#include <iostream>
#include <fstream>

inline const char * ArchiveErrorToStr(const int err)
{
    switch (err)
    {
    case ARCHIVE_EOF:
        return "End of File";
        break;
    case ARCHIVE_OK:
        return "Archive OK";
        break;
    case ARCHIVE_RETRY:
        return "Retry might succeed";
        break;
    case ARCHIVE_WARN:
        return "Partial Success";
        break;
    case ARCHIVE_FAILED:
        return "Current operation failed";
        break;
    case ARCHIVE_FATAL:
        return "No more operations are possible";
        break;
    default:
        return "Archive OK";
        break;
    }
}

ZipArchiver::ZipArchiver() {}

ZipArchiver::~ZipArchiver() {}

bool ZipArchiver::CompressDirectoryToZip(const std::string& archivePath, const std::string& directoryPath, const bool & deleteDirectory)
{
    struct archive* a = archive_write_new();
    if (!a) 
    {
        std::cerr << "Failed to create archive." << std::endl;
        return false;
    }

    // Set the archive format to ZIP
    archive_write_set_format_zip(a);

    // Open the archive file
    if (archive_write_open_filename(a, archivePath.c_str()) != ARCHIVE_OK) 
    {
        std::cerr << "Failed to open archive: " << archive_error_string(a) << std::endl;
        archive_write_free(a);
        return false;
    }

    // Process all files and directories recursively
    try 
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(directoryPath)) 
        {
            if (!AddEntryToArchive(a, entry.path(), directoryPath)) 
            {
                std::cerr << "Error processing entry: " << entry.path() << std::endl;
                archive_write_close(a);
                archive_write_free(a);
                return false;
            }
        }
    } 
    catch (const std::filesystem::filesystem_error& e) 
    {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        archive_write_close(a);
        archive_write_free(a);
        return false;
    }

    // Close and free the archive
    archive_write_close(a);
    archive_write_free(a);
    
    if(deleteDirectory) std::filesystem::remove_all(directoryPath);

    return true;
}

bool ZipArchiver::AddEntryToArchive(struct archive* a, const std::filesystem::path& entryPath, const std::filesystem::path& basePath)
{
    struct archive_entry* entry = archive_entry_new();
    if (!entry) {
        std::cerr << "Failed to create archive entry for: " << entryPath << std::endl;
        return false;
    }

    // Get the relative path of the entry
    std::string relativePath = std::filesystem::relative(entryPath, basePath).string();

    // Set archive entry metadata
    archive_entry_set_pathname(entry, relativePath.c_str());
    if (std::filesystem::is_directory(entryPath)) 
    {
        archive_entry_set_filetype(entry, AE_IFDIR); // Directory
        archive_entry_set_perm(entry, 0755); // Default directory permissions
    } 
    else if (std::filesystem::is_regular_file(entryPath)) 
    {
        archive_entry_set_filetype(entry, AE_IFREG); // Regular file
        archive_entry_set_perm(entry, 0644); // Default file permissions
        archive_entry_set_size(entry, std::filesystem::file_size(entryPath));
    }

    // Write the entry header
    if (archive_write_header(a, entry) != ARCHIVE_OK) 
    {
        std::cerr << "Failed to write header for: " << entryPath 
                  << " Error: " << archive_error_string(a) << std::endl;
        archive_entry_free(entry);
        return false;
    }

    // If it's a file, write its data
    if (std::filesystem::is_regular_file(entryPath)) 
    {
        std::ifstream file(entryPath, std::ios::binary);
        if (!file.is_open()) 
        {
            std::cerr << "Failed to open file: " << entryPath << std::endl;
            archive_entry_free(entry);
            return false;
        }

        char buffer[8192];
        while (file.read(buffer, sizeof(buffer))) 
        {
            archive_write_data(a, buffer, file.gcount());
        }
        if (file.gcount() > 0) 
        {
            archive_write_data(a, buffer, file.gcount());
        }
    }

    // Free the entry
    archive_entry_free(entry);
    return true;
}

