#!/bin/bash
TRACE_DIR=/root/workspace/traces/gups
OUTPUT_FILE=./gups.res

TRACE=$(find $TRACE_DIR -maxdepth 1 -name "drmemtrace*" -type d)
$SIMULATOR_DIR/build/bin64/drrun -t drcachesim \
					-indir $TRACE \
					-pt_dump_file $TRACE_DIR/pt_dump \
					-vt_pt_dump_file $TRACE_DIR/vm_pt_dump \
					-warmup_refs	30000000 \
					-TLB_L1I_entries 64 \
					-TLB_L1I_assoc 8 \
					-TLB_L1D_entries 64 \
					-TLB_L1D_assoc 8 \
					-TLB_L2_entries 1536 \
					-TLB_L2_assoc 12 \
					-L1I_size $(( 32 * 1024 )) \
					-L1I_assoc 8 \
					-L1D_size $(( 32 * 1024 )) \
					-L1D_assoc 8 \
					-L2_size $(( 256 * 1024 )) \
					-L2_assoc 8 \
					-LL_size $(( 16 * 1024 * 1024)) \
					-LL_assoc 16 \
					-static -gpa_way 6 \
					> $OUTPUT_FILE 2>&1 & pid=$! &
					#-NTLB \
					#-verbose 2
