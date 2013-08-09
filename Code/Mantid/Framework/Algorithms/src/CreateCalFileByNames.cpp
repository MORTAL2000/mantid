/*WIKI* 

[[Image:InstrumentTree.jpg|450px|right|Instrument Tree]]

Create a [[CalFile|calibration file]] for diffraction focusing based on list of names of the instrument tree.

If a new file name is specified then offsets in the file are all sets to zero and all detectors are selected. 
If a valid calibration file already exists at the location specified by the [[CalFile|GroupingFileName]] 
then any existing offsets and selection values will be maintained and only the grouping values changed.

Detectors not assigned to any group will appear as group 0, i.e. not included when using AlignDetector or DiffractionFocussing algorithms.

The group number is assigned based on a descent in the instrument tree assembly.
If two assemblies are parented, say Bank1 and module1, and both assembly names
are given in the GroupNames, they will get assigned different grouping numbers.
This allows to isolate a particular sub-assembly of a particular leaf of the tree.
*WIKI*/
/*WIKI_USAGE*
'''Python'''
    CreateCalFileByNames("GEM","output.cal","Bank1,Bank2,Module1")

'''C++'''
    IAlgorithm* alg = FrameworkManager::Instance().createAlgorithm("CreateCalFileByNames");
    alg->setPropertyValue("InstrumentName", "GEM");
    alg->setPropertyValue("GroupingFileName", "output.cal");
    alg->setPropertyValue("GroupNames", "Bank1,Bank2,Module1");
    alg->execute();
*WIKI_USAGE*/

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/CreateCalFileByNames.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/InstrumentDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/WorkspaceValidators.h"
#include "MantidKernel/ConfigService.h"
#include <boost/algorithm/string/detail/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <fstream>
#include <iomanip>
#include <queue>

using namespace Mantid::API;
using namespace Mantid::Kernel;


namespace Mantid
{
  namespace Algorithms
  {

    // Register the class into the algorithm factory
    DECLARE_ALGORITHM(CreateCalFileByNames)
    
    /// Sets documentation strings for this algorithm
    void CreateCalFileByNames::initDocs()
    {
      this->setWikiSummary("Create a [[CalFile|calibration file]] (extension *.cal) for diffraction focusing based on the names of the components in the instrument tree.");
      this->setOptionalMessage("Create a calibration file (extension *.cal) for diffraction focusing based on the names of the components in the instrument tree.");
    }
    

    using namespace Kernel;
    using API::Progress;
    using API::FileProperty;
    using Geometry::Instrument_const_sptr;

    CreateCalFileByNames::CreateCalFileByNames():API::Algorithm(),group_no(0)
    {
    }

    /** Initialisation method. Declares properties to be used in algorithm.
     *
     */
    void CreateCalFileByNames::init()
    {
      declareProperty(new WorkspaceProperty<> ("InstrumentWorkspace", "", Direction::Input, boost::make_shared<InstrumentValidator>()),
        "A workspace that contains a reference to the instrument of interest. "
        "You can use [[LoadEmptyInstrument]] to create such a workspace.");
      declareProperty(new FileProperty("GroupingFileName","",FileProperty::Save, ".cal"),
        "The name of the output [[CalFile]]");
      declareProperty("GroupNames","",
        "A string of the instrument component names to use as separate groups. "
        "/ or , can be used to separate multiple groups.");
    }


    /** Executes the algorithm
     *
     *  @throw Exception::FileError If the grouping file cannot be opened or read successfully
     *  @throw runtime_error If unable to run one of the Child Algorithms successfully
     */

    void CreateCalFileByNames::exec()
    {
      MatrixWorkspace_const_sptr ws = getProperty("InstrumentWorkspace");

      // Get the instrument.
      Instrument_const_sptr inst = ws->getInstrument();

      // Get the names of groups
      std::string groupsname=getProperty("GroupNames");
      groups=groupsname;

      // Split the names of the group and insert in a vector, throw if group empty
      std::vector<std::string> vgroups;
      boost::split( vgroups, groupsname, boost::algorithm::detail::is_any_ofF<char>(",/*"));
      if (vgroups.empty())
      {
        g_log.error("Could not determine group names. Group names should be separated by / or ,");
        throw std::runtime_error("Could not determine group names. Group names should be separated by / or ,");
      }

      // Assign incremental number to each group
      std::map<std::string,int> group_map;
      int index=0;
      for (std::vector<std::string>::iterator it=vgroups.begin();it!=vgroups.end();++it)
      {
        boost::trim(*it);
        group_map[(*it)]=++index;
      }

      // Not needed anymore
      vgroups.clear();

      // Find Detectors that belong to groups
      typedef boost::shared_ptr<const Geometry::ICompAssembly> sptr_ICompAss;
      typedef boost::shared_ptr<const Geometry::IComponent> sptr_IComp;
      typedef boost::shared_ptr<const Geometry::IDetector> sptr_IDet;
      std::queue< std::pair<sptr_ICompAss,int> > assemblies;
      sptr_ICompAss current=boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(inst);
      sptr_IDet currentDet;
      sptr_IComp currentIComp;
      sptr_ICompAss currentchild;

      int top_group, child_group;

      if (current.get())
      {
        top_group=group_map[current->getName()]; // Return 0 if not in map
        assemblies.push(std::make_pair(current,top_group));
      }

      std::string filename=getProperty("GroupingFilename");

      // Check if a template cal file is given
      bool overwrite=groupingFileDoesExist(filename);

      int number=0;
      Progress prog(this,0.0,0.8,assemblies.size());
      while(!assemblies.empty()) //Travel the tree from the instrument point
      {
        current=assemblies.front().first;
        top_group=assemblies.front().second;
        assemblies.pop();
        int nchilds=current->nelements();
        if (nchilds!=0)
        {
          for (int i=0;i<nchilds;++i)
          {
            currentIComp=(*(current.get()))[i]; // Get child
            currentDet=boost::dynamic_pointer_cast<const Geometry::IDetector>(currentIComp);
            if (currentDet.get())// Is detector
            {
              if (overwrite) // Map will contains udet as the key
                instrcalib[currentDet->getID()]=std::make_pair(number++,top_group);
              else          // Map will contains the entry number as the key
                instrcalib[number++]=std::make_pair(currentDet->getID(),top_group);
            }
            else // Is an assembly, push in the queue
            {
              currentchild=boost::dynamic_pointer_cast<const Geometry::ICompAssembly>(currentIComp);
              if (currentchild.get())
              {
                child_group=group_map[currentchild->getName()];
                if (child_group==0)
                  child_group=top_group;
                assemblies.push(std::make_pair(currentchild,child_group));
              }
            }
          }
        }
        prog.report();
      }
      // Write the results in a file
      saveGroupingFile(filename,overwrite);
      progress(0.2);
      return;
    }

    bool CreateCalFileByNames::groupingFileDoesExist(const std::string& filename) const
    {
      std::ifstream file(filename.c_str());
      // Check if the file already exists
      if (!file)
        return false;
      file.close();
      std::ostringstream mess;
      mess << "Calibration file "<< filename << " already exist. Only grouping will be modified";
      g_log.information(mess.str());
      return true;
    }

    /// Creates and saves the output file
    void CreateCalFileByNames::saveGroupingFile(const std::string& filename,bool overwrite) const
    {
      std::ostringstream message;
      std::fstream outfile;
      std::fstream infile;
      if (!overwrite) // Open the file directly
      {
        outfile.open(filename.c_str(), std::ios::out);
        if (!outfile.is_open())
        {
          message << "Can't open Calibration File: " << filename;
          g_log.error(message.str());
          throw std::runtime_error(message.str());
        }
      }
      else
      {
        infile.open(filename.c_str(),std::ios::in);
        std::string newfilename=filename+"2";
        outfile.open(newfilename.c_str(), std::ios::out);
        if (!infile.is_open())
        {
          message << "Can't open input Calibration File: " << filename;
          g_log.error(message.str());
          throw std::runtime_error(message.str());
        }
        if (!outfile.is_open())
        {
          message << "Can't open new Calibration File: " << newfilename;
          g_log.error(message.str());
          throw std::runtime_error(message.str());
        }
      }

      // Write the headers
      writeHeaders(outfile,filename,overwrite);

      if (overwrite)
      {
        int number, udet, select, group;
        double offset;

        instrcalmap::const_iterator it;
        std::string str;
        while(getline(infile,str))
        {
          if (str.empty() || str[0]=='#') // Skip the headers
            continue;
          std::istringstream istr(str);
          istr >> number >> udet >> offset >> select >> group;
          it=instrcalib.find(udet);
          if (it==instrcalib.end()) // Not found, don't assign a group
            group=0;
          group=((*it).second).second; // If found then assign new group
          writeCalEntry(outfile,number,udet,offset,select,group);
        }
      }
      else //
      {
        instrcalmap::const_iterator it=instrcalib.begin();
        for (;it!=instrcalib.end();++it)
          writeCalEntry(outfile,(*it).first,((*it).second).first,0.0,1,((*it).second).second);
      }

      // Closing
      outfile.close();
      if (overwrite)
        infile.close();
      return;
    }

    /// Writes a single calibration line to the output file
    void CreateCalFileByNames::writeCalEntry(std::ostream& os, int number, int udet, double offset, int select, int group)
    {
      os << std::fixed << std::setw(9) << number <<
        std::fixed << std::setw(15) << udet <<
        std::fixed << std::setprecision(7) << std::setw(15)<< offset <<
        std::fixed << std::setw(8) << select <<
        std::fixed << std::setw(8) << group  << "\n";
      return;
    }

    /// Writes out the header to the output file
    void CreateCalFileByNames::writeHeaders(std::ostream& os,const std::string& filename,bool overwrite) const
    {
      os << "# Diffraction focusing calibration file created by Mantid" <<  "\n";
      os << "# Detectors have been grouped using assembly names:" << groups <<"\n";
      if (overwrite)
      {
        os << "# Template file " << filename << " has been used" << "\n";
        os << "# Only grouping has been changed, offset from template file have been copied" << "\n";
      }
      else
        os << "# No template file, all offsets set to 0.0 and select to 1" << "\n";

      os << "#  Number           UDET         offset      select  group" << "\n";
      return;
    }

  } // namespace Algorithm
} // namespace Mantid
