/* **********************************************************
 * Copyright (c) 2015-2016 Google, Inc.  All rights reserved.
 * **********************************************************/

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Google, Inc. nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL VMWARE, INC. OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "tlb.h"
#include "../common/utils.h"
#include <assert.h>
#include <iostream>


void
tlb_t::init_blocks()
{
    std::cerr << "Initialising a TLB with size: " << num_blocks << std::endl;
    for (int i = 0; i < num_blocks; i++) {
        blocks[i] = new tlb_entry_t;
    }
    //Artemiy add reading PT files
}

cache_result_t
tlb_t::request(const memref_t &memref_in, bool)
{
  assert(0);
}

void
tlb_t::request(const memref_t &memref_in)
{
    assert(0);
}

//Artemiy copypaste
bool
tlb_t::request(const memref_t &memref_in, bool is_gva, bool changed2)
{
    memref_t memref;
    addr_t tag = compute_tag(memref_in.data.addr);
    memref_pid_t pid = memref_in.data.pid;

	if (tag & (1UL << 63))
		std::cerr << "tag 63 bit is set!" << std::endl;

	if (is_gva) {
		tag = tag & 0x7fffffffffffffff;
	} else {
		tag = tag | (1UL << 63);
	}

    bool prepare_to_return = false;
    memref = memref_in;
	if (is_gva)
		memref.data.type = TRACE_TYPE_gVA;
	else
		memref.data.type = TRACE_TYPE_gPA;
    int way;
    int block_idx = compute_block_idx(tag);
	int start = 0;
	int end = associativity;

	if (is_static) {
		if (!is_gva) {
			start = 0;
			end = gpa_way;
		} else {
			start = gpa_way;
			end = associativity;
		}
		if (start == end) {
			stats->access(memref, false); //miss
			return false; // miss
		}
	}

    for (way = start; way < end; ++way) {
        if (get_caching_device_block(block_idx, way).tag == tag &&
            ((tlb_entry_t &)get_caching_device_block(block_idx, way)).pid == pid) {
            stats->access(memref, true /*hit*/);
            if (parent != NULL)
                parent->get_stats()->child_access(memref, true);
            //std::cerr << "TLB hit by search" << std::endl; 
            prepare_to_return = true; //found
			break;
        }
    }

    if (way == end) {
        stats->access(memref, false /*miss*/);
        // If no parent we assume we get the data from main memory
        bool result = false;
        if (parent != NULL) {
            parent->get_stats()->child_access(memref, false);
            result = parent->request(memref, is_gva, true /* changed */);
            //Artemiy add return translation not found in the TLBs
            //std::cerr << "TLB get result from parent " << result << std::endl; 
            prepare_to_return = result;
        }

        way = replace_which_way(block_idx, start, end);
        get_caching_device_block(block_idx, way).tag = tag;
        ((tlb_entry_t &)get_caching_device_block(block_idx, way)).pid = pid;
    }

    access_update(block_idx, way);

    return prepare_to_return;
}

void tlb_t::access_update(int block_idx, int way)
{
    int cnt = get_caching_device_block(block_idx, way).counter;
    // Optimization: return early if it is a repeated access.
    if (cnt == 0)
        return;
    // We inc all the counters that are not larger than cnt for LRU.
    for (int i = 0; i < associativity; ++i) {
        if (i != way && get_caching_device_block(block_idx, i).counter <= cnt)
            get_caching_device_block(block_idx, i).counter++;
    }
    // Clear the counter for LRU.
    get_caching_device_block(block_idx, way).counter = 0;
}

int
tlb_t::replace_which_way(int block_idx, int start, int end)
{
    // We implement LRU by picking the slot with the largest counter value.
    int max_counter = 0;
    int max_way = 0;
    for (int way = start; way < end; ++way) {
        if (get_caching_device_block(block_idx, way).tag == TAG_INVALID) {
            max_way = way;
            break;
        }
        if (get_caching_device_block(block_idx, way).counter > max_counter) {
            max_counter = get_caching_device_block(block_idx, way).counter;
            max_way = way;
        }
    }
    // Set to non-zero for later access_update optimization on repeated access
    get_caching_device_block(block_idx, max_way).counter = 1;
    return max_way;
}
