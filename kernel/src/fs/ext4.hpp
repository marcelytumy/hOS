// Extremely small read-only ext4 subset
#pragma once

#include "blockdev.hpp"
#include "vfs.hpp"
#include <cstdint>

namespace fs {

// On-disk ext4 superblock (subset)
struct Ext4Superblock {
  uint32_t inodes_count;         // 0x00
  uint32_t blocks_count_lo;      // 0x04
  uint32_t r_blocks_count_lo;    // 0x08
  uint32_t free_blocks_count_lo; // 0x0C
  uint32_t free_inodes_count;    // 0x10
  uint32_t first_data_block;     // 0x14
  uint32_t log_block_size;       // 0x18
  uint32_t log_cluster_size;     // 0x1C
  uint32_t blocks_per_group;     // 0x20
  uint32_t clusters_per_group;   // 0x24
  uint32_t inodes_per_group;     // 0x28
  uint32_t mtime;                // 0x2C
  uint32_t wtime;                // 0x30
  uint16_t mnt_count;            // 0x34
  uint16_t max_mnt_count;        // 0x36
  uint16_t magic;                // 0x38 should be 0xEF53
  uint16_t state;                // 0x3A
  uint16_t errors;               // 0x3C
  uint16_t minor_rev_level;      // 0x3E
  uint32_t lastcheck;            // 0x40
  uint32_t checkinterval;        // 0x44
  uint32_t creator_os;           // 0x48
  uint32_t rev_level;            // 0x4C
  uint16_t def_resuid;           // 0x50
  uint16_t def_resgid;           // 0x52
  // ... we only read up to magic and block size fields
};

class Ext4 : public Filesystem {
public:
  explicit Ext4(BlockDevice &device)
      : dev_(device), mounted_(false), block_size_(1024) {}

  bool mount() override;
  bool is_mounted() const override { return mounted_; }

  bool read_file_by_path(const char *path, void *buffer, uint64_t max_size,
                         uint64_t &out_size) override;
  bool list_dir_by_path(const char *path, Dirent *entries, uint32_t max_entries,
                        uint32_t &out_count) override;

private:
  BlockDevice &dev_;
  bool mounted_;
  uint32_t block_size_;
  uint32_t inodes_per_group_ = 0;
  uint32_t blocks_per_group_ = 0;
  uint16_t inode_size_ = 0;
  uint32_t first_data_block_ = 0;
};

} // namespace fs
