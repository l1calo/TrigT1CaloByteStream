
#include <utility>

#include "GaudiKernel/IInterface.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/PropertyMgr.h"
#include "GaudiKernel/ToolFactory.h"

#include "TrigT1Calo/TriggerTower.h"
#include "TrigT1Calo/TriggerTowerKey.h"

#include "TrigT1CaloByteStream/ChannelCoordinate.h"
#include "TrigT1CaloByteStream/L1CaloSubBlock.h"
#include "TrigT1CaloByteStream/L1CaloUserHeader.h"
#include "TrigT1CaloByteStream/PpmByteStreamTool.h"
#include "TrigT1CaloByteStream/PpmCrateMappings.h"
#include "TrigT1CaloByteStream/PpmErrorBlock.h"
#include "TrigT1CaloByteStream/PpmSortPermutations.h"
#include "TrigT1CaloByteStream/PpmSubBlock.h"

// Interface ID (copied blind from other examples)

static const InterfaceID IID_IPpmByteStreamTool("PpmByteStreamTool", 1, 1);

static ToolFactory<PpmByteStreamTool> s_factory;
const IToolFactory& PpmByteStreamToolFactory = s_factory;

const InterfaceID& PpmByteStreamTool::interfaceID()
{
  return IID_IPpmByteStreamTool;
}

// Constructor

PpmByteStreamTool::PpmByteStreamTool(const std::string& type,
                                     const std::string& name,
				     const IInterface*  parent)
                  : AlgTool(type, name, parent)
{
  declareInterface<PpmByteStreamTool>(this);

  // Properties for writing bytestream only
  declareProperty("DataVersion", m_version = 1);
  declareProperty("DataFormat", m_dataFormat = 1);
  declareProperty("ChannelsPerSubBlock", m_channelsPerSubBlock = 16);
  declareProperty("SlinksPerCrate", m_slinks = 4);

  // Number of error words per error block
  declareProperty("GlinkPins", m_glinkPins = 16);
}

// Destructor

PpmByteStreamTool::~PpmByteStreamTool()
{
}

// Initialize

StatusCode PpmByteStreamTool::initialize()
{
  m_srcIdMap  = new L1CaloSrcIdMap();
  m_ppmMaps   = new PpmCrateMappings();
  m_sortPerms = new PpmSortPermutations();
  m_crates    = m_ppmMaps->crates();
  m_modules   = m_ppmMaps->modules();
  m_channels  = m_ppmMaps->channels();
  m_towerKey  = new LVL1::TriggerTowerKey();
  m_errorBlock = 0;
  return AlgTool::initialize();
}

// Finalize

StatusCode PpmByteStreamTool::finalize()
{
  delete m_errorBlock;
  delete m_towerKey;
  delete m_sortPerms;
  delete m_ppmMaps;
  delete m_srcIdMap;
  return AlgTool::finalize();
}

// Conversion bytestream to trigger towers

StatusCode PpmByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::TriggerTower>* ttCollection)
{
  using LVL1::TriggerTower;

  MsgStream log( msgSvc(), "PpmByteStreamTool" );
  bool debug = msgSvc()->outputLevel("PpmByteStreamTool") <= MSG::DEBUG;

  // Clear trigger tower map

  m_ttMap.clear();

  // Loop over ROB fragments

  int robCount = 0;
  IROBDataProviderSvc::VROBFRAG::const_iterator rob = robFrags.begin();
  IROBDataProviderSvc::VROBFRAG::const_iterator robEnd = robFrags.end();
  for (; rob != robEnd; ++rob) {

    if (debug) {
      ++robCount;
      log << MSG::DEBUG << "Treating ROB fragment " << robCount << endreq;
    }

    // Unpack ROD data (slinks)

    OFFLINE_FRAGMENTS_NAMESPACE::PointerType payloadBeg;
    OFFLINE_FRAGMENTS_NAMESPACE::PointerType payload;
    OFFLINE_FRAGMENTS_NAMESPACE::PointerType payloadEnd;
    (*rob)->rod_data(payloadBeg);
    payloadEnd = payloadBeg + (*rob)->rod_ndata();
    payload = payloadBeg;

    // Check identifier
    uint32_t sourceID = (*rob)->rod_source_id();
    if (m_srcIdMap->subDet(sourceID) != eformat::TDAQ_CALO_PREPROC ||
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

    // Minor version needed for compression codes
    uint32_t version = (*rob)->rod_version();
    uint16_t minorVersion = version & 0xffff;
    
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
      if (L1CaloSubBlock::wordType(*payload) != L1CaloSubBlock::DATA_HEADER) {
        log << MSG::ERROR << "Missing Sub-block header" << endreq;
        return StatusCode::FAILURE;
      }
      L1CaloSubBlock testBlock;
      payload = testBlock.read(payload, payloadEnd);
      chanPerSubBlock = m_channels;
      if (payload != payloadEnd) {
        if (L1CaloSubBlock::wordType(*payload) == L1CaloSubBlock::DATA_HEADER) {
          testBlock.clear();
          payload = testBlock.read(payload, payloadEnd);
	  if (testBlock.seqno() > 0) chanPerSubBlock = testBlock.seqno();
        }
      }
    }
    if (chanPerSubBlock <= 0 || m_channels%chanPerSubBlock != 0) {
      log << MSG::ERROR << "Invalid channels per sub-block: "
                        << chanPerSubBlock << endreq;
      return StatusCode::FAILURE;
    }
    if (debug) {
      log << MSG::DEBUG << "Channels per sub-block: "
                        << chanPerSubBlock << endreq;
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
        if (L1CaloSubBlock::wordType(*payload) != L1CaloSubBlock::DATA_HEADER) {
          log << MSG::ERROR << "Unexpected data sequence" << endreq;
	  return StatusCode::FAILURE;
        }
        PpmSubBlock* subBlock = new PpmSubBlock(chanPerSubBlock,
	                                        minorVersion, m_sortPerms);
        m_ppmBlocks.push_back(subBlock);
        payload = subBlock->read(payload, payloadEnd);
        if (payload == payloadEnd && block != numSubBlocks - 1) {
          log << MSG::ERROR << "Premature end of data" << endreq;
	  return StatusCode::FAILURE;
        }
        if (subBlock->seqno() != block * chanPerSubBlock) {
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
        if (L1CaloSubBlock::wordType(*payload) == L1CaloSubBlock::ERROR_HEADER) {
	  if (debug) log << MSG::DEBUG << "Error block found" << endreq;
	  m_errorBlock = new PpmErrorBlock(m_glinkPins);
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

      // Get eta/phi mappings for this module

      std::map<int, ChannelCoordinate> coordMap;
      m_ppmMaps->mappings(crate, module, coordMap);

      // Loop over sub-blocks and fill trigger towers

      for (int block = 0; block < numSubBlocks; ++block) {
        PpmSubBlock* subBlock = m_ppmBlocks[block];
        if ( !subBlock->unpack()) {
	  log << MSG::ERROR << "Unpacking PPM sub-block failed" << endreq;
	  return StatusCode::FAILURE;
        }
        for (int chan = 0; chan < chanPerSubBlock; ++chan) {
          int channel = block*chanPerSubBlock + chan;
  	  std::vector<int> lut;
	  std::vector<int> fadc;
	  std::vector<int> bcidLut;
	  std::vector<int> bcidFadc;
	  subBlock->ppmData(chan, lut, fadc, bcidLut, bcidFadc);
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
	  if (m_errorBlock) {
	    error = m_errorBlock->ppmError(channel%m_glinkPins);
          }

	  // Only save non-zero data

	  int any = 0;
	  std::vector<int>::const_iterator pos;
	  for (pos = lut.begin();      pos != lut.end();      ++pos) any |= *pos;
	  for (pos = fadc.begin();     pos != fadc.end();     ++pos) any |= *pos;
	  for (pos = bcidLut.begin();  pos != bcidLut.end();  ++pos) any |= *pos;
	  for (pos = bcidFadc.begin(); pos != bcidFadc.end(); ++pos) any |= *pos;
	  std::map<int, ChannelCoordinate>::const_iterator cpos =
	                                        coordMap.find(channel);
	  if (any && cpos == coordMap.end()) {
	    log << MSG::ERROR << "Unexpected non-zero data words"
	        << endreq;
            return StatusCode::FAILURE;
          }
	  if (cpos == coordMap.end()) continue;

          if (any || error) {
  	    const ChannelCoordinate& coord(cpos->second);
	    ChannelCoordinate::CaloLayer layer = coord.layer();
	    double phi = coord.phi();
	    double eta = etaHack(coord);
	    if (debug) dumpDebug(channel, coord, lut, fadc,
	                         bcidLut, bcidFadc, error, log);
	    TriggerTower* tt = findTriggerTower(eta, phi);
            if ( ! tt) {
	      // make new tower and add to map
	      int key = m_towerKey->ttKey(phi, eta);
	      tt = new TriggerTower(phi, eta, key);
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
  using LVL1::TriggerTower;

  MsgStream log( msgSvc(), "PpmByteStreamTool" );
  bool debug = msgSvc()->outputLevel("PpmByteStreamTool") <= MSG::DEBUG;

  // Clear the event assembler

  m_fea.clear();
  uint16_t minorVersion = 0x1000; // Not known yet
  m_fea.setRodMinorVersion(minorVersion);

  // Pointer to ROD data vector

  FullEventAssembler<L1CaloSrcIdMap>::RODDATA* theROD = 0;

  // Set up trigger tower maps

  setupTTMaps(ttCollection);

  // Create the sub-blocks to do the packing

  PpmSubBlock subBlock(m_channelsPerSubBlock, minorVersion, m_sortPerms);
  PpmErrorBlock errorBlock(m_glinkPins);

  // Dummy vectors for absent trigger towers

  int slicesLut = 1;
  int slicesFadc = 1;
  std::vector<int> dummyLut(slicesLut);
  std::vector<int> dummyFadc(slicesFadc);
  std::vector<int> bcidLut(slicesLut);
  std::vector<int> bcidFadc(slicesFadc);

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
	int trigLut = 0;
	int trigFadc = 0;
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
	                                        eformat::TDAQ_CALO_PREPROC);
	theROD = m_fea.getRodData(rodIdPpm);
	theROD->push_back(userHeader.header());
	dummyLut.resize(slicesLut);
	dummyFadc.resize(slicesFadc);
	bcidLut.resize(slicesLut);
	bcidFadc.resize(slicesFadc);
      }
      if (debug) {
        log << MSG::DEBUG << "Module " << module << endreq;
      }

      // Get eta/phi mappings for this module

      std::map<int, ChannelCoordinate> coordMap;
      m_ppmMaps->mappings(crate, module, coordMap);

      // Check the module exists

      if (coordMap.empty()) {
	if (debug) {
          log << MSG::DEBUG << "Crate " << crate << ", module " << module
	      << " does not exist" << endreq;
	}
	continue;
      }

      // Vector to accumulate errors

      std::vector<uint32_t> errors(m_glinkPins);

      // Find trigger towers corresponding to each eta/phi pair and fill
      // sub-blocks

      for (int chan=0; chan < m_channels; ++chan) {
        if (chan % m_channelsPerSubBlock == 0) {
	  subBlock.clear();
	  subBlock.setHeader(m_version, m_dataFormat, chan, crate,
	                     module, slicesFadc, slicesLut);
        }
        TriggerTower* tt = 0;
	std::map<int, ChannelCoordinate>::iterator pos =
	                                           coordMap.find(chan);
	if (pos != coordMap.end()) {
	  const ChannelCoordinate& coord(pos->second);
	  tt = findLayerTriggerTower(coord);
        }
	if (tt ) {
	  uint32_t err = 0;
	  const ChannelCoordinate& coord(pos->second);
	  ChannelCoordinate::CaloLayer layer = coord.layer();
	  if (layer == ChannelCoordinate::EM) {  // em
	    subBlock.fillPpmData(tt->emLUT(), tt->emADC(),
	                         tt->emBCIDvec(), tt->emBCIDext());
	    err = tt->emError();
	    if (debug) dumpDebug(chan, coord, tt->emLUT(), tt->emADC(),
	                         tt->emBCIDvec(), tt->emBCIDext(), err, log);
          } else {                               // had
	    subBlock.fillPpmData(tt->hadLUT(), tt->hadADC(),
	                         tt->hadBCIDvec(), tt->hadBCIDext());
	    err = tt->hadError();
	    if (debug) dumpDebug(chan, coord, tt->hadLUT(), tt->hadADC(),
	                         tt->hadBCIDvec(), tt->hadBCIDext(), err, log);
          }
	  if (errors[chan%m_glinkPins] != 0 && 
	      errors[chan%m_glinkPins] != err) {
	    log << MSG::ERROR << "Unexpected error difference" << endreq;
          }
	  errors[chan%m_glinkPins] = err;
        } else {
	  subBlock.fillPpmData(dummyLut, dummyFadc, bcidLut, bcidFadc);
        }
        if (chan % m_channelsPerSubBlock == m_channelsPerSubBlock - 1) {
	  // output the packed sub-block
	  if ( !subBlock.pack()) {
	    log << MSG::ERROR << "PPM sub-block packing failed"
	                      << endreq;
	    return StatusCode::FAILURE;
	  }
	  if (chan != m_channels - 1) {
	    // Only put errors in last sub-block
	    subBlock.setStatus(0, false, false, false, false,
	                                        false, false);
	    subBlock.write(theROD);
	  } else {
	    // Last sub-block - Fill error block
            errorBlock.clear();
	    errorBlock.setHeader(m_version, m_dataFormat,
	                         L1CaloSubBlock::errorMarker(),
	                         crate, module, slicesFadc, slicesLut);
            for (int pin = 0; pin < m_glinkPins; ++pin) {
	      errorBlock.fillPpmError(errors[pin]);
            }
	    if ( ! errorBlock.pack()) {
	      log << MSG::ERROR << "PPM error block packing failed"
	                        << endreq;
	      return StatusCode::FAILURE;
	    }
	    bool glinkTimeout = errorBlock.mcmAbsent() ||
	                        errorBlock.timeout();
	    bool daqOverflow = errorBlock.asicFull() ||
	                       errorBlock.fpgaCorrupt();
	    bool bcnMismatch = errorBlock.eventMismatch() ||
	                       errorBlock.bunchMismatch();
	    subBlock.setStatus(0, glinkTimeout, false, daqOverflow,
	                       bcnMismatch, false, false);
            subBlock.write(theROD);
	    errorBlock.setStatus(0, glinkTimeout, false, daqOverflow,
	                         bcnMismatch, false, false);
	    errorBlock.write(theROD);
          }
        }
      }
    }
  }

  // Fill the raw event

  m_fea.fill(re, log);

  return StatusCode::SUCCESS;
}

// Dump data values to DEBUG

void PpmByteStreamTool::dumpDebug(int channel,
                                  const ChannelCoordinate& coord,
                                  const std::vector<int>& lut,
				  const std::vector<int>& fadc,
				  const std::vector<int>& bcidLut,
				  const std::vector<int>& bcidFadc,
				  int error, MsgStream& log)
{
  ChannelCoordinate::CaloLayer layer = coord.layer();
  double phi = coord.phi();
  double eta = etaHack(coord);
  int key = m_towerKey->ttKey(phi, eta);

  log << MSG::DEBUG
      << "key/channel/layer/eta/phi/LUT/FADC/bcidLUT/bcidFADC/error: ";
  std::string layerName;
  switch (layer) {
    case ChannelCoordinate::NONE:  layerName = "NONE";  break;
    case ChannelCoordinate::EM:    layerName = "EM";    break;
    case ChannelCoordinate::HAD:   layerName = "HAD";   break;
    case ChannelCoordinate::FCAL2: layerName = "FCAL2"; break;
    case ChannelCoordinate::FCAL3: layerName = "FCAL3"; break;
    default: break;
  }
  log << MSG::DEBUG << key << "/" << channel << "/" << layerName << "/"
	            << eta << "/" << phi << "/";

  std::vector<int>::const_iterator pos;
  for (pos = lut.begin(); pos != lut.end(); ++pos) {
    log << MSG::DEBUG << *pos << " ";
  }
  log << MSG::DEBUG << "/";
  for (pos = fadc.begin(); pos != fadc.end(); ++pos) {
    log << MSG::DEBUG << *pos << " ";
  }
  log << MSG::DEBUG << "/";
  for (pos = bcidLut.begin(); pos != bcidLut.end(); ++pos) {
    log << MSG::DEBUG << *pos << " ";
  }
  log << MSG::DEBUG << "/";
  for (pos = bcidFadc.begin(); pos != bcidFadc.end(); ++pos) {
    log << MSG::DEBUG << *pos << " ";
  }
  log << MSG::DEBUG << "/" << error << endreq;
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
  using LVL1::TriggerTower;

  double phi = coord.phi();
  double eta = etaHack(coord);
  ChannelCoordinate::CaloLayer layer = coord.layer();
  TriggerTower* tt = 0;
  int key = m_towerKey->ttKey(phi, eta);
  std::map<int, TriggerTower*>::const_iterator mapIter;
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
  using LVL1::TriggerTower;

  TriggerTower* tt = 0;
  int key = m_towerKey->ttKey(phi, eta);
  std::map<int, TriggerTower*>::const_iterator mapIter = m_ttMap.find(key);
  if (mapIter != m_ttMap.end()) tt = mapIter->second;
  return tt;
}

// Set up trigger tower maps

void PpmByteStreamTool::setupTTMaps(const DataVector<LVL1::TriggerTower>*
                                                               ttCollection)
{
  using LVL1::TriggerTower;

  m_ttEmMap.clear();
  m_ttHadMap.clear();
  DataVector<TriggerTower>::const_iterator pos = ttCollection->begin();
  DataVector<TriggerTower>::const_iterator pose = ttCollection->end();
  for (; pos != pose; ++pos) {
    TriggerTower* tt = *pos;
    int key = m_towerKey->ttKey(tt->phi(), tt->eta());
    // Ignore any with zero data
    // EM
    int any = tt->emError();
    std::vector<int>::const_iterator pos;
    for (pos = (tt->emLUT()).begin();
                        pos != (tt->emLUT()).end(); ++pos) any |= *pos;
    for (pos = (tt->emADC()).begin();
                        pos != (tt->emADC()).end(); ++pos) any |= *pos;
    for (pos = (tt->emBCIDvec()).begin();
                    pos != (tt->emBCIDvec()).end(); ++pos) any |= *pos;
    for (pos = (tt->emBCIDext()).begin();
                    pos != (tt->emBCIDext()).end(); ++pos) any |= *pos;
    if (any) m_ttEmMap.insert(std::make_pair(key, tt));

    // Had
    any = tt->hadError();
    for (pos = (tt->hadLUT()).begin();
                       pos != (tt->hadLUT()).end(); ++pos) any |= *pos;
    for (pos = (tt->hadADC()).begin();
                       pos != (tt->hadADC()).end(); ++pos) any |= *pos;
    for (pos = (tt->hadBCIDvec()).begin();
                   pos != (tt->hadBCIDvec()).end(); ++pos) any |= *pos;
    for (pos = (tt->hadBCIDext()).begin();
                   pos != (tt->hadBCIDext()).end(); ++pos) any |= *pos;
    if (any) m_ttHadMap.insert(std::make_pair(key, tt));
  }
}

// Get number of slices and triggered slice offsets for next slink

bool PpmByteStreamTool::slinkSlices(int crate, int module,
                  int modulesPerSlink, int& slicesLut, int& slicesFadc,
		  int& trigLut, int& trigFadc)
{
  using LVL1::TriggerTower;

  int sliceL = -1;
  int sliceF =  1;
  int trigL  =  0;
  int trigF  =  0;
  for (int mod = module; mod < module + modulesPerSlink; ++mod) {
    std::map<int, ChannelCoordinate> coordMap;
    m_ppmMaps->mappings(crate, mod, coordMap);
    if (coordMap.empty()) continue;
    std::map<int, ChannelCoordinate>::const_iterator pos;
    for (int chan = 0; chan < m_channels; ++chan) {
      pos = coordMap.find(chan);
      if (pos == coordMap.end()) continue;
      const ChannelCoordinate& coord(pos->second);
      TriggerTower* tt = findLayerTriggerTower(coord);
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
  if (sliceL < 0) sliceL = 1;
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
                       eformat::TDAQ_CALO_PREPROC);
      uint32_t robId = m_srcIdMap->getRobID(rodId);
      vID.push_back(robId);
    }
  }
}
