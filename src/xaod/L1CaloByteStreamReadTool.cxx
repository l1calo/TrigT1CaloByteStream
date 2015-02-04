// ===========================================================================
// Includes
// ===========================================================================
// STD:
// ===========================================================================
#include <stdexcept>
// ===========================================================================
#include "eformat/SourceIdentifier.h"
#include "../L1CaloSrcIdMap.h"
#include "CaloUserHeader.h"
#include "L1CaloByteStreamReadTool.h"
#include "SubBlockHeader.h"
#include "SubBlockStatus.h"
#include "WordDecoder.h"
#include "CpmWord.h"

using namespace LVL1BS;

namespace {
uint32_t bitFieldSize(uint32_t word, uint8_t offset, uint8_t size) {
  return (word >> offset) & ((1 << size) - 1);
}

uint32_t coolId(uint8_t crate, uint8_t module, uint8_t channel) {
  const uint8_t pin = channel % 16;
  const uint8_t asic = channel / 16;
  return (crate << 24) | (1 << 20) | (module << 16) | (pin << 8) | asic;
}
}

// ===========================================================================

// ===========================================================================
// Constructor
L1CaloByteStreamReadTool::L1CaloByteStreamReadTool(const std::string& name =
    "PpmByteStreamxAODReadTool") :
    AsgTool(name), m_sms("SegMemSvc/SegMemSvc", name),
    m_ppmMaps("LVL1::PpmMappingTool/PpmMappingTool"),
    m_cpmMaps("LVL1::CpmMappingTool/CpmMappingTool"),
    m_robDataProvider("ROBDataProviderSvc", name) {
  declareInterface<L1CaloByteStreamReadTool>(this);
  declareProperty("PpmMappingTool", m_ppmMaps,
      "Crate/Module/Channel to Eta/Phi/Layer mapping tool");
  declareProperty("ROBDataProviderSvc", m_robDataProvider,
        "Get ROB source IDs service");
}

// ===========================================================================
// Initialize

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

StatusCode L1CaloByteStreamReadTool::initialize() {
  ATH_MSG_INFO(
      "Initializing " << name() << " - package version " << PACKAGE_VERSION);

  m_srcIdMap = new L1CaloSrcIdMap();
  CHECK(m_ppmMaps.retrieve());
  CHECK(m_cpmMaps.retrieve());
  CHECK(m_robDataProvider.retrieve());
  return StatusCode::SUCCESS;
}
// ===========================================================================
// Finalize

StatusCode L1CaloByteStreamReadTool::finalize() {
  delete m_srcIdMap;

  return StatusCode::SUCCESS;
}

// Conversion bytestream to trigger towers
StatusCode L1CaloByteStreamReadTool::convert(
    const IROBDataProviderSvc::VROBFRAG& robFrags,
    xAOD::TriggerTowerContainer* const ttCollection) {

  m_triggerTowers = ttCollection;
  m_subDetectorID = eformat::TDAQ_CALO_PREPROC;
  m_requestedType = RequestType::PPM;

  ROBIterator rob = robFrags.begin();
  ROBIterator robEnd = robFrags.end();

  int robCounter = 1;
  for (; rob != robEnd; ++rob, ++robCounter) {
    StatusCode sc = processRobFragment_(rob, RequestType::PPM);
    if (!sc.isSuccess()) {

    }
  }
  m_triggerTowers = nullptr;
  return StatusCode::SUCCESS;
}

// Conversion bytestream to trigger towers
StatusCode L1CaloByteStreamReadTool::convert(
    const IROBDataProviderSvc::VROBFRAG& robFrags,
    xAOD::CPMTowerContainer* const cpmCollection) {
  ATH_MSG_DEBUG("Start converting CPM towers");
  ATH_MSG_DEBUG("Number of Calo Cluster Processor fragments: " << robFrags.size());

  m_cpmTowers = cpmCollection;
  m_subDetectorID = eformat::TDAQ_CALO_CLUSTER_PROC_DAQ;
  m_requestedType = RequestType::CPM;

  ROBIterator rob = robFrags.begin();
  ROBIterator robEnd = robFrags.end();

  int robCounter = 1;
  for (; rob != robEnd; ++rob, ++robCounter) {

    StatusCode sc = processRobFragment_(rob, RequestType::CPM);
    if (!sc.isSuccess()) {

    }
  }
  m_cpmTowers = nullptr;
  return StatusCode::SUCCESS;
}


StatusCode L1CaloByteStreamReadTool::convert(xAOD::TriggerTowerContainer* const ttCollection) {
  const std::vector<uint32_t>& vID(ppmSourceIDs());
  // // get ROB fragments
  IROBDataProviderSvc::VROBFRAG robFrags;
  m_robDataProvider->getROBData(vID, robFrags, "PpmByteStreamxAODReadTool");
  ATH_MSG_DEBUG("Number of ROB fragments:" << robFrags.size());

  CHECK(convert(robFrags, ttCollection));

  return StatusCode::SUCCESS;
}

StatusCode L1CaloByteStreamReadTool::convert(xAOD::CPMTowerContainer* const ttCollection) {
  const std::vector<uint32_t>& vID(cpSourceIDs());
  // get ROB fragments
  IROBDataProviderSvc::VROBFRAG robFrags;
  m_robDataProvider->getROBData(vID, robFrags, "L1CaloByteStreamReadTool");
  ATH_MSG_DEBUG("Number of ROB fragments:" << robFrags.size());

  CHECK(convert(robFrags, ttCollection));

  return StatusCode::SUCCESS;
}

// ===========================================================================
StatusCode L1CaloByteStreamReadTool::processRobFragment_(
    const ROBIterator& robIter, const RequestType& requestedType) {

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
  const auto robSourceID = rob.rod_source_id();
  const auto sourceID = (robSourceID >> 16) & 0xff;
  const auto rodCrate = m_srcIdMap->crate(sourceID);
  const auto rodSlink = m_srcIdMap->slink(sourceID);

  m_rodVer = rob.rod_version() & 0xffff;
  m_verCode = ((m_rodVer & 0xfff) << 4) | 1;
  m_rodRunNumber = rob.rod_run_no() & 0xffffff;


  if (sourceID != m_subDetectorID) {
    ATH_MSG_ERROR("Wrong subdetector source id for requested objects: " << sourceID);
    return StatusCode::FAILURE;
  }

  ATH_MSG_DEBUG("Treating crate " << rodCrate << " slink " << rodSlink);

  m_caloUserHeader = CaloUserHeader(*payload);
  if (!m_caloUserHeader.isValid()) {
    ATH_MSG_ERROR("Invalid or missing user header");
    return StatusCode::FAILURE;
  }

  ATH_MSG_DEBUG(
      "Run number: " << MSG::dec << m_rodRunNumber << endreq
          << "Version code: 0x" << MSG::hex << int(m_verCode) << MSG::dec
          << endreq << "LUT triggered slice offset:  "
          << int(m_caloUserHeader.lut()) << endreq
          << "FADC triggered slice offset: " << int(m_caloUserHeader.ppFadc())
          << endreq << "FADC baseline lower bound:   "
          << int(m_caloUserHeader.ppLowerBound()));

  int indata = 0;
  uint8_t blockType = 0;
  int subBlock = 0;

  for (; payload != payloadEnd; ++payload) {
    if (CaloUserHeader::isValid(*payload) && (subBlock == 0)) {

    } else if (SubBlockHeader::isSubBlockHeader(*payload)) {
      CHECK(processPpmBlock_());

      blockType = (*payload >> 28) & 0xf;

      if ((blockType & 0xd) == 0xc) {
        m_subBlockHeader = SubBlockHeader(*payload);
        ATH_MSG_VERBOSE(
            "SubBlock version #" << int(m_subBlockHeader.version())
             << " format #" << int(m_subBlockHeader.format())
        );
        subBlock = blockType & 0xe;
      } else if (blockType == (subBlock | 1)) {
        m_subBlockStatus = SubBlockStatus(*payload);
        subBlock = 0;
      }
    } else {
      switch(m_subDetectorID){
      case eformat::TDAQ_CALO_PREPROC:
          CHECK(processPpmWord_(*payload, indata));
          break;
      case eformat::TDAQ_CALO_CLUSTER_PROC_DAQ:
          CHECK(processCpWord_(*payload));
          break;
      default:
        break;
      }
    }
  }
  CHECK(processPpmBlock_());
  return StatusCode::SUCCESS;
}

StatusCode L1CaloByteStreamReadTool::processPpmWord_(uint32_t word,
    int indata) {
  if (m_subBlockHeader.format() >= 2 || m_verCode >= 0x41) {
    m_ppBlock.push_back(word);
  } else if (m_verCode == 0x31) {
    return processPpmStandardR3V1_(word, indata);
  }
  return StatusCode::SUCCESS;
}

StatusCode L1CaloByteStreamReadTool::processCpWord_(uint32_t word) {
  if ((m_requestedType == RequestType::CPM) && CpmWord::isValid(word)) {
    switch(m_verCode){
      case 0x41:
        CHECK(processCpmWordR4V1_(word));
        break;
      case 0x31:
        // TODO
        return StatusCode::FAILURE;
        break;
      default:
        break;
    }
  }

  return StatusCode::SUCCESS;
}

StatusCode L1CaloByteStreamReadTool::processCpmWordR4V1_(uint32_t word) {
  CpmWord cpm(word);

  CHECK(cpm.isValid());
  CHECK(addCpmTower_(m_subBlockHeader.crate(), m_subBlockHeader.module(),
      word));

  return StatusCode::SUCCESS;
}

StatusCode L1CaloByteStreamReadTool::processPpmBlock_() {
  if (m_ppBlock.size() > 0) {
    if (m_verCode == 0x32) {
      // TODO: process
    } else if (m_verCode == 0x41 || m_verCode == 0x42) {
      StatusCode sc = processPpmBlockR4V1_();
      m_ppBlock.clear();
      CHECK(sc);
    }
  }
  return StatusCode::SUCCESS;
}

StatusCode L1CaloByteStreamReadTool::processPpmStandardR3V1_(uint32_t word, int indata){
  return StatusCode::FAILURE;
}

StatusCode L1CaloByteStreamReadTool::processPpmBlockR4V1_() {
  if (m_subBlockHeader.format() == 1) {
    CHECK(processPpmStandardR4V1_());
    return StatusCode::SUCCESS;
  } else if (m_subBlockHeader.format() >= 2) {
    // TODO: convert compressed
    return StatusCode::FAILURE;
  }
  return StatusCode::FAILURE;
}

StatusCode L1CaloByteStreamReadTool::processPpmStandardR4V1_() {
  uint8_t numAdc = m_subBlockHeader.nSlice2();
  uint8_t numLut = m_subBlockHeader.nSlice1();
  uint8_t crate = m_subBlockHeader.crate();
  uint8_t module = m_subBlockHeader.module();

  uint8_t bitsPerMcm = 11 * (numAdc + numLut * 3) * 4 + 10;
  uint8_t bcid = 0;
  uint8_t mcmNum = 0;

  m_ppPointer = 0;
  m_ppMaxBit = 31 * m_ppBlock.size();

  for (uint8_t chan = 0; chan < 64; ++chan) {
    //for (uint8_t k = 0; k < 4; ++k) {
    std::vector<uint8_t> lcpVal;
    // std::vector<uint8_t> lcpExt;
    // std::vector<uint8_t> lcpSat;
    // std::vector<uint8_t> lcpPeak;
    std::vector<uint8_t> lcpBcidVec;

    std::vector<uint8_t> ljeVal;
    // std::vector<uint8_t> ljeLow;
    // std::vector<uint8_t> ljeHigh;
    // std::vector<uint8_t> ljeRes;
    std::vector<uint8_t> ljeSat80Vec;



    std::vector<uint16_t> adcVal;
    std::vector<uint8_t> adcExt;
    std::vector<int16_t> pedCor;
    std::vector<uint8_t> pedEn;
    try {
      for (int i = 0; i < numLut; ++i) {
        lcpVal.push_back(getPpmBytestreamField_(8));
        lcpBcidVec.push_back(getPpmBytestreamField_(3));
        // lcpExt.push_back(getPpmBytestreamField_(1));
        // lcpSat.push_back(getPpmBytestreamField_(1));
        // lcpPeak.push_back(getPpmBytestreamField_(1));
      }

      for (int i = 0; i < numLut; ++i) {
        ljeVal.push_back(getPpmBytestreamField_(8));
        ljeSat80Vec.push_back(getPpmBytestreamField_(3));
        // ljeLow.push_back(getPpmBytestreamField_(1));
        // ljeHigh.push_back(getPpmBytestreamField_(1));
        // ljeRes.push_back(getPpmBytestreamField_(1));
      }

      for (int i = 0; i < numAdc; ++i) {
        adcVal.push_back(getPpmBytestreamField_(10));
        adcExt.push_back(getPpmBytestreamField_(1));
      }

      for (int i = 0; i < numLut; ++i) {
        pedCor.push_back(getPpmBytestreamField_(10));
        pedEn.push_back(getPpmBytestreamField_(1));
      }
    } catch (const std::out_of_range& ex) {
      ATH_MSG_ERROR("Failed to decode ppm block " << ex.what());
      return StatusCode::FAILURE;
    }
    CHECK(
        addTriggerTowers_(crate, module, chan, lcpVal, lcpBcidVec,
            ljeVal, ljeSat80Vec, adcVal, adcExt, pedCor, pedEn));

    //}
  }

  return StatusCode::SUCCESS;
}

StatusCode L1CaloByteStreamReadTool::addTriggerTowers_(
    uint8_t crate,
    uint8_t module,
    uint8_t channel,
    const std::vector<uint8_t>& lcpVal,
    const std::vector<uint8_t>& lcpBcidVec,
    const std::vector<uint8_t>& ljeVal,
    const std::vector<uint8_t>& ljeSat80Vec,
    const std::vector<uint16_t>& adcVal,
    const std::vector<uint8_t>& adcExt,
    const std::vector<int16_t>& pedCor,
    const std::vector<uint8_t>& pedEn) {

  int layer;
  int error = 0;
  double eta = 0.;
  double phi = 0.;
  m_ppmMaps->mapping(crate, module, channel, eta, phi, layer);
  uint32_t coolId = ::coolId(crate, module, channel);

  xAOD::TriggerTower* tt = new xAOD::TriggerTower();
  m_triggerTowers->push_back(tt);
  // tt->initialize(
  //         const uint_least32_t& coolId,
  //         const uint_least8_t& layer,
  //         const float& eta,
  //         const float& phi,
  //         const std::vector<uint_least8_t>& lut_cp,
  //         const std::vector<uint_least8_t>& lut_jep,
  //         const std::vector<int_least16_t>& correction,
  //         const std::vector<uint_least8_t>& correctionEnabled,
  //         const std::vector<uint_least8_t>& bcidVec,
  //         const std::vector<uint_least16_t>& adc,
  //         const std::vector<uint_least8_t>& bcidExt,
  //         const std::vector<uint_least8_t>& sat80, 
  //         const uint_least16_t& error,
  //         const uint_least8_t& peak,
  //         const uint_least8_t& adcPeak
  // );
  tt->initialize(coolId, layer, eta, phi, lcpVal, ljeVal, pedCor, pedEn,
      lcpBcidVec, adcVal, adcExt, ljeSat80Vec, error, m_caloUserHeader.lut(),
      m_caloUserHeader.ppFadc());
  return StatusCode::SUCCESS;
}

StatusCode L1CaloByteStreamReadTool::addCpmTower_(
    uint8_t crate,
    uint8_t module,
    const CpmWord& word) {
  // TODO: Handle slices

  std::vector<uint8_t> emEnergy { word.tower0Et()};
  std::vector<uint8_t> hadEnergy { word.tower1Et()};
  std::vector<uint8_t> emError { 0 };
  std::vector<uint8_t> hadError { 0 };



  uint8_t channel = word.ttPair(); // TODO: fix
  double eta = 0.;
  double phi = 0.;
  int layer = 0;
  int peak = 0;
  m_cpmMaps->mapping(crate, module, channel, eta, phi, layer);

  xAOD::CPMTower *cpm = new xAOD::CPMTower();
  m_cpmTowers->push_back(cpm);

  cpm->setEmEnergyVec(emEnergy);
  cpm->setHadEnergyVec(hadEnergy);
  cpm->setEmErrorVec(emError);
  cpm->setHadErrorVec(hadError);
  cpm->setEta(eta);
  cpm->setPhi(phi);
  cpm->setPeak(peak);

  return StatusCode::SUCCESS;
}

// Return reference to vector with all possible Source Identifiers

const std::vector<uint32_t>& L1CaloByteStreamReadTool::ppmSourceIDs() {
  const int crates = 8;
  if (m_ppmSourceIDs.empty()) {
    for (int crate = 0; crate < crates; ++crate) {
      for (int slink = 0; slink < m_srcIdMap->maxSlinks(); ++slink) {
        const uint32_t rodId = m_srcIdMap->getRodID(crate, slink, 0,
            eformat::TDAQ_CALO_PREPROC);
        const uint32_t robId = m_srcIdMap->getRobID(rodId);

        m_ppmSourceIDs.push_back(robId);
      }
    }
  }
  return m_ppmSourceIDs;
}


const std::vector<uint32_t>& L1CaloByteStreamReadTool::cpSourceIDs() {
  const int creates = 4;
  const int crateOffsetHw = 8;

  if (m_cpSourceIDs.empty())
      {
          const int maxCrates = creates + crateOffsetHw;
          const int maxSlinks = m_srcIdMap->maxSlinks();
          for (int hwCrate = crateOffsetHw; hwCrate < maxCrates; ++hwCrate)
          {
              for (int slink = 0; slink < maxSlinks; ++slink)
              {
                  const int daqOrRoi = 0;
                  const uint32_t rodId = m_srcIdMap->getRodID(hwCrate, slink,
                      daqOrRoi, eformat::TDAQ_CALO_CLUSTER_PROC_DAQ);
                  const uint32_t robId = m_srcIdMap->getRobID(rodId);
                  m_cpSourceIDs.push_back(robId);
              }
          }
      }
      return m_cpSourceIDs;
}


uint32_t L1CaloByteStreamReadTool::getPpmBytestreamField_(uint8_t numBits) {
  if ((m_ppPointer + numBits) <= m_ppMaxBit) {
    uint8_t iWord = m_ppPointer / 31;
    uint8_t iBit = m_ppPointer % 31;
    m_ppPointer += numBits;

    uint32_t result;
    if ((iBit + numBits) <= 31) {
      result = ::bitFieldSize(m_ppBlock[iWord], iBit, numBits);
    } else {
      uint8_t nb1 = 31 - iBit;
      uint8_t nb2 = numBits - nb1;
      uint32_t field1 = ::bitFieldSize(m_ppBlock[iWord], iBit, nb1);
      uint32_t field2 = ::bitFieldSize(m_ppBlock[iWord + 1], 0, nb2);
      result = field1 | (field2 << nb1);
    }

    return result;
  }

  throw std::out_of_range("Requested too much bits from ppm block");
}
// ===========================================================================
