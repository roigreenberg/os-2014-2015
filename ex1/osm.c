
#include "osm.h"
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#define SUCCESS 0
#define FAIL -1
#define ITERATIONS 50000
#define REPEAT 10

/* Initialization function that the user must call
 * before running any other library function.
 * Returns 0 uppon success and -1 on failure.
 */
int osm_init()
{
	return SUCCESS;
}


/*
 * If number of iterarions is not valid, this will fix it.
 */  
unsigned int fixIterations (unsigned int osm_iterations)
{

	if (osm_iterations == 0)
        {
	    return (ITERATIONS);
        }
	if (osm_iterations % REPEAT != 0)
        {
            return (((osm_iterations / REPEAT) + 1) * REPEAT);
	}
	else
	{
            return (osm_iterations);
	}
}

/*
 * Gets the difference in Nano-seconds between the timevals t1 and t0.
 */  
unsigned long long time_difference_msec(struct timeval t0, struct timeval t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000000000 + (t1.tv_usec - t0.tv_usec) * 1000;
}

void func(){}

/* Time measurement function for an empty function call.
   returns time in nano-seconds upon success, 
   and -1 upon failure.
   Zero iterations number is invalid.
   */
double osm_function_time(unsigned int osm_iterations)
{
    struct timeval tv1, tv2;
    volatile register int i;
    if (gettimeofday(&tv1, NULL) != SUCCESS )
        return FAIL;
    for (i = 0; i < osm_iterations; i = i + REPEAT)
    {
    	func();
	func();
        func();
        func();
        func();
        func();
        func();
        func();
        func();
        func();
    }
    if (gettimeofday(&tv2, NULL) != SUCCESS )
        return FAIL;
    return ((double)time_difference_msec(tv1, tv2) / osm_iterations);
}


/* Time measurement function for an empty trap into the operating system.
   returns time in nano-seconds upon success, 
   and -1 upon failure.
   Zero iterations number is invalid.
   */
double osm_syscall_time(unsigned int osm_iterations)
{
    struct timeval tv1, tv2;
    volatile register int i;
    if (gettimeofday(&tv1, NULL) != SUCCESS )
        return FAIL;
    for (i = 0; i < osm_iterations; i = i + REPEAT)
	{
    	OSM_NULLSYSCALL;
    	OSM_NULLSYSCALL;
    	OSM_NULLSYSCALL;
    	OSM_NULLSYSCALL;
    	OSM_NULLSYSCALL;
    	OSM_NULLSYSCALL;
    	OSM_NULLSYSCALL;
    	OSM_NULLSYSCALL;
    	OSM_NULLSYSCALL;
    	OSM_NULLSYSCALL;
	}
    if (gettimeofday(&tv2, NULL) != SUCCESS )
        return FAIL;
    return ((double)time_difference_msec(tv1, tv2) / osm_iterations);
}

/* Time measurement function for a simple arithmetic operation.
   returns time in nano-seconds upon success,
   and -1 upon failure.
   Zero iterations number is invalid.
   */
double osm_operation_time(unsigned int osm_iterations)
{
    struct timeval tv1, tv2;
    volatile register int i, a;
    a = 0;
    if (gettimeofday(&tv1, NULL) != SUCCESS )
        return FAIL;
    for (i = a; i < osm_iterations; i = i + REPEAT)
    {
        a = 0;
    	a = 0;
    	a = 0;
    	a = 0;
    	a = 0;
    	a = 0;
        a = 0;
        a = 0;
        a = 0;
        a = 0;
    }
    if (gettimeofday(&tv2, NULL) != SUCCESS )
        return FAIL;
    return ((double)time_difference_msec(tv1, tv2) / osm_iterations);
}    


/*
 * the function that meseaurs all the needed times.
 */
timeMeasurmentStructure measureTimes (unsigned int osm_iterations)
{
	timeMeasurmentStructure result;
	if (gethostname(result.machineName, HOST_NAME_MAX) != SUCCESS)
		result.machineName[0] = '\0';
        osm_iterations = fixIterations(osm_iterations);
	result.numberOfIterations = osm_iterations;
	result.instructionTimeNanoSecond = osm_operation_time(osm_iterations);
	result.functionTimeNanoSecond = osm_function_time(osm_iterations);
	result.trapTimeNanoSecond = osm_syscall_time(osm_iterations);
	result.functionInstructionRatio = result.functionTimeNanoSecond /
			result.instructionTimeNanoSecond;
	result.trapInstructionRatio = result.trapTimeNanoSecond /
				result.instructionTimeNanoSecond;

	return result;
}


