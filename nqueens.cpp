/**
 * @file    nqueens.cpp
 * @author  Patrick Flick <patrick.flick@gmail.com>
 * @brief   Implements the functions for solving the nqueens problem in.
 *
 * Copyright (c) 2016 Georgia Institute of Technology. All Rights Reserved.
 */

#include "nqueens.h"
#include "stdlib.h"

/*********************************************************************
 *                  Implement your solutions here!                   *
 *********************************************************************/

/**
 * @brief   Generates the solutions for the n-queen problem in a specified range.
 *
 *
 * This function will search for all valid solutions in the range
 * [start_level,max_level). This assumes that the partial solution for levels
 * [0,start_level) is a valid solution.
 *
 * The master and the workers will both use this function with different parameters.
 * The master will call the function with start_level=0 and max_level=k, and the
 * workers will call this function with start_level=k and max_level=n and their
 * received partial solution.
 *
 * For every valid solution that is found, the given callback function is called
 * with the current valid solution as parameter.
 *
 * @param pos           A vector of length n that contains the partial solution
 *                      for the range [0, start_level).
 * @param start_level   Gives the level at which the algorithm will start to
 *                      generate solutions. The solution vector `pos` is assumed
 *                      to have a valid solution for positions [0,start_level-1].
 * @param max_level     The level at which the algorithm will stop and pass
 *                      the current valid solution to the callback function.
 * @param success_func  A callback function that is called whenever a valid
 *                      solution of `max_level` levels is found. The found
 *                      solution is passed as parameter to this callback function.
 */




 /**
  * @brief    Searched solution in range [start_level,max_level)
  *
  *For each level starting from start_level till max_level-1, number from 0 till size of array-1 is searched.
  *If queen at this level threaten some other queens,
  *then number at this level is invalid and next number is searched at this level;
  *if queens at this level is fine and does not threaten any other king,
  *then this level is fine, calculation proceeds to next level;
  *if solution is found, then we still proceed to search next number at this level (max_level-1).
  *If all number of crrent level is all searched, calculation goes back to last level,
  *begin to verify the next number of the last level.
  *If all numbers of start_level is searched,
  *then it means all solutions in this range have been searched, function returns.
  */

void nqueens_by_level(std::vector<unsigned int> pos, unsigned int start_level,
                      unsigned int max_level,
                      void (* const success_func)(std::vector<unsigned int>&)) {

  int i,j,k,l,flag=0,size,indexi,stop_sign;
  //flag=0 means in process, flag=1 means that all solutions are found
  std::vector<unsigned int> vec_out(max_level);
  //vec_out used to output
  int *index=NULL;
  //this is a array that shows what number has been searched
  index=new int[max_level];
      

  size=pos.size();

  k=start_level;
  for(k=start_level;k<=max_level-1;k++){
    index[k]=0;
  }
  stop_sign=start_level-1;
  k=start_level;
  i=0;
  j=pos[i];
  while(flag==0){
    if(((i==k)||(j==index[k])||(abs(i-k)==abs(j-index[k])))&&(i<=k-1)) { //if solution is bad
      while((index[k])>=size-1){ //if current index[k] is beyond size of array, all numbers have been searched, k will return back to k-1
	  k=k-1;
	  if(k<=stop_sign){ //if k returns to be even smaller than start_level-1, then every possible solution has been searched, flag=1, all solution are found
	    flag=1;
	  }
	}
	if(flag!=1){
	  index[k]=index[k]+1;
	}
	//for every new search, i has to return to its initial position, i=0
	i=0;
	j=pos[i];
    }


    else{ //if solution is good
      if(i>=k-1){//this process should continue for all i from 0 until i>=k-1. When all [0,k-1) are examined, then we can proceed to next level
	pos[k]=index[k];
	if(k==max_level-1){//if we go to the last row and found solution
	  for (indexi=0;indexi<=max_level-1;indexi++){
	    vec_out[indexi]=pos[indexi];
	  }
	  success_func(vec_out);
	  while((index[k])>=size-1){//if back to the level whose number has already been thoroughly searched; if not, go on to search next level
	    k=k-1;
	    if(k<=stop_sign){
	      flag=1;
	    }
	  }
	  if(flag!=1){
	    index[k]=index[k]+1;
	  }
	  i=0;
	  j=pos[i];
	}
	else{//if it is not the last row, proceed to next row and start from 0
	  k=k+1;
	  index[k]=0;//for the next row, we should start from searching 0
	  i=0;
	  j=pos[i];//to start searching, we should also start i from 0
	}
      }
      else{//if not all i from 0 to k-1 are not examined, go on to examine next i
	i++;
        j=pos[i];
      }
    }
  }
  free(index);
  index=NULL;
}



/*********************************************************************
 *        You don't have to change anything beyond this point        *
 *********************************************************************/


// stores all local solutions.
struct SolutionStore {
  // store solutions in a static member variable
  static std::vector<unsigned int>& solutions() {
    static std::vector<unsigned int> sols;
    return sols;
  }
  static void add_solution(const std::vector<unsigned int>& sol) {
    // add solution to static member
    solutions().insert(solutions().end(), sol.begin(), sol.end());
  }
  static void clear_solutions() {
    solutions().clear();
  }
};

// callback for generating local solutions
void add_solution_callback(std::vector<unsigned int>& solution) {
  SolutionStore::add_solution(solution);
}

// returns all solutions to the n-Queens problem, calculated sequentially
// This function requires that the nqueens_by_level function works properly.
std::vector<unsigned int> nqueens(unsigned int n) {
  std::vector<unsigned int> zero(n, 0);
  nqueens_by_level(zero, 0, n, &add_solution_callback);
  std::vector<unsigned int> allsolutions = SolutionStore::solutions();
  SolutionStore::clear_solutions();
  return allsolutions;
}
