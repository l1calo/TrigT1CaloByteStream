// ===========================================================================
// Includes
// ===========================================================================
// STD:
// ===========================================================================

// ===========================================================================
#include "PpmByteStreamxAODReadTool.h"

#include "eformat/SourceIdentifier.h"
#include "../L1CaloSrcIdMap.h"
#include "CaloUserHeader.h"

// ===========================================================================
namespace LVL1BS {
// ===========================================================================

// ===========================================================================
// Constructor
PpmByteStreamxAODReadTool::PpmByteStreamxAODReadTool(const std::string& name =
    "PpmByteStreamxAODReadTool") :
    AsgTool(name) {
  declareInterface < PpmByteStreamxAODReadTool > (this);
  declareProperty("NCrates", m_crates = 8, "Number of crates");
  declareProperty("NModules", m_modules = 16, "Number of modules");
  declareProperty("NChannels", m_channels = 64, "Number of channels");
  declareProperty("DataSize", m_dataSize = 3584, "Data size");
  declareProperty("MaxSlinks", m_maxSlinks = 4, "Max slinks");
}

// ===========================================================================
// Initialize

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

StatusCode PpmByteStreamxAODReadTool::initialize() {
  ATH_MSG_INFO(
      "Initializing " << name() << " - package version " << PACKAGE_VERSION);

  m_srcIdMap = new L1CaloSrcIdMap();
  return StatusCode::SUCCESS;
}
// ===========================================================================
// Finalize

StatusCode PpmByteStreamxAODReadTool::finalize() {
  delete m_srcIdMap;

  return StatusCode::SUCCESS;
}

// Conversion bytestream to trigger towers
StatusCode PpmByteStreamxAODReadTool::convert(
    const IROBDataProviderSvc::VROBFRAG& robFrags,
    xAOD::TriggerTowerContainer* const /*ttCollection*/) {
  ROBIterator rob = robFrags.begin();
  ROBIterator robEnd = robFrags.end();

  int robCounter = 1;
  for (; rob != robEnd; ++rob, ++robCounter) {
    StatusCode sc = processRobFragment_(rob);
    if (!sc.isSuccess()) {

    }
  }

  return StatusCode::SUCCESS;
}

// ===========================================================================
StatusCode PpmByteStreamxAODReadTool::processRobFragment_(
    const ROBIterator& robIter) {

  auto rob = **robIter;

  ATH_MSG_DEBUG(
      "Treating ROB fragment source id #" << MSG::hex << rob.rob_source_id());

  // -------------------------------------------------------------------------
  // Check Rob status
  if (rob.nstatus() > 0) {
    ROBPointer robData;
    rob.status(robData);
    if (*robData != 0) {
      ATH_MSG_WARNING("ROB status error - skipping fragment");
      return StatusCode::FAILURE;
    }
  }
  // -------------------------------------------------------------------------
  RODPointer payloadBeg;
  RODPointer payloadEnd;
  RODPointer payload;

  rob.rod_data(payloadBeg);
  payloadEnd = payloadBeg + rob.rod_ndata();
  payload = payloadBeg;
  // -------------------------------------------------------------------------
  if (payload == payloadEnd) {
    ATH_MSG_DEBUG("ROB fragment empty");
    return StatusCode::FAILURE;
  }
  // -------------------------------------------------------------------------
  const auto sourceID = rob.rod_source_id();
  const auto rodCrate = m_srcIdMap->crate(sourceID);
  const auto rodSlink = m_srcIdMap->slink(sourceID);

  const auto majorVersion = rob.rod_version() >> 16;
  const auto minorVersion = rob.rod_version() & 0xffff;
  const auto runNumber = rob.rod_run_no() & 0xffffff;

  ATH_MSG_DEBUG("Treating crate " << rodCrate << " slink " << rodSlink);

  CaloUserHeader caloUserHeader(*payload);
  if (!caloUserHeader.isValid()) {
    ATH_MSG_ERROR("Invalid or missing user header");
    return StatusCode::FAILURE;
  }

  ATH_MSG_DEBUG(
        "Run number"
        << runNumber
        << "Major format version number: "
        << MSG::hex  << majorVersion << MSG::dec << endreq
        << "Minor format version number: "
        << MSG::hex << minorVersion << MSG::dec << endreq
        << "LUT triggered slice offset:  " << int(caloUserHeader.lut()) << endreq
        << "FADC triggered slice offset: " << int(caloUserHeader.ppFadc())<< endreq
        << "FADC baseline lower bound:   " << int(caloUserHeader.ppLowerBound()));

//  for (; payload != payloadEnd; ++payload) {
//    if (payload != nullptr) {
//      ATH_MSG_INFO("Payload " << MSG::hex << *payload);
//    } else {
//      ATH_MSG_INFO("Payload failed");
//    }
//  }

  return StatusCode::SUCCESS;
}

// Return reference to vector with all possible Source Identifiers

const std::vector<uint32_t>& PpmByteStreamxAODReadTool::sourceIDs(
    const std::string& /*sgKey*/) {

  if (m_sourceIDs.empty()) {
    for (int crate = 0; crate < m_crates; ++crate) {
      for (int slink = 0; slink < m_srcIdMap->maxSlinks(); ++slink) {
        const uint32_t rodId = m_srcIdMap->getRodID(crate, slink, 0,
            eformat::TDAQ_CALO_PREPROC);
        const uint32_t robId = m_srcIdMap->getRobID(rodId);

        m_sourceIDs.push_back(robId);
      }
    }
  }
  return m_sourceIDs;
}
// ===========================================================================
}// end namespace
