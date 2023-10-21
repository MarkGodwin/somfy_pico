#pragma once


/// @brief Trivial and extremely limited flash block storage with rudimentary wear leveling
class BlockStorage
{
public:
    /// @brief Initialize the block storage
    /// @param base Base address in flash for block storage - must be multiple of FLASH_SECTOR_SIZE (4096)
    /// @param size size of the block storage - must be multiple of FLASH_SECTOR_SIZE (4096)
    /// @param blockSize max size of each block (multiples of 256 - 4 reccomended)
    BlockStorage(uint32_t base, size_t size, size_t blockSize);

    /// @brief Returns a pointer to the block
    /// @param blockId Id of the previously stored block
    /// @return the stored block data (a pointer directly into flash memory). It can't be modified!
    const uint8_t *GetBlock(uint32_t blockId) const;

    /// @brief Stores a block in flash, overwriting any previous block with that ID
    /// @param blockId Id of the block to store
    /// @param data Data to store in the block
    /// @param size Size of the data to store in the block (must be <= blockSize)
    void SaveBlock(uint32_t blockId, const uint8_t *data, size_t size);

    /// @brief Formats (clears) the entire block storage. DANGER!
    void Format();

private:
    uint32_t FindBlock(uint32_t blockId) const;
    void DeletePage(uint32_t pageOffset);

    void FormatEmptySectors();
    void FormatSectors(int sectorNumber, int count);

    uint32_t _base;         // Base address of storage
    uint32_t _sectors;      // Number of sectors allocated to storage
    uint32_t _blockPages;   // Number of pages in each block
};
