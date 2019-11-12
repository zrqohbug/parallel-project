#!/bin/bash
#PBS -k eo
#PBS -m abe
#PBS -M cl2545@cornell.edu
#PBS -N pa2_nqueen
./test $n $p
