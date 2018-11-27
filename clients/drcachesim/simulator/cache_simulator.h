/* **********************************************************
 * Copyright (c) 2015-2018 Google, Inc.  All rights reserved.
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

/* cache_simulator: controls the multi-cache-level simulation.
 */

#ifndef _CACHE_SIMULATOR_H_
#define _CACHE_SIMULATOR_H_ 1

#include <unordered_map>
#include "simulator.h"
#include "cache_simulator_create.h"
#include "cache_stats.h"
#include "cache.h"
#include <map>
#include <unordered_map>

class cache_simulator_t : public simulator_t {
public:
    // This constructor is used when the cache hierarchy is configured
    // using a set of knobs. It assumes a 2-level cache hierarchy with
    // private L1 data and instruction caches and a shared LLC.
    cache_simulator_t(const cache_simulator_knobs_t &knobs, const tlb_simulator_knobs_t &tlb_knobs);

    // This constructor is used when the arbitrary cache hierarchy is
    // defined in a configuration file.
    cache_simulator_t(const std::string &config_file);

    virtual ~cache_simulator_t();
    virtual bool
    process_memref(const memref_t &memref);
    virtual std::pair<bool,bool>
    process_memref(const memref_t &memref, bool changed);
    virtual bool
    print_results();

    // Exposed to make it easy to test
    bool
    check_warmed_up();
    uint64_t
    remaining_sim_refs() const;

protected:
    // Create a cache_t object with a specific replacement policy.
    virtual cache_t *
    create_cache(const std::string &policy);

    cache_simulator_knobs_t knobs;

    // Implement a set of ICaches and DCaches with pointer arrays.
    // This is useful for implementing polymorphism correctly.
    cache_t **l1_icaches;
    cache_t **l1_dcaches;
    cache_t **l2_caches;

    //Artemiy: add TLB
    analysis_tool_t * tlb_sim;

    struct page_table_info_t {
      long long unsigned int VA;
      long long unsigned int PE1;
      long long unsigned int PE2;
      long long unsigned int PE3;
      long long unsigned int PE4;
      long long unsigned int PA;
    };
      
    //Artemiy: add TLB
    //typedef std::map<unsigned int, page_table_info_t> page_table_t;
    typedef std::unordered_map<long long unsigned int, page_table_info_t> page_table_t;
    page_table_t page_table;
    std::vector<uint64_t> hit_statistics;
    std::vector<uint64_t> miss_statistics;

    struct range_info_t {
      long long unsigned int l_bound;
      long long unsigned int h_bound;
    };
    typedef std::vector<range_info_t> range_table_t;
    range_table_t range_table;
  
    typedef std::vector<cache_result_t> page_walk_hm_result_t;
    typedef std::map< page_walk_hm_result_t, uint64_t> hm_full_statistic_t;
    hm_full_statistic_t hm_full_statistic;
    page_walk_hm_result_t page_walk_res;

    // The following unordered maps map a cache's name to a pointer to it.
    std::unordered_map<std::string, cache_t *> llcaches;     // LLC(s)
    std::unordered_map<std::string, cache_t *> other_caches; // Non-L1, non-LLC caches
    std::unordered_map<std::string, cache_t *> all_caches;   // All caches.

private:
    bool is_warmed_up;
};

#endif /* _CACHE_SIMULATOR_H_ */
