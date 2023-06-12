/*
    Copyright (C) 2018-2019 VMware, Inc.
    SPDX-License-Identifier: GPL-2.0
    Linux kernel module to dump process page-tables.
    The kernel-module is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; version 2.
    The kernel-module  is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.
    You should find a copy of v2 of the GNU General Public License somewhere
    on your Linux system; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include <stdio.h>
#include <fcntl.h>      /* open */
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <sys/mman.h>  /* mlock */
#include <stdlib.h>
#include <limits.h>
#include <numa.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>

#include <page-table-dump.h>


#define BUF_SIZE_BITS 24
#define PARAM(ptr, sz) ((unsigned long) sz << 48 | (unsigned long)ptr)
#define PARAM_GET_PTR(p) (void *)(p & 0xffffffffffff)
#define PARAM_GET_BITS(p) (p >> 48)

#define PTABLE_BASE_MASK(x) ((x) & 0xfffffffff000UL)

void dump_numa_info(struct nodemap *map, FILE *opt_file_out)
{
    int i;

    fprintf(opt_file_out, "<numamap>\n");
    for (i = 0; i < map->nr_nodes; i++) {
        fprintf(opt_file_out, "%d %ld %ld\n", map->node[i].id,
            map->node[i].node_start_pfn, map->node[i].node_end_pfn);
    }
    fprintf(opt_file_out, "</numamap>\n");
}

bool is_pte(long pgtable_type, unsigned long pte)
{
	switch (pgtable_type) {
	  case PTDUMP_REGULAR:
		if (!(pte & 0x1))
			return false;
		break;
	  case PTDUMP_ePT:
		if (!(pte & 0x7)) // = if (bit 0,1,2 is clear)
			return false;
		break;
	}
	return true;
}

/*
 * Three types of page-tables can be dumped:
 * 0: Regular host page tables
 * 1: KVM VM's extended page tables
 * 2: KVM VM's shadow page tables
*/
int main(int argc, char *argv[])
{
    long c;

    if (argc < 3) {
        printf("Usage: dodump <pid> <0|1> [outfile]\n");
        return -1;
    }

    long pid = strtol(argv[1], NULL, 10);

    if (pid == 0) {
        pid = getpid();
    }

    long pgtable_type = strtol(argv[2], NULL, 10);
    if (!(pgtable_type == PTDUMP_REGULAR || pgtable_type == PTDUMP_ePT)) {
        printf("Please enter a valid ptables identifier (argument #2). Valid values:\n");
        printf("0\tHOST_PTABLES\n1\tEPT_PTABLES\n");
        exit(0);
    }

    FILE *opt_file_out = NULL;
    if (argc == 4) {
        opt_file_out = fopen(argv[3], "a");
    }

    if (opt_file_out == NULL) {
        opt_file_out = stdout;
    }

    int f = open("/proc/ptdump", 0);
    if (f < 0) {
        printf ("Can't open device file: %s\n", "/proc/ptdump");
        return -1;
    }

    c = ioctl(f, PTDUMP_IOCTL_PGTABLES_TYPE, pgtable_type);
    if (c < 0) {
        printf("Error while setting pgtable_type\n");
        return -1;

    }

    struct nodemap *numa_map = calloc(1, sizeof(*numa_map));
    if (!numa_map)
        return -ENOMEM;

    c = ioctl(f, PTDUMP_IOCTL_MKCMD(PTDUMP_IOCTL_NUMA_NODEMAP, 0, 256),
            PTDUMP_IOCTL_MKARGBUF(numa_map, 0));
    if (c < 0) {
        printf("Error while fetching numa node map\n");
        free(numa_map);
        return -1;
    }
    //dump_numa_info(numa_map, opt_file_out);
    free(numa_map);

    struct ptdump *result = calloc(1, sizeof(*result));
    if (!result) {
        return -1;
    }

	struct ptdump_table *l4 = calloc(NUM_L4, sizeof(*l4));
	if (!l4)
		return -1;
	struct ptdump_table *l3 = calloc(NUM_L3, sizeof(*l4));
	if (!l3)
		return -1;
	struct ptdump_table *l2 = calloc(NUM_L2, sizeof(*l4));
	if (!l2)
		return -1;
	struct ptdump_table *l1 = calloc(NUM_L1, sizeof(*l4));
	if (!l1)
		return -1;

    mlockall(MCL_CURRENT | MCL_FUTURE | MCL_ONFAULT);

    while(1) {
        /* check if the pid still exists before collecting dump.
         * This is racy as a new process may acquire the same pid
         * but the chances of that happenning for us is really really low .
         */
        if (kill(pid, 0) && errno == ESRCH)
            break;

        result->processid = pid;

        c = ioctl(f, PTDUMP_IOCTL_MKCMD(PTDUMP_IOCTL_CMD_DUMP, 0, 256),
                     PTDUMP_IOCTL_MKARGBUF(result, 0));
        if (c < 0) {
            fprintf(opt_file_out,"<ptdump process=\"%ld\" error=\"%ld\"></ptdump>\n", pid, c);
            goto ptdump_error;
        }

		// l4, l3, l2, l1 init
		unsigned long l4_idx = 0;
		unsigned long l3_idx = 0;
		unsigned long l2_idx = 0;
		unsigned long l1_idx = 0;
		for (unsigned long i = 0; i < result->num_tables; i++) {
			int level = PTDUMP_TABLE_EXLEVEL(result->table[i].base);
			switch(level) {
				case 1:
					l1[l1_idx++] = result->table[i];
					break;
				case 2:
					l2[l2_idx++] = result->table[i];
					break;
				case 3:
					l3[l3_idx++] = result->table[i];
					break;
				case 4:
					l4[l4_idx++] = result->table[i];
					break;
				default:
					continue;
			}
		}

        fprintf(opt_file_out, "PID = %ld\n", pid);
		fprintf(opt_file_out, "CR3 = %016lx\n", PTABLE_BASE_MASK(l4[0].base));
		// start ptdump print
		// start from l4
		for (unsigned long l4e = 0; l4e < 512; l4e++) {
			unsigned long l4e_vbase = l4[0].vbase + (l4e << 39);
			unsigned long l4e_pa = PTABLE_BASE_MASK(l4[0].entries[l4e]) >> 12 << 12;
			if (!is_pte(pgtable_type, l4[0].entries[l4e])) {
				continue;
			}
			fprintf(opt_file_out, "PML4 entry #%03ld 0 %016lx %016lx\n",
						l4e, l4e_vbase, l4e_pa);

			// search l3 with l4e
			for (int l3t = 0; l3t < l3_idx; l3t++) {
				unsigned long l3t_base = PTABLE_BASE_MASK(l3[l3t].base);
				if (l3t_base == l4e_pa) {
					// start print l3
					for (unsigned long l3e = 0; l3e < 512; l3e++) {
						unsigned long l3e_vbase = l3[l3t].vbase + (l3e << 30);
						unsigned long l3e_pa = PTABLE_BASE_MASK(l3[l3t].entries[l3e]) >> 12 << 12;
						if (!is_pte(pgtable_type, l3[l3t].entries[l3e]))
							continue;
						fprintf(opt_file_out, "  PDPT entry #%03ld 0 %016lx %016lx\n",
								l3e, l3e_vbase, l3e_pa);

						// search l2 with l3e
						for (int l2t = 0; l2t < l2_idx; l2t++) {
							unsigned long l2t_base = PTABLE_BASE_MASK(l2[l2t].base);
							if (l2t_base == l3e_pa) {
								// start print l2
								for (unsigned long l2e = 0; l2e < 512; l2e++) {
									unsigned long l2e_vbase = l2[l2t].vbase + (l2e << 21);
									unsigned long l2e_pa = 
										PTABLE_BASE_MASK(l2[l2t].entries[l2e]) >> 12 << 12;
									if (!is_pte(pgtable_type, l2[l2t].entries[l2e]))
										continue;
									fprintf(opt_file_out, "    PD entry #%03ld 0 %016lx %016lx\n",
											l2e, l2e_vbase, l2e_pa);

									// search l1 with l2e
									for (int l1t = 0; l1t < l1_idx; l1t++) {
										unsigned long l1t_base = PTABLE_BASE_MASK(l1[l1t].base);
										if (l1t_base == l2e_pa) {
											// start print l1
											for (unsigned long l1e = 0; l1e < 512; l1e++) {
												unsigned long l1e_vbase = l1[l1t].vbase + (l1e << 12);
												unsigned long l1e_pa =
													PTABLE_BASE_MASK(l1[l1t].entries[l1e]) >> 12 << 12;
												if (!is_pte(pgtable_type, l1[l1t].entries[l1e]))
													continue;
												fprintf(opt_file_out, 
														"      PT-page entry #%03ld 0 %016lx %016lx\n",
													l1e, l1e_vbase, l1e_pa);
											}
											break;
										}
									}
								}
								break;
							}
						}
					}
					break;
				}
			}
		}
		break;
    }

ptdump_error:

    free(result);
    close(f);
    return 0;
}
