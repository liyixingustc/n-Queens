#!/bin/sh

# Example PBS script for Jinx

# allocate 4 of the (24 total) sixcore nodes for up to 5 minutes
#PBS -q class
#PBS -l nodes=4:sixcore
#PBS -l walltime=00:10:00
#PBS -N cse6220-nqueens

# change to our project directory
cd $HOME/6220_project_1/programs

# hardcode MPI path
MPIRUN=/usr/lib64/openmpi/bin/mpirun

# set the n-queens master-depth k
MASTER_DEPTH=4
# set size of the problem instance
N=10


# loop over number of processors (our 4 nodes job can run up to 48)
#for p in 6 12 24 48
#for p in 8 16 32
#do
p=6
    $MPIRUN -np $p --hostfile $PBS_NODEFILE ./nqueens -o $N $MASTER_DEPTH
#done
