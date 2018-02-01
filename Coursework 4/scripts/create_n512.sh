#!/bin/bash
for i in $(seq -w 0 32); do
    mkdir -p w
    bin/generate_sparse_layer 512 512 > w/n512_${i}.bin;
done