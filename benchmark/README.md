no flushing
./lru llcf 1000 512 80 0

clflush flushing
./lru llcf 1000 512 80 1

clflushopt flushing
./lru llcf 1000 512 80 2

clwb flushing
./lru llcf 1000 512 80 3

