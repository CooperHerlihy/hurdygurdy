#pragma once

#include "memory.hpp"
#include "maybe.hpp"
#include "strings.hpp"
#include "binary.hpp"

namespace hg {

/**
 * A file path builder
 */
struct FilePath : public StringBuilder {
    /**
     * Construct empty
     */
    FilePath() noexcept = default;

    /**
     * Construct a new file path builder
     */
    FilePath(Arena* arenaVal, StringView str = "")
        : StringBuilder{arenaVal, str}
    {}

    /**
     * Add a directory or file
     */
    void appendPath(StringView other);

    /**
     * Get only the file name at the end of the path
     */
    StringView getFileName() const;

    /**
     * Set the file name (including the extension)
     */
    void setFileName(StringView name);

    /**
     * Get the file name stem (without the extension)
     */
    StringView getStem() const;

    /**
     * Set the file name (leaving the extension)
     */
    void setStem(StringView stem);

    /**
     * Returns whether the file name has an extension
     */
    bool hasExtension() const;

    /**
     * Get the file name extension
     */
    StringView getExtension() const;

    /**
     * Set the file name extension (add one or replace the old one)
     */
    void setExtension(StringView ext);

    /**
     * Get only the directory without the file name at the end
     */
    StringView getDirectoryPart() const;

    /**
     * Returns whether the path is absolute or relative
     */
    bool isAbsolute() const;

};

/**
 * Get the path to the current working directory
 */
FilePath getCurrentDirectory(Arena* arena);

/**
 * Get the path to the home directory
 */
FilePath getHomeDirectory(Arena* arena);

/**
 * Get the path to the root directory
 */
FilePath getRootDirectory(Arena* arena);

/**
 * File or directory information
 */
struct FileInfo {
    /**
     * Is the path a directory or a file
     */
    bool isDirectory = false;
    /**
     * Is the path a symbolic linkg
     */
    bool isSymLink = false;
    /**
     * The size of the file, if a file
     */
    u64 size = 0;
    /**
     * The timestamp when it was last modified
     */
    u64 lastModified = 0;
};

/**
 * Get the info for a file or directory, if it exists
 */
Maybe<FileInfo> getFileInfo(StringView path);

/**
 * An entry in a directory
 */
struct DirectoryEntry {
    /**
     * The file/directory name
     */
    StringView name = "";
    /**
     * The file/directory info
     */
    FileInfo info{};
};

/**
 * Get info for all files and subdirectories in a directory
 */
Span<DirectoryEntry> getFileInfoInDirectory(Arena* arena, StringView path);

/**
 * Make a directory exist, fails if parent directory does not exists
 */
bool makeDirectory(StringView path);

/**
 * Make a directory exist, and all parent directories
 */
bool makeDirectoryRecursive(StringView path);

/**
 * Remove an empty directory
 */
bool removeDirectory(StringView path);

/**
 * Remove a directory and everything contained (use with caution)
 */
bool removeDirectoryRecursive(StringView path);

/**
 * Copy a file from oldPath to newPath
 */
bool copyFile(StringView oldPath, StringView newPath);

/**
 * Move a file from oldPath to newPath
 */
bool moveFile(StringView oldPath, StringView newPath);

/**
 * Remove a file from path
 */
bool removeFile(StringView path);

/**
 * Load a file into memory
 */
Maybe<Binary> loadFile(StringView path);

/**
 * Store a file to disc
 */
bool storeFile(BinaryView bin, StringView path);

} // namespace hg
