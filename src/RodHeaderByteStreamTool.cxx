
#include <algorithm>

#include "GaudiKernel/IInterface.h"

#include "TrigT1CaloEvent/RODHeader.h"

#include "TrigT1CaloByteStream/RodHeaderByteStreamTool.h"

namespace LVL1BS {

// Interface ID

static const InterfaceID IID_IRodHeaderByteStreamTool("RodHeaderByteStreamTool", 1, 1);

const InterfaceID& RodHeaderByteStreamTool::interfaceID()
{
  return IID_IRodHeaderByteStreamTool;
}

// Constructor

RodHeaderByteStreamTool::RodHeaderByteStreamTool(const std::string& type,
                                     const std::string& name,
				     const IInterface*  parent)
                  : AlgTool(type, name, parent),
		    m_srcIdMap(0)
{
  declareInterface<RodHeaderByteStreamTool>(this);

  declareProperty("ROBSourceIDs",        m_sourceIDs,
                  "ROB fragment source identifiers - All except RoIB");
  declareProperty("ROBSourceIDsPP",      m_sourceIDsPP,
                  "ROB fragment source identifiers - PP only");
  declareProperty("ROBSourceIDsCP",      m_sourceIDsCP,
                  "ROB fragment source identifiers - CP DAQ only");
  declareProperty("ROBSourceIDsJEP",     m_sourceIDsJEP,
                  "ROB fragment source identifiers - JEP DAQ only");
  declareProperty("ROBSourceIDsCPRoI",   m_sourceIDsCPRoI,
                  "ROB fragment source identifiers - CP RoI only");
  declareProperty("ROBSourceIDsJEPRoI",  m_sourceIDsJEPRoI,
                  "ROB fragment source identifiers - JEP RoI only");
  declareProperty("ROBSourceIDsCPRoIB",  m_sourceIDsCPRoIB,
                  "ROB fragment source identifiers - CP RoIB only");
  declareProperty("ROBSourceIDsJEPRoIB", m_sourceIDsJEPRoIB,
                  "ROB fragment source identifiers - JEP RoIB only");

}

// Destructor

RodHeaderByteStreamTool::~RodHeaderByteStreamTool()
{
}

// Initialize

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

StatusCode RodHeaderByteStreamTool::initialize()
{
  MsgStream log( msgSvc(), name() );
  log << MSG::INFO << "Initializing " << name() << " - package version "
                   << PACKAGE_VERSION << endreq;

  m_srcIdMap = new L1CaloSrcIdMap();
  return AlgTool::initialize();
}

// Finalize

StatusCode RodHeaderByteStreamTool::finalize()
{
  delete m_srcIdMap;
  return AlgTool::finalize();
}

// Conversion bytestream to RODHeaders

StatusCode RodHeaderByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::RODHeader>* const rhCollection)
{
  MsgStream log( msgSvc(), name() );
  const bool debug = msgSvc()->outputLevel(name()) <= MSG::DEBUG;

  // Loop over ROB fragments

  int robCount = 0;
  ROBIterator rob    = robFrags.begin();
  ROBIterator robEnd = robFrags.end();
  for (; rob != robEnd; ++rob) {

    if (debug) {
      ++robCount;
      log << MSG::DEBUG << "Treating ROB fragment " << robCount << endreq;
    }

    // Unpack ROD header info

    const uint32_t version  = (*rob)->rod_version();
    const uint32_t sourceId = (*rob)->rod_source_id();
    const uint32_t run      = (*rob)->rod_run_no();
    const uint32_t lvl1Id   = (*rob)->rod_lvl1_id();
    const uint32_t bcId     = (*rob)->rod_bc_id();
    const uint32_t trigType = (*rob)->rod_lvl1_trigger_type();
    const uint32_t detType  = (*rob)->rod_detev_type();
    const uint32_t nData    = (*rob)->rod_ndata();

    // Unpack status words

    RODPointer status;
    RODPointer statusEnd;
    (*rob)->rod_status(status);
    statusEnd = status + (*rob)->rod_nstatus();
    std::vector<uint32_t> statusWords;
    for (; status != statusEnd; ++status) statusWords.push_back(*status);

    // Save

    rhCollection->push_back(new LVL1::RODHeader(version, sourceId, run, lvl1Id,
                                bcId, trigType, detType, statusWords, nData));
    if (debug) {
      log << MSG::DEBUG << MSG::hex
          << "ROD Header version/sourceId/run/lvl1Id/bcId/trigType/detType/nData: "
	  << version << "/" << sourceId << "/" << run << "/" << lvl1Id << "/"
	  << bcId << "/" << trigType << "/" << detType << "/" << nData
	  << endreq;
      log << MSG::DEBUG << "ROD Status Words:";
      std::vector<uint32_t>::const_iterator pos  = statusWords.begin();
      std::vector<uint32_t>::const_iterator pose = statusWords.end();
      for (; pos != pose; ++pos) log << MSG::DEBUG << " " << *pos;
      log << MSG::DEBUG << MSG::dec << endreq;
    }
  }

  return StatusCode::SUCCESS;
}

// Return reference to vector with all possible Source Identifiers

const std::vector<uint32_t>& RodHeaderByteStreamTool::sourceIDs(
                                                      const std::string& sgKey)
{
  const bool pp      = isAppended(sgKey, "PP");
  const bool cp      = isAppended(sgKey, "CP");
  const bool jep     = isAppended(sgKey, "JEP");
  const bool cpRoi   = isAppended(sgKey, "CPRoI");
  const bool jepRoi  = isAppended(sgKey, "JEPRoI");
  const bool cpRoib  = isAppended(sgKey, "CPRoIB");
  const bool jepRoib = isAppended(sgKey, "JEPRoIB");
  const bool all = !(pp || cp || jep || cpRoi || jepRoi || cpRoib || jepRoib);
  if (all     && !m_sourceIDs.empty())        return m_sourceIDs;
  if (pp      && !m_sourceIDsPP.empty())      return m_sourceIDsPP;
  if (cp      && !m_sourceIDsCP.empty())      return m_sourceIDsCP;
  if (jep     && !m_sourceIDsJEP.empty())     return m_sourceIDsJEP;
  if (cpRoi   && !m_sourceIDsCPRoI.empty())   return m_sourceIDsCPRoI;
  if (jepRoi  && !m_sourceIDsJEPRoI.empty())  return m_sourceIDsJEPRoI;
  if (cpRoib  && !m_sourceIDsCPRoIB.empty())  return m_sourceIDsCPRoIB;
  if (jepRoib && !m_sourceIDsJEPRoIB.empty()) return m_sourceIDsJEPRoIB;
  // PP
  if (all || pp) {
    std::vector<int> slinks(4);
    for (int i = 1; i < 4; ++i) slinks[i] = i;
    fillRobIds(all, 8, 0, slinks, 0, eformat::TDAQ_CALO_PREPROC,
                                                  m_sourceIDsPP);
    if (pp) return m_sourceIDsPP;
  }
  // CP
  if (all || cp) {
    std::vector<int> slinks(2);
    slinks[1] = 2;
    fillRobIds(all, 4, 8, slinks, 0, eformat::TDAQ_CALO_CLUSTER_PROC_DAQ,
                                                           m_sourceIDsCP);
    if (cp) return m_sourceIDsCP;
  }
  // CP RoI
  if (all || cpRoi) {
    const std::vector<int> slinks(1);
    fillRobIds(all, 4, 8, slinks, 1, eformat::TDAQ_CALO_CLUSTER_PROC_ROI,
                                                        m_sourceIDsCPRoI);
    if (cpRoi) return m_sourceIDsCPRoI;
  }
  // JEP
  if (all || jep) {
    std::vector<int> slinks(4);
    for (int i = 1; i < 4; ++i) slinks[i] = i;
    fillRobIds(all, 2, 12, slinks, 0, eformat::TDAQ_CALO_JET_PROC_DAQ,
                                                       m_sourceIDsJEP);
    if (jep) return m_sourceIDsJEP;
  }
  // JEP RoI
  if (all || jepRoi) {
    const std::vector<int> slinks(1);
    fillRobIds(all, 2, 12, slinks, 1, eformat::TDAQ_CALO_JET_PROC_ROI,
                                                    m_sourceIDsJEPRoI);
    if (jepRoi) return m_sourceIDsJEPRoI;
  }
  // Don't include RoIBs (LVL2) in complete set
  // CP RoIB
  if (cpRoib) {
    const std::vector<int> slinks(1, 2);
    fillRobIds(false, 4, 8, slinks, 1, eformat::TDAQ_CALO_CLUSTER_PROC_ROI,
                                                         m_sourceIDsCPRoIB);
    return m_sourceIDsCPRoIB;
  }
  // JEP RoIB
  if (jepRoib) {
    const std::vector<int> slinks(1, 2);
    fillRobIds(false, 2, 12, slinks, 1, eformat::TDAQ_CALO_JET_PROC_ROI,
                                                     m_sourceIDsJEPRoIB);
    return m_sourceIDsJEPRoIB;
  }
  return m_sourceIDs;
}

// Fill vector with ROB IDs for given sub-detector

void RodHeaderByteStreamTool::fillRobIds(const bool all, const int numCrates,
                                         const int crateOffset,
				         const std::vector<int>& slinks,
				 	 const int daqOrRoi,
				         const eformat::SubDetector subdet,
				         std::vector<uint32_t>& detSourceIDs)
{
  if (all && !detSourceIDs.empty()) {
    std::copy(detSourceIDs.begin(), detSourceIDs.end(),
                                    std::back_inserter(m_sourceIDs));
  } else {
    for (int crate = 0; crate < numCrates; ++crate) {
      const int numSlinks = slinks.size();
      for (int i = 0; i < numSlinks; ++i) {
        const uint32_t rodId = m_srcIdMap->getRodID(crate + crateOffset,
	                                   slinks[i], daqOrRoi, subdet);
        const uint32_t robId = m_srcIdMap->getRobID(rodId);
	if (all) m_sourceIDs.push_back(robId);
	else     detSourceIDs.push_back(robId);
      }
    }
  }
}

// Return true if StoreGate key ends in given string

bool RodHeaderByteStreamTool::isAppended(const std::string& sgKey,
                                         const std::string& flag) const
{
  const std::string::size_type pos = sgKey.find(flag);
  return (pos != std::string::npos && pos == sgKey.length() - flag.length());
}

} // end namespace
