#include "MantidMDAlgorithms/ConvertToQ3DdE.h"

#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/NumericAxis.h"

#include "MantidKernel/PhysicalConstants.h"

#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/ProgressText.h"
#include "MantidAPI/Progress.h"

#include "MantidMDEvents/MDEvent.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"

#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ArrayProperty.h"

#include <float.h>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
//using namespace Mantid::MDEvents;

namespace Mantid
{
namespace MDAlgorithms
{
// logger for loading workspaces  
   Kernel::Logger& ConvertToQ3DdE::convert_log =Kernel::Logger::get("MD-Algorithms");

// the variable describes the locations of the preprocessed detectors, which can be stored and reused it the algorithm runs for more once;
preprocessed_detectors ConvertToQ3DdE::det_loc;
// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ConvertToQ3DdE)
  
/// Sets documentation strings for this algorithm
void ConvertToQ3DdE::initDocs()
{
    this->setWikiSummary("Create a MDEventWorkspace with in the reciprocal space of momentums (Qx, Qy, Qz) and the energy transfer dE from an input transformed to energy workspace. If the OutputWorkspace exists, then events are added to it.");
    this->setOptionalMessage("Create a MDEventWorkspace with in the reciprocal space of momentums (Qx, Qy, Qz) and the energy transfer dE from an input transformed to energy workspace. If the OutputWorkspace exists, then events are added to it.");
}

//----------------------------------------------------------------------------------------------
/** Constructor
*/
ConvertToQ3DdE::ConvertToQ3DdE()
{
}
    
//----------------------------------------------------------------------------------------------
/** Destructor
 */
ConvertToQ3DdE::~ConvertToQ3DdE()
{
}

/// MDEvent dimensions used 
typedef MDEvents::MDEvent<4> MDE;
//
const double rad2deg = 180.0 / M_PI;
/// helper function to create empty MDEventWorkspace with 4 dimensions 
boost::shared_ptr<MDEvents::MDEventWorkspace<MDE,4> > create_empty4DEventWS(const std::string dimensionNames[4],const std::string dimensionUnits[4],
                                                                              const std::vector<double> &dimMin,const std::vector<double> &dimMax)
{

       boost::shared_ptr<MDEvents::MDEventWorkspace<MDE,4> > ws = 
       boost::shared_ptr<MDEvents::MDEventWorkspace<MDE, 4> >(new MDEvents::MDEventWorkspace<MDE, 4>());
    
      // Give all the dimensions
      for (size_t d=0; d<4; d++)
      {
        MDHistoDimension * dim = new MDHistoDimension(dimensionNames[d], dimensionNames[d], dimensionUnits[d], dimMin[d], dimMax[d], 10);
        ws->addDimension(MDHistoDimension_sptr(dim));
      }
      ws->initialize();

      // Build up the box controller
      MDEvents::BoxController_sptr bc = ws->getBoxController();
      bc->setSplitInto(5);
//      bc->setSplitThreshold(1500);
      bc->setSplitThreshold(10);
      bc->setMaxDepth(20);
      // We always want the box to be split (it will reject bad ones)
      ws->splitBox();
      return ws;
}

/// helper function to preprocess the detectors directions
void 
ConvertToQ3DdE::process_detectors_positions(const DataObjects::Workspace2D_const_sptr inputWS)
{

    const size_t nHist = inputWS->getNumberHistograms();

    det_loc.det_dir.resize(nHist);
    det_loc.det_id.resize(nHist);
     // Loop over the spectra
   size_t ic(0);
   for (size_t i = 0; i < nHist; i++){

     Geometry::IDetector_const_sptr spDet;
     try{
        spDet= inputWS->getDetector(i);
     }catch(Kernel::Exception::NotFoundError &){
        continue;
     }
 
    // Check that we aren't dealing with monitor...
    if (spDet->isMonitor())continue;   

     det_loc.det_id[ic] = spDet->getID();
    // dist     =  spDet->getDistance(*sample);
     double polar    =  inputWS->detectorTwoTheta(spDet);
     double azim     =  spDet->getPhi();    

     double sPhi=sin(polar);
     double ez = cos(polar);
     double ex = sPhi*cos(azim);
     double ey = sPhi*sin(azim);
 
     det_loc.det_dir[ic].setX(ex);
     det_loc.det_dir[ic].setY(ey);
     det_loc.det_dir[ic].setZ(ez);

     ic++;
   }
   // 
   if(ic<nHist){
       det_loc.det_dir.resize(ic);
       det_loc.det_id.resize(ic);
   }
}
//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void 
ConvertToQ3DdE::init()
{
      CompositeWorkspaceValidator<> *ws_valid = new CompositeWorkspaceValidator<>;
      ws_valid->add(new WorkspaceUnitValidator<>("DeltaE"));
      ws_valid->add(new HistogramValidator<>);
      ws_valid->add(new InstrumentValidator<>);


    declareProperty(new WorkspaceProperty<MatrixWorkspace>("InputWorkspace","",Direction::Input,ws_valid),
        "An input Matrix Workspace 2D, processed by Convert to energy (homer) algorithm and its x-axis has to be in the units of energy transfer with energy in mev.");
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("OutputWorkspace","",Direction::Output),
        "Name of the output MDEventWorkspace. If the workspace already exists, then the events will be added to it.");

     BoundedValidator<double> *minEn = new BoundedValidator<double>();
     minEn->setLower(0);
     declareProperty("EnergyInput",0.000000001,minEn,"The value for the incident energy of the neutrons leaving the source (meV)",Direction::InOut);

     // this property is mainly for subalgorithms to set-up as they have to identify 
    declareProperty(new PropertyWithValue<bool>("UsePreprocessedDetectors", true, Direction::Input), 
        "Store the part of the detectors transfromation into reciprocal space to save/reuse it later;");
    // // this property is mainly for subalgorithms to set-up as they have to identify 
    //declareProperty(new ArrayProperty<double>("u"), "first base vecotor");
    //declareProperty(new ArrayProperty<double>("v"), "second base vecotors");


     declareProperty(new ArrayProperty<double>("MinQdE_values"),
         "An array containing minimal values for Q[A^-1] and energy transfer[meV] in a form qx_min,qy_min,qz_min, dE min\n"
         "(momentum and energy transfer values lower that this one will be ignored if this is set.\n"
         " If a minimal output workspace range is higer then specified, the workspace range will be used intstead)" );

     declareProperty(new ArrayProperty<double>("MaxQdE_values"),
         "An array containing maximal values for Q[A^-1] and energy transfer[meV] in a form qx_max,qy_max,qz_max, dE_max\n"
         "(momentum and energy transfer values higher that this one will be ignored if this is set.\n"
         " If a maximal output workspace ranges is lower, then one of specified, the workspace range will be used instead)" );


}

void 
ConvertToQ3DdE::check_max_morethen_min(const std::vector<double> &min,const std::vector<double> &max){
    for(size_t i=0; i<min.size();i++){
        if(max[i]<=min[i]){
            convert_log.error()<<" min value "<<min[i]<<" not less then max value"<<max[i]<<" in direction: "<<i<<std::endl;
            throw(std::invalid_argument("min limit not smaller then max limit"));
        }
    }
}
 

 
  //----------------------------------------------------------------------------------------------
  /* Execute the algorithm.   */
void ConvertToQ3DdE::exec(){
    Timer tim, timtotal;
    CPUTimer cputim, cputimtotal;

     // -------- Input workspace 
    MatrixWorkspace_sptr inMatrixWS = getProperty("InputWorkspace");
    Workspace2D_sptr inWS2D         = boost::dynamic_pointer_cast<Workspace2D>(inMatrixWS);


    // get the energy axis
    NumericAxis *pEnAxis = dynamic_cast<NumericAxis *>(inWS2D->getAxis(0));
    if(!pEnAxis){
       convert_log.error()<<"Can not get proper energy axis from processed workspace\n";
       throw(std::invalid_argument("Input workspace is not propwer converted to energy workspace"));
    }
    size_t lastInd = inWS2D->getAxis(0)->length()-1;
    double E_max = inWS2D->getAxis(0)->operator()(lastInd);
    double E_min = inWS2D->getAxis(0)->operator()(0);

    //double E_min = inWS2D->readX(0).front();
    //double E_max = inWS2D->readX(0).back();
    if(E_min>=E_max){
        convert_log.error()<<" expecting to process energy form "<<E_min<<" to "<<E_max<<" but Emin>=Emax\n";
        throw(std::invalid_argument(" Emin>=Emax"));
    }
    //*** Input energy
    // get and check input energy 
    double Ei = getProperty("EnergyInput");
    // check if workspace knows better 
    if(inWS2D->run().hasProperty("Ei")){
        double Ei_t = boost::lexical_cast<double>(inWS2D->run().getProperty("Ei")->value());
        if(std::fabs(Ei-Ei_t)>double(FLT_EPSILON)){
            g_log.information()<<" energy: "<<Ei<<" obtained from the algorithm parameters has been replaced by the energy:"<<Ei_t<<", obtained from the workspace\n";
            Ei=Ei_t;
            setProperty("EnergyInput", Ei);
        }

    }
    if (E_max >Ei){
        convert_log.error()<<"Maximal elergy transferred to sample eq "<<E_max<<" and exceeds the input energy "<<Ei<<std::endl;
        throw(std::invalid_argument("Maximal transferred energy exceeds input energy"));
    }

    // the wawe vector of input neutrons;
    double ki=sqrt(Ei/PhysicalConstants::E_mev_toNeutronWavenumberSq);
    std::vector<double> QEmin = getProperty("MinQdE_values");
    std::vector<double> QEmax = getProperty("MaxQdE_values");
    
    // Try to get the output workspace
    IMDEventWorkspace_sptr i_out = getProperty("OutputWorkspace");
    boost::shared_ptr<MDEvents::MDEventWorkspace<MDE,4> > ws  = boost::dynamic_pointer_cast<MDEvents::MDEventWorkspace<MDE,4> >( i_out );

    std::string dimensionNames[4] = {"Q_x", "Q_y", "Q_z","DeltaE"};
    if (ws){
       //check existing worspace limits and agree these with new limits if they were specified;
       if(QEmin.empty())QEmin.assign(4, FLT_MAX);
       if(QEmax.empty())QEmax.assign(4,-FLT_MAX);
       for(int i=0;i<4;i++){
           // Check that existing workspace dimensions make sense with the desired one (using the name)
            if (ws->getDimension(i)->getName() != dimensionNames[i]){
               convert_log.error()<<"The existing MDEventWorkspace " + ws->getName() + " has different dimensions than were requested! Either give a different name for the output, or change the OutputDimensions parameter.\n";
               throw std::runtime_error("The existing MDEventWorkspace " + ws->getName() + " has different dimensions than were requested! Either give a different name for the output, or change the OutputDimensions parameter.");
            }
            // coordinate existing and nwe 
            double ws_min = ws->getDimension(i)->getMinimum();
            double ws_max = ws->getDimension(i)->getMaximum();
            if(ws_min>QEmin[i])QEmin[i]=ws_min;
            if(ws_max<QEmax[i])QEmax[i]=ws_max;
       }
       // verify that final limits are correct
       check_max_morethen_min(QEmin,QEmax);
    }else{
        if(QEmin.empty()||QEmax.empty()){
            convert_log.error()<<" min and max Q-dE values can not be empty when creating new workspace";
            throw(std::invalid_argument(" min-max property is empty"));
        }
        if(QEmin.size()!=4||QEmax.size()!=4){
            convert_log.error()<<" min and max Q-dE values have to had 4 elements each";
            throw(std::invalid_argument(" min-max is not 4Dimensional"));
        }
        check_max_morethen_min(QEmin,QEmax);
        std::string dimensionUnits[4] = {"Amgstroms^-1", "Amgstroms^-1", "Amgstroms^-1","meV"};
        ws = create_empty4DEventWS(dimensionNames,dimensionUnits,QEmin,QEmax);
    }
    ws->splitBox();
    //BoxController_sptr bc = ws->getBoxController();
   // if (!bc)
   //   throw std::runtime_error("Output MDEventWorkspace does not have a BoxController!");

    // copy experiment info into 
    ExperimentInfo_sptr ExperimentInfo(inWS2D->cloneExperimentInfo());
    uint16_t runIndex = ws->addExperimentInfo(ExperimentInfo);

 

    // Initalize the matrix to 3x3 identity
    Kernel::Matrix<double> mat = Kernel::Matrix<double>(3,3, true);
    // Set the matrix based on UB etc.
    Kernel::Matrix<double> ub = inWS2D->sample().getOrientedLattice().getUB();
    Kernel::Matrix<double> gon =inWS2D->run().getGoniometer().getR();
    // As per Busing and Levy 1967, HKL = Goniometer * UB * q_lab_frame
    mat = gon * ub;
    std::vector<double> rotMat = mat.get_vector();

    const size_t numSpec  = inWS2D->getNumberHistograms();
    const size_t specSize = inWS2D->blocksize();    

    // Initialise the progress reporting object
    Progress progress(this,0.0,1.0,numSpec);
   // Try to check if one should use preprocessed detector positions or try to calculate the new one
    bool reuse_preprocecced_detectors = getProperty("UsePreprocessedDetectors");
    if(!(reuse_preprocecced_detectors&&det_loc.is_defined()))process_detectors_positions(inWS2D);
 

    // allocate the events buffer;
   //   std::vector<MDE> out_events;
   //   out_events.reserve(specSize);
    // To track when to split up boxes
  //   size_t eventsAdded = 0;
  //   size_t lastNumBoxes = ws->getBoxController()->getTotalNumMDBoxes();

//    // Create the thread pool that will run all of these.
//    ThreadScheduler * ts = new ThreadSchedulerLargestCost();
//    ThreadPool tp(ts);
//      boost::function<void ()> func;
//        func = boost::bind(&ConvertToQ3DdE::convertEventList<TofEvent>, &*this, static_cast<int>(wi));
//
      size_t n_added_events(0);
      size_t SPLIT_LEVEL(1000);
     // PARALLEL_FOR1(inWS2D)
      for (int64_t i = 0; i < int64_t(numSpec); ++i)
      {
   //     PARALLEL_START_INTERUPT_REGION
        const MantidVec& E_transfer = inWS2D->readX(i);
        const MantidVec& Signal     = inWS2D->readY(i);
        const MantidVec& Error      = inWS2D->readE(i);
        int32_t det_id              = det_loc.det_id[i];
    
        coord_t QE[4];
        for (size_t j = 0; j < specSize; ++j)
        {
            // drop emtpy events 
            if(Signal[j]<FLT_EPSILON)continue;

            double E_tr = 0.5*(E_transfer[j]+E_transfer[j+1]);
            if(E_tr<E_min||E_tr>=E_max)continue;

            double k_tr = sqrt((Ei-E_tr)/PhysicalConstants::E_mev_toNeutronWavenumberSq);
   
            double  ex = det_loc.det_dir[i].X();
            double  ey = det_loc.det_dir[i].Y();
            double  ez = det_loc.det_dir[i].Z();
            double  qx  =  -ex*k_tr;                
            double  qy  =  -ey*k_tr;
            double  qz  = ki - ez*k_tr;

            QE[0]  = (coord_t)(rotMat[0]*qx+rotMat[3]*qy+rotMat[6]*qz);  if(QE[0]<QEmin[0]||QE[0]>=QEmax[0])continue;
            QE[1]  = (coord_t)(rotMat[1]*qx+rotMat[4]*qy+rotMat[7]*qz);  if(QE[1]<QEmin[1]||QE[1]>=QEmax[1])continue;
            QE[2]  = (coord_t)(rotMat[2]*qx+rotMat[5]*qy+rotMat[8]*qz);  if(QE[2]<QEmin[2]||QE[2]>=QEmax[2])continue;
            QE[3]  = (coord_t)E_tr;
            float ErrSq = float(Error[j]*Error[j]);
            ws->addEvent(MDE(float(Signal[j]),ErrSq,runIndex,det_id,QE));
            n_added_events++;
        }
  
      // This splits up all the boxes according to split thresholds and sizes.
        //Kernel::ThreadScheduler * ts = new ThreadSchedulerFIFO();
        //ThreadPool tp(NULL);
        if(n_added_events>SPLIT_LEVEL){
            ws->splitAllIfNeeded(NULL);
            n_added_events=0;
        }
        //tp.joinAll();
        progress.report(i);  
   //       PARALLEL_END_INTERUPT_REGION
      }
 //    PARALLEL_CHECK_INTERUPT_REGION
        if(n_added_events>0){
            ws->splitAllIfNeeded(NULL);
            n_added_events=0;
        }
        ws->refreshCache();
        progress.report();      


    // Save the output
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<IMDEventWorkspace>(ws));

//
//
//    // ------------------- Cache values that are common for all ---------------------------
//    // Extract some parameters global to the instrument
//    in_ws->getInstrument()->getInstrumentParameters(l1,beamline,beamline_norm, samplePos);
//    beamline_norm = beamline.norm();
//    beamDir = beamline / beamline.norm();
//
//    //To get all the detector ID's
//    in_ws->getInstrument()->getDetectors(allDetectors);
//
//    size_t totalCost = in_ws->getNumberEvents();
//    prog = new Progress(this, 0, 1.0, totalCost);
////    if (DODEBUG) prog = new ProgressText(0, 1.0, totalCost, true);
////    if (DODEBUG) prog->setNotifyStep(1);
//
//    // Create the thread pool that will run all of these.
//    ThreadScheduler * ts = new ThreadSchedulerLargestCost();
//    ThreadPool tp(ts);
//
//    if (DODEBUG) std::cout << cputim << ": initial setup. There are " << lastNumBoxes << " MDBoxes.\n";
//
//    for (size_t wi=0; wi < in_ws->getNumberHistograms(); wi++)
//    {
//      // Equivalent of: this->convertEventList(wi);
//      EventList & el = in_ws->getEventList(wi);
//
//      // We want to bind to the right templated function, so we have to know the type of TofEvent contained in the EventList.
//      boost::function<void ()> func;
//      switch (el.getEventType())
//      {
//      case TOF:
//        func = boost::bind(&ConvertToQ3DdE::convertEventList<TofEvent>, &*this, static_cast<int>(wi));
//        break;
//      case WEIGHTED:
//        func = boost::bind(&ConvertToQ3DdE::convertEventList<WeightedEvent>, &*this, static_cast<int>(wi));
//        break;
//      case WEIGHTED_NOTIME:
//        func = boost::bind(&ConvertToQ3DdE::convertEventList<WeightedEventNoTime>, &*this, static_cast<int>(wi));
//        break;
//      default:
//        throw std::runtime_error("EventList had an unexpected data type!");
//      }
//
//      // Give this task to the scheduler
//      double cost = double(el.getNumberEvents());
//      ts->push( new FunctionTask( func, cost) );
//
//      // Keep a running total of how many events we've added
//      eventsAdded += el.getNumberEvents();
//      if (bc->shouldSplitBoxes(eventsAdded, lastNumBoxes))
//      {
//        if (DODEBUG) std::cout << cputim << ": Added tasks worth " << eventsAdded << " events.\n";
//        // Do all the adding tasks
//        tp.joinAll();
//        if (DODEBUG) std::cout << cputim << ": Performing the addition of these events.\n";
//
//        // Now do all the splitting tasks
//        ws->splitAllIfNeeded(ts);
//        if (ts->size() > 0)
//          prog->doReport("Splitting Boxes");
//        tp.joinAll();
//
//        // Count the new # of boxes.
//        lastNumBoxes = ws->getBoxController()->getTotalNumMDBoxes();
//        if (DODEBUG) std::cout << cputim << ": Performing the splitting. There are now " << lastNumBoxes << " boxes.\n";
//        eventsAdded = 0;
//      }
//    }
//
//    if (DODEBUG) std::cout << cputim << ": We've added tasks worth " << eventsAdded << " events.\n";
//
//    tp.joinAll();
//    if (DODEBUG) std::cout << cputim << ": Performing the FINAL addition of these events.\n";
//
//    // Do a final splitting of everything
//    ws->splitAllIfNeeded(ts);
//    tp.joinAll();
//    if (DODEBUG) std::cout << cputim << ": Performing the FINAL splitting of boxes. There are now " << ws->getBoxController()->getTotalNumMDBoxes() <<" boxes\n";
//
//
//    // Recount totals at the end.
//    cputim.reset();
//    ws->refreshCache();
//    if (DODEBUG) std::cout << cputim << ": Performing the refreshCache().\n";
//
//    //TODO: Centroid in parallel, maybe?
//    ws->getBox()->refreshCentroid(NULL);
//    if (DODEBUG) std::cout << cputim << ": Performing the refreshCentroid().\n";
//
//
//    if (DODEBUG)
//    {
//      std::cout << "Workspace has " << ws->getNPoints() << " events. This took " << cputimtotal << " in total.\n";
//      std::vector<std::string> stats = ws->getBoxControllerStats();
//      for (size_t i=0; i<stats.size(); ++i)
//        std::cout << stats[i] << "\n";
//      std::cout << std::endl;
//    }
//
//
  }

 ////----------------------------------------------------------------------------------------------
 // /** Convert an event list to 3D q-space and add it to the MDEventWorkspace
 //  *
 //  * @tparam T :: the type of event in the input EventList (TofEvent, WeightedEvent, etc.)
 //  * @param workspaceIndex :: index into the workspace
 //  */
 // template <class T>
 // void ConvertToQ3DdE::convertEventList(int workspaceIndex)
 // {
 //   EventList & el = in_ws->getEventList(workspaceIndex);
 //   size_t numEvents = el.getNumberEvents();

 //   // Get the position of the detector there.
 //   std::set<detid_t>& detectors = el.getDetectorIDs();
 //   if (detectors.size() > 0)
 //   {
 //     // The 3D MDEvents that will be added into the MDEventWorkspce
 //     std::vector<MDE> out_events;
 //     out_events.reserve( el.getNumberEvents() );

 //     // Get the detector (might be a detectorGroup for multiple detectors)
 //     IDetector_const_sptr det = in_ws->getDetector(workspaceIndex);

 //     // Vector between the sample and the detector
 //     V3D detPos = det->getPos() - samplePos;

 //     // Neutron's total travelled distance
 //     double distance = detPos.norm() + l1;

 //     // Detector direction normalized to 1
 //     V3D detDir = detPos / detPos.norm();

 //     // The direction of momentum transfer in the inelastic convention ki-kf
 //     //  = input beam direction (normalized to 1) - output beam direction (normalized to 1)
 //     V3D Q_dir_lab_frame = beamDir - detDir;

 //     // Multiply by the rotation matrix to convert to Q in the sample frame (take out goniometer rotation)
 //     // (or to HKL, if that's what the matrix is)
 //     V3D Q_dir = mat * Q_dir_lab_frame;

 //     // For speed we extract the components.
 //     double Q_dir_x = Q_dir.X();
 //     double Q_dir_y = Q_dir.Y();
 //     double Q_dir_z = Q_dir.Z();

 //     // For lorentz correction, calculate  sin(theta))^2
 //     double sin_theta_squared = 0;
 //     if (LorentzCorrection)
 //     {
 //       // Scattering angle = angle between neutron beam direction and the detector (scattering) direction
 //       double theta = detDir.angle(beamDir);
 //       sin_theta_squared = sin(theta);
 //       sin_theta_squared = sin_theta_squared * sin_theta_squared; // square it
 //     }

 //     /** Constant that you divide by tof (in usec) to get wavenumber in ang^-1 :
 //      * Wavenumber (in ang^-1) =  (PhysicalConstants::NeutronMass * distance) / ((tof (in usec) * 1e-6) * PhysicalConstants::h) * 1e-10; */
 //     const double wavenumber_in_angstrom_times_tof_in_microsec =
 //         (PhysicalConstants::NeutronMass * distance * 1e-10) / (1e-6 * PhysicalConstants::h);

 //     //std::cout << wi << " : " << el.getNumberEvents() << " events. Pos is " << detPos << std::endl;
 //     //std::cout << Q_dir.norm() << " Qdir norm" << std::endl;

 //     // This little dance makes the getting vector of events more general (since you can't overload by return type).
 //     typename std::vector<T> * events_ptr;
 //     getEventsFrom(el, events_ptr);
 //     typename std::vector<T> & events = *events_ptr;

 //     // Iterators to start/end
 //     typename std::vector<T>::iterator it = events.begin();
 //     typename std::vector<T>::iterator it_end = events.end();

 //     for (; it != it_end; it++)
 //     {
 //       // Get the wavenumber in ang^-1 using the previously calculated constant.
 //       double wavenumber = wavenumber_in_angstrom_times_tof_in_microsec / it->tof();

 //       // Q vector = K_final - K_initial = wavenumber * (output_direction - input_direction)
 //       coord_t center[3] = {Q_dir_x * wavenumber, Q_dir_y * wavenumber, Q_dir_z * wavenumber};

 //       if (LorentzCorrection)
 //       {
 //         //double lambda = 1.0/wavenumber;
 //         // (sin(theta))^2 / wavelength^4
 //         float correct = float( sin_theta_squared * wavenumber*wavenumber*wavenumber*wavenumber * sin_theta_squared );
 //         // Push the MDLeanEvent but correct the weight.
 //         out_events.push_back( MDE(float(it->weight()*correct), float(it->errorSquared()*correct*correct), center) );
 //       }
 //       else
 //       {
 //         // Push the MDLeanEvent with the same weight
 //         out_events.push_back( MDE(float(it->weight()), float(it->errorSquared()), center) );
 //       }
 //     }

 //     // Clear out the EventList to save memory
 //     if (ClearInputWorkspace)
 //     {
 //       // Track how much memory you cleared
 //       size_t memoryCleared = el.getMemorySize();
 //       // Clear it now
 //       el.clear();
 //       // For Linux with tcmalloc, make sure memory goes back, if you've cleared 200 Megs
 //       MemoryManager::Instance().releaseFreeMemoryIfAccumulated(memoryCleared, (size_t)2e8);
 //     }

 //     // Add them to the MDEW
 //     ws->addEvents(out_events);
 //   }
 //   prog->reportIncrement(numEvents, "Adding Events");
 // }



} // namespace Mantid
} // namespace MDAlgorithms

