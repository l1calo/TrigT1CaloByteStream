// ===========================================================================
// Includes
// ===========================================================================
// STD:
// ===========================================================================
#include <stdexcept>
// ===========================================================================
#include "eformat/SourceIdentifier.h"
#include "CaloUserHeader.h"
#include "SubBlockHeader.h"
#include "SubBlockStatus.h"
#include "WordDecoder.h"
#include "CpmWord.h"
#include "../L1CaloSrcIdMap.h"

#include "L1CaloByteStreamReadTool.h"
// ===========================================================================

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
namespace LVL1BS {
// ===========================================================================
// Constructor
L1CaloByteStreamReadTool::L1CaloByteStreamReadTool(const std::string& name =
    "PpmByteStreamxAODReadTool") :
    AsgTool(name), m_sms("SegMemSvc/SegMemSvc", name),
    m_errorTool("LVL1BS::L1CaloErrorByteStreamTool/L1CaloErrorByteStreamTool"),
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
  CHECK(m_errorTool.retrieve());
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
    const ROBIterator& robIter, const RequestType& /*requestedType*/) {

  auto rob = **robIter;

  ATH_MSG_DEBUG(
      "Treating ROB fragment source id #" << MSG::hex << rob.rob_source_id());


  const auto robSourceID = rob.rod_source_id();
  const auto sourceID = (robSourceID >> 16) & 0xff;
  const auto rodCrate = m_srcIdMap->crate(sourceID);
  const auto rodSlink = m_srcIdMap->slink(sourceID);
  // -------------------------------------------------------------------------
  // Check Rob status
  if (rob.nstatus() > 0) {
    ROBPointer robData;
    rob.status(robData);
    if (*robData != 0) {
      ATH_MSG_WARNING("ROB status error - skipping fragment");
      m_errorTool->robError(robSourceID, *robData);
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
      indata = 0;
      CHECK(processPpmBlock_());
      
      m_ppLuts.clear();
      m_ppFadcs.clear();
      m_ppBlock.clear();

      blockType = (*payload >> 28) & 0xf;

      if ((blockType & 0xd) == 0xc) {
        m_subBlockHeader = SubBlockHeader(*payload);
        ATH_MSG_VERBOSE(
            "SubBlock version #" << int(m_subBlockHeader.version())
             << " format #" << int(m_subBlockHeader.format())
             << " nslice1 #" << int(m_subBlockHeader.nSlice1())
             << " nslice2 #" << int(m_subBlockHeader.nSlice2())
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
      indata++;
    }
  }
  CHECK(processPpmBlock_());
  return StatusCode::SUCCESS;
}

StatusCode L1CaloByteStreamReadTool::processPpmWord_(uint32_t word,
    int indata) {
  if (m_subBlockHeader.format() >= 2 || m_verCode >= 0x41) {
    m_ppBlock.push_back(word);
  } else if ((m_verCode == 0x21) || (m_verCode == 0x31)) {
    return processPpmStandardR3V1_(word, indata);
  } else {
    ATH_MSG_ERROR("Unsupported PPM version:format (" 
      << m_verCode << ":" << format 
      <<") combination");
    return StatusCode::FAILURE;
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
    m_ppPointer = 0;
    if (m_verCode == 0x31) {
      StatusCode sc = processPpmCompressedR3V1_();
      m_ppBlock.clear();
      CHECK(sc);
    } else if (m_verCode == 0x41 || m_verCode == 0x42) {
      StatusCode sc = processPpmBlockR4V1_();
      m_ppBlock.clear();
      CHECK(sc);
    }
  }

  if (m_ppLuts.size() > 0) {
    if (m_verCode == 0x21 || m_verCode == 0x31) {
      StatusCode sc = processPpmBlockR3V1_();
      m_ppLuts.clear();
      m_ppFadcs.clear();
      CHECK(sc);
    }
  }

  return StatusCode::SUCCESS;
}

StatusCode L1CaloByteStreamReadTool::processPpmCompressedR3V1_() {
  uint8_t chan = 0;
  m_ppPointer = 0;
  m_ppMaxBit = 31 * m_ppBlock.size();
  try{
    while (chan < 64) {
      uint8_t present = 1;
      if (m_subBlockHeader.format() == 3) {
        present = getPpmBytestreamField_(1); 
      } 

      if (present == 1) {
        uint8_t lutVal = 0;
        uint8_t fmt = 6;
        uint8_t lutSat=0;
        uint8_t lutExt=0;
        uint8_t lutPeak=0;

        std::vector<uint16_t> adcVal = {0 , 0, 0, 0, 0};
        std::vector<uint8_t> adcExt = {0 , 0, 0, 0, 0};

        uint8_t minHeader = getPpmBytestreamField_(4);
        uint8_t minIndex = minHeader % 5;
        if (minHeader < 15) { // Formats 0-5
          if (minHeader < 10) { // Formats 0-1
            fmt = minHeader / 5;
          } else { // Formats 2-5
            fmt = 2 + getPpmBytestreamField_(2);
            uint8_t haveLut = getPpmBytestreamField_(1);
            if (fmt == 2) {
              if (haveLut == 1) {
                lutVal = getPpmBytestreamField_(3);
                lutPeak = 1; // Even if LutVal==0 it seems
              }
            } else {
              uint8_t haveExt = getPpmBytestreamField_(1);
              if (haveLut == 1) {
                lutVal = getPpmBytestreamField_(8);
                lutExt = getPpmBytestreamField_(1);
                lutSat = getPpmBytestreamField_(1);
                lutPeak = getPpmBytestreamField_(1);
              }

              if (haveExt == 1){
                for(uint8_t i = 0; i < 5; ++i) {
                  adcExt[i] = getPpmBytestreamField_(1);
                }
              } else {
                adcExt[2] = lutExt;
              }
            }
          }
          adcVal = getPpmAdcSamplesR3_(fmt, minIndex);
        } else {
          uint8_t haveAdc = getPpmBytestreamField_(1);
          if (haveAdc == 1) {
            uint16_t val = getPpmBytestreamField_(10);
            for(uint8_t i = 0; i < 5; ++i) {
                  adcVal[i] = val;
            }
          }
        }
        // Add Trigger Tower
        //std::vector<uint8_t> luts = {lutVal};
        CHECK(addTriggerTowerV1_(
          m_subBlockHeader.crate(),
          m_subBlockHeader.module(),
          chan,
          std::vector<uint8_t> {lutVal},
          std::vector<uint8_t> {uint8_t(lutExt | (lutSat << 1) | (lutPeak << 2))},
          adcVal,
          adcExt
        ));
      }
      chan++;
    }
  }catch (const std::out_of_range& ex) {
      ATH_MSG_ERROR("Failed to decode ppm block " << ex.what());
      return StatusCode::FAILURE;
  } 
  return StatusCode::SUCCESS;
}

std::vector<uint16_t> L1CaloByteStreamReadTool::getPpmAdcSamplesR3_(
  uint8_t format, uint8_t minIndex) {

  std::vector<uint16_t> adc = {0, 0, 0, 0, 0};
  uint8_t minAdc = 0;

  for(uint8_t i = 0; i <5; ++i) {
    uint8_t longField = 0;
    uint8_t numBits = 0;
    if (format > 2) {
      longField = getPpmBytestreamField_(1);
      numBits = longField == 0? 4: (format * 2);
    } else {
      numBits = i == 0? 4: (format + 2);
    }

    if (i == 0) {
      minAdc = getPpmBytestreamField_(numBits);
      if (longField == 0) {
        minAdc += m_caloUserHeader.ppLowerBound();
      }
    } else {
        adc[i] = minAdc + getPpmBytestreamField_(numBits);
    }
  }

  if (minIndex == 0) {
    adc[0] = minAdc;
  } else {
    adc[0] = adc[minIndex];
    adc[minIndex] = minAdc;
  }
  return adc;
}



StatusCode L1CaloByteStreamReadTool::processPpmStandardR3V1_(uint32_t word,
    int inData){
  bool error = false;
  if (m_subBlockHeader.seqNum() == 63) { // Error block
    ATH_MSG_DEBUG("Error PPM subblock");
    //TODO: errorTool
  } else {
    const uint8_t numAdc = m_subBlockHeader.nSlice2();
    const uint8_t numLut = m_subBlockHeader.nSlice1();
    const uint8_t nTotal = numAdc + numLut;
    const uint8_t wordsPerBlock = 8; // 16 towers (4 MCMs) / 2 per word
    const uint8_t iBlk =  inData / wordsPerBlock;
    uint8_t iChan =  m_subBlockHeader.seqNum() + 2 * (inData % wordsPerBlock);
    
    if (iBlk < numLut) { // First all LUT values
      for(int i = 0; i < 2; ++i) {
        uint16_t subword = (word >> 16 * i) & 0x7ff;
        m_ppLuts[iChan].push_back(subword);
        iChan++;
      }
    } else if (iBlk < nTotal) { // Next all FADC values
      for(int i = 0; i < 2; ++i) {
        uint16_t subword = (word >> 16 * i) & 0x7ff;
        m_ppFadcs[iChan].push_back(subword);
        iChan++;
      }
    
    } else{
      ATH_MSG_WARNING("Error decoding Ppm word (run1)");
      error = true;
    }
 
  }
  return !error;
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

StatusCode L1CaloByteStreamReadTool::processPpmBlockR3V1_() {
  if (m_subBlockHeader.format() == 1) {
    CHECK(processPpmStandardR3V1_());
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


  m_ppPointer = 0;
  m_ppMaxBit = 31 * m_ppBlock.size();

  for (uint8_t chan = 0; chan < 64; ++chan) {
    //for (uint8_t k = 0; k < 4; ++k) {
    std::vector<uint8_t> lcpVal;
    std::vector<uint8_t> lcpBcidVec;

    std::vector<uint8_t> ljeVal;
    std::vector<uint8_t> ljeSat80Vec;



    std::vector<uint16_t> adcVal;
    std::vector<uint8_t> adcExt;
    std::vector<int16_t> pedCor;
    std::vector<uint8_t> pedEn;
    try {
      for (int i = 0; i < numLut; ++i) {
        lcpVal.push_back(getPpmBytestreamField_(8));
        lcpBcidVec.push_back(getPpmBytestreamField_(3));
      }

      for (int i = 0; i < numLut; ++i) {
        ljeVal.push_back(getPpmBytestreamField_(8));
        ljeSat80Vec.push_back(getPpmBytestreamField_(3));
      }

      for (int i = 0; i < numAdc; ++i) {
        adcVal.push_back(getPpmBytestreamField_(10));
        adcExt.push_back(getPpmBytestreamField_(1));
      }

      for (int i = 0; i < numLut; ++i) {
        uint16_t pc = getPpmBytestreamField_(10);
        pedCor.push_back(((((pc &(0x200))>>9)==1)?-1:+1) * (pc & 0x1fff));
        pedEn.push_back(getPpmBytestreamField_(1));
      }
    } catch (const std::out_of_range& ex) {
      ATH_MSG_ERROR("Failed to decode ppm block " << ex.what());
      return StatusCode::FAILURE;
    }
    CHECK(
        addTriggerTowerV2_(crate, module, chan, lcpVal, lcpBcidVec,
            ljeVal, ljeSat80Vec, adcVal, adcExt, pedCor, pedEn));
  }

  return StatusCode::SUCCESS;
}

StatusCode L1CaloByteStreamReadTool::processPpmStandardR3V1_() {
    for(auto lut : m_ppLuts) {
      CHECK(addTriggerTowerV1_(
        m_subBlockHeader.crate(), 
        m_subBlockHeader.module(),
        lut.first,
        lut.second,
        m_ppFadcs[lut.first]));;
    }
    return StatusCode::SUCCESS;
}

StatusCode L1CaloByteStreamReadTool::addTriggerTowerV2_(
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

StatusCode L1CaloByteStreamReadTool::addTriggerTowerV1_(
    uint8_t crate,
    uint8_t module,
    uint8_t channel,
    const std::vector<uint8_t>& luts,
    const std::vector<uint8_t>& lcpBcidVec,
    const std::vector<uint16_t>& fadc,
    const std::vector<uint8_t>& bcidExt
  ) {

    std::vector<uint8_t> ljeVal;
    std::vector<uint8_t> ljeSat80Vec;

    std::vector<int16_t> pedCor;
    std::vector<uint8_t> pedEn;

   CHECK(addTriggerTowerV2_(crate, module, channel, luts, lcpBcidVec,
            ljeVal, ljeSat80Vec, fadc, bcidExt, pedCor, pedEn));

   return StatusCode::SUCCESS;
}

StatusCode L1CaloByteStreamReadTool::addTriggerTowerV1_(
    uint8_t crate,
    uint8_t module,
    uint8_t channel,
    const std::vector<uint16_t>& luts,
    const std::vector<uint16_t>& fadc
  ) {

    std::vector<uint8_t> lcpVal;
    std::vector<uint8_t> lcpBcidVec;

    std::vector<uint16_t> adcVal;
    std::vector<uint8_t> adcExt;

    for(auto lut: luts) {
      lcpVal.push_back(BitField::get<uint8_t>(lut, 0, 8));
      lcpBcidVec.push_back(BitField::get<uint8_t>(lut, 8, 3));
    }

    for(auto f: fadc) {
      adcExt.push_back(BitField::get<uint8_t>(f, 0, 1));
      adcVal.push_back(BitField::get<uint8_t>(f, 1, 10));
    }

   CHECK(addTriggerTowerV1_(crate, module, channel, lcpVal, lcpBcidVec,
            adcVal, adcExt));

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
} // end namespace
// ===========================================================================