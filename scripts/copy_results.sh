#!/bin/bash

PIN_ROOT=$1
SPEC_ROOT=$2
OUT_DIR=$3
file_name="inscount.out_branches"

# Locate PIN analysis results in the SPEC
for x in `find ${SPEC_ROOT} -name "${file_name}"`; do
x2=${x#"${SPEC_ROOT}/benchspec/CPU2006/"}
x4=${x2%%/*}
mv $x ${OUT_DIR}/${x4}_branches
done

for x in `find ${PIN_ROOT} -name "${file_name}"`; do
x2=${x#"${PIN_ROOT}/External/SPEC/"}
x2=${x2#*/}
x2=${x2%%/*}
mv ${x} ${OUT_DIR}/${x2}_branches
done
