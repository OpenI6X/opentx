/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 * th9x - http://code.google.com/p/th9x
 * er9x - http://code.google.com/p/er9x
 * gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */


#ifndef _BIN_ALLOCATOR_H_
#define _BIN_ALLOCATOR_H_

#include "debug.h"
#include <cstddef> // For size_t
#include <cstring> // For memset

// A dummy PACK macro for demonstration if not defined elsewhere.
// On GCC/Clang, this would typically be: #define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#ifndef PACK
#define PACK( __Declaration__ ) __Declaration__
#endif

template <int SIZE_SLOT, int NUM_BINS> class BinAllocator {
private:
    // The Bin struct now uses a union.
    // When a bin is allocated, we use the 'data' member.
    // When it's free, we use the 'next_free' member to form a linked list of free bins.
    PACK(struct Bin {
        union {
            char data[SIZE_SLOT];
            Bin* next_free;
        };
    });

    Bin Bins[NUM_BINS];
    int NoUsedBins;
    Bin* free_list_head; // A pointer to the first free bin in the list.

public:
    BinAllocator() : NoUsedBins(0), free_list_head(nullptr) {
        // Initialize the free list by linking all bins together.
        // We iterate backwards so the head of the list ends up being Bins[0].
        for (int i = NUM_BINS - 1; i >= 0; --i) {
            Bins[i].next_free = free_list_head;
            free_list_head = &Bins[i];
        }
    }

    // free() is now an O(1) operation.
    bool free(void * ptr) {
        if (!is_member(ptr)) {
            return false;
        }

        // The provided pointer is to the 'data' member of a Bin.
        // We can cast it back to a Bin pointer because 'data' is the first member.
        Bin* bin_to_free = reinterpret_cast<Bin*>(ptr);

        // Add the newly freed bin to the front of the free list.
        bin_to_free->next_free = free_list_head;
        free_list_head = bin_to_free;

        --NoUsedBins;
        return true;
    }

    // is_member() checks if a pointer belongs to this allocator's memory pool.
    bool is_member(void * ptr) {
        // The pointer must be within the bounds of the entire Bins array.
        const char* start_address = reinterpret_cast<const char*>(&Bins[0]);
        const char* end_address = reinterpret_cast<const char*>(&Bins[NUM_BINS]); // Address just after the last bin

        auto ptr_char = reinterpret_cast<const char*>(ptr);

        // Check if the pointer is within the array's memory range
        if (ptr_char < start_address || ptr_char >= end_address) {
            return false;
        }

        // Check if the pointer is aligned to the start of a Bin.
        // The difference between the pointer and the start of the array
        // must be a multiple of the size of a single Bin.
        return ((ptr_char - start_address) % sizeof(Bin)) == 0;
    }

    // malloc() is now an O(1) operation.
    void * malloc(size_t size) {
        // Ensure requested size is not too large and that there are free bins.
        if (size > SIZE_SLOT || free_list_head == nullptr) {
            return nullptr;
        }

        // Get the first bin from the free list.
        Bin* allocated_bin = free_list_head;

        // Update the head to point to the next free bin.
        free_list_head = allocated_bin->next_free;

        ++NoUsedBins;

        // Return a pointer to the data portion of the bin.
        return allocated_bin->data;
    }

    size_t size(void * ptr) {
        return is_member(ptr) ? SIZE_SLOT : 0;
    }

    bool can_fit(void * ptr, size_t size) {
        return is_member(ptr) && size <= SIZE_SLOT;
    }

    unsigned int capacity() { return NUM_BINS; }
    unsigned int used_bins() { return NoUsedBins; }
};

#if defined(SIMU)
typedef BinAllocator<39,300> BinAllocator_slots1;
typedef BinAllocator<79,100> BinAllocator_slots2;
#else
typedef BinAllocator<27,200> BinAllocator_slots1;
typedef BinAllocator<91,50> BinAllocator_slots2;
#endif

#if defined(USE_BIN_ALLOCATOR)
extern BinAllocator_slots1 slots1;
extern BinAllocator_slots2 slots2;

void *bin_l_alloc (void *ud, void *ptr, size_t osize, size_t nsize);
#endif

#endif // _BIN_ALLOCATOR_H_
