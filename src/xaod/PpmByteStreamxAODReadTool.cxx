// ===========================================================================
// Includes
// ===========================================================================
// STD:
// ===========================================================================
#include <stdexcept>
// ===========================================================================
#include "PpmByteStreamxAODReadTool.h"

#include "eformat/SourceIdentifier.h"
#include "../L1CaloSrcIdMap.h"
#include "CaloUserHeader.h"
#include "SubBlockHeader.h"
#include "SubBlockStatus.h"

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
PpmByteStreamxAODReadTool::PpmByteStreamxAODReadTool(const std::string& name =
    "PpmByteStreamxAODReadTool") :
    AsgTool(name),
    m_ppmMaps("LVL1::PpmCoolOrBuiltinMappingTool/PpmCoolOrBuiltinMappingTool")
{
  declareInterface < PpmByteStreamxAODReadTool > (this);
  declareProperty("NCrates", m_crates = 8, "Number of crates");
  declareProperty("NModules", m_modules = 16, "Number of modules");
  declareProperty("NChannels", m_channels = 64, "Number of channels");
  declareProperty("DataSize", m_dataSize = 3584, "Data size");
  declareProperty("MaxSlinks", m_maxSlinks = 4, "Max slinks");

  declareProperty("PpmMappingTool", m_ppmMaps,
      "Crate/Module/Channel to Eta/Phi/Layer mapping tool");
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
    xAOD::TriggerTowerContainer* const ttCollection) {

  m_triggerTowers = ttCollection;

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
  const auto robSourceID = rob.rod_source_id();
  const auto sourceID = (robSourceID >> 16) & 0xff;
  const auto rodCrate = m_srcIdMap->crate(sourceID);
  const auto rodSlink = m_srcIdMap->slink(sourceID);

  m_rodVer = rob.rod_version() & 0xffff;
  m_verCode = ((m_rodVer & 0xfff) << 4) | 1;
  m_rodRunNumber = rob.rod_run_no() & 0xffffff;

  if (sourceID != eformat::TDAQ_CALO_PREPROC) {
    ATH_MSG_ERROR(
        "Wrong source id for trigger tower preprocessor: " << sourceID);
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
        subBlock = blockType & 0xe;
      } else if (blockType == (subBlock | 1)) {
        m_subBlockStatus = SubBlockStatus(*payload);
        subBlock = 0;
      }
    } else {
      CHECK(processPpmWord_(*payload, indata));
    }
  }
  return StatusCode::SUCCESS;
}

StatusCode PpmByteStreamxAODReadTool::processPpmWord_(uint32_t word,
    int indata) {
  if (m_subBlockHeader.format() >= 2 || m_verCode >= 0x41) {
    m_ppBlock.push_back(word);
  }

  return StatusCode::SUCCESS;
}

StatusCode PpmByteStreamxAODReadTool::processPpmBlock_() {
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

StatusCode PpmByteStreamxAODReadTool::processPpmBlockR4V1_() {
  if (m_subBlockHeader.format() == 1) {
    CHECK(processPpmStandardR4V1_());
    return StatusCode::SUCCESS;
  } else if (m_subBlockHeader.format() >= 2) {
    // TODO: convert compressed
    return StatusCode::FAILURE;
  }
  return StatusCode::FAILURE;
}

StatusCode PpmByteStreamxAODReadTool::processPpmStandardR4V1_() {
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
    std::vector<uint8_t> lcpExt;
    std::vector<uint8_t> lcpSat;
    std::vector<uint8_t> lcpPeak;

    std::vector<uint8_t> ljeVal;
    std::vector<uint8_t> ljeLow;
    std::vector<uint8_t> ljeHigh;
    std::vector<uint8_t> ljeRes;

    std::vector<uint16_t> adcVal;
    std::vector<uint8_t> adcExt;
    std::vector<int16_t> pedCor;
    std::vector<uint8_t> pedEn;
    try {
      for (int i = 0; i < numLut; ++i) {
        lcpVal.push_back(getPpmBytestreamField_(8));
        lcpExt.push_back(getPpmBytestreamField_(1));
        lcpSat.push_back(getPpmBytestreamField_(1));
        lcpPeak.push_back(getPpmBytestreamField_(1));
      }

      for (int i = 0; i < numLut; ++i) {
        ljeVal.push_back(getPpmBytestreamField_(8));
        ljeLow.push_back(getPpmBytestreamField_(1));
        ljeHigh.push_back(getPpmBytestreamField_(1));
        ljeRes.push_back(getPpmBytestreamField_(1));
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
        addTriggerTowers_(crate, module, chan, lcpVal, lcpExt, lcpSat,
            lcpPeak, ljeVal, ljeLow, ljeHigh, ljeRes, adcVal, adcExt, pedCor,
            pedEn)
    );

    //}
  }

  return StatusCode::SUCCESS;
}

StatusCode PpmByteStreamxAODReadTool::addTriggerTowers_(
    uint8_t crate,
    uint8_t module,
    uint8_t channel,
    const std::vector<uint8_t>& lcpVal,
    const std::vector<uint8_t>& lcpExt,
    const std::vector<uint8_t>& lcpSat,
    const std::vector<uint8_t>& lcpPeak,

    const std::vector<uint8_t>& ljeVal,
    const std::vector<uint8_t>& ljeLow,
    const std::vector<uint8_t>& ljeHigh,
    const std::vector<uint8_t>& ljeRes,
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
  //         const uint_least16_t& error,
  //         const uint_least8_t& peak,
  //         const uint_least8_t& adcPeak
  // );
  tt->initialize(coolId, layer, eta, phi, lcpVal, ljeVal, pedCor, pedEn, lcpSat,
      adcVal, adcExt, error, m_caloUserHeader.lut(), m_caloUserHeader.ppFadc());
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

uint32_t PpmByteStreamxAODReadTool::getPpmBytestreamField_(uint8_t numBits) {
ATH_MSG_DEBUG("-> " << m_ppPointer << " " << int(numBits) << " " << m_ppMaxBit);
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
  ATH_MSG_DEBUG("result=" << result);
  return result;
}

throw std::out_of_range("Requested too much bits from ppm block");
}
// ===========================================================================
