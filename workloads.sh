## CMDS
declare -A cmds
# gups
cmds["gups"]="../apps/gups/gups 65536"
# canneal
cmds["canneal"]="../apps/parsec-3.0/pkgs/kernels/canneal/src/canneal 1 30000 4000 ../datasets/canneal_32G 6000"
# XSBench
cmds["xsbench"]="../apps/xsbench/openmp-threading/XSBench -t 1 -s XL"
# liblinear
cmds["liblinear"]="../apps/liblinear/train -E /tmp/liblinear ../datasets/kdd12"
# hashjoin
cmds["hashjoin"]="../apps/hashjoin/bin/bench_hashjoin_st"
#memcached
cmds["memcached"]="../apps/memcached_script/client/mutilate-master/mutilate -i fb_ia -s 127.0.0.1 -u 0.03 --noload -t 10 -K fb_key -V fb_value -T 1 -C 1 -S -q 100000 -d 1000000000 --affinity -r 1300000000"
# graphBIG
cmds["bfs"]="../apps/graphBIG/benchmark/bench_BFS/bfs --dataset ../datasets/twitter --enabler /tmp/bfs"
cmds["dc"]="../apps/graphBIG/benchmark/bench_degreeCentr/dc --dataset ../datasets/twitter --enabler /tmp/dc"
cmds["cc"]="../apps/graphBIG/benchmark/bench_connectedComp/connectedcomponent --dataset ../datasets/twitter --enabler /tmp/cc"


declare -A sleeps
sleeps["gups"]=240
sleeps["xsbench"]=3000
sleeps["liblinear"]=7200

declare -A enabler
enabler["liblinear"]="/tmp/liblinear"
enabler["bfs"]="/tmp/bfs"
enabler["dc"]="/tmp/dc"
enabler["cc"]="/tmp/cc"
enabler["hashjoin"]="/tmp/hashjoin"
enabler["canneal"]="/tmp/canneal"
enabler["memcached"]="/tmp/memcached"
