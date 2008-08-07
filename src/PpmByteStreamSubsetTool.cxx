
#include <numeric>

#include "GaudiKernel/IInterface.h"

#include "TrigT1Calo/DataError.h"
#include "TrigT1Calo/TriggerTower.h"
#include "TrigT1Calo/TriggerTowerKey.h"

#include "TrigT1CaloByteStream/ChannelCoordinate.h"
#include "TrigT1CaloByteStream/L1CaloSrcIdMap.h"
#include "TrigT1CaloByteStream/L1CaloSubBlock.h"
#include "TrigT1CaloByteStream/L1CaloUserHeader.h"
#include "TrigT1CaloByteStream/PpmByteStreamSubsetTool.h"
#include "TrigT1CaloByteStream/PpmSubBlock.h"

namespace LVL1BS {

// Constructor

PpmByteStreamSubsetTool::PpmByteStreamSubsetTool(const std::string& type,
               const std::string& name, const IInterface*  parent)
             : AlgTool(type, name, parent),
	       m_ppmMaps("LVL1BS::PpmCrateMappingTool/PpmCrateMappingTool"),
	       m_log(msgSvc(), name),
	       m_channels(64), m_crates(8), m_modules(16),
	       m_subDetector(eformat::TDAQ_CALO_PREPROC),
	       m_srcIdMap(0), m_towerKey(0), m_errorBlock(0)
{
  declareInterface<IPpmByteStreamSubsetTool>(this);

  declareProperty("ZeroSuppress", m_zeroSuppress = true,
                  "Only make trigger towers with non-zero EM or Had energy");
}

// Destructor

PpmByteStreamSubsetTool::~PpmByteStreamSubsetTool()
{
}

// Initialize

StatusCode PpmByteStreamSubsetTool::initialize()
{
  m_log.setLevel(outputLevel());
  m_debug = outputLevel() <= MSG::DEBUG;

  StatusCode sc = AlgTool::initialize();
  if (sc.isFailure()) {
    m_log << MSG::ERROR << "Problem initializing AlgTool " <<  endreq;
    return sc;
  }

  // Retrieve mapping tool

  sc = m_ppmMaps.retrieve();
  if ( sc.isFailure() ) {
    m_log << MSG::ERROR << "Couldn't retrieve PpmCrateMappingTool" << endreq;
    return sc;
  }

  m_srcIdMap = new L1CaloSrcIdMap();
  m_towerKey = new LVL1::TriggerTowerKey();

  m_log << MSG::INFO << "Initialization completed" << endreq;

  return StatusCode::SUCCESS;
}

// Finalize

StatusCode PpmByteStreamSubsetTool::finalize()
{
  delete m_errorBlock;
  delete m_towerKey;
  delete m_srcIdMap;
  return AlgTool::finalize();
}

// Conversion bytestream to trigger towers

StatusCode PpmByteStreamSubsetTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::TriggerTower>* const ttCollection,
			    const std::vector<unsigned int> chanIds)
{

  // Index wanted channels by crate/module

  ChannelMap chanMap;
  std::vector<unsigned int>::const_iterator pos  = chanIds.begin();
  std::vector<unsigned int>::const_iterator posE = chanIds.end();
  std::vector<unsigned int>::const_iterator posB = pos;
  unsigned int lastKey = (pos != posE) ? (*pos)/64 : 9999;
  for (; pos != posE; ++pos) {
    unsigned int key = (*pos)/64;
    if (key != lastKey) {
      if (m_debug) {
        m_log << MSG::DEBUG << "Adding key to index: " << lastKey << endreq;
      }
      chanMap.insert(std::make_pair(lastKey, std::make_pair(posB, pos)));
      lastKey = key;
      posB = pos;
    }
  }
  if (lastKey != 9999) {
    if (m_debug) {
      m_log << MSG::DEBUG << "Adding key to index: " << lastKey << endreq;
    }
    chanMap.insert(std::make_pair(lastKey, std::make_pair(posB, posE)));
  }
  if (m_debug) {
    m_log << MSG::DEBUG << "Number of channels wanted: " << chanIds.size()
                        << "  Channel map size: " << chanMap.size() << endreq;
  }

  // Clear trigger tower map

  m_ttMap.clear();
  TriggerTowerCollection ttTemp;

  // Loop over ROB fragments

  int robCount = 0;
  ROBIterator rob    = robFrags.begin();
  ROBIterator robEnd = robFrags.end();
  for (; rob != robEnd; ++rob) {

    if (m_debug) {
      m_log << MSG::DEBUG << "Treating ROB fragment " << ++robCount << endreq;
    }

    // Unpack ROD data (slinks)

    RODPointer payloadBeg;
    RODPointer payload;
    RODPointer payloadEnd;
    (*rob)->rod_data(payloadBeg);
    payloadEnd = payloadBeg + (*rob)->rod_ndata();
    payload = payloadBeg;
    if (payload == payloadEnd) {
      if (m_debug) m_log << MSG::DEBUG << "ROB fragment empty" << endreq;
      continue;
    }

    // Check identifier
    const uint32_t sourceID = (*rob)->rod_source_id();
    if (m_debug) {
      if (m_srcIdMap->subDet(sourceID) != m_subDetector ||
          m_srcIdMap->daqOrRoi(sourceID) != 0) {
        m_log << MSG::DEBUG << "Wrong source identifier in data: "
	      << MSG::hex << sourceID << MSG::dec << endreq;
      }
    }
    const int rodCrate = m_srcIdMap->crate(sourceID);
    if (m_debug) {
      m_log << MSG::DEBUG << "Treating crate " << rodCrate 
                          << " slink " << m_srcIdMap->slink(sourceID)
	  		  << endreq;
    }

    // First word is User Header
    L1CaloUserHeader userHeader(*payload);
    const int minorVersion = (*rob)->rod_version() & 0xffff;
    userHeader.setVersion(minorVersion);
    const int headerWords = userHeader.words();
    if (headerWords != 1 && m_debug) {
      m_log << MSG::DEBUG << "Unexpected number of user header words: "
            << headerWords << endreq;
    }
    for (int i = 0; i < headerWords; ++i) ++payload;
    // triggered slice offsets
    const int trigLut  = userHeader.ppmLut();
    const int trigFadc = userHeader.ppmFadc();
    // FADC baseline lower bound
    const int fadcBaseline = userHeader.lowerBound();
    if (m_debug) {
      m_log << MSG::DEBUG << "Minor format version number: " << MSG::hex 
                          << minorVersion << MSG::dec << endreq;
      m_log << MSG::DEBUG << "LUT triggered slice offset:  " << trigLut
                          << endreq;
      m_log << MSG::DEBUG << "FADC triggered slice offset: " << trigFadc
                          << endreq;
      m_log << MSG::DEBUG << "FADC baseline lower bound:   " << fadcBaseline
                          << endreq;
    }

    // Find the number of channels per sub-block

    int chanPerSubBlock = 0;
    if (payload != payloadEnd) {
      if (L1CaloSubBlock::wordType(*payload) != L1CaloSubBlock::HEADER) {
        m_log << MSG::ERROR << "Missing Sub-block header" << endreq;
        return StatusCode::FAILURE;
      }
      PpmSubBlock testBlock;
      payload = testBlock.read(payload, payloadEnd);
      chanPerSubBlock = testBlock.channelsPerSubBlock();
      if (chanPerSubBlock == 0) {
        m_log << MSG::ERROR << "Unsupported version/data format: "
                            << testBlock.version() << "/"
                            << testBlock.format()  << endreq;
        return StatusCode::FAILURE;
      }
      if (m_channels%chanPerSubBlock != 0) {
        m_log << MSG::ERROR << "Invalid channels per sub-block: "
                            << chanPerSubBlock << endreq;
        return StatusCode::FAILURE;
      }
      if (m_debug) {
        m_log << MSG::DEBUG << "Channels per sub-block: "
                            << chanPerSubBlock << endreq;
      }
    } else {
      if (m_debug) m_log << MSG::DEBUG
                         << "ROB fragment contains user header only" << endreq;
      continue;
    }
    const int numSubBlocks = m_channels/chanPerSubBlock;

    // Loop over PPMs

    payload = payloadBeg;
    for (int i = 0; i < headerWords; ++i) ++payload;
    while (payload != payloadEnd) {

      // Get all sub-blocks for one PPM

      int crate = 0;
      int module = 0;
      m_ppmBlocks.clear();
      for (int block = 0; block < numSubBlocks; ++block) {
        if (L1CaloSubBlock::wordType(*payload) != L1CaloSubBlock::HEADER
	                           || PpmSubBlock::errorBlock(*payload)) {
          m_log << MSG::ERROR << "Unexpected data sequence" << endreq;
	  return StatusCode::FAILURE;
        }
        if (chanPerSubBlock != m_channels && 
	    L1CaloSubBlock::seqno(*payload) != block * chanPerSubBlock) {
	  if (m_debug) {
            m_log << MSG::DEBUG << "Unexpected channel sequence number: "
	          << L1CaloSubBlock::seqno(*payload) << " expected " 
	          << block * chanPerSubBlock << endreq;
	  }
	  if ( !m_ppmBlocks.empty()) break;
	  else return StatusCode::FAILURE;
        }
        PpmSubBlock* const subBlock = new PpmSubBlock();
        m_ppmBlocks.push_back(subBlock);
        payload = subBlock->read(payload, payloadEnd);
        if (block == 0) {
          crate  = subBlock->crate();
	  module = subBlock->module();
	  if (m_debug) {
	    m_log << MSG::DEBUG << "Module " << module << endreq;
	    if (crate != rodCrate) {
	      m_log << MSG::DEBUG
	            << "Inconsistent crate number in ROD source ID" << endreq;
	    }
          }
        } else {
          if (subBlock->crate() != crate) {
	    m_log << MSG::ERROR << "Inconsistent crate number in sub-blocks"
	          << endreq;
	    return StatusCode::FAILURE;
          }
          if (subBlock->module() != module) {
	    m_log << MSG::ERROR << "Inconsistent module number in sub-blocks"
	          << endreq;
	    return StatusCode::FAILURE;
          }
        }
        if (payload == payloadEnd && block != numSubBlocks - 1) {
          if (m_debug) m_log << MSG::DEBUG << "Premature end of data" << endreq;
	  break;
        }
      }

      // Is there an error block?

      delete m_errorBlock;
      m_errorBlock = 0;
      if (payload != payloadEnd) {
        if (L1CaloSubBlock::wordType(*payload) == L1CaloSubBlock::HEADER
	                            && PpmSubBlock::errorBlock(*payload)) {
	  if (m_debug) m_log << MSG::DEBUG << "Error block found" << endreq;
	  m_errorBlock = new PpmSubBlock();
	  payload = m_errorBlock->read(payload, payloadEnd);
          if (m_errorBlock->crate() != crate) {
	    m_log << MSG::ERROR << "Inconsistent crate number in error block"
	          << endreq;
	    return StatusCode::FAILURE;
          }
          if (m_errorBlock->module() != module) {
	    m_log << MSG::ERROR << "Inconsistent module number in error block"
	          << endreq;
	    return StatusCode::FAILURE;
          }
	  if (m_errorBlock->dataWords() && !m_errorBlock->unpack()) {
	    if (m_debug) m_log << MSG::DEBUG
	                       << "Unpacking error block failed" << endreq;
	  }
        }
      }

      // Loop over wanted channels and fill trigger towers

      const int actualSubBlocks = m_ppmBlocks.size();
      int lastBlock = -1;
      unsigned int key = crate*16 + module;
      ChannelMap::iterator mapIter = chanMap.find(key);
      if (mapIter == chanMap.end()) {
        if (m_debug) {
	  m_log << MSG::DEBUG << "Key not found: " << key << endreq;
	}
	continue;
      }
      IteratorPair ipair = mapIter->second;
      pos  = ipair.first;
      posE = ipair.second;
      PpmSubBlock* subBlock = 0;
      for (; pos != posE; ++pos) {
        const int channel = (*pos) % 64;
	const int block = channel/chanPerSubBlock;
	if (block >= actualSubBlocks) {
	  if (m_debug) {
	    m_log << MSG::DEBUG << "channel/block/actualSubBlocks: "
	          << channel << "/" << block << "/" << actualSubBlocks
		  << endreq;
	  }
	  break;
	}
	if (block != lastBlock) {
	  lastBlock = block;
          subBlock = m_ppmBlocks[block];
	  subBlock->setLutOffset(trigLut);
	  subBlock->setFadcOffset(trigFadc);
	  subBlock->setFadcBaseline(fadcBaseline);
          if (subBlock->dataWords() && !subBlock->unpack()) {
	    if (m_debug) m_log << MSG::DEBUG
	                       << "Unpacking PPM sub-block failed" << endreq;
          }
	}
  	std::vector<int> lut;
	std::vector<int> fadc;
	std::vector<int> bcidLut;
	std::vector<int> bcidFadc;
	subBlock->ppmData(channel, lut, fadc, bcidLut, bcidFadc);
	int trigLutKeep  = trigLut;
	int trigFadcKeep = trigFadc;
	if (lut.size() < size_t(trigLut + 1)) {
	  if (m_debug) {
	    m_log << MSG::DEBUG << "Triggered LUT slice from header "
	          << "inconsistent with number of slices: "
	          << trigLut << ", " << lut.size() << ", reset to 0" << endreq;
	  }
	  trigLutKeep = 0;
        }
	if (fadc.size() < size_t(trigFadc + 1)) {
	  if (m_debug) {
	    m_log << MSG::DEBUG << "Triggered FADC slice from header "
	          << "inconsistent with number of slices: "
	          << trigFadc << ", " << fadc.size() << ", reset to 0"
		  << endreq;
	  }
	  trigFadcKeep = 0;
        }
	LVL1::DataError errorBits(0);
	if (m_errorBlock) {
	  errorBits.set(LVL1::DataError::PPMErrorWord,
	                           m_errorBlock->ppmError(channel));
	  errorBits.set(LVL1::DataError::SubStatusWord,
	                           m_errorBlock->subStatus());
        } else {
	  errorBits.set(LVL1::DataError::PPMErrorWord,
	                           subBlock->ppmError(channel));
	  const PpmSubBlock* const lastBlock =
	                                    m_ppmBlocks[actualSubBlocks - 1];
	  errorBits.set(LVL1::DataError::SubStatusWord,
	                           lastBlock->subStatus());
        }
	const int error = errorBits.error();

	// Only save non-zero data

        const bool any =
	             std::accumulate(lut.begin(),      lut.end(),      0) ||
	             std::accumulate(fadc.begin(),     fadc.end(),     0) ||
		     std::accumulate(bcidLut.begin(),  bcidLut.end(),  0) ||
		     std::accumulate(bcidFadc.begin(), bcidFadc.end(), 0);

        if (any || error) {
	  if (m_debug) {
	    m_log << MSG::VERBOSE
	          << "channel/LUT/FADC/bcidLUT/bcidFADC/error: "
	          << channel << "/";
	    printVec(lut,      MSG::VERBOSE);
	    printVec(fadc,     MSG::VERBOSE);
	    printVec(bcidLut,  MSG::VERBOSE);
	    printVec(bcidFadc, MSG::VERBOSE);
	    m_log << MSG::VERBOSE << MSG::hex << error << MSG::dec << "/";
          }
	  ChannelCoordinate coord;
	  if (!m_ppmMaps->mapping(crate, module, channel, coord)) {
	    if (m_debug && any) {
	      m_log << MSG::VERBOSE << " Unexpected non-zero data words"
	            << endreq;
	    }
            continue;
          }
	  const ChannelCoordinate::CaloLayer layer = coord.layer();
	  const double phi = coord.phi();
	  const double eta = etaSim(coord);
	  if (m_debug) {
	    m_log << MSG::VERBOSE << " eta/etaSim/phi/layer: " << coord.eta()
	          << "/" << eta << "/" << phi << "/" << coord.layerName(layer)
	          << "/" << endreq;
          }
	  LVL1::TriggerTower* tt = findTriggerTower(eta, phi);
          if ( ! tt) {
	    // make new tower and add to map
	    const unsigned int key = m_towerKey->ttKey(phi, eta);
	    tt = new LVL1::TriggerTower(phi, eta, key);
            m_ttMap.insert(std::make_pair(key, tt));
	    ttTemp.push_back(tt);
          }
          if (layer == ChannelCoordinate::EM) {  // EM
	    tt->addEM(fadc, lut, bcidFadc, bcidLut, error,
	                                         trigLutKeep, trigFadcKeep);
          } else {                               // Had
	    tt->addHad(fadc, lut, bcidFadc, bcidLut, error,
	                                         trigLutKeep, trigFadcKeep);
          }
        }
      }
    }
  }

  // Swap wanted trigger towers into final container

  const int size = ttTemp.size();
  for (int index = 0; index < size; ++index) {
    if ( !m_zeroSuppress || ttTemp[index]->emEnergy()
                         || ttTemp[index]->hadEnergy() ) {
      LVL1::TriggerTower* ttIn  = 0;
      LVL1::TriggerTower* ttOut = 0;
      ttTemp.swapElement(index, ttIn, ttOut);
      ttCollection->push_back(ttOut);
    }
  }
  ttTemp.clear();

  return StatusCode::SUCCESS;
}

// Correction for Had FCAL eta which is adjusted to EM value in TriggerTower

double PpmByteStreamSubsetTool::etaSim(const ChannelCoordinate& coord) const
{
  double eta = coord.eta();
  const ChannelCoordinate::CaloLayer layer = coord.layer();
  if (layer == ChannelCoordinate::FCAL2 || layer == ChannelCoordinate::FCAL3) {
    const double etaCorrection = coord.etaGranularity()/4.;
    if (layer == ChannelCoordinate::FCAL2) eta -= etaCorrection;
    else eta += etaCorrection;
  }
  return eta;
}

// Find a trigger tower given eta, phi

LVL1::TriggerTower* PpmByteStreamSubsetTool::findTriggerTower(const double eta,
                                                              const double phi)
{
  LVL1::TriggerTower* tt = 0;
  const unsigned int key = m_towerKey->ttKey(phi, eta);
  TriggerTowerMap::const_iterator mapIter;
  mapIter = m_ttMap.find(key);
  if (mapIter != m_ttMap.end()) tt = mapIter->second;
  return tt;
}

// Print a vector

void PpmByteStreamSubsetTool::printVec(const std::vector<int>& vec,
                                       const MSG::Level level) const
{
  std::vector<int>::const_iterator pos;
  for (pos = vec.begin(); pos != vec.end(); ++pos) {
    if (pos != vec.begin()) m_log << level << ",";
    m_log << level << *pos;
  }
  m_log << level << "/";
}


} // end namespace
