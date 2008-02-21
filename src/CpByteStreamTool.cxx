
#include <numeric>
#include <utility>

#include "GaudiKernel/IInterface.h"

#include "TrigT1Calo/CMMCPHits.h"
#include "TrigT1Calo/CPMHits.h"
#include "TrigT1Calo/CPMTower.h"
#include "TrigT1Calo/CPBSCollection.h"
#include "TrigT1Calo/DataError.h"
#include "TrigT1Calo/TriggerTowerKey.h"

#include "TrigT1CaloByteStream/ChannelCoordinate.h"
#include "TrigT1CaloByteStream/CmmCpSubBlock.h"
#include "TrigT1CaloByteStream/CmmSubBlock.h"
#include "TrigT1CaloByteStream/CpmCrateMappings.h"
#include "TrigT1CaloByteStream/CpmSubBlock.h"
#include "TrigT1CaloByteStream/CpByteStreamTool.h"
//#include "TrigT1CaloByteStream/L1CaloRodStatus.h"
#include "TrigT1CaloByteStream/L1CaloSubBlock.h"
#include "TrigT1CaloByteStream/L1CaloUserHeader.h"
#include "TrigT1CaloByteStream/ModifySlices.h"

namespace LVL1BS {

// Interface ID

static const InterfaceID IID_ICpByteStreamTool("CpByteStreamTool", 1, 1);

const InterfaceID& CpByteStreamTool::interfaceID()
{
  return IID_ICpByteStreamTool;
}

// Constructor

CpByteStreamTool::CpByteStreamTool(const std::string& type,
                                   const std::string& name,
				   const IInterface*  parent)
                  : AlgTool(type, name, parent),
		    m_coreOverlap(1),
		    m_srcIdMap(0), m_cpmMaps(0), m_towerKey(0),
		    m_rodStatus(0)
{
  declareInterface<CpByteStreamTool>(this);

  declareProperty("CrateOffsetHw",  m_crateOffsetHw  = 8,
                  "Offset of CP crate numbers in bytestream");
  declareProperty("CrateOffsetSw",  m_crateOffsetSw  = 0,
                  "Offset of CP crate numbers in RDOs");

  // Properties for reading bytestream only
  declareProperty("ROBSourceIDs",       m_sourceIDs,
                  "ROB fragment source identifiers");

  // Properties for writing bytestream only
  declareProperty("DataVersion",    m_version        = 1,
                  "Format version number in sub-block header");
  declareProperty("DataFormat",     m_dataFormat     = 1,
                  "Format identifier (0-1) in sub-block header");
  declareProperty("SlinksPerCrate", m_slinks         = 2,
                  "The number of S-Links per crate");
  declareProperty("SimulSlicesCPM", m_dfltSlicesCpm  = 1,
                  "The number of CPM slices in the simulation");
  declareProperty("SimulSlicesCMM", m_dfltSlicesCmm  = 1,
                  "The number of CMM slices in the simulation");
  declareProperty("ForceSlicesCPM", m_forceSlicesCpm = 0,
                  "If >0, the number of CPM slices in bytestream");
  declareProperty("ForceSlicesCMM", m_forceSlicesCmm = 0,
                  "If >0, the number of CMM slices in bytestream");

}

// Destructor

CpByteStreamTool::~CpByteStreamTool()
{
}

// Initialize

StatusCode CpByteStreamTool::initialize()
{
  m_subDetector = eformat::TDAQ_CALO_CLUSTER_PROC_DAQ;
  m_srcIdMap    = new L1CaloSrcIdMap();
  m_cpmMaps     = new CpmCrateMappings();
  m_crates      = m_cpmMaps->crates();
  m_modules     = m_cpmMaps->modules();
  m_channels    = m_cpmMaps->channels();
  m_towerKey    = new LVL1::TriggerTowerKey();
  m_rodStatus   = new std::vector<uint32_t>(2);
  return AlgTool::initialize();
}

// Finalize

StatusCode CpByteStreamTool::finalize()
{
  delete m_rodStatus;
  delete m_towerKey;
  delete m_cpmMaps;
  delete m_srcIdMap;
  return AlgTool::finalize();
}

// Conversion bytestream to CPM towers

StatusCode CpByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::CPMTower>* const ttCollection)
{
  m_ttCollection = ttCollection;
  m_ttMap.clear();
  return convertBs(robFrags, CPM_TOWERS);
}

// Conversion bytestream to CPM hits

StatusCode CpByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::CPMHits>* const hitCollection)
{
  m_hitCollection = hitCollection;
  m_hitsMap.clear();
  return convertBs(robFrags, CPM_HITS);
}

// Conversion bytestream to CMM-CP hits

StatusCode CpByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::CMMCPHits>* const hitCollection)
{
  m_cmmHitCollection = hitCollection;
  m_cmmHitsMap.clear();
  return convertBs(robFrags, CMM_CP_HITS);
}

// Conversion of CP container to bytestream

StatusCode CpByteStreamTool::convert(const LVL1::CPBSCollection* const cp,
                                           RawEventWrite* const re)
{
  MsgStream log( msgSvc(), name() );
  const bool debug = msgSvc()->outputLevel(name()) <= MSG::DEBUG;

  // Clear the event assembler

  m_fea.clear();
  const uint16_t minorVersion = 0x1001;
  m_fea.setRodMinorVersion(minorVersion);
  m_rodStatusMap.clear();

  // Pointer to ROD data vector

  FullEventAssembler<L1CaloSrcIdMap>::RODDATA* theROD = 0;

  // Set up the container maps

  setupCpmTowerMap(cp->towers());
  setupCpmHitsMap(cp->hits());
  setupCmmCpHitsMap(cp->cmmHits());

  // Loop over data

  const bool neutralFormat   = m_dataFormat == L1CaloSubBlock::NEUTRAL;
  const int  modulesPerSlink = m_modules / m_slinks;
  int timeslices    = 1;
  int timeslicesCmm = 1;
  int trigCpm       = 0;
  int trigCmm       = 0;
  int timeslicesNew    = 1;
  int timeslicesCmmNew = 1;
  int trigCpmNew       = 0;
  int trigCmmNew       = 0;
  for (int crate=0; crate < m_crates; ++crate) {
    const int hwCrate = crate + m_crateOffsetHw;

    // Get CMM number of slices and triggered slice offset for this crate

    if ( ! slinkSlicesCmm(crate, timeslicesCmm, trigCmm)) {
      log << MSG::ERROR << "Inconsistent number of CMM slices or "
          << "triggered slice offsets in data for crate " << hwCrate << endreq;
      return StatusCode::FAILURE;
    }
    timeslicesCmmNew = (m_forceSlicesCmm) ? m_forceSlicesCmm : timeslicesCmm;
    trigCmmNew       = ModifySlices::peak(trigCmm, timeslicesCmm,
                                                   timeslicesCmmNew);
    if (debug) {
      log << MSG::DEBUG << "CMM Slices/offset: " << timeslicesCmm
                        << " " << trigCmm;
      if (timeslicesCmm != timeslicesCmmNew) {
        log << MSG::DEBUG << " modified to " << timeslicesCmmNew
	                  << " " << trigCmmNew;
      }
      log << MSG::DEBUG << endreq;
    }

    // Unfortunately CPM modules are numbered 1 to m_modules
    for (int module=1; module <= m_modules; ++module) {
      const int mod = module - 1;

      // Pack required number of modules per slink

      if (mod%modulesPerSlink == 0) {
	const int daqOrRoi = 0;
	const int slink = (m_slinks == 2) ? 2*(mod/modulesPerSlink)
	                                  : mod/modulesPerSlink;
        if (debug) {
          log << MSG::DEBUG << "Treating crate " << hwCrate
                            << " slink " << slink << endreq;
        }
	// Get number of CPM slices and triggered slice offset
	// for this slink
	if ( ! slinkSlices(crate, module, modulesPerSlink,
	                                  timeslices, trigCpm)) {
	  log << MSG::ERROR << "Inconsistent number of slices or "
	      << "triggered slice offsets in data for crate "
	      << hwCrate << " slink " << slink << endreq;
	  return StatusCode::FAILURE;
        }
	timeslicesNew = (m_forceSlicesCpm) ? m_forceSlicesCpm : timeslices;
	trigCpmNew    = ModifySlices::peak(trigCpm, timeslices, timeslicesNew);
        if (debug) {
	  log << MSG::DEBUG << "Data Version/Format: " << m_version
	                    << " " << m_dataFormat << endreq;
          log << MSG::DEBUG << "Slices/offset: " << timeslices
                            << " " << trigCpm;
	  if (timeslices != timeslicesNew) {
	    log << MSG::DEBUG << " modified to " << timeslicesNew
	                      << " " << trigCpmNew;
          }
	  log << MSG::DEBUG << endreq;
        }
        L1CaloUserHeader userHeader;
        userHeader.setCpm(trigCpmNew);
        userHeader.setCpCmm(trigCmmNew);
	const uint32_t rodIdCpm = m_srcIdMap->getRodID(hwCrate, slink, daqOrRoi,
	                                                        m_subDetector);
	theROD = m_fea.getRodData(rodIdCpm);
	theROD->push_back(userHeader.header());
	m_rodStatusMap.insert(make_pair(rodIdCpm, m_rodStatus));
      }
      if (debug) {
        log << MSG::DEBUG << "Module " << module << endreq;
      }

      // Create a sub-block for each slice (except Neutral format)

      m_cpmBlocks.clear();
      for (int slice = 0; slice < timeslicesNew; ++slice) {
        CpmSubBlock* const subBlock = new CpmSubBlock();
	subBlock->setCpmHeader(m_version, m_dataFormat, slice,
	                       hwCrate, module, timeslicesNew);
        m_cpmBlocks.push_back(subBlock);
	if (neutralFormat) break;
      }

      // Find CPM towers corresponding to each eta/phi pair and fill
      // sub-blocks

      for (int chan=0; chan < m_channels; ++chan) {
	ChannelCoordinate coord;
	if (m_cpmMaps->mapping(crate, module, chan, coord)) {
	  const double eta = coord.eta();
	  const double phi = coord.phi();
          const LVL1::CPMTower* const tt = findCpmTower(eta, phi);
	  if (tt ) {
	    std::vector<int> emData;
	    std::vector<int> hadData;
	    std::vector<int> emError;
	    std::vector<int> hadError;
	    ModifySlices::data(tt->emEnergyVec(),  emData,   timeslicesNew);
	    ModifySlices::data(tt->hadEnergyVec(), hadData,  timeslicesNew);
	    ModifySlices::data(tt->emErrorVec(),   emError,  timeslicesNew);
	    ModifySlices::data(tt->hadErrorVec(),  hadError, timeslicesNew);
            for (int slice = 0; slice < timeslicesNew; ++slice) {
	      const LVL1::DataError emErrBits(emError[slice]);
	      const LVL1::DataError hadErrBits(hadError[slice]);
	      const int emErr  =
	                      (emErrBits.get(LVL1::DataError::LinkDown) << 1) |
	                       emErrBits.get(LVL1::DataError::Parity);
	      const int hadErr =
	                     (hadErrBits.get(LVL1::DataError::LinkDown) << 1) |
	                      hadErrBits.get(LVL1::DataError::Parity);
	      const int index  = ( neutralFormat ) ? 0 : slice;
              CpmSubBlock* const subBlock = m_cpmBlocks[index];
              subBlock->fillTowerData(slice, chan, emData[slice],
	                              hadData[slice], emErr, hadErr);
	    }
          }
        }
      }

      // Add CPM hits

      const LVL1::CPMHits* const hits = findCpmHits(crate, module);
      if (hits) {
        std::vector<unsigned int> vec0;
        std::vector<unsigned int> vec1;
	ModifySlices::data(hits->HitsVec0(), vec0, timeslicesNew);
	ModifySlices::data(hits->HitsVec1(), vec1, timeslicesNew);
        for (int slice = 0; slice < timeslicesNew; ++slice) {
	  const int index = ( neutralFormat ) ? 0 : slice;
	  CpmSubBlock* const subBlock = m_cpmBlocks[index];
	  subBlock->setHits(slice, vec0[slice], vec1[slice]);
        }
      }
      
      // Pack and write the sub-blocks

      DataVector<CpmSubBlock>::const_iterator pos;
      for (pos = m_cpmBlocks.begin(); pos != m_cpmBlocks.end(); ++pos) {
        CpmSubBlock* const subBlock = *pos;
	if ( !subBlock->pack()) {
	  log << MSG::ERROR << "CPM sub-block packing failed" << endreq;
	  return StatusCode::FAILURE;
	}
        if (debug) {
          log << MSG::DEBUG << "CPM sub-block data words: "
	                    << subBlock->dataWords() << endreq;
        }
	subBlock->write(theROD);
      }
    }

    // Append CMMs to last S-Link of the crate

    // Create a sub-block for each slice (except Neutral format)

    m_cmmHit0Blocks.clear();
    m_cmmHit1Blocks.clear();
    const int summing = (crate == m_crates - 1) ? CmmSubBlock::SYSTEM
                                                : CmmSubBlock::CRATE;
    for (int slice = 0; slice < timeslicesCmmNew; ++slice) {
      CmmCpSubBlock* const h0Block = new CmmCpSubBlock();
      h0Block->setCmmHeader(m_version, m_dataFormat, slice, hwCrate,
                            summing, CmmSubBlock::CMM_CP,
			    CmmSubBlock::RIGHT, timeslicesCmmNew);
      m_cmmHit0Blocks.push_back(h0Block);
      CmmCpSubBlock* const h1Block = new CmmCpSubBlock();
      h1Block->setCmmHeader(m_version, m_dataFormat, slice, hwCrate,
                            summing, CmmSubBlock::CMM_CP,
			    CmmSubBlock::LEFT, timeslicesCmmNew);
      m_cmmHit1Blocks.push_back(h1Block);
      if (neutralFormat) break;
    }

    // CMM-CP

    const int maxDataID = LVL1::CMMCPHits::MAXID;
    for (int dataID = 1; dataID < maxDataID; ++dataID) {
      int source = dataID;
      if (dataID > m_modules) {
        if (summing == CmmSubBlock::CRATE && 
	    dataID != LVL1::CMMCPHits::LOCAL) continue;
	switch (dataID) {
	  case LVL1::CMMCPHits::LOCAL:
	    source = CmmCpSubBlock::LOCAL;
	    break;
	  case LVL1::CMMCPHits::REMOTE_0:
	    source = CmmCpSubBlock::REMOTE_0;
	    break;
	  case LVL1::CMMCPHits::REMOTE_1:
	    source = CmmCpSubBlock::REMOTE_1;
	    break;
	  case LVL1::CMMCPHits::REMOTE_2:
	    source = CmmCpSubBlock::REMOTE_2;
	    break;
	  case LVL1::CMMCPHits::TOTAL:
	    source = CmmCpSubBlock::TOTAL;
	    break;
          default:
	    continue;
        }
      }
      const LVL1::CMMCPHits* const ch = findCmmCpHits(crate, dataID);
      if ( ch ) {
        std::vector<unsigned int> hits0;
        std::vector<unsigned int> hits1;
        std::vector<int> err0;
        std::vector<int> err1;
	ModifySlices::data(ch->HitsVec0(),  hits0, timeslicesCmmNew);
	ModifySlices::data(ch->HitsVec1(),  hits1, timeslicesCmmNew);
	ModifySlices::data(ch->ErrorVec0(), err0,  timeslicesCmmNew);
	ModifySlices::data(ch->ErrorVec1(), err1,  timeslicesCmmNew);
	for (int slice = 0; slice < timeslicesCmmNew; ++slice) {
	  const LVL1::DataError err0Bits(err0[slice]);
	  const LVL1::DataError err1Bits(err1[slice]);
	  const int index = ( neutralFormat ) ? 0 : slice;
	  CmmCpSubBlock* subBlock = m_cmmHit0Blocks[index];
	  subBlock->setHits(slice, source, hits0[slice],
	                           err0Bits.get(LVL1::DataError::Parity));
	  subBlock = m_cmmHit1Blocks[index];
	  subBlock->setHits(slice, source, hits1[slice],
	                           err1Bits.get(LVL1::DataError::Parity));
        }
      }
    }
    DataVector<CmmCpSubBlock>::const_iterator cos = m_cmmHit0Blocks.begin();
    for (; cos != m_cmmHit0Blocks.end(); ++cos) {
      CmmCpSubBlock* const subBlock = *cos;
      if ( !subBlock->pack()) {
        log << MSG::ERROR << "CMM-Cp sub-block packing failed" << endreq;
	return StatusCode::FAILURE;
      }
      if (debug) {
        log << MSG::DEBUG << "CMM-Cp sub-block data words: "
	                  << subBlock->dataWords() << endreq;
      }
      subBlock->write(theROD);
    }
    cos = m_cmmHit1Blocks.begin();
    for (; cos != m_cmmHit1Blocks.end(); ++cos) {
      CmmCpSubBlock* const subBlock = *cos;
      if ( !subBlock->pack()) {
        log << MSG::ERROR << "CMM-Cp sub-block packing failed" << endreq;
	return StatusCode::FAILURE;
      }
      if (debug) {
        log << MSG::DEBUG << "CMM-Cp sub-block data words: "
	                  << subBlock->dataWords() << endreq;
      }
      subBlock->write(theROD);
    }
  }

  // Fill the raw event

  m_fea.fill(re, log);

  // Set ROD status words

  //L1CaloRodStatus::setStatus(re, m_rodStatusMap, m_srcIdMap);

  return StatusCode::SUCCESS;
}

// Return reference to vector with all possible Source Identifiers

const std::vector<uint32_t>& CpByteStreamTool::sourceIDs(
                                               const std::string& sgKey)
{
  // Check if overlap tower channels wanted
  const std::string flag("Overlap");
  const std::string::size_type pos = sgKey.find(flag);
  m_coreOverlap =
   (pos == std::string::npos || pos != sgKey.length() - flag.length()) ? 1 : 2;

  if (m_sourceIDs.empty()) {
    const int maxCrates = m_crates + m_crateOffsetHw;
    const int maxSlinks = m_srcIdMap->maxSlinks();
    for (int hwCrate = m_crateOffsetHw; hwCrate < maxCrates; ++hwCrate) {
      for (int slink = 0; slink < maxSlinks; ++slink) {
        const int daqOrRoi = 0;
        const uint32_t rodId = m_srcIdMap->getRodID(hwCrate, slink, daqOrRoi,
                                                             m_subDetector);
        const uint32_t robId = m_srcIdMap->getRobID(rodId);
        m_sourceIDs.push_back(robId);
      }
    }
  }
  return m_sourceIDs;
}

// Convert bytestream to given container type

StatusCode CpByteStreamTool::convertBs(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            const CollectionType collection)
{
  MsgStream log( msgSvc(), name() );
  const bool debug = msgSvc()->outputLevel(name()) <= MSG::DEBUG;

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

    // Check identifier
    const uint32_t sourceID = (*rob)->rod_source_id();
    if (debug) {
      if (m_srcIdMap->subDet(sourceID)   != m_subDetector ||
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
    const L1CaloUserHeader userHeader(*payload);
    const int headerWords = userHeader.words();
    if (headerWords != 1 && debug) {
      log << MSG::DEBUG << "Unexpected number of user header words: "
          << headerWords << endreq;
    }
    for (int i = 0; i < headerWords; ++i) ++payload;
    // triggered slice offsets
    const int trigCpm = userHeader.cpm();
    const int trigCmm = userHeader.cpCmm();
    if (debug) {
      log << MSG::DEBUG << "CPM triggered slice offset: " << trigCpm
                        << endreq;
      log << MSG::DEBUG << "CMM triggered slice offset: " << trigCmm
                        << endreq;
    }

    // Loop over sub-blocks

    while (payload != payloadEnd) {
      
      if (L1CaloSubBlock::wordType(*payload) != L1CaloSubBlock::HEADER) {
        log << MSG::ERROR << "Unexpected data sequence" << endreq;
        return StatusCode::FAILURE;
      }
      if (CmmSubBlock::cmmBlock(*payload)) {
        // CMM
        CmmCpSubBlock subBlock;
        payload = subBlock.read(payload, payloadEnd);
	if (collection == CMM_CP_HITS) {
	  StatusCode sc = decodeCmmCp(subBlock, trigCmm);
	  if (sc.isFailure() && debug) {
	    log << MSG::DEBUG << "decodeCmmCp failed" << endreq;
	  }
        }
      } else {
        // CPM
        CpmSubBlock subBlock;
        payload = subBlock.read(payload, payloadEnd);
	if (collection == CPM_TOWERS || collection == CPM_HITS) {
	  StatusCode sc = decodeCpm(subBlock, trigCpm, collection);
	  if (sc.isFailure() && debug) {
	    log << MSG::DEBUG << "decodeCpm failed" << endreq;
	  }
        }
      }
    }
  }

  return StatusCode::SUCCESS;
}

// Unpack CMM-CP sub-block

StatusCode CpByteStreamTool::decodeCmmCp(CmmCpSubBlock& subBlock, int trigCmm)
{
  MsgStream log( msgSvc(), name() );
  const bool debug = msgSvc()->outputLevel(name()) <= MSG::DEBUG;

  const int hwCrate    = subBlock.crate();
  const int module     = subBlock.cmmPosition();
  const int summing    = subBlock.cmmSumming();
  const int timeslices = subBlock.timeslices();
  const int sliceNum   = subBlock.slice();
  if (debug) {
    log << MSG::DEBUG << "CMM-CP: Crate "  << hwCrate
                      << "  Module "       << module
		      << "  Summing "      << summing
                      << "  Total slices " << timeslices
                      << "  Slice "        << sliceNum    << endreq;
  }
  if (hwCrate < m_crateOffsetHw || hwCrate >= m_crateOffsetHw + m_crates) {
    log << MSG::ERROR << "Unexpected crate number: " << hwCrate << endreq;
    return StatusCode::FAILURE;
  }
  if (timeslices <= trigCmm) {
    log << MSG::DEBUG << "Triggered CMM slice from header "
        << "inconsistent with number of slices: "
        << trigCmm << ", " << timeslices << ", reset to 0" << endreq;
    trigCmm = 0;
    //return StatusCode::FAILURE;
  }
  if (timeslices <= sliceNum) {
    log << MSG::ERROR << "Total slices inconsistent with slice number: "
        << timeslices << ", " << sliceNum << endreq;
    return StatusCode::FAILURE;
  }
  // Unpack sub-block
  if (subBlock.dataWords() && !subBlock.unpack()) {
    log << MSG::DEBUG << "CMM-CP sub-block unpacking failed" << endreq;
    //return StatusCode::FAILURE;
  }

  // Retrieve required data

  const bool neutralFormat = subBlock.format() == L1CaloSubBlock::NEUTRAL;
  const int crate    = hwCrate - m_crateOffsetHw;
  const int swCrate  = crate   + m_crateOffsetSw;
  const int maxSid   = CmmCpSubBlock::MAX_SOURCE_ID;
  const int sliceBeg = ( neutralFormat ) ? 0          : sliceNum;
  const int sliceEnd = ( neutralFormat ) ? timeslices : sliceNum + 1;
  for (int slice = sliceBeg; slice < sliceEnd; ++slice) {

    // Jet hit counts

    for (int source = 1; source < maxSid; ++source) {
      int dataID = source;
      if (source > m_modules) {
        if (summing == CmmSubBlock::CRATE &&
	    source != CmmCpSubBlock::LOCAL) continue;
	switch (source) {
	  case CmmCpSubBlock::LOCAL:
	    dataID = LVL1::CMMCPHits::LOCAL;
	    break;
	  case CmmCpSubBlock::REMOTE_0:
	    dataID = LVL1::CMMCPHits::REMOTE_0;
	    break;
	  case CmmCpSubBlock::REMOTE_1:
	    dataID = LVL1::CMMCPHits::REMOTE_1;
	    break;
	  case CmmCpSubBlock::REMOTE_2:
	    dataID = LVL1::CMMCPHits::REMOTE_2;
	    break;
	  case CmmCpSubBlock::TOTAL:
	    dataID = LVL1::CMMCPHits::TOTAL;
	    break;
          default:
	    continue;
        }
      }
      const unsigned int hits = subBlock.hits(slice, source);
      int err = subBlock.hitsError(slice, source);
      LVL1::DataError errorBits;
      errorBits.set(LVL1::DataError::Parity, err & 0x1);
      errorBits.set(LVL1::DataError::SubStatusWord, subBlock.subStatus());
      err = errorBits.error();
      if (hits || err) {
        LVL1::CMMCPHits* ch = findCmmCpHits(crate, dataID);
	if ( ! ch ) {   // create new CMM hits
	  std::vector<unsigned int> hitsVec0(timeslices);
	  std::vector<unsigned int> hitsVec1(timeslices);
	  std::vector<int> errVec0(timeslices);
	  std::vector<int> errVec1(timeslices);
	  if (module == CmmSubBlock::RIGHT) {
	    hitsVec0[slice] = hits;
	    errVec0[slice]  = err;
	  } else {
	    hitsVec1[slice] = hits;
	    errVec1[slice]  = err;
	  }
	  ch = new LVL1::CMMCPHits(swCrate, dataID, hitsVec0, hitsVec1,
	                                            errVec0, errVec1, trigCmm);
          const int key = crate*100 + dataID;
	  m_cmmHitsMap.insert(std::make_pair(key, ch));
	  m_cmmHitCollection->push_back(ch);
        } else {
	  std::vector<unsigned int> hitsVec0(ch->HitsVec0());
	  std::vector<unsigned int> hitsVec1(ch->HitsVec1());
	  std::vector<int> errVec0(ch->ErrorVec0());
	  std::vector<int> errVec1(ch->ErrorVec1());
	  if (module == CmmSubBlock::RIGHT) {
	    hitsVec0[slice] = hits;
	    errVec0[slice]  = err;
	  } else {
	    hitsVec1[slice] = hits;
	    errVec1[slice]  = err;
	  }
	  ch->addHits(hitsVec0, hitsVec1, errVec0, errVec1);
        }
      }
    }
  }
  
  return StatusCode::SUCCESS;
}

// Unpack CPM sub-block

StatusCode CpByteStreamTool::decodeCpm(CpmSubBlock& subBlock,
                                 int trigCpm, const CollectionType collection)
{
  MsgStream log( msgSvc(), name() );
  const bool debug = msgSvc()->outputLevel(name()) <= MSG::DEBUG;

  const int hwCrate    = subBlock.crate();
  const int module     = subBlock.module();
  const int timeslices = subBlock.timeslices();
  const int sliceNum   = subBlock.slice();
  if (debug) {
    log << MSG::DEBUG << "CPM: Crate "     << hwCrate
                      << "  Module "       << module
                      << "  Total slices " << timeslices
                      << "  Slice "        << sliceNum    << endreq;
  }
  if (hwCrate < m_crateOffsetHw || hwCrate >= m_crateOffsetHw + m_crates) {
    log << MSG::ERROR << "Unexpected crate number: " << hwCrate << endreq;
    return StatusCode::FAILURE;
  }
  if (timeslices <= trigCpm) {
    log << MSG::DEBUG << "Triggered CPM slice from header "
        << "inconsistent with number of slices: "
        << trigCpm << ", " << timeslices << ", reset to 0" << endreq;
    trigCpm = 0;
    //return StatusCode::FAILURE;
  }
  if (timeslices <= sliceNum) {
    log << MSG::ERROR << "Total slices inconsistent with slice number: "
        << timeslices << ", " << sliceNum << endreq;
    return StatusCode::FAILURE;
  }
  // Unpack sub-block
  if (subBlock.dataWords() && !subBlock.unpack()) {
    log << MSG::DEBUG << "CPM sub-block unpacking failed" << endreq;
    //return StatusCode::FAILURE;
  }

  // Retrieve required data

  const bool neutralFormat = subBlock.format() == L1CaloSubBlock::NEUTRAL;
  const int crate    = hwCrate - m_crateOffsetHw;
  const int swCrate  = crate   + m_crateOffsetSw;
  const int sliceBeg = ( neutralFormat ) ? 0          : sliceNum;
  const int sliceEnd = ( neutralFormat ) ? timeslices : sliceNum + 1;
  for (int slice = sliceBeg; slice < sliceEnd; ++slice) {

    if (collection == CPM_TOWERS) {

      // Loop over tower channels and fill CPM towers

      for (int chan = 0; chan < m_channels; ++chan) {
	const int em  = subBlock.emData(slice, chan);
	const int had = subBlock.hadData(slice, chan);
	int emErr     = subBlock.emError(slice, chan);
	int hadErr    = subBlock.hadError(slice, chan);
	LVL1::DataError emErrBits;
	emErrBits.set(LVL1::DataError::Parity, emErr & 0x1);
	emErrBits.set(LVL1::DataError::LinkDown, (emErr >> 1) & 0x1);
	emErrBits.set(LVL1::DataError::SubStatusWord, subBlock.subStatus());
	int emErr1 = emErrBits.error();
	LVL1::DataError hadErrBits;
	hadErrBits.set(LVL1::DataError::Parity, hadErr & 0x1);
	hadErrBits.set(LVL1::DataError::LinkDown, (hadErr >> 1) & 0x1);
	hadErrBits.set(LVL1::DataError::SubStatusWord, subBlock.subStatus());
	int hadErr1 = hadErrBits.error();
        if (em || had || emErr1 || hadErr1) {
	  ChannelCoordinate coord;
	  int coreOrOverlap = m_cpmMaps->mapping(crate, module, chan, coord);
	  if (coreOrOverlap == m_coreOverlap) {
	    const double eta = coord.eta();
	    const double phi = coord.phi();
	    LVL1::CPMTower* tt = findCpmTower(eta, phi);
	    if ( ! tt ) {   // create new CPM tower
	      std::vector<int> emVec(timeslices);
	      std::vector<int> hadVec(timeslices);
	      std::vector<int> emErrVec(timeslices);
	      std::vector<int> hadErrVec(timeslices);
	      emVec[slice]     = em;
	      hadVec[slice]    = had;
	      emErrVec[slice]  = emErr1;
	      hadErrVec[slice] = hadErr1;
	      tt = new LVL1::CPMTower(phi, eta, emVec, emErrVec,
	                                        hadVec, hadErrVec, trigCpm);
	      const unsigned int key = m_towerKey->ttKey(phi, eta);
	      m_ttMap.insert(std::make_pair(key, tt));
	      m_ttCollection->push_back(tt);
            } else {
	      std::vector<int> emVec(tt->emEnergyVec());
	      std::vector<int> hadVec(tt->hadEnergyVec());
	      std::vector<int> emErrVec(tt->emErrorVec());
	      std::vector<int> hadErrVec(tt->hadErrorVec());
	      emVec[slice]     = em;
	      hadVec[slice]    = had;
	      emErrVec[slice]  = emErr1;
	      hadErrVec[slice] = hadErr1;
	      tt->fill(emVec, emErrVec, hadVec, hadErrVec, trigCpm);
	    }
          } else if (debug && (em || had || emErr || hadErr)
	                   && !coreOrOverlap) {
	    log << MSG::VERBOSE
	        << "Non-zero data but no channel mapping for channel "
	        << chan << endreq;
          }
        } else if (debug) {
	  log << MSG::VERBOSE << "No CPM tower data for channel "
	                      << chan << " slice " << slice << endreq;
        }
      }
    } else if (collection == CPM_HITS) {

      // Get CPM hits

      const unsigned int hits0 = subBlock.hits0(slice);
      const unsigned int hits1 = subBlock.hits1(slice);
      if (hits0 || hits1) {
        LVL1::CPMHits* ch = findCpmHits(crate, module);
	if ( ! ch ) {   // create new CPM hits
	  std::vector<unsigned int> hitsVec0(timeslices);
	  std::vector<unsigned int> hitsVec1(timeslices);
	  hitsVec0[slice] = hits0;
	  hitsVec1[slice] = hits1;
	  ch = new LVL1::CPMHits(swCrate, module, hitsVec0, hitsVec1, trigCpm);
	  m_hitsMap.insert(std::make_pair(crate*m_modules+module-1, ch));
	  m_hitCollection->push_back(ch);
        } else {
	  std::vector<unsigned int> hitsVec0(ch->HitsVec0());
	  std::vector<unsigned int> hitsVec1(ch->HitsVec1());
	  hitsVec0[slice] = hits0;
	  hitsVec1[slice] = hits1;
	  ch->addHits(hitsVec0, hitsVec1);
        }
      } else if (debug) {
        log << MSG::VERBOSE << "No CPM hits data for crate/module/slice "
                            << hwCrate << "/" << module << "/" << slice
   			    << endreq;
      }
    }
  }
  return StatusCode::SUCCESS;
}

// Find a CPM tower given eta, phi

LVL1::CPMTower* CpByteStreamTool::findCpmTower(const double eta,
                                               const double phi)
{
  LVL1::CPMTower* tt = 0;
  const unsigned int key = m_towerKey->ttKey(phi, eta);
  CpmTowerMap::const_iterator mapIter;
  mapIter = m_ttMap.find(key);
  if (mapIter != m_ttMap.end()) tt = mapIter->second;
  return tt;
}

// Find CPM hits for given crate, module

LVL1::CPMHits* CpByteStreamTool::findCpmHits(const int crate, const int module)
{
  LVL1::CPMHits* hits = 0;
  CpmHitsMap::const_iterator mapIter;
  mapIter = m_hitsMap.find(crate*m_modules + module - 1);
  if (mapIter != m_hitsMap.end()) hits = mapIter->second;
  return hits;
}

// Find CMM-CP hits for given crate, dataID

LVL1::CMMCPHits* CpByteStreamTool::findCmmCpHits(const int crate,
                                                 const int dataID)
{
  LVL1::CMMCPHits* hits = 0;
  CmmCpHitsMap::const_iterator mapIter;
  mapIter = m_cmmHitsMap.find(crate*100 + dataID);
  if (mapIter != m_cmmHitsMap.end()) hits = mapIter->second;
  return hits;
}

// Set up CPM tower map

void CpByteStreamTool::setupCpmTowerMap(const CpmTowerCollection*
                                                          const ttCollection)
{
  m_ttMap.clear();
  if (ttCollection) {
    CpmTowerCollection::const_iterator pos  = ttCollection->begin();
    CpmTowerCollection::const_iterator pose = ttCollection->end();
    for (; pos != pose; ++pos) {
      LVL1::CPMTower* const tt = *pos;
      const unsigned int key = m_towerKey->ttKey(tt->phi(), tt->eta());
      m_ttMap.insert(std::make_pair(key, tt));
    }
  }
}

// Set up CPM hits map

void CpByteStreamTool::setupCpmHitsMap(const CpmHitsCollection*
                                                        const hitCollection)
{
  m_hitsMap.clear();
  if (hitCollection) {
    CpmHitsCollection::const_iterator pos  = hitCollection->begin();
    CpmHitsCollection::const_iterator pose = hitCollection->end();
    for (; pos != pose; ++pos) {
      LVL1::CPMHits* const hits = *pos;
      const int crate = hits->crate() - m_crateOffsetSw;
      const int key   = m_modules * crate + hits->module() - 1;
      m_hitsMap.insert(std::make_pair(key, hits));
    }
  }
}

// Set up CMM-CP hits map

void CpByteStreamTool::setupCmmCpHitsMap(const CmmCpHitsCollection*
                                                          const hitCollection)
{
  m_cmmHitsMap.clear();
  if (hitCollection) {
    CmmCpHitsCollection::const_iterator pos  = hitCollection->begin();
    CmmCpHitsCollection::const_iterator pose = hitCollection->end();
    for (; pos != pose; ++pos) {
      LVL1::CMMCPHits* const hits = *pos;
      const int crate = hits->crate() - m_crateOffsetSw;
      const int key   = crate*100 + hits->dataID();
      m_cmmHitsMap.insert(std::make_pair(key, hits));
    }
  }
}

// Get number of slices and triggered slice offset for next slink

bool CpByteStreamTool::slinkSlices(const int crate, const int module,
                  const int modulesPerSlink, int& timeslices, int& trigCpm)
{
  int slices = -1;
  int trigC  = m_dfltSlicesCpm/2;
  for (int mod = module; mod < module + modulesPerSlink; ++mod) {
    for (int chan = 0; chan < m_channels; ++chan) {
      ChannelCoordinate coord;
      if ( !m_cpmMaps->mapping(crate, mod, chan, coord)) continue;
      const LVL1::CPMTower* const tt = findCpmTower(coord.phi(), coord.eta());
      if ( !tt ) continue;
      const int numdat = 4;
      std::vector<int> sums(numdat);
      std::vector<int> sizes(numdat);
      sums[0] = std::accumulate((tt->emEnergyVec()).begin(),
                                (tt->emEnergyVec()).end(), 0);
      sums[1] = std::accumulate((tt->hadEnergyVec()).begin(),
                                (tt->hadEnergyVec()).end(), 0);
      sums[2] = std::accumulate((tt->emErrorVec()).begin(),
                                (tt->emErrorVec()).end(), 0);
      sums[3] = std::accumulate((tt->hadErrorVec()).begin(),
                                (tt->hadErrorVec()).end(), 0);
      sizes[0] = (tt->emEnergyVec()).size();
      sizes[1] = (tt->hadEnergyVec()).size();
      sizes[2] = (tt->emErrorVec()).size();
      sizes[3] = (tt->hadErrorVec()).size();
      const int peak = tt->peak();
      for (int i = 0; i < numdat; ++i) {
        if (sums[i] == 0) continue;
        if (slices < 0) {
	  slices = sizes[i];
	  trigC  = peak;
	} else if (slices != sizes[i] || trigC != peak) return false;
      }
    }
    const LVL1::CPMHits* const hits = findCpmHits(crate, mod);
    if (hits) {
      const int numdat = 2;
      std::vector<unsigned int> sums(numdat);
      std::vector<int> sizes(numdat);
      sums[0] = std::accumulate((hits->HitsVec0()).begin(),
                                         (hits->HitsVec0()).end(), 0);
      sums[1] = std::accumulate((hits->HitsVec1()).begin(),
                                         (hits->HitsVec1()).end(), 0);
      sizes[0] = (hits->HitsVec0()).size();
      sizes[1] = (hits->HitsVec1()).size();
      const int peak = hits->peak();
      for (int i = 0; i < numdat; ++i) {
        if (sums[i] == 0) continue;
        if (slices < 0) {
	  slices = sizes[i];
	  trigC  = peak;
        } else if (slices != sizes[i] || trigC != peak) return false;
      }
    }
  }
  if (slices < 0) slices = m_dfltSlicesCpm;
  timeslices = slices;
  trigCpm    = trigC;
  return true;
}

// Get number of CMM slices and triggered slice offset for current crate

bool CpByteStreamTool::slinkSlicesCmm(const int crate, int& timeslices,
                                                       int& trigCmm)
{
  int slices = -1;
  int trigC  = m_dfltSlicesCmm/2;
  const int maxDataID = LVL1::CMMCPHits::MAXID;
  for (int dataID = 0; dataID < maxDataID; ++dataID) {
    const int numdat = 4;
    std::vector<unsigned int> sums(numdat);
    std::vector<int> sizes(numdat);
    const LVL1::CMMCPHits* const hits = findCmmCpHits(crate, dataID);
    if (hits) {
      sums[0] = std::accumulate((hits->HitsVec0()).begin(),
                                           (hits->HitsVec0()).end(), 0);
      sums[1] = std::accumulate((hits->HitsVec1()).begin(),
                                           (hits->HitsVec1()).end(), 0);
      sums[2] = std::accumulate((hits->ErrorVec0()).begin(),
                                           (hits->ErrorVec0()).end(), 0);
      sums[3] = std::accumulate((hits->ErrorVec1()).begin(),
                                           (hits->ErrorVec1()).end(), 0);
      sizes[0] = (hits->HitsVec0()).size();
      sizes[1] = (hits->HitsVec1()).size();
      sizes[2] = (hits->ErrorVec0()).size();
      sizes[3] = (hits->ErrorVec1()).size();
      const int peak = hits->peak();
      for (int i = 0; i < 2; ++i) {
        if (sums[i] == 0) continue;
        if (slices < 0) {
	  slices = sizes[i];
	  trigC  = peak;
        } else if (slices != sizes[i] || trigC != peak) return false;
      }
    }
  }
  if (slices < 0) slices = m_dfltSlicesCmm;
  timeslices = slices;
  trigCmm    = trigC;
  return true;
}

} // end namespace
