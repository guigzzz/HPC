#ifndef workload_hpp
#define workload_hpp

#include <string>

#include "unif01.h"

//! Moves to the next RNG instance.
/*! Call this function to tell workload_Create to move onto
    the next instance.
    For assesment purposes it can be assumed that there are
    an infinite number of RNG instances.
    
    This must not be called in parallel with itself or with
    workload_Next or workload_Name.
*/
void workload_Next();

//! Name of the instance
/*! This can be called in parallel with itself and all other functions.
    */
std::string workload_Name(unif01_Gen *gen);

//! Creates an instance of the currently selected RNG instance
/*! RNG instances always start in the same state, and will generate
    the same stream of numbers. Any two RNG instances are independent,
    and can be used in parallel.
    
    If workload_Next() is called, then the following call to workload_Create
    will create a different instance.
    
    This can be called in parallel with itself and all other functions
    except workload_Next.
*/
unif01_Gen *workload_Create();

//! Take a current RNG, and return an RNG in the same state
/*! A clone of an RNG can operate independently of the original,
    but will generate exactly the same sequence of numbers.
    
    This can be called in parallel with itself and any other workload_* function.
    */
unif01_Gen *workload_Clone(unif01_Gen *original);

//! Destroy (free) the given RNG instance.
/*! This can be called in parallel with itself any other workload_* function. */
void workload_Destroy(unif01_Gen *gen);

#endif
