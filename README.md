1. Set up environment variables
```bash
source source.sh
```

2. Install/Build the tracer
```bash
./install.sh
```

3. Run the simulator 
Example:
```bash
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

# SETTTINGS
TRACE_DIR=/disk/local/traces/mcf/
OUTPUT_FILE=./mcf.res

#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TRACE=$(find $TRACE_DIR -maxdepth 1 -name "drmemtrace*" -type d)
$SIMULATOR_DIR/build/bin64/drrun -t drcachesim \
                    -indir $TRACE \
                    -pt_dump_file $TRACE_DIR/pt_dump \
                    -warmup_refs     300000000                   \
                    -TLB_L1I_entries 64                          \
                    -TLB_L1I_assoc   8                           \
                    -TLB_L1D_entries 64                          \
                    -TLB_L1D_assoc   8                           \
                    -TLB_L2_entries  1024                        \
                    -TLB_L1D_assoc   8                           \
                    -L1I_size  $(( 32 * 1024 ))                  \
                    -L1I_assoc 8                                 \
                    -L1D_size  $(( 32 * 1024 ))                  \
                    -L1D_assoc 8                                 \
                    -L2_size   $(( 256 * 1024 ))                 \
                    -L2_assoc  8                                 \
                    -LL_size   $(( 16 * 1024 * 1024 ))           \
                    -LL_assoc  16                                \
                    > $OUTPUT_FILE 2>&1 & pid=$! &
```
