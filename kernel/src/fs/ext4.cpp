#include "ext4.hpp"

namespace fs {

static constexpr uint64_t EXT4_SUPERBLOCK_OFFSET = 1024;

struct GdDesc {
  uint32_t bg_block_bitmap;
  uint32_t bg_inode_bitmap;
  uint32_t bg_inode_table; // starting block of inode table
  uint16_t bg_free_blocks_count;
  uint16_t bg_free_inodes_count;
  uint16_t bg_used_dirs_count;
  uint16_t bg_pad;
  uint32_t bg_reserved[3];
};

struct InodeRaw {
  uint16_t i_mode;
  uint16_t i_uid;
  uint32_t i_size_lo;
  uint32_t i_atime;
  uint32_t i_ctime;
  uint32_t i_mtime;
  uint32_t i_dtime;
  uint16_t i_gid;
  uint16_t i_links_count;
  uint32_t i_blocks_lo;
  uint32_t i_flags;
  uint32_t i_osd1;
  uint32_t i_block[15]; // or extent header
};

struct ExtentHeader {
  uint16_t eh_magic;   // 0xF30A
  uint16_t eh_entries; // number of valid entries
  uint16_t eh_max;     // capacity of entries
  uint16_t eh_depth;   // 0 = leaf nodes
  uint32_t eh_generation;
};

struct ExtentLeaf {
  uint32_t ee_block;    // first logical block extent covers
  uint16_t ee_len;      // number of blocks covered
  uint16_t ee_start_hi; // high 16 bits of physical block
  uint32_t ee_start_lo; // low 32 bits of physical block
};

struct DirEnt2 {
  uint32_t inode;
  uint16_t rec_len;
  uint8_t name_len;
  uint8_t file_type;
};

bool Ext4::mount() {
  Ext4Superblock sb{};
  if (!dev_.read(EXT4_SUPERBLOCK_OFFSET, sizeof(Ext4Superblock), &sb)) {
    mounted_ = false;
    return false;
  }
  if (sb.magic != 0xEF53) {
    mounted_ = false;
    return false;
  }
  block_size_ = 1024u << sb.log_block_size;
  if (block_size_ > 4096u) {
    mounted_ = false;
    return false;
  }
  inodes_per_group_ = sb.inodes_per_group;
  blocks_per_group_ = sb.blocks_per_group;
  first_data_block_ = sb.first_data_block;
  inode_size_ = 256;
  mounted_ = true;
  return true;
}

bool Ext4::read_file_by_path(const char * /*path*/, void * /*buffer*/,
                             uint64_t /*max_size*/, uint64_t & /*out_size*/) {
  return false;
}

static inline uint64_t mul_u32_u32(uint32_t a, uint32_t b) {
  return static_cast<uint64_t>(a) * static_cast<uint64_t>(b);
}

bool Ext4::list_dir_by_path(const char *path, Dirent *entries,
                            uint32_t max_entries, uint32_t &out_count) {
  out_count = 0;
  if (!mounted_)
    return false;
  if (!path || path[0] == '\0' || (path[0] == '/' && path[1] == '\0')) {
  } else {
    return false;
  }

  const uint64_t inode_num = 2;
  uint64_t group = (inode_num - 1) / inodes_per_group_;
  uint64_t index_in_group = (inode_num - 1) % inodes_per_group_;

  uint64_t gdt_block = (block_size_ == 1024u) ? 2u : 1u;
  uint64_t gdt_offset =
      mul_u32_u32(static_cast<uint32_t>(gdt_block), block_size_) +
      group * static_cast<uint64_t>(sizeof(GdDesc));

  GdDesc gd{};
  if (!dev_.read(gdt_offset, sizeof(GdDesc), &gd)) {
    return false;
  }

  uint64_t inode_table_block = gd.bg_inode_table;
  uint64_t inode_offset =
      mul_u32_u32(static_cast<uint32_t>(inode_table_block), block_size_) +
      index_in_group * inode_size_;

  uint8_t inode_buf[512];
  if (inode_size_ > sizeof(inode_buf)) {
    return false;
  }
  if (!dev_.read(inode_offset, inode_size_, inode_buf)) {
    return false;
  }
  const InodeRaw &inode = *reinterpret_cast<const InodeRaw *>(inode_buf);

  const uint16_t S_IFDIR = 0x4000;
  if ((inode.i_mode & S_IFDIR) == 0) {
    return false;
  }

  auto parse_dir_block = [&](const uint8_t *blk_data) {
    uint32_t offset = 0;
    while (offset + 8 <= block_size_) {
      const DirEnt2 *de = reinterpret_cast<const DirEnt2 *>(blk_data + offset);
      if (de->rec_len < 8)
        break;
      if (de->rec_len == 0)
        break;
      if (offset + de->rec_len > block_size_)
        break;
      if (de->inode != 0 && de->name_len > 0) {
        if (out_count < max_entries) {
          static char names_bank[128][64];
          if (de->name_len < 64) {
            for (uint32_t i = 0; i < de->name_len; ++i) {
              names_bank[out_count][i] =
                  reinterpret_cast<const char *>(de + 1)[i];
            }
            names_bank[out_count][de->name_len] = '\0';
            entries[out_count].name = names_bank[out_count];
            entries[out_count].name_len = de->name_len;
            entries[out_count].inode = de->inode;
            entries[out_count].type = (de->file_type == 2) ? NodeType::Directory
                                      : (de->file_type == 1)
                                          ? NodeType::File
                                          : NodeType::Unknown;
            out_count++;
          }
        }
      }
      offset += de->rec_len;
    }
  };

  const ExtentHeader *eh =
      reinterpret_cast<const ExtentHeader *>(inode.i_block);
  const uint16_t EXT4_EXT_MAGIC = 0xF30A;
  uint8_t block_buf[4096];
  if (eh->eh_magic == EXT4_EXT_MAGIC) {
    if (eh->eh_depth != 0) {
      return false;
    }
    const ExtentLeaf *exts = reinterpret_cast<const ExtentLeaf *>(eh + 1);
    uint16_t n = eh->eh_entries;
    for (uint16_t i = 0; i < n; ++i) {
      uint64_t phys = (static_cast<uint64_t>(exts[i].ee_start_hi) << 32) |
                      static_cast<uint64_t>(exts[i].ee_start_lo);
      uint32_t count = exts[i].ee_len & 0x7FFFu;
      for (uint32_t b = 0; b < count; ++b) {
        uint64_t blk = phys + b;
        if (!dev_.read(mul_u32_u32(static_cast<uint32_t>(blk), block_size_),
                       block_size_, block_buf)) {
          return false;
        }
        parse_dir_block(block_buf);
        if (out_count >= max_entries)
          return true;
      }
    }
    return true;
  }

  for (uint32_t i = 0; i < 12; ++i) {
    uint32_t blk = inode.i_block[i];
    if (blk == 0)
      continue;
    if (!dev_.read(mul_u32_u32(blk, block_size_), block_size_, block_buf)) {
      return false;
    }
    parse_dir_block(block_buf);
    if (out_count >= max_entries)
      return true;
  }
  return true;
}

} // namespace fs
