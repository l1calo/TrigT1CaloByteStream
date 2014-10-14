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
#include "xAODTrigL1Calo/TriggerTowerAuxContainer.h"
#include "xAODTrigL1Calo/TriggerTowerContainer.h"

#include "TrigT1CaloUtils/DataError.h"

#include "L1CaloSrcIdMap.h"
#include "TrigT1CaloMappingToolInterfaces/IL1CaloMappingTool.h"
#include "L1CaloErrorByteStreamTool.h"
#include "PpmSubBlock.h"
#include "CmmSubBlock.h"
#include "L1CaloUserHeader.h"
// ===========================================================================
#include "PpmByteStreamV2Tool.h"
// ===========================================================================
namespace LVL1BS {
// ===========================================================================

// Interface ID
static const InterfaceID IID_IPpmByteStreamV2Tool("PpmByteStreamV2Tool", 1, 1);

const InterfaceID& PpmByteStreamV2Tool::interfaceID() {
  return IID_IPpmByteStreamV2Tool;
}
// ===========================================================================
// Constructor
PpmByteStreamV2Tool::PpmByteStreamV2Tool(const std::string& type,
    const std::string& name, const IInterface* parent) :
    AthAlgTool(type, name, parent),
        m_sms("SegMemSvc/SegMemSvc", name),
        m_srcIdMap(0),
        m_ppmMaps("LVL1::PpmMappingTool/PpmMappingTool"),
        m_errorTool("LVL1BS::L1CaloErrorByteStreamTool/L1CaloErrorByteStreamTool"),
        m_subDetector(eformat::TDAQ_CALO_PREPROC),
        m_channelsType(ChannelsType::Data) {
  
  declareInterface<PpmByteStreamV2Tool>(this);

  declareProperty("PpmMappingTool", m_ppmMaps,
                  "Crate/Module/Channel to Eta/Phi/Layer mapping tool");

  declareProperty("NCrates", m_crates = 8, "Number of crates");
  declareProperty("NModules", m_modules = 16, "Number of modules");
  declareProperty("NChannels", m_channels = 64, "Number of channels");
  declareProperty("DataSize", m_dataSize = 3584, "Data size");
  declareProperty("MaxSlinks", m_maxSlinks = 4, "Max slinks");

  declareProperty("PrintCompStats",     m_printCompStats  = 0,
                  "Print compressed format statistics");
  declareProperty("FADCBaseline", m_fadcBaseline = 0,
                  "FADC baseline lower bound for compressed formats");

}
// ===========================================================================
// Destructor
PpmByteStreamV2Tool::~PpmByteStreamV2Tool() {
}
// ===========================================================================
// Initialize

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

StatusCode PpmByteStreamV2Tool::initialize() {
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

StatusCode PpmByteStreamV2Tool::finalize() {
  delete m_srcIdMap;
  return StatusCode::SUCCESS;
}
// ===========================================================================
void PpmByteStreamV2Tool::reserveMemory() {
  const int maxChannels = m_crates * m_modules * m_channels;
  const int chanBitVecSize = maxChannels/32;
  const int modBitVecSize = (m_crates * m_modules)/32;
  
  // TriggerTowerMap ttMap;
  // TriggerTowerMap::iterator itt;

  int dataCount = 0;

  if (m_ttData.empty()) {
     m_ttData.reserve(m_dataSize);
     
     m_dataChan.resize(chanBitVecSize);
     m_chanLayer.resize(chanBitVecSize);
     m_dataMod.resize(modBitVecSize);
     m_ttPos.resize(maxChannels);

     for (int crate = 0; crate < m_crates; ++crate) {
        for (int module = 0; module < m_modules; ++module) {
          const int index2 = (crate << 4) + module;
          const int word2  = index2 / 32;
          const int bit2   = index2 % 32;
          for (int channel = 0; channel < m_channels; ++channel) {
            const int index = (crate << 10) + (module << 6) + channel;
            const int word  = index / 32;
            const int bit   = index % 32;
            double eta = 0.;
            double phi = 0.;
            int layer = 0;
            // unsigned int key = 0;
            if (m_ppmMaps->mapping(crate, module, channel, eta, phi, layer)) {
              xAOD::TriggerTowerAuxContainer* aux = new xAOD::TriggerTowerAuxContainer();
              xAOD::TriggerTower* tt =  new (m_sms->allocate<xAOD::TriggerTower>(SegMemSvc::JOB))xAOD::TriggerTower();
              tt->setStore(aux);


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
              const std::vector<uint_least32_t> dummy_vector32 {0};
              const std::vector<int_least16_t> dummy_svector16 {0};
              const std::vector<uint_least16_t> dummy_vector16 {0};
              const std::vector<uint_least8_t> dummy_vector8 {0};
              tt->initialize(
                0,
                0,
                eta,
                phi,
                dummy_vector8,
                dummy_vector8,
                dummy_svector16,
                dummy_vector8,
                dummy_vector8,
                dummy_vector16,
                dummy_vector8,
                0,
                0,
                0
              );


              m_ttData.push_back(tt);
              m_ttPos[index] = dataCount++;
              // ttMap.insert(std::make_pair(key,count));
              m_chanLayer[word] |= (layer << bit);
              m_dataChan[word]  |= (1 << bit);
              m_dataMod[word2]  |= (1 << bit2);
            }
          }
        }
      }
  } 
}

void PpmByteStreamV2Tool::collectTriggerTowers(
  const IROBDataProviderSvc::VROBFRAG& robFrags
) 
{
  const bool debug   = msgLvl(MSG::DEBUG);
  const bool verbose = msgLvl(MSG::VERBOSE);
  if (debug) msg(MSG::DEBUG);

  TriggerTowerVector& ttColRef = m_ttData;
  ChannelBitVector& colChan = m_dataChan;
  ChannelBitVector& colMod = m_dataMod;

  const int maxChannels = m_crates * m_modules * m_channels;
  const int chanBitVecSize = maxChannels/32;
  m_foundChan.assign(chanBitVecSize, 0);

  int ttCount = 0;
  
  // Vectors to unpack into
  std::vector<uint_least8_t> lutCp;
  std::vector<uint_least8_t> lutJep;
  std::vector<uint_least16_t> fadc;
  
  std::vector<uint_least8_t> bcidLutCp;
  std::vector<uint_least8_t> satLutJep;
  std::vector<uint_least8_t> bcidFadc;
  std::vector<int_least16_t> correction;
  std::vector<uint_least8_t> correctionEnabled;
  
  // =========================================================================
  // Loop over ROB fragments
  // =========================================================================
  int robCount = 0;
  std::set<uint32_t> dupCheck;
  ROBIterator rob    = robFrags.begin();
  ROBIterator robEnd = robFrags.end();  

  for (; rob != robEnd; ++rob) {
    msg(MSG::DEBUG) << "Treating ROB fragment " << robCount << endreq;
    ++robCount;
    
    // Skip fragments with ROB status errors
    uint32_t robid = (*rob)->source_id();

    if ((*rob)->nstatus() > 0) {
      ROBPointer robData;
      (*rob)->status(robData);
      if (*robData != 0) {
        m_errorTool->robError(robid, *robData);
        if (debug) {
          msg(MSG::WARNING) << "ROB status error - skipping fragment" << endreq;
        }
        continue;
      }
    }
    // -----------------------------------------------------------------------
    // Unpack ROD data (slinks)
    // -----------------------------------------------------------------------
    RODPointer payloadBeg;
    RODPointer payload;
    RODPointer payloadEnd;
    (*rob)->rod_data(payloadBeg);
    payloadEnd = payloadBeg + (*rob)->rod_ndata();
    payload = payloadBeg;
    
    if (payload == payloadEnd) {
      msg(MSG::DEBUG) << "ROB fragment empty" << endreq;
      continue;
    } 
    // -----------------------------------------------------------------------    
    const uint32_t sourceID = (*rob)->rod_source_id();
    const int rodCrate = m_srcIdMap->crate(sourceID);
    const int rodSlink = m_srcIdMap->slink(sourceID);
    // -----------------------------------------------------------------------
    // Check identifier
    // Sasha: why we need this check: ROD == ROB
    // -----------------------------------------------------------------------
    // if (
    //     m_srcIdMap->getRobID(sourceID) != robid         ||
    //     m_srcIdMap->subDet(sourceID)   != m_subDetector ||
    //     m_srcIdMap->daqOrRoi(sourceID) != 0             ||
    //     rodSlink    >= m_maxSlinks   ||
    //     rodCrate    >= m_crates) {
    //       m_errorTool->rodError(robid, L1CaloSubBlock::ERROR_ROD_ID);
    //     if (debug) {
    //       msg(MSG::DEBUG) << "Wrong source identifier in data: ROD "
    //       << MSG::hex << sourceID << "  ROB " << robid
    //       << MSG::dec << endreq;
    //     }
    //     continue;
    // }
    // Skip duplicate fragments
    if (!dupCheck.insert(robid).second) {
      m_errorTool->rodError(robid, L1CaloSubBlock::ERROR_DUPLICATE_ROB);
      if (debug) {
          msg(MSG::DEBUG) << "Skipping duplicate ROB fragment" << endreq;
      }
      continue;
    }
    // -----------------------------------------------------------------------
    // Check minor version
    // -----------------------------------------------------------------------
    const int minorVersion = (*rob)->rod_version() & 0xffff;
    // if (minorVersion > m_srcIdMap->minorVersionPreLS1()) {
    //   if (debug) msg() << "Skipping post-LS1 data" << endreq;
    //   continue;
    // }
    // -----------------------------------------------------------------------

    if (debug) {
      msg(MSG::DEBUG) << "Treating crate " << rodCrate 
            << " slink " << rodSlink << endreq;
    }

    // First word should be User Header
    if ( !L1CaloUserHeader::isValid(*payload) ) {
      m_errorTool->rodError(robid, L1CaloSubBlock::ERROR_USER_HEADER);
      if (debug) {
          msg(MSG::DEBUG) << "Invalid or missing user header" << endreq;
      }
      continue;
    }

    L1CaloUserHeader userHeader(*payload);
    userHeader.setVersion(minorVersion);

    ++payload; // Skip word

    // triggered slice offsets
    const uint_least8_t trigLut  = userHeader.ppmLut();
    const uint_least8_t trigFadc = userHeader.ppmFadc();
    // FADC baseline lower bound
    m_fadcBaseline = userHeader.lowerBound();

    if (debug) {
      msg(MSG::DEBUG) << "Minor format version number: "
            << MSG::hex << minorVersion << MSG::dec              << endreq
            << "LUT triggered slice offset:  " << int(trigLut)        << endreq
            << "FADC triggered slice offset: " << int(trigFadc)       << endreq
            << "FADC baseline lower bound:   " << m_fadcBaseline << endreq;
    }

    const int runNumber = (*rob)->rod_run_no() & 0xffffff;

    // --------------------------------------------------------------------
    int chanPerSubBlock = 0;
    bool firstBlock = false; // Sasha: not used. delete?
    uint32_t firstWord = 0;  // Sasha: not used. delete?
    RODPointer payloadFirst;
    if (payload != payloadEnd) {
      // --------------------------------------------------------------------
      if (
            L1CaloSubBlock::wordType(*payload) != L1CaloSubBlock::HEADER ||
            CmmSubBlock::cmmBlock(*payload)
         ) {
              m_errorTool->rodError(robid, L1CaloSubBlock::ERROR_MISSING_HEADER);
              if (debug) {
                  msg(MSG::DEBUG) << "Missing Sub-block header" << endreq;
                }
              continue;
      }
      // --------------------------------------------------------------------
      firstBlock = true;
      firstWord = *payload;
      // --------------------------------------------------------------------
      // Sasha: Check first sublock for errors?
      // --------------------------------------------------------------------
      if (m_ppmBlocks.empty()) {
        m_ppmBlocks.push_back(new PpmSubBlock());
      }
      // --------------------------------------------------------------------
      PpmSubBlock* const subBlock = m_ppmBlocks[0];
      subBlock->clear();
      payloadFirst = subBlock->read(payload, payloadEnd);

      chanPerSubBlock = subBlock->channelsPerSubBlock();
      if (chanPerSubBlock == 0) {
        m_errorTool->rodError(robid, subBlock->unpackErrorCode());
        if (debug) {
            msg(MSG::DEBUG) << "Unsupported version/data format: "
                         << subBlock->version() << "/"
                         << subBlock->format()  << endreq;
        }
        continue;
      }
      
      if (debug) {
          msg(MSG::DEBUG) << "Channels per sub-block: "
                       << chanPerSubBlock << endreq;
      } 
    } else {
      if (debug) {
        msg() << "ROB fragment contains user header only" << endreq;
      }
      continue;
    }

    const int numSubBlocks = m_channels/chanPerSubBlock;
    const int size = m_ppmBlocks.size();
    if (numSubBlocks > size) {
      // Sasha: Insert empty subblocks?
      for (int i = size; i < numSubBlocks; ++i) {
        m_ppmBlocks.push_back(new PpmSubBlock());
      }
    }
    // -----------------------------------------------------------------------
    // Loop over PPMs
    // -----------------------------------------------------------------------
    payload = payloadBeg;
    ++payload;
    unsigned int rodErr = L1CaloSubBlock::ERROR_NONE;
    // -----------------------------------------------------------------------
    // Sasha: Why we separetly check first block? Rewrite!
    // -----------------------------------------------------------------------
    while (payload != payloadEnd) {

      // Get all sub-blocks for one PPM (first already read in above)
      int crate = 0;
      int module = 0;
      int nPpmBlocks = 0;
      for (int block = 0; block < numSubBlocks; ++block) {
        const uint32_t word = (firstBlock) ? firstWord : *payload;
        if ( L1CaloSubBlock::wordType(word) != L1CaloSubBlock::HEADER ||
             CmmSubBlock::cmmBlock(word) ||
             PpmSubBlock::errorBlock(word)) {
          
          if (debug) {
            msg(MSG::DEBUG) << "Unexpected data sequence" << endreq;
          }
          rodErr = L1CaloSubBlock::ERROR_MISSING_HEADER;
          break;
        }
        
        if ( chanPerSubBlock != m_channels && 
             L1CaloSubBlock::seqno(word) != block * chanPerSubBlock) {
          if (debug) {
            msg(MSG::DEBUG) << "Unexpected channel sequence number: "
            << L1CaloSubBlock::seqno(word) << " expected " 
            << block * chanPerSubBlock << endreq;
          }
          rodErr = L1CaloSubBlock::ERROR_MISSING_SUBBLOCK;
          break;
        }
        
        PpmSubBlock* const subBlock = m_ppmBlocks[block];
        nPpmBlocks++;
        
        if (firstBlock) {
          payload = payloadFirst;
          firstBlock = false;
        } else {
          subBlock->clear();
          payload = subBlock->read(payload, payloadEnd);
        }
        
        if (block == 0) {
          crate = subBlock->crate();
          module = subBlock->module();
          if (debug) {
            msg(MSG::DEBUG) << "Crate " << crate << "  Module " << module << endreq;
          }
          if (crate != rodCrate) {
            if (debug) {
              msg() << "Inconsistent crate number in ROD source ID" << endreq;
            }
            rodErr = L1CaloSubBlock::ERROR_CRATE_NUMBER;
            break;
          }
        } else {
            if (subBlock->crate() != crate) {
              if (debug) {
                msg(MSG::DEBUG) << "Inconsistent crate number in sub-blocks"
                       << endreq;
              }
              rodErr = L1CaloSubBlock::ERROR_CRATE_NUMBER;
              break;
            }
            if (subBlock->module() != module) {
              if (debug) {
                  msg(MSG::DEBUG) << "Inconsistent module number in sub-blocks"
                       << endreq;
              }
              rodErr = L1CaloSubBlock::ERROR_MODULE_NUMBER;
              break;
            }
        }
        
        if (payload == payloadEnd && block != numSubBlocks - 1) {
          if (debug) {
            msg(MSG::DEBUG) << "Premature end of data" << endreq;
          }
          rodErr = L1CaloSubBlock::ERROR_MISSING_SUBBLOCK;
          break;
        }

      } // for

      if (rodErr != L1CaloSubBlock::ERROR_NONE) break;
      // Is there an error block?
      bool isErrBlock = false;
      if (payload != payloadEnd) {
        if (
            L1CaloSubBlock::wordType(*payload) == L1CaloSubBlock::HEADER && 
            !CmmSubBlock::cmmBlock(*payload) && 
            PpmSubBlock::errorBlock(*payload)) {
          if (debug) {
            msg(MSG::DEBUG) << "Error block found" << endreq;
          }
          if (!m_errorBlock) {
            m_errorBlock = new PpmSubBlock();
          } else {
            m_errorBlock->clear();
          }
          isErrBlock = true;
          payload = m_errorBlock->read(payload, payloadEnd);
          if (m_errorBlock->crate() != crate) {
            if (debug) {
              msg(MSG::DEBUG) << "Inconsistent crate number in error block"
                       << endreq;
            }
            rodErr = L1CaloSubBlock::ERROR_CRATE_NUMBER;
            break;
          }
          if (m_errorBlock->module() != module) {
            if (debug) {
              msg(MSG::DEBUG) << "Inconsistent module number in error block"
                       << endreq;
            }
            rodErr = L1CaloSubBlock::ERROR_MODULE_NUMBER;
            break;
          }
          if (m_errorBlock->dataWords() && !m_errorBlock->unpack()) {
            if (debug) {
              std::string errMsg(m_errorBlock->unpackErrorMsg());
              msg(MSG::DEBUG) << "Unpacking error block failed: " << errMsg << endreq;
            }
            rodErr = m_errorBlock->unpackErrorCode();
            break;
          }
        }
      }

      // Don't bother unpacking modules that aren't used for required collection

      const int index2 = (crate << 4) + module;
      const int word2  = index2 / 32;
      const int bit2   = index2 % 32;
      if (!((colMod[word2] >> bit2) & 1)) continue;

      // Loop over sub-blocks and fill trigger towers

      for (int block = 0; block < nPpmBlocks; ++block) {
        PpmSubBlock* const subBlock = m_ppmBlocks[block];
        subBlock->setLutOffset(trigLut);
        subBlock->setFadcOffset(trigFadc);
        subBlock->setPedestal(m_pedestal);
        subBlock->setFadcBaseline(m_fadcBaseline);
        subBlock->setRunNumber(runNumber);
        subBlock->setRodVersion(minorVersion);
        
        if (debug) {
          msg(MSG::DEBUG) << "Unpacking sub-block version/format/seqno: "
          << subBlock->version() << "/" << subBlock->format() << "/"
          << subBlock->seqno() << endreq;
        }
        if (subBlock->dataWords() && !subBlock->unpack()) {
          if (debug) {
            std::string errMsg(subBlock->unpackErrorMsg());
            msg(MSG::DEBUG) << "Unpacking PPM sub-block failed: " << errMsg << endreq;
          }
          rodErr = subBlock->unpackErrorCode();
          break;
        }
        if (m_printCompStats) {
          addCompStats(subBlock->compStats());
        }
        
        for (int chan = 0; chan < chanPerSubBlock; ++chan) {
          const int channel = block*chanPerSubBlock + chan;
          const int index = (crate << 10) + (module << 6) + channel;
          const int word  = index / 32;
          const int bit   = index %   32;
        
          if ( !((colChan[word] >> bit) & 1)) continue; // skip unwanted channels
          if (((m_foundChan[word] >> bit) & 1)) {
            if (debug) {
                msg(MSG::DEBUG) << "Duplicate data for crate/module/channel: "
                           << crate << "/" << module << "/" << channel
               << endreq;
            }
            rodErr = L1CaloSubBlock::ERROR_DUPLICATE_DATA;
            break;
          }
         // Get data
          subBlock->ppmData(channel, lutCp, lutJep, fadc, bcidLutCp, satLutJep,
            bcidFadc, correction, correctionEnabled);
          
          if (lutCp.size() < size_t(trigLut + 1)) {
            if (debug) {
              msg(MSG::DEBUG) << "Triggered LUT slice from header "
                    << "inconsistent with number of slices: "
              << trigLut << ", " << lutCp.size() << endreq;
            }
            rodErr = L1CaloSubBlock::ERROR_SLICES;
            break;
          }
      
          if (fadc.size() < size_t(trigFadc + 1)) {
            if (debug) {
              msg(MSG::DEBUG) << "Triggered FADC slice from header "
                    << "inconsistent with number of slices: "
              << trigFadc << ", " << fadc.size() << endreq;
            }
            rodErr = L1CaloSubBlock::ERROR_SLICES;
            break;
          }
          
          LVL1::DataError errorBits(0);
          if (isErrBlock) {
            errorBits.set(LVL1::DataError::PPMErrorWord,
                                     m_errorBlock->ppmError(channel));
            errorBits.set(LVL1::DataError::SubStatusWord,
                                     m_errorBlock->subStatus());
          } else {
              errorBits.set(LVL1::DataError::PPMErrorWord,
                                       subBlock->ppmError(channel));
              const PpmSubBlock* const lastBlock =
                                                m_ppmBlocks[nPpmBlocks - 1];
              errorBits.set(LVL1::DataError::SubStatusWord,
                                       lastBlock->subStatus());
          }
          // Wrong bit set for compressed formats 1.01 to 1.03
          if (subBlock->format() > 1 && subBlock->seqno() < 4) {
            errorBits.set(LVL1::DataError::ModuleError,
                 (errorBits.error() >> (LVL1::DataError::ModuleError+1)) & 0x1);
          }
          
          const int error = errorBits.error();

        // Save to TriggerTower

          if (verbose) {
            msg(MSG::VERBOSE) << "|channel/LUTCP/LUTJEP/FADC/bcidLUTCP/satLUTJEP/bcidFADC/correction/correctionEnabled/error: "
                              << channel << "/";
            printVec(lutCp);
            printVec(lutJep);
            printVec(fadc);
            
            printVec(bcidLutCp);
            printVec(satLutJep);
            printVec(bcidFadc);
            
            printVec(correction);
            printVec(correctionEnabled);
            
            msg(MSG::VERBOSE) << MSG::hex << error << MSG::dec << "|";
          }
          
          m_foundChan[word] |= (1 << bit);
          ++ttCount;
          xAOD::TriggerTower* tt = ttColRef[m_ttPos[index]];
          
          const int layer = ((m_chanLayer[word] >> bit) & 1);
          // =================================================================
          // Update Trigger Towers objects
          // =================================================================
          tt->setCoolId(coolId(crate, module, channel));
          tt->setLayer(layer);
          tt->setLut_cp(lutCp);
          tt->setLut_jep(lutJep);
          tt->setAdc(fadc);
          
          tt->setBcidVec(bcidLutCp);
          tt->setBcidExt(bcidFadc);

          tt->setCorrection(correction);
          tt->setCorrectionEnabled(correctionEnabled);
          
          tt->setPeak(trigLut);
          tt->setAdcPeak(trigFadc);
          // =================================================================

        } // for chan
        if (rodErr != L1CaloSubBlock::ERROR_NONE) break;
      } // for block
      
      if (rodErr != L1CaloSubBlock::ERROR_NONE) break;
    } //while
    
    if (rodErr != L1CaloSubBlock::ERROR_NONE) {
      m_errorTool->rodError(robid, rodErr);
    }

    // TODO: (sasha) Reset any missing channels (should be rare)
    // -----------------------------------------------------------------------
    // Reset any missing channels (should be rare)
    // -----------------------------------------------------------------------
    // if (ttCount != colSize) {
    //   if (debug) {
    //     msg(MSG::DEBUG) << "Found " << ttCount << " channels, expected " << colSize << endreq;
    //   }
      
    //   std::vector<int> dummy(1);
    //   for (int word = 0; word < chanBitVecSize; ++word) {
    //     if (m_foundChan[word] != colChan[word]) {
    //       for (int bit = 0; bit < 32; ++bit) {
    //         if (((m_foundChan[word]>>bit)&1) != ((colChan[word]>>bit)&1)) {
    //           const int index = word*32 + bit;
    //           xAOD::TriggerTower* tt = ttColRef[m_ttPos[index]];
    //           const int layer = ((m_chanLayer[word] >> bit) & 1);
    //           tt->setLayer(layer);
    //           tt->setAdc(fadc);
    //           tt->setPeak(trigLut);
    //           tt->setAdcPeak(trigFadc);

    //         } // if
    //       } // for
    //     } // if
    //   }  // for
    // } // if   

    // ---------------------------------------------------------------------
  } // for

}

uint_least32_t PpmByteStreamV2Tool::coolId(int crate, int module, 
  int channel) const {
  const int pin  = channel % 16;
  const int asic = channel / 16;
  return (crate << 24) | (1 << 20) | (module << 16) | (pin << 8) | asic;
}

// Conversion bytestream to trigger towers
StatusCode PpmByteStreamV2Tool::convert(
    const IROBDataProviderSvc::VROBFRAG& robFrags,
    xAOD::TriggerTowerContainer* const ttCollection) {

  reserveMemory();
  collectTriggerTowers(robFrags);

  for (auto* tt: m_ttData) {
    ttCollection->push_back(tt);
  }



  return StatusCode::SUCCESS;
}
// ===========================================================================
// Conversion of trigger towers to bytestream

StatusCode PpmByteStreamV2Tool::convert(
    const xAOD::TriggerTowerContainer* const /*ttCollection*/,
    RawEventWrite* const /*re*/) {

  // TODO: (sasha) implement PpmByteStreamV2Tool::convert
  return StatusCode::FAILURE;
}
// ===========================================================================

// Return reference to vector with all possible Source Identifiers

const std::vector<uint32_t>& PpmByteStreamV2Tool::sourceIDs(
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

// Add compression stats to totals
void PpmByteStreamV2Tool::addCompStats(const std::vector<uint32_t>& stats)
{
  if (stats.empty()) return;
  const int n = stats.size();
  if (m_compStats.empty()) m_compStats.resize(n);
  for (int i = 0; i < n; ++i) m_compStats[i] += stats[i];
}

// Print compression stats
void PpmByteStreamV2Tool::printCompStats() const
{
  msg() << "Compression stats format/count: ";
  const int n = m_compStats.size();
  for (int i = 0; i < n; ++i) {
    msg() << " " << i << "/" << m_compStats[i];
  }
  msg() << endreq;
}

// Print a vector

// template <type
// void PpmByteStreamV2Tool::printVec(const std::vector<int>& vec) const
// {
//   std::vector<int>::const_iterator pos;
//   for (pos = vec.begin(); pos != vec.end(); ++pos) {
//     if (pos != vec.begin()) msg() << ",";
//     msg() << *pos;
//   }
//   msg() << "/";
// }
// ===========================================================================
}// end namespace
