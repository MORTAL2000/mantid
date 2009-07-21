#ifndef LOADISISNEXUSTEST_H_
#define LOADISISNEXUSTEST_H_

#include "MantidDataHandling/LoadInstrument.h" 

#include "MantidNexus/LoadISISNexus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FrameworkManager.h"
using namespace Mantid::NeXus;
using namespace Mantid::API;
using namespace Mantid::Kernel;

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceGroup.h"

class LoadISISNexusTest : public CxxTest::TestSuite
{
public:
    void testExec()
    {
	//	std::string s;
	//	std::getline(std::cin,s);

        Mantid::API::FrameworkManager::Instance();
        LoadISISNexus ld;
        ld.initialize();
        ld.setPropertyValue("Filename","../../../../Test/Nexus/LOQ49886.nxs");
        ld.setPropertyValue("OutputWorkspace","outWS");
        TS_ASSERT_THROWS_NOTHING(ld.execute());
        TS_ASSERT(ld.isExecuted());

        MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outWS"));
        TS_ASSERT_EQUALS(ws->blocksize(),5);
        TS_ASSERT_EQUALS(ws->getNumberHistograms(),17792);
        TS_ASSERT_EQUALS(ws->readX(0)[0],5.);
        TS_ASSERT_EQUALS(ws->readX(0)[1],4005.);
        TS_ASSERT_EQUALS(ws->readX(0)[2],8005.);
        
        TS_ASSERT_EQUALS(ws->readY(5)[1],1.);
        TS_ASSERT_EQUALS(ws->readY(6)[0],1.);
        TS_ASSERT_EQUALS(ws->readY(8)[3],1.);

        TS_ASSERT_EQUALS(ws->spectraMap().nElements(),17792);

        const std::vector< Property* >& logs = ws->getSample()->getLogData();
        TS_ASSERT_EQUALS(logs.size(),18);

        TimeSeriesProperty<std::string>* slog = dynamic_cast<TimeSeriesProperty<std::string>*>(ws->getSample()->getLogData("icp_event"));
        TS_ASSERT(slog);
        std::string str = slog->value();
        TS_ASSERT_EQUALS(str.size(),1012);
        TS_ASSERT_EQUALS(str.substr(0,37),"2009-Apr-28 09:20:29  CHANGE_PERIOD 1");

        slog = dynamic_cast<TimeSeriesProperty<std::string>*>(ws->getSample()->getLogData("icp_debug"));
        TS_ASSERT(slog);
        TS_ASSERT_EQUALS(slog->size(),50);

        TimeSeriesProperty<double>* dlog = dynamic_cast<TimeSeriesProperty<double>*>(ws->getSample()->getLogData("total_counts"));
        TS_ASSERT(dlog);
        TS_ASSERT_EQUALS(dlog->size(),172);

        dlog = dynamic_cast<TimeSeriesProperty<double>*>(ws->getSample()->getLogData("period"));
        TS_ASSERT(dlog);
        TS_ASSERT_EQUALS(dlog->size(),172);

        TimeSeriesProperty<bool>* blog = dynamic_cast<TimeSeriesProperty<bool>*>(ws->getSample()->getLogData("period 1"));
        TS_ASSERT(blog);
        TS_ASSERT_EQUALS(blog->size(),1);

        TS_ASSERT_EQUALS(ws->getSample()->getName(),"PMMA_SAN25_1.5%_TRANS_150");
    }
    void testExec2()
    {
        Mantid::API::FrameworkManager::Instance();
        LoadISISNexus ld;
        ld.initialize();
        ld.setPropertyValue("Filename","../../../../Test/Nexus/LOQ49886.nxs");
        ld.setPropertyValue("OutputWorkspace","outWS");
        ld.setPropertyValue("SpectrumMin","10");
        ld.setPropertyValue("SpectrumMax","20");
        ld.setPropertyValue("SpectrumList","30,33,38");
        TS_ASSERT_THROWS_NOTHING(ld.execute());
        TS_ASSERT(ld.isExecuted());

	
		
		

        MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outWS"));
        TS_ASSERT_EQUALS(ws->blocksize(),5);
        TS_ASSERT_EQUALS(ws->getNumberHistograms(),14);

        TS_ASSERT_EQUALS(ws->readX(0)[0],5.);
        TS_ASSERT_EQUALS(ws->readX(0)[1],4005.);
        TS_ASSERT_EQUALS(ws->readX(0)[2],8005.);
        
        TS_ASSERT_EQUALS(ws->readY(5)[1],0.);
        TS_ASSERT_EQUALS(ws->readY(6)[0],0.);
        TS_ASSERT_EQUALS(ws->readY(8)[3],0.);

        TS_ASSERT_EQUALS(ws->readY(7)[1],2.);
        TS_ASSERT_EQUALS(ws->readY(9)[3],1.);
        TS_ASSERT_EQUALS(ws->readY(12)[1],1.);
    }
	 void testMultiPeriodEntryNumberZero()
    {
        Mantid::API::FrameworkManager::Instance();
        LoadISISNexus ld;
        ld.initialize();
        ld.setPropertyValue("Filename","../../../../Test/Nexus/TEST00000008.nxs");
        ld.setPropertyValue("OutputWorkspace","outWS");
        ld.setPropertyValue("SpectrumMin","10");
        ld.setPropertyValue("SpectrumMax","20");
 		ld.setPropertyValue("EntryNumber","0");
        TS_ASSERT_THROWS_NOTHING(ld.execute());
        TS_ASSERT(ld.isExecuted());
		
		WorkspaceGroup_sptr grpout;//=WorkspaceGroup_sptr(new WorkspaceGroup);
		TS_ASSERT_THROWS_NOTHING(grpout=boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("outWS")));

        MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outWS_1"));
        TS_ASSERT_EQUALS(ws->blocksize(),995);
        TS_ASSERT_EQUALS(ws->getNumberHistograms(),30);

        TS_ASSERT_EQUALS(ws->readX(0)[0],5.);
        TS_ASSERT_EQUALS(ws->readX(0)[1],6.);
        TS_ASSERT_EQUALS(ws->readX(0)[2],7.);
        
        TS_ASSERT_EQUALS(ws->readY(5)[1],0.);
        TS_ASSERT_EQUALS(ws->readY(6)[0],0.);
        TS_ASSERT_EQUALS(ws->readY(8)[3],0.);

        TS_ASSERT_EQUALS(ws->readY(7)[1],0.);
        TS_ASSERT_EQUALS(ws->readY(9)[3],0.);
        TS_ASSERT_EQUALS(ws->readY(12)[1],0.);
    }
	  void testMultiPeriodEntryNumberNonZero()
    {
        Mantid::API::FrameworkManager::Instance();
        LoadISISNexus ld;
        ld.initialize();
        ld.setPropertyValue("Filename","../../../../Test/Nexus/TEST00000008.nxs");
        ld.setPropertyValue("OutputWorkspace","outWS");
        ld.setPropertyValue("SpectrumMin","10");
        ld.setPropertyValue("SpectrumMax","20");
  		ld.setPropertyValue("EntryNumber","5");
        TS_ASSERT_THROWS_NOTHING(ld.execute());
        TS_ASSERT(ld.isExecuted());
		
		//WorkspaceGroup_sptr grpout;//=WorkspaceGroup_sptr(new WorkspaceGroup);
		//TS_ASSERT_THROWS_NOTHING(grpout=boost::dynamic_pointer_cast<WorkspaceGroup>(AnalysisDataService::Instance().retrieve("outWS")));

        MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("outWS_5"));
        TS_ASSERT_EQUALS(ws->blocksize(),995);
        TS_ASSERT_EQUALS(ws->getNumberHistograms(),30);

        TS_ASSERT_EQUALS(ws->readX(0)[0],5.);
        TS_ASSERT_EQUALS(ws->readX(0)[1],6.);
        TS_ASSERT_EQUALS(ws->readX(0)[2],7.);
        
        TS_ASSERT_EQUALS(ws->readY(5)[1],0.);
        TS_ASSERT_EQUALS(ws->readY(6)[0],0.);
        TS_ASSERT_EQUALS(ws->readY(8)[3],0.);

        TS_ASSERT_EQUALS(ws->readY(7)[1],0.);
        TS_ASSERT_EQUALS(ws->readY(9)[3],0.);
        TS_ASSERT_EQUALS(ws->readY(12)[1],0.);
    }
};

#endif /*LOADISISNEXUSTEST_H_*/
