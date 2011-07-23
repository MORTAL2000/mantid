#include "MantidAPI/ImplicitFunction.h"
#include "MantidAPI/ImplicitFunctionFactory.h"
#include "MantidGeometry/MDGeometry/MDBoxImplicitFunction.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Utils.h"
#include "MantidMDEvents/BinToMDHistoWorkspace.h"
#include "MantidMDEvents/IMDBox.h"
#include "MantidMDEvents/MDBoxIterator.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEventWorkspace.h"
#include "MantidMDEvents/MDHistoWorkspace.h"
#include <boost/algorithm/string.hpp>

using Mantid::Kernel::CPUTimer;

namespace Mantid
{
namespace MDEvents
{

  // Register the algorithm into the AlgorithmFactory
  DECLARE_ALGORITHM(BinToMDHistoWorkspace)

  using namespace Mantid::Kernel;
  using namespace Mantid::API;
  using namespace Mantid::Geometry;


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  BinToMDHistoWorkspace::BinToMDHistoWorkspace()
  {
  }

  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  BinToMDHistoWorkspace::~BinToMDHistoWorkspace()
  {
  }


  //----------------------------------------------------------------------------------------------
  /// Sets documentation strings for this algorithm
  void BinToMDHistoWorkspace::initDocs()
  {
    this->setWikiSummary("Take a [[MDEventWorkspace]] and bin into into a dense, multi-dimensional histogram workspace ([[MDHistoWorkspace]]).");
    this->setOptionalMessage("Take a MDEventWorkspace and bin into into a dense, multi-dimensional histogram workspace (MDHistoWorkspace).");
    this->setWikiDescription(
        "This algorithm performs dense binning of the events in multiple dimensions of an input [[MDEventWorkspace]] "
        "and places them into a dense MDHistoWorkspace with 1-4 dimensions."
        "\n\n"
        "The input MDEventWorkspace may have more dimensions than the number of output dimensions. "
        "The names of the dimensions in the DimX, etc. parameters are used to find the corresponding dimensions that will "
        "be created in the output."
        "\n\n"
        "An ImplicitFunction can be defined using the ImplicitFunctionXML parameter; any points NOT belonging inside of the "
        "ImplicitFunction will be set as NaN (not-a-number). "
        "\n\n"
        "As of now, binning is only performed along axes perpendicular to the dimensions defined "
        "in the MDEventWorkspace."
        );
  }

  //----------------------------------------------------------------------------------------------
  /** Initialize the algorithm's properties.
   */
  void BinToMDHistoWorkspace::init()
  {
    std::string dimHelp = "Enter it as a comma-separated list of values with the format: 'name,minimum,maximum,number_of_bins'. Leave blank for NONE.";
    declareProperty(new WorkspaceProperty<IMDEventWorkspace>("InputWorkspace","",Direction::InOut), "An input MDEventWorkspace.");
    declareProperty(new PropertyWithValue<std::string>("DimX","",Direction::Input), "Binning parameters for the X dimension.\n" + dimHelp);
    declareProperty(new PropertyWithValue<std::string>("DimY","",Direction::Input), "Binning parameters for the Y dimension.\n" + dimHelp);
    declareProperty(new PropertyWithValue<std::string>("DimZ","",Direction::Input), "Binning parameters for the Z dimension.\n" + dimHelp);
    declareProperty(new PropertyWithValue<std::string>("DimT","",Direction::Input), "Binning parameters for the T dimension.\n" + dimHelp);
    declareProperty(new PropertyWithValue<std::string>("ImplicitFunctionXML","",Direction::Input), "XML string describing the implicit function determining which bins to use.");
    declareProperty(new PropertyWithValue<bool>("IterateEvents",true,Direction::Input),
        "Alternative binning method where you iterate through every event, placing them in the proper bin.\n"
        "This may be faster for workspaces with few events and lots of output bins.");
    declareProperty(new WorkspaceProperty<Workspace>("OutputWorkspace","",Direction::Output), "A name for the output MDHistoWorkspace.");
  }


  //----------------------------------------------------------------------------------------------
  /** Generate a MDHistoDimension_sptr from a comma-sep string.
   *
   * @param str :: name,minimum,maximum,number_of_bins
   * @return
   */
  MDHistoDimension_sptr makeMDHistoDimensionFromString(const std::string & str)
  {
    if (str.empty())
    {
      // Make a blank dimension
      MDHistoDimension_sptr out;
      return out;
    }
    else
    {
      std::string name, id;
      double min, max;
      int numBins = 0;
      std::vector<std::string> strs;
      boost::split(strs, str, boost::is_any_of(","));
      if (strs.size() != 4)
        throw std::invalid_argument("Wrong number of values (4 are expected) in the dimensions string: " + str);
      // Extract the arguments
      name = Strings::strip(strs[0]);
      id = name;
      Strings::convert(strs[1], min);
      Strings::convert(strs[2], max);
      Strings::convert(strs[3], numBins);
      if (name.size() == 0)
        throw std::invalid_argument("Name should not be blank.");
      if (min >= max)
        throw std::invalid_argument("Min should be > max.");
      if (numBins < 1)
        throw std::invalid_argument("Number of bins should be >= 1.");

      MDHistoDimension_sptr out(new MDHistoDimension(name, id, "", min, max, numBins));
      return out;
    }
  }


  //-----------------------------------------------------------------------------------------------
  /** Performs template argument deduction and uses local call to fabricate an adapter instance of Point3D.
   * Templated function may have a more suitable location.
   */
  template <typename AdapteeType>
  Mantid::API::Point3D* makePoint3D(const AdapteeType& adaptee)
  {

    //Local adapter.
    class PointAdapter : public Mantid::API::Point3D
    {
    private:
      AdapteeType m_adaptee;
    public:
      PointAdapter(const AdapteeType& adaptee) : m_adaptee(adaptee)
      {
      }
      virtual double getX() const
      {
        return m_adaptee.m_min[0] + ((m_adaptee.m_max[0] - m_adaptee.m_min[0]) /2);
      }
      virtual double getY() const
      {
        return m_adaptee.m_min[1] + ((m_adaptee.m_max[1] - m_adaptee.m_min[1]) /2);
      }
      virtual double getZ() const
      {
        return m_adaptee.m_min[2] + ((m_adaptee.m_max[2] - m_adaptee.m_min[2]) /2);
      }
    };
    return new PointAdapter(adaptee);
  }


  //----------------------------------------------------------------------------------------------
  /** Perform binning by iterating through every event and placing them in the output workspace
   *
   * @param ws :: MDEventWorkspace of the given type.
   */
  template<typename MDE, size_t nd>
  void BinToMDHistoWorkspace::binByIterating(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {

    // Number of output binning dimensions found
    size_t numBD = binDimensions.size();

    // Cache some data to speed up accessing them a bit
    coord_t * min = new coord_t[numBD];
    coord_t * max = new coord_t[numBD];
    coord_t * step = new coord_t[numBD];
    size_t * indexMultiplier = new size_t[numBD];
    for (size_t d=0; d<numBD; d++)
    {
      min[d] = binDimensions[d]->getMinimum();
      max[d] = binDimensions[d]->getMaximum();
      step[d] = binDimensions[d]->getBinWidth();
      if (d > 0)
        indexMultiplier[d] = outWS->getIndexMultiplier()[d-1];
      else
        indexMultiplier[d] = 1;
    }
    signal_t * signals = outWS->getSignalArray();
    signal_t * errors = outWS->getErrorSquaredArray();

    // Start with signal at 0.0
    outWS->setTo(0.0, 0.0);

    // Build an implicit function (it needs to be in the space of the MDEventWorkspace)
    std::vector<coord_t> function_min(nd, -1e50); // default to all space if the dimension is not specified
    std::vector<coord_t> function_max(nd, +1e50); // default to all space if the dimension is not specified
    for (size_t bd=0; bd<numBD; bd++)
    {
      // Dimension in the MDEventWorkspace
      size_t d = dimensionToBinFrom[bd];
      function_min[d] = binDimensions[bd]->getMinimum();
      function_max[d] = binDimensions[bd]->getMaximum();
    }
    MDBoxImplicitFunction * function = new MDBoxImplicitFunction(function_min, function_max);

    // Use getBoxes() to get an array with a pointer to each box
    std::vector<IMDBox<MDE,nd>*> boxes;
    // Leaf-only; no depth limit; with the implicit function passed to it.
    ws->getBox()->getBoxes(boxes, 1000, true, function);

    std::cout << "Found " << boxes.size() << " boxes within the implicit function." << std::endl;

    // For progress reporting, the # of boxes
    if (prog)
      prog->setNumSteps( boxes.size() );

    for (size_t i=0; i<boxes.size(); i++)
    {
      // Get the next box in the iterator
      MDBox<MDE,nd> * box = dynamic_cast<MDBox<MDE,nd> *>(boxes[i]);
      if (box)
      {
        const std::vector<MDE> & events = box->getEvents();
        typename std::vector<MDE>::const_iterator it = events.begin();
        typename std::vector<MDE>::const_iterator it_end = events.end();
        for (; it != it_end; it++)
        {
          // Cache the center of the event (again for speed)
          const coord_t * center = it->getCenter();

          // To build up the linear index
          size_t linearIndex = 0;
          // To mark events outside range
          bool badEvent = false;

          /// Loop through the dimensions on which we bin
          for (size_t bd=0; bd<numBD; bd++)
          {
            // Dimension in the MDEventWorkspace
            size_t d = dimensionToBinFrom[bd];
            // Where the event is in that dimension?
            coord_t x = center[d];
            // Within range?
            if ((x >= min[bd]) && (x < max[bd]))
            {
              // Build up the linear index
              linearIndex += indexMultiplier[bd] * size_t((x - min[bd])/step[bd]);
            }
            else
            {
              // Outside the range
              badEvent = true;
              break;
            }
          } // (for each dim in MDHisto)

          if (!badEvent)
          {
            signals[linearIndex] += it->getSignal();
            errors[linearIndex] += it->getErrorSquared();
          }
        }
        // Done with the events list
        box->releaseEvents();
      }

      // Progress reporting
      if (prog) prog->report();

    }// for each box in the vector

    // Now the implicit function
    if (implicitFunction)
    {
      prog->report("Applying implicit function.");
      signal_t nan = std::numeric_limits<signal_t>::quiet_NaN();
      outWS->applyImplicitFunction(implicitFunction, nan, nan);
    }

    delete [] min;
    delete [] max;
    delete [] step;
  }

  //----------------------------------------------------------------------------------------------
  /** Templated method to apply the binning operation to the particular
   * MDEventWorkspace passed in.
   *
   * @param ws :: MDEventWorkspace of the given type
   */
  template<typename MDE, size_t nd>
  void BinToMDHistoWorkspace::do_centerpointBin(typename MDEventWorkspace<MDE, nd>::sptr ws)
  {
    bool DODEBUG = true;

    CPUTimer tim;

    // Number of output binning dimensions found
    size_t numBD = binDimensions.size();

    //Since the costs are not known ahead of time, use a simple FIFO buffer.
    ThreadScheduler * ts = new ThreadSchedulerFIFO();

    // Create the threadpool with: all CPUs, a progress reporter
    ThreadPool tp(ts, 0, prog);

    // Big efficiency gain is obtained by grouping a few bins per task.
    size_t binsPerTask = 100;

    // For progress reporting, the approx  # of tasks
    if (prog)
      prog->setNumSteps( int(outWS->getNPoints()/100) );

    // The root-level box.
    IMDBox<MDE,nd> * rootBox = ws->getBox();

    // This is the limit to loop over in each dimension
    size_t * index_max = new size_t[numBD];
    for (size_t bd=0; bd<numBD; bd++) index_max[bd] = binDimensions[bd]->getNBins();

    // Cache a calculation to convert indices x,y,z,t into a linear index.
    size_t * index_maker = new size_t[numBD];
    Utils::NestedForLoop::SetUpIndexMaker(numBD, index_maker, index_max);

    int numPoints = int(outWS->getNPoints());
    // Run in OpenMP with dynamic scheduling and a smallish chunk size (binsPerTask)
    PRAGMA_OMP(parallel for schedule(dynamic, binsPerTask))
    for (int i=0; i < numPoints; i++)
    {
      size_t linear_index = size_t(i);
      // nd >= numBD in all cases so this is safe.
      size_t index[nd];

      // Get the index at each dimension for this bin.
      Utils::NestedForLoop::GetIndicesFromLinearIndex(numBD, linear_index, index_maker, index_max, index);

      // Construct the bin and its coordinates
      MDBin<MDE,nd> bin;
      for (size_t bd=0; bd<numBD; bd++)
      {
        // Index in this binning dimension (i_x, i_y, etc.)
        size_t idx = index[bd];
        // Dimension in the MDEventWorkspace
        size_t d = dimensionToBinFrom[bd];
        // Corresponding extents
        bin.m_min[d] = binDimensions[bd]->getX(idx);
        bin.m_max[d] = binDimensions[bd]->getX(idx+1);
      }
      bin.m_index = linear_index;

      // Check if the bin is in the ImplicitFunction (if any)
      bool binContained = true;
      if (implicitFunction)
      {
        binContained = implicitFunction->evaluate(makePoint3D(bin));
      }

      if (binContained)
      {
        // Array of bools set to true when a dimension is fully contained (binary splitting only)
        bool fullyContained[nd];
        for (size_t d=0; d<nd; d++)
          fullyContained[d] = false;

        // This will recursively bin into the sub grids
        rootBox->centerpointBin(bin, fullyContained);

        // Save the data into the dense histogram
        outWS->setSignalAt(linear_index, bin.m_signal);
        outWS->setErrorAt(linear_index, bin.m_errorSquared);
      }

      // Report progress but not too often.
      if (((linear_index % 100) == 0) && prog ) prog->report();
    } // (for each linear index)

    if (DODEBUG) std::cout << tim << " to run the openmp loop.\n";

    delete [] index_max;
    delete [] index_maker;
  }



  //----------------------------------------------------------------------------------------------
  /** Execute the algorithm.
   */
  void BinToMDHistoWorkspace::exec()
  {
    // Create the dimensions based on the strings from the user
    binDimensionsIn.push_back( makeMDHistoDimensionFromString( getPropertyValue("DimX")) );
    binDimensionsIn.push_back( makeMDHistoDimensionFromString( getPropertyValue("DimY")) );
    binDimensionsIn.push_back( makeMDHistoDimensionFromString( getPropertyValue("DimZ")) );
    binDimensionsIn.push_back( makeMDHistoDimensionFromString( getPropertyValue("DimT")) );

    IMDEventWorkspace_sptr in_ws = getProperty("InputWorkspace");

    // De serialize the implicit function
    std::string ImplicitFunctionXML = getPropertyValue("ImplicitFunctionXML");
    implicitFunction = NULL;
    if (!ImplicitFunctionXML.empty())
    {
      implicitFunction = Mantid::API::ImplicitFunctionFactory::Instance().createUnwrapped(ImplicitFunctionXML);
    }

    prog = new Progress(this, 0, 1.0, 1); // This gets deleted by the thread pool; don't delete it in here.


    // --------------- Create the output MDHistoWorkspace ------------------
    CPUTimer tim;

    // Thin down the input dimensions for any invalid ones
    for (size_t i = 0; i < binDimensionsIn.size(); ++i)
    {
      if (binDimensionsIn[i]) // (valid pointer?)
      {
        if (binDimensionsIn[i]->getNBins() == 0)
          throw std::runtime_error("Dimension " + binDimensionsIn[i]->getName() + " was set to have 0 bins. Cannot continue.");

        try {
          size_t dim_index = in_ws->getDimensionIndexByName(binDimensionsIn[i]->getName());
          dimensionToBinFrom.push_back(dim_index);
          binDimensions.push_back(binDimensionsIn[i]);
        }
        catch (std::runtime_error &)
        {
          // The dimension was not found, so we are not binning across it. TODO: Log message?
          if (binDimensionsIn[i]->getNBins() > 1)
            throw std::runtime_error("Dimension " + binDimensionsIn[i]->getName() + " was not found in the MDEventWorkspace and has more than one bin! Cannot continue.");
        }
      }
    }
    // Number of output binning dimensions found
    size_t numBD = binDimensions.size();

    // Create the dense histogram. This allocates the memory
    outWS = MDHistoWorkspace_sptr(new MDHistoWorkspace(binDimensions));
    //if (DODEBUG) std::cout << tim << " to create the MDHistoWorkspace.\n";

    if (numBD == 0)
      throw std::runtime_error("No output dimensions were found in the MDEventWorkspace. Cannot bin!");
    if (numBD > in_ws->getNumDims())
      throw std::runtime_error("More output dimensions were specified than input dimensions exist in the MDEventWorkspace. Cannot bin!");



    // Wrapper to cast to MDEventWorkspace then call the function
    bool IterateEvents = getProperty("IterateEvents");
    if (IterateEvents)
    {
      CALL_MDEVENT_FUNCTION(this->binByIterating, in_ws);
    }
    else
    {
      CALL_MDEVENT_FUNCTION(this->do_centerpointBin, in_ws);
    }


    // Save the output
    setProperty("OutputWorkspace", boost::dynamic_pointer_cast<Workspace>(outWS));
  }



} // namespace Mantid
} // namespace MDEvents
