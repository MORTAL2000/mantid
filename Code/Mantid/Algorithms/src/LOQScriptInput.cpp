//-----------------------------------
// Includes
//-----------------------------------
#include "MantidAlgorithms/LOQScriptInput.h"
#include "MantidKernel/FileValidator.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"

using namespace Mantid::Algorithms;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(LOQScriptInput)

using namespace Mantid::Kernel;
using namespace Mantid::API;

void LOQScriptInput::init()
{
  //First the data files
  MandatoryValidator<std::string> *notEmpty = new MandatoryValidator<std::string>();

  declareProperty<std::string>("SampleWorkspace","",notEmpty, "", Direction::Input);
  declareProperty<std::string>("EmptyCanWorkspace","",notEmpty->clone(), "", Direction::Input);
  declareProperty<std::string>("TransmissionSampleWorkspace","",notEmpty->clone(), "", Direction::Input);
  declareProperty<std::string>("TransmissionDirectWorkspace","",notEmpty->clone(), "", Direction::Input);
  declareProperty<std::string>("TransmissionEmptyCanWorkspace","",notEmpty->clone(), "", Direction::Input);  
 
  //Some of these number need to be strictly  positive
  BoundedValidator<double> *mustBePositive = new BoundedValidator<double>();
  mustBePositive->setLower(0.0);
  
  //Radius range
  declareProperty<double>("RadiusMin",38.0, mustBePositive);
  declareProperty<double>("RadiusMax",419.0, mustBePositive->clone());

  //Wavelength range and binning
  declareProperty<double>("WavelengthMin",2.2, mustBePositive->clone());
  declareProperty<double>("WavelengthMax",10.0, mustBePositive->clone());
  declareProperty<double>("WavelengthDelta",-0.035);

  //Q range and binning
  declareProperty<double>("QMin",0.008, mustBePositive->clone());
  declareProperty<double>("Q_max",0.28, mustBePositive->clone());
  declareProperty<double>("Q_delta",0.02);

  //Beam centre position
  declareProperty<double>("BeamCentreX", 317.5);
  declareProperty<double>("BeamCentreY", 317.5);
  
  //Efficiency correction
  declareProperty("EfficiencyCorrectionFile", "", new FileValidator());
 
}

void LOQScriptInput::exec()
{
  //This algorithm doesn't actually do anything, it is a placeholder for retrieving
  //properties
}

