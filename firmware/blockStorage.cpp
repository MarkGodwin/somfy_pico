// Copyright (c) 2023 Mark Godwin.
// SPDX-License-Identifier: MIT

#include <stdio.h>
#include "picoSomfy.h"
#include "pico/flash.h"
#include <string.h>
#include "blockStorage.h"

#define BLOCK_FREE 0xFFFFFFFF
#define BLOCK_EMPTY 0

BlockStorage::BlockStorage(uint32_t base, size_t size, size_t blockSize)
:   _base(base)
{
    _base = (base / FLASH_SECTOR_SIZE) * FLASH_SECTOR_SIZE;
    _sectors = (size + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE;
    _blockPages = (blockSize + 4 + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE;

    PrintStorageStats();

}

const uint8_t *BlockStorage::GetBlock(uint32_t blockId) const
{
    auto block = FindBlock(blockId);
    if( block == -1)
        return nullptr;

    // Don't return the header
    return (uint8_t *)(block + XIP_BASE + sizeof(uint32_t));
}

uint32_t BlockStorage::FindBlock(uint32_t blockId) const
{
    // Search through the pages until we find the block we are looking for...
    auto offset = _base;
    auto end = offset + _sectors * FLASH_SECTOR_SIZE - _blockPages * FLASH_PAGE_SIZE + 1;

    while(offset < end)
    {
        auto id = (uint32_t *)(XIP_BASE + offset);
        if(*id == blockId)
            return offset;
        offset += _blockPages * FLASH_PAGE_SIZE;
    }

    // We didn't find the block with that ID
    return -1;
}


void BlockStorage::SaveBlock(uint32_t blockId, const uint8_t *data, size_t size)
{
    // Find a free block to store our data
    auto freeBlock = FindBlock(BLOCK_FREE);
    if(freeBlock == -1)
    {
        DBG_PUT("No free blocks found");

        // If there are no free blocks, try to reformat all contiguous empty sectors.
        FormatEmptySectors();
        freeBlock = FindBlock(BLOCK_FREE);
        if(freeBlock == -1)
        {
            DBG_PUT("ERROR: Still no free blocks found");
            return;
        }
    }

    DBG_PRINT("Free block found at 0x%08x\n", freeBlock);

    // First find any existing block and "delete" it by nulling out it's first page
    auto existing = FindBlock(blockId);
    if(existing != -1)
    {
        DBG_PRINT("Deleting existing record at 0x%08x\n", existing);
        DeletePage(existing);
    }

    // Finally, save the block in the free location
    struct PgmData
    {
        uint32_t blockId;
        uint32_t offset;
        const uint8_t *data;
        size_t size;
    };
    PgmData d = {blockId, freeBlock, data, size};

    DBG_PRINT("Flashing new record at... 0x%08x (0x%08x)\n", d.offset, d.size);
    auto result = flash_safe_execute([](void *p) {
        // Copy the first pages directly
        auto params = (PgmData *)p;

        uint8_t tmpPage[FLASH_PAGE_SIZE];

        // The first page includes the ID
        *(uint32_t *)tmpPage = params->blockId;
        uint8_t off = sizeof(uint32_t); // Offset into the page buffer
        uint32_t space = FLASH_PAGE_SIZE - off;
        while(params->size)
        {
            auto bytes = params->size > space ? space : params->size;
            ::memcpy(tmpPage + off, params->data, bytes);
            params->size -= bytes;
            params->data += bytes;
            space -= bytes;
            off += bytes;
            if(space > 0)
                ::memset(tmpPage + off, 0, space);
            DBG_PRINT("Flashing one page at 0x%08x\n", params->offset);

            flash_range_program(params->offset, tmpPage, sizeof(tmpPage));
            params->offset += sizeof(d);
            off = 0; // Next page has no header
            space = sizeof(d);
        }

    }, &d, 1000);
    if(result != PICO_OK)
    {
        DBG_PRINT("Write failed (%d)\n", result);
    }

}

void BlockStorage::ClearBlock(uint32_t blockId)
{
    auto existing = FindBlock(blockId);
    if(existing != -1)
    {
        DBG_PRINT("Deleting existing record at 0x%08x\n", existing);
        DeletePage(existing);
    }
    else
    {
        DBG_PRINT("Record ID %08x not found to delete\n", blockId);
    }
}

void BlockStorage::Format()
{
    DBG_PRINT("Formatting entire block storage: 0x%08x - 0x%08x\n", _base, _sectors * FLASH_SECTOR_SIZE);
    auto result = flash_safe_execute([](void *p) {
        auto pthis = (BlockStorage *)p;
        flash_range_erase(pthis->_base, pthis->_sectors * FLASH_SECTOR_SIZE);
    }, this, 1000);

    if(result == PICO_OK)
        DBG_PUT("Formatting complete");
    else
        DBG_PRINT("Format failed (%d)\n", result);
}

void BlockStorage::PrintStorageStats()
{
    // Print statistics about the block storage...
    auto offset = _base;
    auto end = offset + _sectors * FLASH_SECTOR_SIZE - _blockPages * FLASH_PAGE_SIZE + 1;

    auto usedCount = 0;
    auto freeCount = 0;
    auto clearedCount = 0;
    auto clearedSectors = 0;    // Number of sectors cleared and ready for reformat

    uint32_t emptyStart = offset;

    while(offset < end)
    {
        auto id = (uint32_t *)(XIP_BASE + offset);
        if(*id == 0)
        {
            // A previously cleared block
            clearedCount++;
        }
        else
        {
            auto emptySectorStart = (emptyStart - _base + FLASH_SECTOR_SIZE - 1) / FLASH_SECTOR_SIZE;
            auto emptySectorEnd = (offset - _base) / FLASH_SECTOR_SIZE;
            if(emptySectorStart < emptySectorEnd)
                clearedSectors += (emptySectorEnd - emptySectorStart);

            emptyStart = offset + _blockPages * FLASH_PAGE_SIZE;
            if(*id == 0xFFFFFFFF)
            {
                // A clean formatted block
                freeCount++;
            }
            else
            {
                // A block with data
                usedCount++;
            }
        }
        offset += _blockPages * FLASH_PAGE_SIZE;
    }

    printf("Storage statistics:\n    Sectors:  %d\n    Blocks:   %d\n    Used:     %d\n    Free:     %d\n    Del:      %d\n    Del Sect: %d\n\n", _sectors, (_sectors * FLASH_SECTOR_SIZE) / (_blockPages * FLASH_PAGE_SIZE), usedCount, freeCount, clearedCount, clearedSectors);
}

void BlockStorage::DeletePage(uint32_t pageOffset)
{
    flash_safe_execute( [](void *pg) {

        auto offset = (uint32_t)pg;
        uint8_t zeros[FLASH_PAGE_SIZE];
        ::memset(zeros, 0, FLASH_PAGE_SIZE);
        flash_range_program(offset, zeros, sizeof(zeros));
    }, (void *)pageOffset, 1000);
}

void BlockStorage::FormatEmptySectors()
{

    // Iterate through the blocks, keeping track of fully empty pages
    auto offset = 0;
    auto end = _sectors * FLASH_SECTOR_SIZE;

    // Assume the first sector is empty
    auto emptySectorStart = 0;

    while(offset < end)
    {
        if(*(uint32_t *)(XIP_BASE + _base + offset) != BLOCK_EMPTY)
        {
            // We know the current sector is not empty
            auto emptySectorEnd = offset / FLASH_SECTOR_SIZE;

            // If there were empty sectors up to this sector, format them
            if(emptySectorEnd > emptySectorStart)
                FormatSectors(emptySectorStart, emptySectorEnd - emptySectorStart);

            // Assume the next sector _after_ the end of this block is empty until we know otherwise
            emptySectorStart = emptySectorEnd + 1;
        }
        offset += _blockPages * FLASH_PAGE_SIZE;
    }

    // Format the remaining empty blocks at the end of the storage
    if(_sectors > emptySectorStart)
        FormatSectors(emptySectorStart, _sectors - emptySectorStart);
}


void BlockStorage::FormatSectors(int sector, int count)
{
    DBG_PRINT("Formatting sectors %d to %d\n", sector, sector + count - 1);
    struct FmtParams
    {
        uint32_t base;
        uint32_t size;
    };
    FmtParams params = { _base + sector * FLASH_SECTOR_SIZE,  count * FLASH_SECTOR_SIZE };
    flash_safe_execute( [] (void *p) {
        auto params = (FmtParams *)p;
        flash_range_erase(params->base, params->size);
    }, &params, 1000);
}