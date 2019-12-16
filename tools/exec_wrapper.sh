#!/bin/bash
# Script wrapping execution for GDB, sourcing parameters from external script
friendly_name=$1
binary_path=$2
shift 2
test -f tmp-run-${friendly_name}.sh && . ./tmp-run-${friendly_name}.sh
exec -a ${binary_path} $@ $RUC_PARAMS
