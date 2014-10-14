// ===========================================================================
// Includes
// ===========================================================================
// STD:
// ===========================================================================
#include <numeric>
#include <set>
#include <utility>
// ===========================================================================
// Athena
// ===========================================================================
#include "GaudiKernel/IInterface.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/StatusCode.h"
#include "StoreGate/SegMemSvc.h"
// ===========================================================================
// TrigT1
// ===========================================================================
#include "xAODTrigL1Calo/TriggerTower.h"
#include "xAODTrigL1Calo/TriggerTowerContainer.h"

#include "L1CaloSrcIdMap.h"
// ===========================================================================
#include "PpmByteStreamTool2.h"
// ===========================================================================
namespace LVL1BS {
// ===========================================================================

// Interface ID
static const InterfaceID IID_IPpmByteStreamTool2("PpmByteStreamTool2", 1, 1);

const InterfaceID& PpmByteStreamTool2::interfaceID() {
  return IID_IPpmByteStreamTool2;
}
// ===========================================================================
// Constructor
PpmByteStreamTool2::PpmByteStreamTool2(const std::string& type,
    const std::string& name, const IInterface* parent) :
    AthAlgTool(type, name, parent),
        m_sms("SegMemSvc/SegMemSvc", name),
        m_srcIdMap(0),
        m_subDetector(eformat::TDAQ_CALO_PREPROC),
        m_channelsType(ChannelsType::Data) {
  declareInterface<PpmByteStreamTool2>(this);

  declareProperty("NCrates", m_crates = 8, "Number of crates");
  declareProperty("NModules", m_modules = 16, "Number of modules");
  declareProperty("NChannels", m_channels = 64, "Number of channels");
  declareProperty("DataSize", m_dataSize = 3584, "Data size");
  declareProperty("MaxSlinks", m_maxSlinks = 4, "Max slinks");
}
// ===========================================================================
// Destructor
PpmByteStreamTool2::~PpmByteStreamTool2() {
}
// ===========================================================================
// Initialize

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

StatusCode PpmByteStreamTool2::initialize() {
  msg(MSG::INFO) << "Initializing " << name() << " - package version "
      << PACKAGE_VERSION << endreq;

  StatusCode sc = m_sms.retrieve();
  if (sc.isFailure()) {
    msg(MSG::ERROR) << "Failed to retrieve service " << m_sms << endreq;
    return sc;
  } else
    msg(MSG::INFO) << "Retrieved service " << m_sms << endreq;

  m_srcIdMap = new L1CaloSrcIdMap { };
  return StatusCode::SUCCESS;
}
// ===========================================================================
// Finalize

StatusCode PpmByteStreamTool2::finalize() {
  delete m_srcIdMap;
  return StatusCode::SUCCESS;
}
// ===========================================================================
void PpmByteStreamTool2::reserveMemory() {

}

// Conversion bytestream to trigger towers
StatusCode PpmByteStreamTool2::convert(
    const IROBDataProviderSvc::VROBFRAG& robFrags,
    xAOD::TriggerTowerContainer* const ttCollection) {
  const bool debug = msgLvl(MSG::DEBUG);
  const bool verbose = msgLvl(MSG::VERBOSE);

  double phi { 0.0 }, eta { 0.0 };
  xAOD::TriggerTower* tt =
      new (m_sms->allocate<xAOD::TriggerTower>(SegMemSvc::JOB))
      xAOD::TriggerTower();
  // xAOD::TriggerTower_v2* tt = new xAOD::TriggerTower_v2();
  ttCollection->push_back(tt);



  return StatusCode::SUCCESS;
}
// ===========================================================================
// Conversion of trigger towers to bytestream

StatusCode PpmByteStreamTool2::convert(
    const xAOD::TriggerTowerContainer* const ttCollection,
    RawEventWrite* const re) {
  const bool debug = msgLvl(MSG::DEBUG);
  if (debug)
    msg(MSG::DEBUG);

  return StatusCode::SUCCESS;
}
// ===========================================================================

// Return reference to vector with all possible Source Identifiers

const std::vector<uint32_t>& PpmByteStreamTool2::sourceIDs(
    const std::string& sgKey) {
  // -------------------------------------------------------------------------
  const std::string SPARE_FLAG("Spare");
  const std::string MUON_FLAG("Muon");

  // Check if spare channels wanted
  std::string::size_type pos = sgKey.find(SPARE_FLAG);
  if (pos != std::string::npos
      && pos == (sgKey.length() - SPARE_FLAG.length())) {
    m_channelsType = ChannelsType::Spare;
  } else {
    // Check if muon channels wanted
    pos = sgKey.find(MUON_FLAG);
    if (pos != std::string::npos
        && pos == (sgKey.length() - MUON_FLAG.length())) {
      m_channelsType = ChannelsType::Muon;
    } else {
      // Default - all channels
      m_channelsType = ChannelsType::Data;
    }
  }
  // -------------------------------------------------------------------------
  if (m_sourceIDs.empty()) {
    for (int crate = 0; crate < m_crates; ++crate) {
      for (int slink = 0; slink < m_srcIdMap->maxSlinks(); ++slink) {
        //       const int daqOrRoi = 0;
        const uint32_t rodId = m_srcIdMap->getRodID(crate, slink, 0,
            m_subDetector);
        // In principle rodId === robId
        const uint32_t robId = m_srcIdMap->getRobID(rodId);

        // In m_sourceIDs we store all ROB Ids from L1Calo
        m_sourceIDs.push_back(robId);

        // Spare IDs in crates 2,3,4,5
        if (m_channelsType != ChannelsType::Data) {
          const int BEGIN_SPARE_CRATE { 2 };
          const int END_SPARE_CRATE { 5 };

          if (crate >= BEGIN_SPARE_CRATE && crate < END_SPARE_CRATE) {
            if (m_channelsType == ChannelsType::Spare) {
              m_sourceIDsSpare.push_back(robId);
            }
            // Muon ids in create 2,3 and slink=0
            const int END_MUON_CRATE { 3 };
            const int SLINK_MUON { 0 };

            if (m_channelsType == ChannelsType::Muon && crate <= END_MUON_CRATE
                && slink == SLINK_MUON) {
              m_sourceIDsMuon.push_back(robId);
            }
          }
        }
      }
    }
  }
  // -------------------------------------------------------------------------
  switch (m_channelsType) {
  case ChannelsType::Spare:
    return m_sourceIDsSpare;
  case ChannelsType::Muon:
    return m_sourceIDsMuon;
  default:
    return m_sourceIDs;
  }
}
// ===========================================================================
}// end namespace
