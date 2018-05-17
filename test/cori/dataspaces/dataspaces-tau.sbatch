#!/bin/bash

#SBATCH -J JOB-HT-DS-TAU
#SBATCH -N 3
#SBATCH -A m3084
#SBATCH -q debug
#SBATCH -t 00:30:00
#SBATCH -L SCRATCH
#SBATCH -C haswell

source /opt/cray/pe/modules/3.2.10.6/init/bash

# Edit to point at desired software environment with TAU
# dataspaces_server must be in PATH
source /global/project/projectdirs/m3084/cluster2018/source-me-tau.sh

EXE_BASE=../../../

ulimit -c unlimited

export TAU_PROFILE=1

export PROFILEDIR=tau.dataspaces
srun -N1 -n 1 dataspaces_server -s 1 -c 18 &

sleep 5

export PROFILEDIR=tau.stage
srun -N1 -n 2 $EXE_BASE/stage_write/stage_write_tau heat.bp staged.bp \
    DATASPACES "" POSIX "" "T" "none" &

export PROFILEDIR=tau.heat
srun -N1 -n 16 $EXE_BASE/heat_transfer_adios2_tau heat  4 4  32 32  10 2
