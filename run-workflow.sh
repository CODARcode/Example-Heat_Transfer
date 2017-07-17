#!/bin/sh
set -eu

if [ ${#} -lt 1 ]
then
  echo "Usage: $0 PROCS [DATASPACES|FLEXPATH]"
  exit 1
fi

PROCS=$1
WORKFLOW=workflow

if [ ${#} -gt 1 ]
then
  if [ "$2" = "DATASPACES" ]
  then
    WORKFLOW=workflow-ds
    rm -f conf srv.lock *bp
  elif [ "$2" = "FLEXPATH" ]
  then
    WORKFLOW=workflow
  fi 
fi

# USER: Set this to the correct location:
LAUNCH=/home/pdavis/codar/spack/opt/spack/linux-ubuntu16-x86_64/gcc-5.4.0/mpix-launch-swift-develop-zcj3o456mlrlnmpoxkv5vtrmwx2t5rfh/src

MACHINE=${MACHINE:-}

THIS=$( dirname $0 )

rm -f heat.bp_*_info.txt
stc -p -u -I $LAUNCH -r $LAUNCH ${WORKFLOW}.swift
turbine -n $PROCS $MACHINE ${WORKFLOW}.tic
