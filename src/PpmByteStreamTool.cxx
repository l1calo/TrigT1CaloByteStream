
#include <numeric>
#include <utility>

#include "GaudiKernel/IInterface.h"

#include "TrigT1Calo/DataError.h"
#include "TrigT1Calo/TriggerTower.h"
#include "TrigT1Calo/TriggerTowerKey.h"

#include "TrigT1CaloByteStream/ChannelCoordinate.h"
//#include "TrigT1CaloByteStream/L1CaloRodStatus.h"
#include "TrigT1CaloByteStream/L1CaloSubBlock.h"
#include "TrigT1CaloByteStream/L1CaloUserHeader.h"
#include "TrigT1CaloByteStream/ModifySlices.h"
#include "TrigT1CaloByteStream/PpmByteStreamTool.h"
#include "TrigT1CaloByteStream/PpmCrateMappings.h"
#include "TrigT1CaloByteStream/PpmSubBlock.h"

namespace LVL1BS {

// Interface ID

static const InterfaceID IID_IPpmByteStreamTool("PpmByteStreamTool", 1, 1);

const InterfaceID& PpmByteStreamTool::interfaceID()
{
  return IID_IPpmByteStreamTool;
}

// Constructor

PpmByteStreamTool::PpmByteStreamTool(const std::string& type,
                                     const std::string& name,
				     const IInterface*  parent)
                  : AlgTool(type, name, parent),
		    m_version(1), m_compVers(4),
		    m_srcIdMap(0), m_ppmMaps(0), m_towerKey(0),
		    m_errorBlock(0), m_rodStatus(0)
{
  declareInterface<PpmByteStreamTool>(this);

  declareProperty("PrintCompStats",     m_printCompStats  = 0,
                  "Print compressed format statistics");

  // Properties for reading bytestream only
  declareProperty("ZeroSuppress",       m_zeroSuppress    = 0,
                  "Only make trigger towers with non-zero EM or Had energy");
  declareProperty("ROBSourceIDs",       m_sourceIDs,
                  "ROB fragment source identifiers");
  declareProperty("PedestalValue",      m_pedestal        = 10,
                  "Pedestal value - needed for compressed formats 0,1 only");

  // Properties for writing bytestream only
  declareProperty("DataFormat",         m_dataFormat      = 1,
                  "Format identifier (0-3) in sub-block header");
  declareProperty("FADCBaseline",       m_fadcBaseline    = 0,
                  "FADC baseline lower bound for compressed formats");
  declareProperty("FADCThreshold",      m_fadcThreshold   = 1,
                  "FADC threshold for super-compressed format");
  declareProperty("SlinksPerCrate",     m_slinks          = 4,
                  "The number of S-Links per crate");
  declareProperty("SimulSlicesLUT",     m_dfltSlicesLut   = 1,
                  "The number of LUT slices in the simulation");
  declareProperty("SimulSlicesFADC",    m_dfltSlicesFadc  = 7,
                  "The number of FADC slices in the simulation");
  declareProperty("ForceSlicesLUT",     m_forceSlicesLut  = 0,
                  "If >0, the number of LUT slices in bytestream");
  declareProperty("ForceSlicesFADC",    m_forceSlicesFadc = 0,
                  "If >0, the number of FADC slices in bytestream");

}

// Destructor

PpmByteStreamTool::~PpmByteStreamTool()
{
}

// Initialize

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

StatusCode PpmByteStreamTool::initialize()
{
  MsgStream log( msgSvc(), name() );
  log << MSG::INFO << "Initializing " << name() << " - package version "
                   << PACKAGE_VERSION << endreq;

  m_subDetector = eformat::TDAQ_CALO_PREPROC;
  m_srcIdMap    = new L1CaloSrcIdMap();
  m_ppmMaps     = new PpmCrateMappings();
  m_crates      = m_ppmMaps->crates();
  m_modules     = m_ppmMaps->modules();
  m_channels    = m_ppmMaps->channels();
  m_towerKey    = new LVL1::TriggerTowerKey();
  m_rodStatus   = new std::vector<uint32_t>(2);
  return AlgTool::initialize();
}

// Finalize

StatusCode PpmByteStreamTool::finalize()
{
  if (m_printCompStats) {
    MsgStream log( msgSvc(), name() );
    printCompStats(log, MSG::INFO);
  }
  delete m_rodStatus;
  delete m_errorBlock;
  delete m_towerKey;
  delete m_ppmMaps;
  delete m_srcIdMap;
  return AlgTool::finalize();
}

// Conversion bytestream to trigger towers

StatusCode PpmByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::TriggerTower>* const ttCollection)
{
  MsgStream log( msgSvc(), name() );
  const bool debug = msgSvc()->outputLevel(name()) <= MSG::DEBUG;

  // Clear trigger tower map

  m_ttMap.clear();
  TriggerTowerCollection ttTemp;

  // Loop over ROB fragments

  int robCount = 0;
  ROBIterator rob    = robFrags.begin();
  ROBIterator robEnd = robFrags.end();
  for (; rob != robEnd; ++rob) {

    if (debug) {
      ++robCount;
      log << MSG::DEBUG << "Treating ROB fragment " << robCount << endreq;
    }

    // Unpack ROD data (slinks)

    RODPointer payloadBeg;
    RODPointer payload;
    RODPointer payloadEnd;
    (*rob)->rod_data(payloadBeg);
    payloadEnd = payloadBeg + (*rob)->rod_ndata();
    payload = payloadBeg;
    if (payload == payloadEnd) {
      if (debug) log << MSG::DEBUG << "ROB fragment empty" << endreq;
      continue;
    }

    // Check identifier
    const uint32_t sourceID = (*rob)->rod_source_id();
    if (debug) {
      if (m_srcIdMap->subDet(sourceID) != m_subDetector ||
          m_srcIdMap->daqOrRoi(sourceID) != 0) {
        log << MSG::DEBUG << "Wrong source identifier in data: "
	    << MSG::hex << sourceID << MSG::dec << endreq;
      }
    }
    const int rodCrate = m_srcIdMap->crate(sourceID);
    if (debug) {
      log << MSG::DEBUG << "Treating crate " << rodCrate 
                        << " slink " << m_srcIdMap->slink(sourceID)
			<< endreq;
    }

    // First word is User Header
    L1CaloUserHeader userHeader(*payload);
    const int minorVersion = (*rob)->rod_version() & 0xffff;
    userHeader.setVersion(minorVersion);
    const int headerWords = userHeader.words();
    if (headerWords != 1 && debug) {
      log << MSG::DEBUG << "Unexpected number of user header words: "
          << headerWords << endreq;
    }
    for (int i = 0; i < headerWords; ++i) ++payload;
    // triggered slice offsets
    const int trigLut  = userHeader.ppmLut();
    const int trigFadc = userHeader.ppmFadc();
    // FADC baseline lower bound
    m_fadcBaseline = userHeader.lowerBound();
    if (debug) {
      log << MSG::DEBUG << "Minor format version number: " << MSG::hex 
                        << minorVersion << MSG::dec << endreq;
      log << MSG::DEBUG << "LUT triggered slice offset:  " << trigLut
                        << endreq;
      log << MSG::DEBUG << "FADC triggered slice offset: " << trigFadc
                        << endreq;
      log << MSG::DEBUG << "FADC baseline lower bound:   " << m_fadcBaseline
                        << endreq;
    }

    // Find the number of channels per sub-block

    int chanPerSubBlock = 0;
    if (payload != payloadEnd) {
      if (L1CaloSubBlock::wordType(*payload) != L1CaloSubBlock::HEADER) {
        log << MSG::ERROR << "Missing Sub-block header" << endreq;
        return StatusCode::FAILURE;
      }
      PpmSubBlock testBlock;
      payload = testBlock.read(payload, payloadEnd);
      chanPerSubBlock = testBlock.channelsPerSubBlock();
      if (chanPerSubBlock == 0) {
        log << MSG::ERROR << "Unsupported version/data format: "
                          << testBlock.version() << "/"
                          << testBlock.format()  << endreq;
        return StatusCode::FAILURE;
      }
      if (m_channels%chanPerSubBlock != 0) {
        log << MSG::ERROR << "Invalid channels per sub-block: "
                          << chanPerSubBlock << endreq;
        return StatusCode::FAILURE;
      }
      if (debug) {
        log << MSG::DEBUG << "Channels per sub-block: "
                          << chanPerSubBlock << endreq;
      }
    } else {
      if (debug) log << MSG::DEBUG << "ROB fragment contains user header only"
                                   << endreq;
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
          log << MSG::ERROR << "Unexpected data sequence" << endreq;
	  return StatusCode::FAILURE;
        }
        if (chanPerSubBlock != m_channels && 
	    L1CaloSubBlock::seqno(*payload) != block * chanPerSubBlock) {
          log << MSG::DEBUG << "Unexpected channel sequence number: "
	      << L1CaloSubBlock::seqno(*payload) << " expected " 
	      << block * chanPerSubBlock << endreq;
	  if ( !m_ppmBlocks.empty()) break;
	  else return StatusCode::FAILURE;
        }
        PpmSubBlock* const subBlock = new PpmSubBlock();
        m_ppmBlocks.push_back(subBlock);
        payload = subBlock->read(payload, payloadEnd);
        if (block == 0) {
          crate = subBlock->crate();
	  module = subBlock->module();
	  if (debug) {
	    log << MSG::DEBUG << "Module " << module << endreq;
	    if (crate != rodCrate) {
	      log << MSG::DEBUG << "Inconsistent crate number in ROD source ID"
                  << endreq;
	    }
          }
        } else {
          if (subBlock->crate() != crate) {
	    log << MSG::ERROR << "Inconsistent crate number in sub-blocks"
	        << endreq;
	    return StatusCode::FAILURE;
          }
          if (subBlock->module() != module) {
	    log << MSG::ERROR << "Inconsistent module number in sub-blocks"
	        << endreq;
	    return StatusCode::FAILURE;
          }
        }
        if (payload == payloadEnd && block != numSubBlocks - 1) {
          log << MSG::DEBUG << "Premature end of data" << endreq;
	  break;
	  //return StatusCode::FAILURE;
        }
      }

      // Is there an error block?

      delete m_errorBlock;
      m_errorBlock = 0;
      if (payload != payloadEnd) {
        if (L1CaloSubBlock::wordType(*payload) == L1CaloSubBlock::HEADER
	                            && PpmSubBlock::errorBlock(*payload)) {
	  if (debug) log << MSG::DEBUG << "Error block found" << endreq;
	  m_errorBlock = new PpmSubBlock();
	  payload = m_errorBlock->read(payload, payloadEnd);
          if (m_errorBlock->crate() != crate) {
	    log << MSG::ERROR << "Inconsistent crate number in error block"
	        << endreq;
	    return StatusCode::FAILURE;
          }
          if (m_errorBlock->module() != module) {
	    log << MSG::ERROR << "Inconsistent module number in error block"
	        << endreq;
	    return StatusCode::FAILURE;
          }
	  if (m_errorBlock->dataWords() && !m_errorBlock->unpack()) {
	    log << MSG::DEBUG << "Unpacking error block failed" << endreq;
	    //return StatusCode::FAILURE;
	  }
        }
      }

      // Loop over sub-blocks and fill trigger towers

      const int actualSubBlocks = m_ppmBlocks.size();
      for (int block = 0; block < actualSubBlocks; ++block) {
        PpmSubBlock* const subBlock = m_ppmBlocks[block];
	subBlock->setLutOffset(trigLut);
	subBlock->setFadcOffset(trigFadc);
	subBlock->setPedestal(m_pedestal);
	subBlock->setFadcBaseline(m_fadcBaseline);
        if (subBlock->dataWords() && !subBlock->unpack()) {
	  log << MSG::DEBUG << "Unpacking PPM sub-block failed" << endreq;
	  //return StatusCode::FAILURE;
        }
	if (m_printCompStats) addCompStats(subBlock->compStats());
        for (int chan = 0; chan < chanPerSubBlock; ++chan) {
          const int channel = block*chanPerSubBlock + chan;
  	  std::vector<int> lut;
	  std::vector<int> fadc;
	  std::vector<int> bcidLut;
	  std::vector<int> bcidFadc;
	  subBlock->ppmData(channel, lut, fadc, bcidLut, bcidFadc);
	  int trigLutKeep  = trigLut;
	  int trigFadcKeep = trigFadc;
	  if (lut.size() < size_t(trigLut + 1)) {
	    log << MSG::DEBUG << "Triggered LUT slice from header "
	        << "inconsistent with number of slices: "
		<< trigLut << ", " << lut.size() << ", reset to 0" << endreq;
	    trigLutKeep = 0;
	    //return StatusCode::FAILURE;
          }
	  if (fadc.size() < size_t(trigFadc + 1)) {
	    log << MSG::DEBUG << "Triggered FADC slice from header "
	        << "inconsistent with number of slices: "
		<< trigFadc << ", " << fadc.size() << ", reset to 0" << endreq;
	    trigFadcKeep = 0;
	    //return StatusCode::FAILURE;
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
	  // Wrong bit set for compressed formats 1.01 to 1.03
	  if (subBlock->format() > 1 && subBlock->seqno() < 4) {
	    errorBits.set(LVL1::DataError::ModuleError,
	         (errorBits.error() >> (LVL1::DataError::ModuleError+1)) & 0x1);
	  }
	  const int error = errorBits.error();

	  // Only save non-zero data

          const bool any =
	             std::accumulate(lut.begin(),      lut.end(),      0) ||
	             std::accumulate(fadc.begin(),     fadc.end(),     0) ||
		     std::accumulate(bcidLut.begin(),  bcidLut.end(),  0) ||
		     std::accumulate(bcidFadc.begin(), bcidFadc.end(), 0);

          if (any || error) {
	    if (debug) {
	      log << MSG::VERBOSE
	          << "channel/LUT/FADC/bcidLUT/bcidFADC/error: "
		  << channel << "/";
	      printVec(lut,      log, MSG::VERBOSE);
	      printVec(fadc,     log, MSG::VERBOSE);
	      printVec(bcidLut,  log, MSG::VERBOSE);
	      printVec(bcidFadc, log, MSG::VERBOSE);
	      log << MSG::VERBOSE << MSG::hex << error << MSG::dec << "/";
            }
	    ChannelCoordinate coord;
	    if (!m_ppmMaps->mapping(crate, module, channel, coord)) {
	      if (debug && any) {
	        log << MSG::VERBOSE << " Unexpected non-zero data words"
	            << endreq;
	      }
              continue;
            }
	    const ChannelCoordinate::CaloLayer layer = coord.layer();
	    const double phi = coord.phi();
	    const double eta = etaHack(coord);
	    if (debug) {
	      log << MSG::VERBOSE << " eta/etaHack/phi/layer: " << coord.eta()
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
  }

  // Swap wanted trigger towers into StoreGate container

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

// Conversion of trigger towers to bytestream

StatusCode PpmByteStreamTool::convert(
                      const DataVector<LVL1::TriggerTower>* const ttCollection,
                            RawEventWrite* const re)
{
  MsgStream log( msgSvc(), name() );
  const bool debug = msgSvc()->outputLevel(name()) <= MSG::DEBUG;

  // Clear the event assembler

  m_fea.clear();
  const uint16_t minorVersion = 0x1002;
  m_fea.setRodMinorVersion(minorVersion);
  m_rodStatusMap.clear();

  // Pointer to ROD data vector

  FullEventAssembler<L1CaloSrcIdMap>::RODDATA* theROD = 0;

  // Set up trigger tower maps

  setupTTMaps(ttCollection);

  // Create the sub-blocks to do the packing

  PpmSubBlock subBlock;
  const int chanPerSubBlock = PpmSubBlock::channelsPerSubBlock(m_version,
                                                               m_dataFormat);
  if (chanPerSubBlock == 0) {
    log << MSG::ERROR << "Unsupported version/data format: "
                      << m_version << "/" << m_dataFormat << endreq;
    return StatusCode::FAILURE;
  }
  PpmSubBlock errorBlock;

  int slicesLut  = 1;
  int slicesFadc = 1;
  int trigLut    = 0;
  int trigFadc   = 0;
  int slicesLutNew  = 1;
  int slicesFadcNew = 1;
  int trigLutNew    = 0;
  int trigFadcNew   = 0;
  const int modulesPerSlink = m_modules / m_slinks;
  for (int crate=0; crate < m_crates; ++crate) {
    for (int module=0; module < m_modules; ++module) {

      // Pack required number of modules per slink

      if (module%modulesPerSlink == 0) {
	const int daqOrRoi = 0;
	const int slink = module/modulesPerSlink;
        if (debug) {
          log << MSG::DEBUG << "Treating crate " << crate
                            << " slink " << slink << endreq;
        }
	// Get number of slices and triggered slice offsets
	// for this slink
	if ( ! slinkSlices(crate, module, modulesPerSlink,
	                   slicesLut, slicesFadc, trigLut, trigFadc)) {
	  log << MSG::ERROR << "Inconsistent number of slices or "
	      << "triggered slice offsets in data for crate "
	      << crate << " slink " << slink << endreq;
	  return StatusCode::FAILURE;
        }
	slicesLutNew  = (m_forceSlicesLut)  ? m_forceSlicesLut  : slicesLut;
	slicesFadcNew = (m_forceSlicesFadc) ? m_forceSlicesFadc : slicesFadc;
	trigLutNew    = ModifySlices::peak(trigLut,  slicesLut,  slicesLutNew);
	trigFadcNew   = ModifySlices::peak(trigFadc, slicesFadc, slicesFadcNew);
        if (debug) {
	  log << MSG::DEBUG << "Data Version/Format: " << m_version
	                    << " " << m_dataFormat << endreq;
          log << MSG::DEBUG << "LUT slices/offset: " << slicesLut
                            << " " << trigLut;
          if (slicesLut != slicesLutNew) {
	    log << MSG::DEBUG << " modified to " << slicesLutNew
	                      << " " << trigLutNew;
          }
	  log << MSG::DEBUG << endreq;
          log << MSG::DEBUG << "FADC slices/offset: " << slicesFadc
                            << " " << trigFadc;
          if (slicesFadc != slicesFadcNew) {
	    log << MSG::DEBUG << " modified to " << slicesFadcNew
	                      << " " << trigFadcNew;
          }
	  log << MSG::DEBUG << endreq;
        }
        L1CaloUserHeader userHeader;
        userHeader.setPpmLut(trigLutNew);
        userHeader.setPpmFadc(trigFadcNew);
	userHeader.setLowerBound(m_fadcBaseline);
	const uint32_t rodIdPpm = m_srcIdMap->getRodID(crate, slink, daqOrRoi,
	                                                       m_subDetector);
	theROD = m_fea.getRodData(rodIdPpm);
	theROD->push_back(userHeader.header());
	m_rodStatusMap.insert(make_pair(rodIdPpm, m_rodStatus));
      }
      if (debug) {
        log << MSG::DEBUG << "Module " << module << endreq;
      }

      // Find trigger towers corresponding to each eta/phi pair and fill
      // sub-blocks

      bool upstreamError = false;
      for (int channel=0; channel < m_channels; ++channel) {
	const int chan = channel % chanPerSubBlock;
        if (channel == 0 && m_dataFormat == L1CaloSubBlock::UNCOMPRESSED) {
          errorBlock.clear();
	  errorBlock.setPpmErrorHeader(m_version, m_dataFormat, crate,
	                               module, slicesFadcNew, slicesLutNew);
	}
        if (chan == 0) {
	  subBlock.clear();
	  if (m_dataFormat >= L1CaloSubBlock::COMPRESSED) {
	    subBlock.setPpmHeader(m_version, m_dataFormat, m_compVers, crate,
	                          module, slicesFadcNew, slicesLutNew);
          } else {
	    subBlock.setPpmHeader(m_version, m_dataFormat, channel, crate,
	                          module, slicesFadcNew, slicesLutNew);
	  }
	  subBlock.setLutOffset(trigLutNew);
	  subBlock.setFadcOffset(trigFadcNew);
	  subBlock.setFadcBaseline(m_fadcBaseline);
	  subBlock.setFadcThreshold(m_fadcThreshold);
        }
        const LVL1::TriggerTower* tt = 0;
	ChannelCoordinate coord;
	if (m_ppmMaps->mapping(crate, module, channel, coord)) {
	  tt = findLayerTriggerTower(coord);
        }
	if (tt ) {
	  int err = 0;
	  std::vector<int> lut;
	  std::vector<int> fadc;
	  std::vector<int> bcidLut;
	  std::vector<int> bcidFadc;
	  const ChannelCoordinate::CaloLayer layer = coord.layer();
	  if (layer == ChannelCoordinate::EM) {  // em
	    ModifySlices::data(tt->emLUT(),     lut,      slicesLutNew);
	    ModifySlices::data(tt->emADC(),     fadc,     slicesFadcNew);
	    ModifySlices::data(tt->emBCIDvec(), bcidLut,  slicesLutNew);
	    ModifySlices::data(tt->emBCIDext(), bcidFadc, slicesFadcNew);
	    err = tt->emError();
          } else {                               // had
	    ModifySlices::data(tt->hadLUT(),     lut,      slicesLutNew);
	    ModifySlices::data(tt->hadADC(),     fadc,     slicesFadcNew);
	    ModifySlices::data(tt->hadBCIDvec(), bcidLut,  slicesLutNew);
	    ModifySlices::data(tt->hadBCIDext(), bcidFadc, slicesFadcNew);
	    err = tt->hadError();
          }
	  subBlock.fillPpmData(channel, lut, fadc, bcidLut, bcidFadc);
	  if (err) {
	    const LVL1::DataError errorBits(err);
	    const int errpp = errorBits.get(LVL1::DataError::PPMErrorWord);
	    if (m_dataFormat == L1CaloSubBlock::UNCOMPRESSED) {
	      errorBlock.fillPpmError(channel, errpp);
	    } else subBlock.fillPpmError(channel, errpp);
	    if (errpp >> 2) upstreamError = true;
          }
        }
        if (chan == chanPerSubBlock - 1) {
	  // output the packed sub-block
	  if ( !subBlock.pack()) {
	    log << MSG::ERROR << "PPM sub-block packing failed"
	                      << endreq;
	    return StatusCode::FAILURE;
	  }
	  if (m_printCompStats) addCompStats(subBlock.compStats());
	  if (channel != m_channels - 1) {
	    // Only put errors in last sub-block
	    subBlock.setStatus(0, false, false, false, false,
	                                 false, false, false);
	    if (debug) {
	      log << MSG::DEBUG << "PPM sub-block data words: "
	                        << subBlock.dataWords() << endreq;
	    }
	    subBlock.write(theROD);
	  } else {
	    // Last sub-block - write error block
	    bool glinkTimeout = false;
	    bool daqOverflow  = false;
	    bool bcnMismatch  = false;
	    bool glinkParity  = false;
	    if (m_dataFormat == L1CaloSubBlock::UNCOMPRESSED) {
	      glinkTimeout = errorBlock.mcmAbsent() ||
	                     errorBlock.timeout();
	      daqOverflow  = errorBlock.asicFull() ||
	                     errorBlock.fpgaCorrupt();
	      bcnMismatch  = errorBlock.eventMismatch() ||
	                     errorBlock.bunchMismatch();
	      glinkParity  = errorBlock.glinkPinParity();
	    } else {
	      glinkTimeout = subBlock.mcmAbsent() ||
	                     subBlock.timeout();
	      daqOverflow  = subBlock.asicFull() ||
	                     subBlock.fpgaCorrupt();
	      bcnMismatch  = subBlock.eventMismatch() ||
	                     subBlock.bunchMismatch();
	      glinkParity  = subBlock.glinkPinParity();
	    }
	    subBlock.setStatus(0, glinkTimeout, false, upstreamError,
	                       daqOverflow, bcnMismatch, false, glinkParity);
	    if (debug) {
	      log << MSG::DEBUG << "PPM sub-block data words: "
	                        << subBlock.dataWords() << endreq;
	    }
            subBlock.write(theROD);
	    // Only uncompressed format has a separate error block
	    if (m_dataFormat == L1CaloSubBlock::UNCOMPRESSED) {
	      if ( ! errorBlock.pack()) {
	        log << MSG::ERROR << "PPM error block packing failed"
	                          << endreq;
	        return StatusCode::FAILURE;
	      }
	      errorBlock.setStatus(0, glinkTimeout, false, upstreamError, 
	                       daqOverflow, bcnMismatch, false, glinkParity);
	      errorBlock.write(theROD);
	      if (debug) {
	        log << MSG::DEBUG << "PPM error block data words: "
	                          << errorBlock.dataWords() << endreq;
	      }
	    }
          }
        }
      }
    }
  }

  // Fill the raw event

  m_fea.fill(re, log);

  // Set ROD status words

  //L1CaloRodStatus::setStatus(re, m_rodStatusMap, m_srcIdMap);

  return StatusCode::SUCCESS;
}

// Add compression stats to totals

void PpmByteStreamTool::addCompStats(const std::vector<uint32_t>& stats)
{
  if (stats.empty()) return;
  const int n = stats.size();
  if (m_compStats.empty()) m_compStats.resize(n);
  for (int i = 0; i < n; ++i) m_compStats[i] += stats[i];
}

// Print compression stats

void PpmByteStreamTool::printCompStats(MsgStream& log,
                                       const MSG::Level level) const
{
  log << level << "Compression stats format/count: ";
  const int n = m_compStats.size();
  for (int i = 0; i < n; ++i) {
    log << level << " " << i << "/" << m_compStats[i];
  }
  log << level << endreq;
}

// Hack for Had FCAL eta which is adjusted to EM value in TriggerTower

double PpmByteStreamTool::etaHack(const ChannelCoordinate& coord) const
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

// Find a trigger tower using separate layer maps

const LVL1::TriggerTower* PpmByteStreamTool::findLayerTriggerTower(
                                             const ChannelCoordinate& coord)
{
  const double phi = coord.phi();
  const double eta = etaHack(coord);
  const ChannelCoordinate::CaloLayer layer = coord.layer();
  const LVL1::TriggerTower* tt = 0;
  const unsigned int key = m_towerKey->ttKey(phi, eta);
  TriggerTowerMapConst::const_iterator mapIter;
  if (layer == ChannelCoordinate::EM) {
    mapIter = m_ttEmMap.find(key);
    if (mapIter != m_ttEmMap.end()) tt = mapIter->second;
  } else {
    mapIter = m_ttHadMap.find(key);
    if (mapIter != m_ttHadMap.end()) tt = mapIter->second;
  }
  return tt;
}

// Find a trigger tower given eta, phi

LVL1::TriggerTower* PpmByteStreamTool::findTriggerTower(const double eta,
                                                        const double phi)
{
  LVL1::TriggerTower* tt = 0;
  const unsigned int key = m_towerKey->ttKey(phi, eta);
  TriggerTowerMap::const_iterator mapIter;
  mapIter = m_ttMap.find(key);
  if (mapIter != m_ttMap.end()) tt = mapIter->second;
  return tt;
}

// Set up trigger tower maps

void PpmByteStreamTool::setupTTMaps(const TriggerTowerCollection*
                                                           const ttCollection)
{
  using std::accumulate;

  m_ttEmMap.clear();
  m_ttHadMap.clear();
  TriggerTowerCollection::const_iterator pos  = ttCollection->begin();
  TriggerTowerCollection::const_iterator pose = ttCollection->end();
  for (; pos != pose; ++pos) {
    const LVL1::TriggerTower* tt = *pos;
    const unsigned int key = m_towerKey->ttKey(tt->phi(), tt->eta());
    // Ignore any with zero data
    // EM
    if (accumulate((tt->emLUT()).begin(),     (tt->emLUT()).end(),     0) ||
        accumulate((tt->emADC()).begin(),     (tt->emADC()).end(),     0) ||
	accumulate((tt->emBCIDvec()).begin(), (tt->emBCIDvec()).end(), 0) ||
	accumulate((tt->emBCIDext()).begin(), (tt->emBCIDext()).end(), 0) ||
	tt->emError()) {
      m_ttEmMap.insert(std::make_pair(key, tt));
    }
    // Had
    if (accumulate((tt->hadLUT()).begin(),     (tt->hadLUT()).end(),     0) ||
        accumulate((tt->hadADC()).begin(),     (tt->hadADC()).end(),     0) ||
	accumulate((tt->hadBCIDvec()).begin(), (tt->hadBCIDvec()).end(), 0) ||
	accumulate((tt->hadBCIDext()).begin(), (tt->hadBCIDext()).end(), 0) ||
	tt->hadError()) {
      m_ttHadMap.insert(std::make_pair(key, tt));
    }
  }
}

// Get number of slices and triggered slice offsets for next slink

bool PpmByteStreamTool::slinkSlices(const int crate, const int module,
                  const int modulesPerSlink, int& slicesLut, int& slicesFadc,
		  int& trigLut, int& trigFadc)
{
  int sliceL = -1;
  int sliceF =  m_dfltSlicesFadc;
  int trigL  =  m_dfltSlicesLut/2;
  int trigF  =  m_dfltSlicesFadc/2;
  for (int mod = module; mod < module + modulesPerSlink; ++mod) {
    for (int chan = 0; chan < m_channels; ++chan) {
      ChannelCoordinate coord;
      if (!m_ppmMaps->mapping(crate, mod, chan, coord)) continue;
      const LVL1::TriggerTower* const tt = findLayerTriggerTower(coord);
      if ( !tt ) continue;
      const ChannelCoordinate::CaloLayer layer = coord.layer();
      if (layer == ChannelCoordinate::EM) {
        if (sliceL < 0) { // initialise
	  sliceL = (tt->emLUT()).size();
	  sliceF = (tt->emADC()).size();
	  trigL  = tt->emPeak();
	  trigF  = tt->emADCPeak();
        } else { // check consistent
	  if ((tt->emLUT()).size() != size_t(sliceL) ||
	      (tt->emADC()).size() != size_t(sliceF) ||
	      tt->emPeak() != trigL || tt->emADCPeak() != trigF) {
            return false;
          }
        }
      } else {
        if (sliceL < 0) {
	  sliceL = (tt->hadLUT()).size();
	  sliceF = (tt->hadADC()).size();
	  trigL  = tt->hadPeak();
	  trigF  = tt->hadADCPeak();
        } else {
	  if ((tt->hadLUT()).size() != size_t(sliceL) ||
	      (tt->hadADC()).size() != size_t(sliceF) ||
	      tt->hadPeak() != trigL || tt->hadADCPeak() != trigF) {
            return false;
          }
        }
      }
    }
  }
  if (sliceL < 0) sliceL = m_dfltSlicesLut;
  slicesLut  = sliceL;
  slicesFadc = sliceF;
  trigLut    = trigL;
  trigFadc   = trigF;
  return true;
}

// Return reference to vector with all possible Source Identifiers

const std::vector<uint32_t>& PpmByteStreamTool::sourceIDs()
{
  if (m_sourceIDs.empty()) {
    const int maxlinks = m_srcIdMap->maxSlinks();
    for (int crate = 0; crate < m_crates; ++crate) {
      for (int slink = 0; slink < maxlinks; ++slink) {
        const int daqOrRoi = 0;
        const uint32_t rodId = m_srcIdMap->getRodID(crate, slink, daqOrRoi,
                                                            m_subDetector);
        const uint32_t robId = m_srcIdMap->getRobID(rodId);
        m_sourceIDs.push_back(robId);
      }
    }
  }
  return m_sourceIDs;
}

// Print a vector

void PpmByteStreamTool::printVec(const std::vector<int>& vec, MsgStream& log,
                                                const MSG::Level level) const
{
  std::vector<int>::const_iterator pos;
  for (pos = vec.begin(); pos != vec.end(); ++pos) {
    if (pos != vec.begin()) log << level << ",";
    log << level << *pos;
  }
  log << level << "/";
}


} // end namespace
