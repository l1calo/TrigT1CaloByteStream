
#include <numeric>
#include <utility>

#include "GaudiKernel/IInterface.h"

#include "TrigT1Calo/TriggerTower.h"
#include "TrigT1Calo/TriggerTowerKey.h"

#include "TrigT1CaloByteStream/ChannelCoordinate.h"
#include "TrigT1CaloByteStream/L1CaloSubBlock.h"
#include "TrigT1CaloByteStream/L1CaloUserHeader.h"
#include "TrigT1CaloByteStream/PpmByteStreamTool.h"
#include "TrigT1CaloByteStream/PpmCrateMappings.h"
#include "TrigT1CaloByteStream/PpmSubBlock.h"

// Interface ID (copied blind from other examples)

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
		    m_srcIdMap(0), m_ppmMaps(0), m_towerKey(0),
		    m_errorBlock(0)
{
  declareInterface<PpmByteStreamTool>(this);

  declareProperty("PrintCompStats", m_printCompStats = 0);

  // Properties for writing bytestream only
  declareProperty("DataVersion", m_version = 1);
  declareProperty("DataFormat", m_dataFormat = 1);
  declareProperty("CompressionVersion", m_compVers = 1);
  declareProperty("SlinksPerCrate", m_slinks = 4);
  declareProperty("DefaultSlicesLUT", m_dfltSlicesLut = 1);
  declareProperty("DefaultSlicesFADC", m_dfltSlicesFadc = 7);
  declareProperty("ForceSlicesFADC", m_forceSlicesFadc = 0);

}

// Destructor

PpmByteStreamTool::~PpmByteStreamTool()
{
}

// Initialize

StatusCode PpmByteStreamTool::initialize()
{
  m_subDetector = eformat::TDAQ_CALO_PREPROC;
  m_srcIdMap    = new L1CaloSrcIdMap();
  m_ppmMaps     = new PpmCrateMappings();
  m_crates      = m_ppmMaps->crates();
  m_modules     = m_ppmMaps->modules();
  m_channels    = m_ppmMaps->channels();
  m_towerKey    = new LVL1::TriggerTowerKey();
  return AlgTool::initialize();
}

// Finalize

StatusCode PpmByteStreamTool::finalize()
{
  if (m_printCompStats) {
    MsgStream log( msgSvc(), name() );
    printCompStats(log, MSG::INFO);
  }
  delete m_errorBlock;
  delete m_towerKey;
  delete m_ppmMaps;
  delete m_srcIdMap;
  return AlgTool::finalize();
}

// Conversion bytestream to trigger towers

StatusCode PpmByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::TriggerTower>* ttCollection)
{
  MsgStream log( msgSvc(), name() );
  bool debug = msgSvc()->outputLevel(name()) <= MSG::DEBUG;

  // Clear trigger tower map

  m_ttMap.clear();

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
    uint32_t sourceID = (*rob)->rod_source_id();
    if (m_srcIdMap->subDet(sourceID) != m_subDetector ||
        m_srcIdMap->daqOrRoi(sourceID) != 0) {
      log << MSG::ERROR << "Wrong source identifier in data" << endreq;
      return StatusCode::FAILURE;
    }
    int rodCrate = m_srcIdMap->crate(sourceID);
    if (debug) {
      log << MSG::DEBUG << "Treating crate " << rodCrate 
                        << " slink " << m_srcIdMap->slink(sourceID)
			<< endreq;
    }

    // First word is User Header
    L1CaloUserHeader userHeader(*payload);
    int headerWords = userHeader.words();
    if (headerWords != 1) {
      log << MSG::ERROR << "Unexpected number of user header words: "
          << headerWords << endreq;
      return StatusCode::FAILURE;
    }
    // triggered slice offsets
    int trigLut  = userHeader.ppmLut();
    int trigFadc = userHeader.ppmFadc();
    if (debug) {
      log << MSG::DEBUG << "LUT triggered slice offset: " << trigLut
                        << endreq;
      log << MSG::DEBUG << "FADC triggered slice offset: " << trigFadc
                        << endreq;
    }
    ++payload;

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
    int numSubBlocks = m_channels/chanPerSubBlock;

    // Loop over PPMs

    payload = payloadBeg;
    ++payload;
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
        PpmSubBlock* subBlock = new PpmSubBlock();
        m_ppmBlocks.push_back(subBlock);
        payload = subBlock->read(payload, payloadEnd);
        if (payload == payloadEnd && block != numSubBlocks - 1) {
          log << MSG::ERROR << "Premature end of data" << endreq;
	  return StatusCode::FAILURE;
        }
        if (chanPerSubBlock != m_channels && 
	    subBlock->seqno() != block * chanPerSubBlock) {
          log << MSG::ERROR << "Unexpected first channel: "
	      << subBlock->seqno() << endreq;
	  return StatusCode::FAILURE;
        }
        if (block == 0) {
          crate = subBlock->crate();
	  module = subBlock->module();
	  if (debug) log << MSG::DEBUG << "Module " << module << endreq;
	  if (crate != rodCrate) {
	    log << MSG::ERROR << "Inconsistent crate number in ROD source ID"
                << endreq;
	    return StatusCode::FAILURE;
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
	  if ( !m_errorBlock->unpack()) {
	    log << MSG::ERROR << "Unpacking error block failed" << endreq;
	    return StatusCode::FAILURE;
	  }
        }
      }

      // Loop over sub-blocks and fill trigger towers

      for (int block = 0; block < numSubBlocks; ++block) {
        PpmSubBlock* subBlock = m_ppmBlocks[block];
	subBlock->setLutOffset(trigLut);
	subBlock->setFadcOffset(trigFadc);
        if ( !subBlock->unpack()) {
	  log << MSG::ERROR << "Unpacking PPM sub-block failed" << endreq;
	  return StatusCode::FAILURE;
        }
	if (m_printCompStats) addCompStats(subBlock->compStats());
        for (int chan = 0; chan < chanPerSubBlock; ++chan) {
          int channel = block*chanPerSubBlock + chan;
  	  std::vector<int> lut;
	  std::vector<int> fadc;
	  std::vector<int> bcidLut;
	  std::vector<int> bcidFadc;
	  subBlock->ppmData(channel, lut, fadc, bcidLut, bcidFadc);
	  if (lut.size() < size_t(trigLut + 1)) {
	    log << MSG::ERROR << "Triggered LUT slice from header "
	        << "inconsistent with number of slices: "
		<< trigLut << ", " << lut.size() << endreq;
	    return StatusCode::FAILURE;
          }
	  if (fadc.size() < size_t(trigFadc + 1)) {
	    log << MSG::ERROR << "Triggered FADC slice from header "
	        << "inconsistent with number of slices: "
		<< trigFadc << ", " << fadc.size() << endreq;
	    return StatusCode::FAILURE;
          }
	  int error = 0;
	  if (m_errorBlock) error = m_errorBlock->ppmError(channel);
	  else error = subBlock->ppmError(channel);

	  // Only save non-zero data

          bool any = std::accumulate(lut.begin(),      lut.end(),      0) ||
	             std::accumulate(fadc.begin(),     fadc.end(),     0) ||
		     std::accumulate(bcidLut.begin(),  bcidLut.end(),  0) ||
		     std::accumulate(bcidFadc.begin(), bcidFadc.end(), 0);

          if (any || error) {
	    ChannelCoordinate coord;
	    if (!m_ppmMaps->mapping(crate, module, channel, coord)) {
	      if (any) log << MSG::WARNING << "Unexpected non-zero data words"
	                   << endreq;
              continue;
            }
	    ChannelCoordinate::CaloLayer layer = coord.layer();
	    double phi = coord.phi();
	    double eta = etaHack(coord);
	    if (debug) printData(channel, coord, lut, fadc, bcidLut, bcidFadc,
	                         error, log, MSG::VERBOSE);
	    LVL1::TriggerTower* tt = findTriggerTower(eta, phi);
            if ( ! tt) {
	      // make new tower and add to map
	      unsigned int key = m_towerKey->ttKey(phi, eta);
	      tt = new LVL1::TriggerTower(phi, eta, key);
              m_ttMap.insert(std::make_pair(key, tt));
	      ttCollection->push_back(tt);
            }
            if (layer == ChannelCoordinate::EM) {  // EM
	      tt->addEM(fadc, lut, bcidFadc, bcidLut, error,
	                                           trigLut, trigFadc);
            } else {                               // Had
	      tt->addHad(fadc, lut, bcidFadc, bcidLut, error,
	                                            trigLut, trigFadc);
            }
          }
        }
      }
    }
  }

  return StatusCode::SUCCESS;
}

// Conversion of trigger towers to bytestream

StatusCode PpmByteStreamTool::convert(
                            const DataVector<LVL1::TriggerTower>* ttCollection,
                            RawEventWrite* re)
{
  MsgStream log( msgSvc(), name() );
  bool debug = msgSvc()->outputLevel(name()) <= MSG::DEBUG;

  // Clear the event assembler

  m_fea.clear();
  uint16_t minorVersion = 0x1000; // Not known yet
  m_fea.setRodMinorVersion(minorVersion);

  // Pointer to ROD data vector

  FullEventAssembler<L1CaloSrcIdMap>::RODDATA* theROD = 0;

  // Set up trigger tower maps

  setupTTMaps(ttCollection);

  // Create the sub-blocks to do the packing

  PpmSubBlock subBlock;
  int chanPerSubBlock = PpmSubBlock::channelsPerSubBlock(m_version,
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
  int modulesPerSlink = m_modules / m_slinks;
  for (int crate=0; crate < m_crates; ++crate) {
    for (int module=0; module < m_modules; ++module) {

      // Pack required number of modules per slink

      if (module%modulesPerSlink == 0) {
	int daqOrRoi = 0;
	int slink = module/modulesPerSlink;
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
        if (debug) {
	  log << MSG::DEBUG << "Data Version/Format: " << m_version
	                    << " " << m_dataFormat << endreq;
          log << MSG::DEBUG << "LUT slices/offset: " << slicesLut
                            << " " << trigLut << endreq;
          log << MSG::DEBUG << "FADC slices/offset: " << slicesFadc
                            << " " << trigFadc << endreq;
        }
        L1CaloUserHeader userHeader;
        userHeader.setPpmLut(trigLut);
        userHeader.setPpmFadc(trigFadc);
	uint32_t rodIdPpm = m_srcIdMap->getRodID(crate, slink, daqOrRoi,
	                                                 m_subDetector);
	theROD = m_fea.getRodData(rodIdPpm);
	theROD->push_back(userHeader.header());
      }
      if (debug) {
        log << MSG::DEBUG << "Module " << module << endreq;
      }

      // Find trigger towers corresponding to each eta/phi pair and fill
      // sub-blocks

      bool upstreamError = false;
      for (int channel=0; channel < m_channels; ++channel) {
	int chan = channel % chanPerSubBlock;
        if (channel == 0 && m_dataFormat == L1CaloSubBlock::UNCOMPRESSED) {
          errorBlock.clear();
	  errorBlock.setPpmErrorHeader(m_version, m_dataFormat, crate,
	                               module, slicesFadc, slicesLut);
	}
        if (chan == 0) {
	  subBlock.clear();
	  if (m_dataFormat == L1CaloSubBlock::COMPRESSED) {
	    subBlock.setPpmHeader(m_version, m_dataFormat, m_compVers, crate,
	                          module, slicesFadc, slicesLut);
          } else {
	    subBlock.setPpmHeader(m_version, m_dataFormat, channel, crate,
	                          module, slicesFadc, slicesLut);
	  }
	  subBlock.setLutOffset(trigLut);
	  subBlock.setFadcOffset(trigFadc);
        }
        LVL1::TriggerTower* tt = 0;
	ChannelCoordinate coord;
	if (m_ppmMaps->mapping(crate, module, channel, coord)) {
	  tt = findLayerTriggerTower(coord);
        }
	if (tt ) {
	  uint32_t err = 0;
	  ChannelCoordinate::CaloLayer layer = coord.layer();
	  if (layer == ChannelCoordinate::EM) {  // em
	    subBlock.fillPpmData(channel, tt->emLUT(), tt->emADC(),
	                                  tt->emBCIDvec(), tt->emBCIDext());
	    err = tt->emError();
	    if (debug) printData(channel, coord, tt->emLUT(), tt->emADC(),
	                         tt->emBCIDvec(), tt->emBCIDext(), err,
				 log, MSG::VERBOSE);
          } else {                               // had
	    subBlock.fillPpmData(channel, tt->hadLUT(), tt->hadADC(),
	                                  tt->hadBCIDvec(), tt->hadBCIDext());
	    err = tt->hadError();
	    if (debug) printData(channel, coord, tt->hadLUT(), tt->hadADC(),
	                         tt->hadBCIDvec(), tt->hadBCIDext(), err,
				 log, MSG::VERBOSE);
          }
	  if (err) {
	    if (m_dataFormat == L1CaloSubBlock::UNCOMPRESSED) {
	      errorBlock.fillPpmError(channel, err);
	    } else subBlock.fillPpmError(channel, err);
	    if (err >> 2) upstreamError = true;
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
	    }
          }
        }
      }
    }
  }

  // Fill the raw event

  m_fea.fill(re, log);

  return StatusCode::SUCCESS;
}

// Add compression stats to totals

void PpmByteStreamTool::addCompStats(const std::vector<uint32_t>& stats)
{
  if (stats.empty()) return;
  int n = stats.size();
  if (m_compStats.empty()) m_compStats.resize(n);
  for (int i = 0; i < n; ++i) m_compStats[i] += stats[i];
}

// Print compression stats

void PpmByteStreamTool::printCompStats(MsgStream& log, MSG::Level level)
{
  log << level << "Compression stats format/count: ";
  int n = m_compStats.size();
  for (int i = 0; i < n; ++i) {
    log << level << " " << i << "/" << m_compStats[i];
  }
  log << level << endreq;
}

// Print data values

void PpmByteStreamTool::printData(int channel,
                                  const ChannelCoordinate& coord,
                                  const std::vector<int>& lut,
				  const std::vector<int>& fadc,
				  const std::vector<int>& bcidLut,
				  const std::vector<int>& bcidFadc,
				  int error, MsgStream& log, MSG::Level level)
{
  ChannelCoordinate::CaloLayer layer = coord.layer();
  double phi = coord.phi();
  double eta = etaHack(coord);
  unsigned int key = m_towerKey->ttKey(phi, eta);

  log << level
      << "key/channel/layer/eta/phi/LUT/FADC/bcidLUT/bcidFADC/error: ";
  std::string layerName = ChannelCoordinate::layerName(layer);
  log << level << key << "/" << channel << "/" << layerName << "/"
	       << eta << "/" << phi << "/";

  printVec(lut,      log, level);
  printVec(fadc,     log, level);
  printVec(bcidLut,  log, level);
  printVec(bcidFadc, log, level);
  log << level << error << "/" << endreq;
}

// Print vector values

void PpmByteStreamTool::printVec(const std::vector<int>& vec, MsgStream& log,
                                                            MSG::Level level)
{
  std::vector<int>::const_iterator pos;
  for (pos = vec.begin(); pos != vec.end(); ++pos) {
    if (pos != vec.begin()) log << level << ",";
    log << level << *pos;
  }
  log << level << "/";
}

// Hack for Had FCAL eta which is adjusted to EM value in TriggerTower

double PpmByteStreamTool::etaHack(const ChannelCoordinate& coord)
{
  double eta = coord.eta();
  ChannelCoordinate::CaloLayer layer = coord.layer();
  if (layer == ChannelCoordinate::FCAL2 || layer == ChannelCoordinate::FCAL3) {
    double etaCorrection = coord.etaGranularity()/4.;
    if (layer == ChannelCoordinate::FCAL2) eta -= etaCorrection;
    else eta += etaCorrection;
  }
  return eta;
}

// Find a trigger tower using separate layer maps

LVL1::TriggerTower* PpmByteStreamTool::findLayerTriggerTower(
                                       const ChannelCoordinate& coord)
{
  double phi = coord.phi();
  double eta = etaHack(coord);
  ChannelCoordinate::CaloLayer layer = coord.layer();
  LVL1::TriggerTower* tt = 0;
  unsigned int key = m_towerKey->ttKey(phi, eta);
  TriggerTowerMap::const_iterator mapIter;
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

LVL1::TriggerTower* PpmByteStreamTool::findTriggerTower(double eta, double phi)
{
  LVL1::TriggerTower* tt = 0;
  unsigned int key = m_towerKey->ttKey(phi, eta);
  TriggerTowerMap::const_iterator mapIter;
  mapIter = m_ttMap.find(key);
  if (mapIter != m_ttMap.end()) tt = mapIter->second;
  return tt;
}

// Modify the number of trigger tower FADC slices

LVL1::TriggerTower* PpmByteStreamTool::modFadcSlices(LVL1::TriggerTower* tt)
{
  std::vector<int> emADC;
  std::vector<int> hadADC;
  std::vector<int> emBCIDext;
  std::vector<int> hadBCIDext;
  std::vector<int> emLUT(tt->emLUT());
  std::vector<int> hadLUT(tt->hadLUT());
  std::vector<int> emBCIDvec(tt->emBCIDvec());
  std::vector<int> hadBCIDvec(tt->hadBCIDvec());
  int oldsize    = (tt->emADC()).size();
  int oldsizeHad = (tt->hadADC()).size();
  if (oldsizeHad > oldsize) oldsize = oldsizeHad;
  int newsize = m_forceSlicesFadc;
  if (newsize < oldsize) {
    int offset = (oldsize - newsize) / 2;
    for (int i = 0; i < newsize; ++i) {
      emADC.push_back((tt->emADC())[i + offset]);
      hadADC.push_back((tt->hadADC())[i + offset]);
      emBCIDext.push_back((tt->emBCIDext())[i + offset]);
      hadBCIDext.push_back((tt->hadBCIDext())[i + offset]);
    }
    int peak = newsize / 2;
    unsigned int key = m_towerKey->ttKey(tt->phi(), tt->eta());
    LVL1::TriggerTower* ttNew = new LVL1::TriggerTower(
                        tt->phi(), tt->eta(), key, emADC, emLUT,
			emBCIDext, emBCIDvec, tt->emError(),
			tt->emPeak(), peak, hadADC, hadLUT,
			hadBCIDext, hadBCIDvec, tt->hadError(),
			tt->hadPeak(), peak);
    m_ttModFadc.push_back(ttNew);
    return ttNew;
  }
  return tt;
}

// Set up trigger tower maps

void PpmByteStreamTool::setupTTMaps(const TriggerTowerCollection* ttCollection)
{
  using std::accumulate;

  m_ttEmMap.clear();
  m_ttHadMap.clear();
  if (m_forceSlicesFadc) m_ttModFadc.clear();
  TriggerTowerCollection::const_iterator pos  = ttCollection->begin();
  TriggerTowerCollection::const_iterator pose = ttCollection->end();
  for (; pos != pose; ++pos) {
    LVL1::TriggerTower* tt = *pos;
    if (m_forceSlicesFadc) tt = modFadcSlices(tt);
    unsigned int key = m_towerKey->ttKey(tt->phi(), tt->eta());
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

bool PpmByteStreamTool::slinkSlices(int crate, int module,
                  int modulesPerSlink, int& slicesLut, int& slicesFadc,
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
      LVL1::TriggerTower* tt = findLayerTriggerTower(coord);
      if ( !tt ) continue;
      ChannelCoordinate::CaloLayer layer = coord.layer();
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

// Fill a vector with all possible Source Identifiers

void PpmByteStreamTool::sourceIDs(std::vector<uint32_t>& vID) const
{
  int maxlinks = m_srcIdMap->maxSlinks();
  for (int crate = 0; crate < m_crates; ++crate) {
    for (int slink = 0; slink < maxlinks; ++slink) {
      const int daqOrRoi = 0;
      uint32_t rodId = m_srcIdMap->getRodID(crate, slink, daqOrRoi,
                                                          m_subDetector);
      uint32_t robId = m_srcIdMap->getRobID(rodId);
      vID.push_back(robId);
    }
  }
}
