// Block device abstractions for the kernel VFS layer
#pragma once

#include <cstdint>

namespace fs {

class BlockDevice {
public:
  virtual ~BlockDevice() = default;

  // Read exactly size bytes starting at byte offset into buffer.
  // Returns true on full success, false on any out-of-bounds or device error.
  virtual bool read(uint64_t offset, uint64_t size, void *buffer) = 0;

  // Total size of the device in bytes
  virtual uint64_t size() const = 0;

  // Preferred block size in bytes (e.g., 512 or 4096)
  virtual uint32_t block_size() const = 0;
};

class MemoryBlockDevice : public BlockDevice {
public:
  MemoryBlockDevice(const void *base_address, uint64_t total_size,
                    uint32_t preferred_block_size = 512)
      : base_(reinterpret_cast<const uint8_t *>(base_address)),
        size_(total_size), block_size_(preferred_block_size) {}

  bool read(uint64_t offset, uint64_t bytes, void *buffer) override {
    if (buffer == nullptr) {
      return false;
    }
    if (offset > size_) {
      return false;
    }
    if (bytes > (size_ - offset)) {
      return false;
    }
    const uint8_t *src = base_ + offset;
    uint8_t *dst = reinterpret_cast<uint8_t *>(buffer);
    for (uint64_t i = 0; i < bytes; ++i) {
      dst[i] = src[i];
    }
    return true;
  }

  uint64_t size() const override { return size_; }

  uint32_t block_size() const override { return block_size_; }

private:
  const uint8_t *base_;
  uint64_t size_;
  uint32_t block_size_;
};

} // namespace fs
