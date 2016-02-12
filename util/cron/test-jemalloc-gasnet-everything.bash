#!/usr/bin/env bash
#
# Test gasnet (segment everything) w/ jemalloc against full suite on linux64.

CWD=$(cd $(dirname $0) ; pwd)
source $CWD/common-gasnet.bash
source $CWD/common-jemalloc.bash

export CHPL_NIGHTLY_TEST_CONFIG_NAME="jemalloc-gasnet-everything"

export GASNET_QUIET=Y

# Test a GASNet compile using the default segment (everything for linux64)
export CHPL_GASNET_SEGMENT=everything

$CWD/nightly -cron