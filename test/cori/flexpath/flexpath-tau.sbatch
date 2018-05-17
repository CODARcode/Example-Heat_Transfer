#!/bin/bash

#SBATCH -J JOB-HT-FLEX-TAU
#SBATCH -N 2
#SBATCH -A m3084
#SBATCH -q debug
#SBATCH -t 00:30:00
#SBATCH -L SCRATCH
#SBATCH -C haswell

source /opt/cray/pe/modules/3.2.10.6/init/bash

# Edit this to point at desired software stack with TAU
source /global/project/projectdirs/m3084/cluster2018/source-me-tau.sh

EXE_BASE=../../../

ulimit -c unlimited

export TAU_PROFILE=1
export PROFILEDIR=tau.heat
srun -N1 -n 16 $EXE_BASE/heat_transfer_adios2_tau heat  4 4  32 32  10 2 &
export PROFILEDIR=tau.stage
srun -N1 -n 2 $EXE_BASE/stage_write/stage_write_tau heat.bp staged.bp FLEXPATH "" MPI ""
