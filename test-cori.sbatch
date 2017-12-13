#!/bin/bash

#SBATCH -J JOB-heat-test
#SBATCH -p debug
#SBATCH -N 2
#SBATCH -n 14
#SBATCH -t 00:30:00
#SBATCH -C haswell
#SBATCH -L SCRATCH,project
#SBATCH -A m2759

# update this path as needed
SPACK_HOME=$HOME/codar/spack

source $SPACK_HOME/share/spack/setup-env.sh
spack load adios@develop mpich sz dataspaces \
    libevpath libffs gtkorvo-atl gtkorvo-cercs-env gtkorvo-dill

srun -n 12 ./heat_transfer_adios2 heat  4 3  40 50  6 500 &
srun -n 2 ./stage_write/stage_write heat.bp staged.bp FLEXPATH "" MPI ""
