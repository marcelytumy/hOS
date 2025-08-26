// Minimal VFS interfaces and path helpers
#pragma once

#include <cstdint>

namespace fs {

enum class NodeType : uint8_t { Unknown = 0, File = 1, Directory = 2 };

struct Dirent {
  const char *name;  // not owned
  uint32_t name_len; // excluding null
  NodeType type;     // file or directory
  uint64_t inode;    // filesystem-specific inode number
};

class FileHandle {
public:
  virtual ~FileHandle() = default;
  virtual int64_t read(void *buffer, uint64_t bytes) = 0;
  virtual bool seek(uint64_t offset) = 0;
  virtual uint64_t size() const = 0;
};

class Filesystem {
public:
  virtual ~Filesystem() = default;
  virtual bool mount() = 0;
  virtual bool is_mounted() const = 0;
  virtual bool read_file_by_path(const char *path, void *buffer,
                                 uint64_t max_size, uint64_t &out_size) = 0;
  virtual bool list_dir_by_path(const char *path, Dirent *entries,
                                uint32_t max_entries, uint32_t &out_count) = 0;
};

} // namespace fs
