/**
 * @file    mpi_nqueens.cpp
 * @author  Patrick Flick <patrick.flick@gmail.com>
 * @brief   Implements the parallel, master-worker nqueens solver.
 *
 * Copyright (c) 2016 Georgia Institute of Technology. All Rights Reserved.
 */

/*********************************************************************
 *                  Implement your solutions here!                   *
 *********************************************************************/

#include "mpi_nqueens.h"

#include <mpi.h>
#include <vector>
#include "nqueens.h"

//defines the message types used for MPI send and recieve in a readable format
enum Message_Type
{
    result_tag = 1,
    partial_result_tag = 2,
    termination_tag = 3,
    work_request_tag = 4,
    solution_recieved_tag = 5
};

//defines which process is the one distributing work
enum Process_Type
{
    master_process = 0
};

//defines if the worker which is ready for work has a solution or not
enum Ready_Status
{
    not_ready = 0,
    initial_ready = 1,
    solution_ready = 2,
    no_solution_ready = 3
};

//tells whether the partial solution has been recieved by the worker or not
enum Recieved_Status
{
    partial_solution_not_recieved = 0,
    partial_solution_recieved = 1
};

//tells the worker if it should keep looking for work or terminate
enum Running_status
{
    terminate = 0,
    working = 1
};


// stores all local solutions. copied from nqueens.cpp (because it's defined locally there, not in the header)
// each worker uses this to store its local solution and the master uses it to collocate all solutions
struct SolutionStore 
{
    // store solutions in a static member variable
    static std::vector<unsigned int>& solutions() 
    {
        static std::vector<unsigned int> sols;
        return sols;
    }
    static void add_solution(const std::vector<unsigned int>& sol) { solutions().insert(solutions().end(), sol.begin(), sol.end()); }
    static void clear_solutions() { solutions().clear(); }
};


//stores the number of workers who currently have work.  Used to gather all solutions once
//all work has been distributed
struct ActiveWorkers
{
    static unsigned int& active_workers()
    {
        static unsigned int workers;
        return workers;
    }
    static void initialize_workers() { active_workers() = 0; }
    static void add_worker() { ++active_workers(); }
    static void remove_worker() { --active_workers(); }
};

/**
 * @brief Function which obtains a solution, if availible, from a worker and stores it.  Returns which worker sent the result so more work can be given to it.
 */
unsigned int recieve_solution()
{
    unsigned int worker_ready = not_ready;
    MPI_Status ready_status, result_status;
    MPI_Recv(&worker_ready, 1, MPI_UNSIGNED, MPI_ANY_SOURCE, work_request_tag, MPI_COMM_WORLD, &ready_status); //pick any ready worker to do the work
    unsigned int next_worker = ready_status.MPI_SOURCE;
    if(worker_ready == solution_ready) //if the worker has a solution ready to send
    {
        //get the size of the solution to recieve and allocate solution vector
        int solution_size = 0;
        MPI_Probe(next_worker, result_tag, MPI_COMM_WORLD, &result_status);
        MPI_Get_count(&result_status, MPI_UNSIGNED, &solution_size);
        std::vector<unsigned int> recieved_solution(solution_size);

        //recieve and store the solutions
        MPI_Recv(&recieved_solution[0], solution_size, MPI_UNSIGNED, next_worker, result_tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE); 
        SolutionStore::add_solution(recieved_solution); 

        ActiveWorkers::remove_worker(); //this worker is now finished
    }
    if(worker_ready == no_solution_ready) ActiveWorkers::remove_worker(); //the worker found no solutions, this worker is now finished

    return next_worker; //return which worker just reported its solution
}

void distribute_parameters(unsigned int& n, unsigned int& k)
{
    MPI_Bcast(&n, 1, MPI_UNSIGNED, master_process, MPI_COMM_WORLD); // send the total size of the nqueens problem to each worker
    MPI_Bcast(&k, 1, MPI_UNSIGNED, master_process, MPI_COMM_WORLD); // send the number of levels that the master will compute to each worker
}

/**
 * @brief The master's call back function for each found solution.
 *
 * This is the callback function for the master process, that is called
 * from within the nqueens solver, whenever a valid solution of level
 * `k` is found.
 *
 * This function will send the partial solution to a worker which has
 * completed his previously assigned work. As such this function must
 * also first receive the solution from the worker before sending out
 * the new work.
 *
 * @param solution      The valid solution. This is passed from within the
 *                      nqueens solver function.
 */
void master_solution_func(std::vector<unsigned int>& solution) 
{
    // receive solutions or work-requests from a worker and then proceed to send this partial solution to that worker.
    unsigned int next_worker = recieve_solution();
    MPI_Send(&solution[0], solution.size(), MPI_UNSIGNED, next_worker, partial_result_tag, MPI_COMM_WORLD);
    ActiveWorkers::add_worker(); //this worker is now active
}

/**
 * @brief   Performs the master's main work.
 *
 * This function performs the master process' work. It will sets up the data
 * structure to save the solution and call the nqueens solver by passing
 * the master's callback function.
 * After all work has been dispatched, this function will send the termination
 * message to all worker processes, receive any remaining results, and then return.
 *
 * @param n     The size of the nqueens problem.
 * @param k     The number of levels the master process will solve before
 *              passing further work to a worker process.
 */
std::vector<unsigned int> master_main(unsigned int n, unsigned int k) {
    //send the size and number of levels that the master process will solve to all workers
    distribute_parameters(n, k);

    //initialize active workers to 0, this will change as they report in asking for work
    ActiveWorkers::initialize_workers();

    // allocate the vector for the solution permutations
    std::vector<unsigned int> pos(n);

    // generate all partial solutions (up to level k) and call the
    // master solution function
    nqueens_by_level(pos, 0, k, &master_solution_func);

    //get remaining solutions from workers
    while(ActiveWorkers::active_workers() > 0) recieve_solution();

    //tell every process to terminate
    unsigned int keep_running = terminate;
    int number_of_processes;
    MPI_Comm_size(MPI_COMM_WORLD, &number_of_processes);
    for(int current_process = 1; current_process < number_of_processes; ++current_process)
        MPI_Send(&keep_running, 1, MPI_UNSIGNED, current_process, termination_tag, MPI_COMM_WORLD); //tell the workers to stop running

    //return all combined solutions
    std::vector<unsigned int> allsolutions = SolutionStore::solutions();
    SolutionStore::clear_solutions();
    return allsolutions;
}

/**
 * @brief The workers' call back function for each found solution.
 *
 * This is the callback function for the worker processes, that is called
 * from within the nqueens solver, whenever a valid solution is found.
 *
 * This function saves the solution into the worker's solution cache.
 *
 * @param solution      The valid solution. This is passed from within the
 *                      nqueens solver function.
 */
void worker_solution_func(std::vector<unsigned int>& solution) {
    SolutionStore::add_solution(solution); //save the solution into a local cache
}

/**
 * @brief   Performs the worker's main work.
 *
 * This function implements the functionality of the worker process.
 * The worker will receive partially completed work items from the
 * master process and will then complete the assigned work and send
 * back the results. Then again the worker will receive more work from the
 * master process.
 * If no more work is available (termination message is received instead of
 * new work), then this function will return.
 */
void worker_main() {
    //recieve the problem size and number of levels that the master process solved
    unsigned int n, k;
    distribute_parameters(n, k);

    //allocate space for partial solution
    std::vector<unsigned int> pos(n);

    //send initial ready signal to the master
    unsigned int ready_status = initial_ready;
    MPI_Send(&ready_status, 1, MPI_UNSIGNED, master_process, work_request_tag, MPI_COMM_WORLD);

    //set up termination condition.  When the master sends a message telling the process to terminate, computation will end upon completion of the current loop
    unsigned int keep_working = working;
    MPI_Request termination_request;
    MPI_Irecv(&keep_working, 1, MPI_UNSIGNED, master_process, termination_tag, MPI_COMM_WORLD, &termination_request);

    //prepare to recieve partially completed solution
    MPI_Status recieve_status;
    MPI_Request work_request;
    MPI_Irecv(&pos[0], n, MPI_UNSIGNED, master_process, partial_result_tag, MPI_COMM_WORLD, &work_request);
    while(keep_working == working)
    {
        int recieved_flag = partial_solution_not_recieved;
        MPI_Test(&work_request, &recieved_flag, &recieve_status); //test to see if there is any new work from the master
        if(recieved_flag == partial_solution_recieved) //if there is work, do it, otherwise keep waiting, will terminate if the termination request is recieved
        {
            //compute all solutions for given initial configuration
            nqueens_by_level(pos, k, n, &worker_solution_func);
            std::vector<unsigned int> allsolutions = SolutionStore::solutions();
            SolutionStore::clear_solutions();

            //return all solutions, if any, to the master thread
            if(!allsolutions.empty())
            {
                ready_status = solution_ready;
                MPI_Send(&ready_status, 1, MPI_UNSIGNED, master_process, work_request_tag, MPI_COMM_WORLD);
                MPI_Send(&allsolutions[0], allsolutions.size(), MPI_UNSIGNED, master_process, result_tag, MPI_COMM_WORLD);
            }
            else
            {
                ready_status = no_solution_ready;
                MPI_Send(&ready_status, 1, MPI_UNSIGNED, master_process, work_request_tag, MPI_COMM_WORLD);
            }

            //prepare to recieve the next unit of work from the master
            MPI_Irecv(&pos[0], n, MPI_UNSIGNED, master_process, partial_result_tag, MPI_COMM_WORLD, &work_request);
        }
    }
    MPI_Request_free(&work_request); //free the request for more work - none is coming
}


