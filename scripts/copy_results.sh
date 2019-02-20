#!/bin/bash

TEST_SUITE_BUILD=$1
SPEC_ROOT=$2
FILE_NAME=$3
OUT_DIR=$4
#file_name="inscount.out_branches"

# Locate PIN analysis results in the SPEC
for x in `find ${SPEC_ROOT} -name "${FILE_NAME}"`; do
x2=${x#"${SPEC_ROOT}/benchspec/CPU2006/"}
x4=${x2%%/*}
mv $x ${OUT_DIR}/${x4}_branches
done

for x in `find ${TEST_SUITE_BUILD} -name "${FILE_NAME}"`; do
x2=${x#"${TEST_SUITE_BUILD}/External/SPEC/"}
x2=${x2#*/}
x2=${x2%%/*}
mv ${x} ${OUT_DIR}/${x2}_branches
done
