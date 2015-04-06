/*
 *  ssa_direct_driver.cpp
 *  
 */
#include "boost_headers.h"

#include <iostream>
#include <string>
#include "StandardEventHandler.h"
#include "StandardDriverTypes.h"
#include "SerialIntervalSimulationDriver.h"
#include "TauLeapingExplicitAdaptive.h"

using namespace STOCHKIT;

template<typename _solverType, typename _outputType>
bool simulate(_solverType& solver, std::size_t realizations, double startTime, double endTime, _outputType& output)
{
	solver.template simulate<_outputType>(realizations, startTime, endTime, output);
	return true;
};

int main(int ac, char* av[])
{

  typedef TauLeapingExplicitAdaptive<StandardDriverTypes::populationType,
    StandardDriverTypes::stoichiometryType, 
    StandardDriverTypes::propensitiesType,
    StandardDriverTypes::graphType> solverType;
  typedef StandardEventHandler<StandardDriverTypes::populationType> eventHandlerType;
  typedef StandardDriverTypes::outputType outputType;
  
  SerialIntervalSimulationDriver<solverType, eventHandlerType, outputType> driver(ac,av,simulate<solverType, outputType>);

  solverType solver=driver.createMixedSolver();
  
  //set solver-specific parameters
  solver.setEpsilon(driver.getCommandLine().getEpsilon());
  solver.setThreshold(driver.getCommandLine().getThreshold());
  solver.setSSASteps(driver.getCommandLine().getSSASteps());

  driver.callSimulate(solver);

  driver.writeOutput();

  return 0;
}
