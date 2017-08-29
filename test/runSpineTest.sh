#!/bin/bash
SCRIPT=`pwd`/$0

FILENAME=`basename $SCRIPT`
PATHNAME=`dirname $SCRIPT`
NVM_CHECK="$PATHNAME"/checkNvm.sh
ROOT=$PATHNAME/..
NVM_CHECK="$ROOT"/scripts/checkNvm.sh

. $NVM_CHECK

export LD_LIBRARY_PATH=/opt/licode/erizo/build/erizo

cd $ROOT/spine
node runSpineClients -s ../results/config_${TESTPREFIX}_${TESTID}.json -t $DURATION -i 1 -o $ROOT/results/output_${TESTPREFIX}_${TESTID}.json
