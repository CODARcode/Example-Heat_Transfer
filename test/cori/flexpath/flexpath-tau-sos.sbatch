#!/bin/bash -l

#SBATCH -J JOB-HT-DS-TAU-SOS
#SBATCH -N 3
#SBATCH -A m3084
#SBATCH -q debug
#SBATCH -t 00:05:00
#SBATCH -L SCRATCH
#SBATCH -C haswell

source /opt/cray/pe/modules/3.2.10.6/init/bash

# Edit to point at desired software environment with TAU
source /global/project/projectdirs/m3084/cluster2018/source-me-tau.sh

EXE_BASE=../../../

ulimit -c unlimited

heat_exe="$EXE_BASE/heat_transfer_adios2_tau"
stage_write_exe="$EXE_BASE/stage_write/stage_write_tau"

export TAU_PLUGINS=libTAU-sos-plugin.so
export TAU_PLUGINS_PATH=$TAU_ROOT/craycnl/lib/shared-gnu-papi-mpi-pthread-pdt-sos-adios
export TAU_SOS=1
export SOS_IN_MEMORY_DATABASE=0
export TAU_SOS_TRACING=0


pwd=`pwd`
#export cwd=`pwd`
#export SOS_WORK=${cwd} # This is new
#export SOS_WORK_DIR=${cwd}
export sos_cmd="$SOS_ROOT/bin/sosd -l 2 -a 1 -w ${pwd}"
export SOS_FORK_COMMAND="${sos_cmd} -k @LISTENER_RANK@ -r listener"
export SOS_CMD_PORT=22500
export SOS_EVPATH_MEETUP=${pwd}
export TAU_SOS=1
rm -rf sosd.* profile* *.out

srun -n 1 -N 1 ${sos_cmd} -k 0 -r aggregator &> sosd.out &
sleep 5

export SOS_APP_RANKS_PER_NODE=16
export SOS_LISTENER_RANK_OFFSET=1
export PROFILEDIR=${pwd}/stage_write_tau_out
srun -n 1 -N 1 $stage_write_exe heat_sim_data.bp stage_output.bp FLEXPATH "" POSIX "" "T" "bzip2:9" &> stage_write.out &
sleep 5


export SOS_APP_RANKS_PER_NODE=16
export SOS_LISTENER_RANK_OFFSET=2
export PROFILEDIR=${pwd}/heat_tau_out
srun -n 4 -N 1 $heat_exe heat_sim_data  2 2 32 32 10 2 &> heat.out &

wait
sleep 5
wait

