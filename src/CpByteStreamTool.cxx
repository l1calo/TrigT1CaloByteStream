
#include <numeric>
#include <utility>

#include "GaudiKernel/IInterface.h"

#include "TrigT1Calo/CMMCPHits.h"
#include "TrigT1Calo/CPMHits.h"
#include "TrigT1Calo/CPMTower.h"
#include "TrigT1Calo/CPBSCollection.h"
#include "TrigT1Calo/TriggerTowerKey.h"

#include "TrigT1CaloByteStream/ChannelCoordinate.h"
#include "TrigT1CaloByteStream/CmmCpSubBlock.h"
#include "TrigT1CaloByteStream/CmmSubBlock.h"
#include "TrigT1CaloByteStream/CpmCrateMappings.h"
#include "TrigT1CaloByteStream/CpmSubBlock.h"
#include "TrigT1CaloByteStream/CpByteStreamTool.h"
#include "TrigT1CaloByteStream/L1CaloSubBlock.h"
#include "TrigT1CaloByteStream/L1CaloUserHeader.h"

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
		    m_srcIdMap(0), m_cpmMaps(0), m_towerKey(0)
{
  declareInterface<CpByteStreamTool>(this);

  // Properties for writing bytestream only
  declareProperty("DataVersion",    m_version    = 1);
  declareProperty("DataFormat",     m_dataFormat = 1);
  declareProperty("SlinksPerCrate", m_slinks     = 2);

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
  return AlgTool::initialize();
}

// Finalize

StatusCode CpByteStreamTool::finalize()
{
  delete m_towerKey;
  delete m_cpmMaps;
  delete m_srcIdMap;
  return AlgTool::finalize();
}

// Conversion bytestream to CPM towers

StatusCode CpByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::CPMTower>* ttCollection)
{
  m_ttCollection = ttCollection;
  m_ttMap.clear();
  return convertBs(robFrags, CPM_TOWERS);
}

// Conversion bytestream to CPM hits

StatusCode CpByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::CPMHits>* hitCollection)
{
  m_hitCollection = hitCollection;
  m_hitsMap.clear();
  return convertBs(robFrags, CPM_HITS);
}

// Conversion bytestream to CMM-CP hits

StatusCode CpByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::CMMCPHits>* hitCollection)
{
  m_cmmHitCollection = hitCollection;
  m_cmmHitsMap.clear();
  return convertBs(robFrags, CMM_CP_HITS);
}

// Conversion of CP container to bytestream

StatusCode CpByteStreamTool::convert(const LVL1::CPBSCollection* cp,
                                           RawEventWrite* re)
{
  MsgStream log( msgSvc(), name() );
  bool debug = msgSvc()->outputLevel(name()) <= MSG::DEBUG;

  // Clear the event assembler

  m_fea.clear();
  uint16_t minorVersion = 0x1001;
  m_fea.setRodMinorVersion(minorVersion);

  // Pointer to ROD data vector

  FullEventAssembler<L1CaloSrcIdMap>::RODDATA* theROD = 0;

  // Set up the container maps

  setupCpmTowerMap(cp->towers());
  setupCpmHitsMap(cp->hits());
  setupCmmCpHitsMap(cp->cmmHits());

  // Loop over data

  int modulesPerSlink = m_modules / m_slinks;
  int timeslices = 0;
  int timeslicesCmm = 0;
  int trigCpm = 0;
  int trigCmm = 0;
  for (int crate=0; crate < m_crates; ++crate) {

    // Get CMM number of slices and triggered slice offset for this crate

    if ( ! slinkSlicesCmm(crate, timeslicesCmm, trigCmm)) {
      log << MSG::ERROR << "Inconsistent number of CMM slices or "
          << "triggered slice offsets in data for crate " << crate << endreq;
      return StatusCode::FAILURE;
    }

    // Unfortunately CPM modules are numbered 1 to m_modules
    for (int module=1; module <= m_modules; ++module) {
      int mod = module - 1;

      // Pack required number of modules per slink

      if (mod%modulesPerSlink == 0) {
	int daqOrRoi = 0;
	int slink = mod/modulesPerSlink;
        if (debug) {
          log << MSG::DEBUG << "Treating crate " << crate
                            << " slink " << slink << endreq;
        }
	// Get number of CPM slices and triggered slice offset
	// for this slink
	if ( ! slinkSlices(crate, module, modulesPerSlink,
	                                  timeslices, trigCpm)) {
	  log << MSG::ERROR << "Inconsistent number of slices or "
	      << "triggered slice offsets in data for crate "
	      << crate << " slink " << slink << endreq;
	  return StatusCode::FAILURE;
        }
        if (debug) {
	  log << MSG::DEBUG << "Data Version/Format: " << m_version
	                    << " " << m_dataFormat << endreq;
          log << MSG::DEBUG << "Slices/offset: " << timeslices
                            << " " << trigCpm << endreq;
        }
        L1CaloUserHeader userHeader;
        userHeader.setCpm(trigCpm);
        userHeader.setCpCmm(trigCmm);
	uint32_t rodIdCpm = m_srcIdMap->getRodID(crate, slink, daqOrRoi,
	                                                       m_subDetector);
	theROD = m_fea.getRodData(rodIdCpm);
	theROD->push_back(userHeader.header());
      }
      if (debug) {
        log << MSG::DEBUG << "Module " << module << endreq;
      }

      // Create a sub-block for each slice (except Neutral format)

      m_cpmBlocks.clear();
      for (int slice = 0; slice < timeslices; ++slice) {
        CpmSubBlock* subBlock = new CpmSubBlock();
	subBlock->setCpmHeader(m_version, m_dataFormat, slice,
	                       crate, module, timeslices);
        m_cpmBlocks.push_back(subBlock);
	if (m_dataFormat == L1CaloSubBlock::NEUTRAL) break;
      }

      // Find CPM towers corresponding to each eta/phi pair and fill
      // sub-blocks

      for (int chan=0; chan < m_channels; ++chan) {
	ChannelCoordinate coord;
	if (m_cpmMaps->mapping(crate, module, chan, coord)) {
	  double eta = coord.eta();
	  double phi = coord.phi();
          LVL1::CPMTower* tt = findCpmTower(eta, phi);
	  if (tt ) {
	    std::vector<int> emData(tt->emEnergyVec());
	    std::vector<int> hadData(tt->hadEnergyVec());
	    std::vector<int> emError(tt->emErrorVec());
	    std::vector<int> hadError(tt->hadErrorVec());
	    // Make sure all vectors are the right size - vectors with only
	    // zeroes are allowed to be different.  This has been checked
	    // already in slinkSlices.
	    emData.resize(timeslices);
	    hadData.resize(timeslices);
	    emError.resize(timeslices);
	    hadError.resize(timeslices);
            for (int slice = 0; slice < timeslices; ++slice) {
	      int index = slice;
	      if (m_dataFormat == L1CaloSubBlock::NEUTRAL) index = 0;
              CpmSubBlock* subBlock = m_cpmBlocks[index];
              subBlock->fillTowerData(slice, chan, emData[slice],
	                   hadData[slice], emError[slice], hadError[slice]);
	    }
          }
        }
      }

      // Add CPM hits

      LVL1::CPMHits* hits = findCpmHits(crate, module);
      if (hits) {
        std::vector<unsigned int> vec0(hits->HitsVec0());
        std::vector<unsigned int> vec1(hits->HitsVec1());
	vec0.resize(timeslices);
	vec1.resize(timeslices);
        for (int slice = 0; slice < timeslices; ++slice) {
	  int index = slice;
	  if (m_dataFormat == L1CaloSubBlock::NEUTRAL) index = 0;
	  CpmSubBlock* subBlock = m_cpmBlocks[index];
	  subBlock->setHits(slice, vec0[slice], vec1[slice]);
        }
      }
      
      // Pack and write the sub-blocks

      DataVector<CpmSubBlock>::const_iterator pos;
      for (pos = m_cpmBlocks.begin(); pos != m_cpmBlocks.end(); ++pos) {
        CpmSubBlock* subBlock = *pos;
	if ( !subBlock->pack()) {
	  log << MSG::ERROR << "CPM sub-block packing failed" << endreq;
	  return StatusCode::FAILURE;
	}
	subBlock->write(theROD);
      }
    }

    // Append CMMs to last S-Link of the crate

    // Create a sub-block for each slice (except Neutral format)

    m_cmmHit0Blocks.clear();
    m_cmmHit1Blocks.clear();
    int summing = CmmSubBlock::CRATE;
    if (crate == m_crates - 1) summing = CmmSubBlock::SYSTEM;
    for (int slice = 0; slice < timeslices; ++slice) {
      CmmCpSubBlock* h0Block = new CmmCpSubBlock();
      h0Block->setCmmHeader(m_version, m_dataFormat, slice, crate,
                            summing, CmmSubBlock::CMM_CP,
			    CmmSubBlock::RIGHT, timeslicesCmm);
      m_cmmHit0Blocks.push_back(h0Block);
      CmmCpSubBlock* h1Block = new CmmCpSubBlock();
      h1Block->setCmmHeader(m_version, m_dataFormat, slice, crate,
                            summing, CmmSubBlock::CMM_CP,
			    CmmSubBlock::LEFT, timeslicesCmm);
      m_cmmHit1Blocks.push_back(h1Block);
      if (m_dataFormat == L1CaloSubBlock::NEUTRAL) break;
    }

    // CMM-CP

    int maxDataID = LVL1::CMMCPHits::MAXID;
    for (int dataID = 0; dataID < maxDataID; ++dataID) {
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
      LVL1::CMMCPHits* ch = findCmmCpHits(crate, dataID);
      if ( ch ) {
        std::vector<unsigned int> hits0(ch->HitsVec0());
        std::vector<unsigned int> hits1(ch->HitsVec1());
        std::vector<int> err0(ch->ErrorVec0());
        std::vector<int> err1(ch->ErrorVec1());
	hits0.resize(timeslicesCmm);
	hits1.resize(timeslicesCmm);
	err0.resize(timeslicesCmm);
	err1.resize(timeslicesCmm);
	for (int slice = 0; slice < timeslicesCmm; ++slice) {
	  int index = slice;
	  if (m_dataFormat == L1CaloSubBlock::NEUTRAL) index = 0;
	  CmmCpSubBlock* subBlock = m_cmmHit0Blocks[index];
	  subBlock->setHits(slice, source, hits0[slice], err0[slice]);
	  subBlock = m_cmmHit1Blocks[index];
	  subBlock->setHits(slice, source, hits1[slice], err1[slice]);
        }
      }
    }
    DataVector<CmmCpSubBlock>::const_iterator cos;
    cos = m_cmmHit0Blocks.begin();
    for (; cos != m_cmmHit0Blocks.end(); ++cos) {
      CmmCpSubBlock* subBlock = *cos;
      if ( !subBlock->pack()) {
        log << MSG::ERROR << "CMM-Cp sub-block packing failed" << endreq;
	return StatusCode::FAILURE;
      }
      subBlock->write(theROD);
    }
    cos = m_cmmHit1Blocks.begin();
    for (; cos != m_cmmHit1Blocks.end(); ++cos) {
      CmmCpSubBlock* subBlock = *cos;
      if ( !subBlock->pack()) {
        log << MSG::ERROR << "CMM-Cp sub-block packing failed" << endreq;
	return StatusCode::FAILURE;
      }
      subBlock->write(theROD);
    }
  }

  // Fill the raw event

  m_fea.fill(re, log);

  return StatusCode::SUCCESS;
}

// Fill a vector with all possible Source Identifiers

void CpByteStreamTool::sourceIDs(std::vector<uint32_t>& vID) const
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

// Convert bytestream to given container type

StatusCode CpByteStreamTool::convertBs(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            CollectionType collection)
{
  MsgStream log( msgSvc(), name() );
  bool debug = msgSvc()->outputLevel(name()) <= MSG::DEBUG;

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
    uint32_t sourceID = (*rob)->rod_source_id();
    if (m_srcIdMap->subDet(sourceID)   != m_subDetector ||
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
    if (headerWords != 1 && debug) {
      log << MSG::DEBUG << "Unexpected number of user header words: "
          << headerWords << endreq;
    }
    for (int i = 0; i < headerWords; ++i) ++payload;
    // triggered slice offsets
    int trigCpm = userHeader.cpm();
    int trigCmm = userHeader.cpCmm();
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
	  if (sc.isFailure()) return sc;
        }
      } else {
        // CPM
        CpmSubBlock subBlock;
        payload = subBlock.read(payload, payloadEnd);
	if (collection == CPM_TOWERS || collection == CPM_HITS) {
	  StatusCode sc = decodeCpm(subBlock, trigCpm, collection);
	  if (sc.isFailure()) return sc;
        }
      }
    }
  }

  return StatusCode::SUCCESS;
}

// Unpack CMM-CP sub-block

StatusCode CpByteStreamTool::decodeCmmCp(CmmCpSubBlock& subBlock,
                                                           int trigCmm)
{
  MsgStream log( msgSvc(), name() );
  bool debug = msgSvc()->outputLevel(name()) <= MSG::DEBUG;

  int crate      = subBlock.crate();
  int module     = subBlock.cmmPosition();
  int summing    = subBlock.cmmSumming();
  int timeslices = subBlock.timeslices();
  int sliceNum   = subBlock.slice();
  if (debug) {
    log << MSG::DEBUG << "CMM-CP: Crate "  << crate
                      << "  Module "       << module
		      << "  Summing "      << summing
                      << "  Total slices " << timeslices
                      << "  Slice "        << sliceNum    << endreq;
  }
  if (timeslices <= trigCmm) {
    log << MSG::ERROR << "Triggered CMM slice from header "
        << "inconsistent with number of slices: "
        << trigCmm << ", " << timeslices << endreq;
    return StatusCode::FAILURE;
  }
  if (timeslices <= sliceNum) {
    log << MSG::ERROR << "Total slices inconsistent with slice number: "
        << timeslices << ", " << sliceNum << endreq;
    return StatusCode::FAILURE;
  }
  // Unpack sub-block
  if ( !subBlock.unpack()) {
    log << MSG::ERROR << "CMM-CP sub-block unpacking failed" << endreq;
    return StatusCode::FAILURE;
  }

  // Retrieve required data

  int sliceBeg = sliceNum;
  int sliceEnd = sliceNum + 1;
  if (subBlock.format() == L1CaloSubBlock::NEUTRAL) {
    sliceBeg = 0;
    sliceEnd = timeslices;
  }
  int maxSid = CmmCpSubBlock::MAX_SOURCE_ID;
  for (int slice = sliceBeg; slice < sliceEnd; ++slice) {

    // Jet hit counts

    for (int source = 0; source < maxSid; ++source) {
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
      unsigned int hits = subBlock.hits(slice, source);
      int err = subBlock.hitsError(slice, source);
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
	  ch = new LVL1::CMMCPHits(crate, dataID, hitsVec0, hitsVec1,
	                                          errVec0, errVec1, trigCmm);
          int key = crate*100 + dataID;
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

StatusCode CpByteStreamTool::decodeCpm(CpmSubBlock& subBlock, int trigCpm,
                                                 CollectionType collection)
{
  MsgStream log( msgSvc(), name() );
  bool debug = msgSvc()->outputLevel(name()) <= MSG::DEBUG;

  int crate      = subBlock.crate();
  int module     = subBlock.module();
  int timeslices = subBlock.timeslices();
  int sliceNum   = subBlock.slice();
  if (debug) {
    log << MSG::DEBUG << "CPM: Crate "     << crate
                      << "  Module "       << module
                      << "  Total slices " << timeslices
                      << "  Slice "        << sliceNum    << endreq;
  }
  if (timeslices <= trigCpm) {
    log << MSG::ERROR << "Triggered CPM slice from header "
        << "inconsistent with number of slices: "
        << trigCpm << ", " << timeslices << endreq;
    return StatusCode::FAILURE;
  }
  if (timeslices <= sliceNum) {
    log << MSG::ERROR << "Total slices inconsistent with slice number: "
        << timeslices << ", " << sliceNum << endreq;
    return StatusCode::FAILURE;
  }
  // Unpack sub-block
  if ( !subBlock.unpack()) {
    log << MSG::ERROR << "CPM sub-block unpacking failed" << endreq;
    return StatusCode::FAILURE;
  }

  // Retrieve required data

  int sliceBeg = sliceNum;
  int sliceEnd = sliceNum + 1;
  if (subBlock.format() == L1CaloSubBlock::NEUTRAL) {
    sliceBeg = 0;
    sliceEnd = timeslices;
  }
  for (int slice = sliceBeg; slice < sliceEnd; ++slice) {

    if (collection == CPM_TOWERS) {

      // Loop over tower channels and fill CPM towers

      for (int chan = 0; chan < m_channels; ++chan) {
	int em     = subBlock.emData(slice, chan);
	int had    = subBlock.hadData(slice, chan);
	int emErr  = subBlock.emError(slice, chan);
	int hadErr = subBlock.hadError(slice, chan);
        if (em || had || emErr || hadErr) {
	  ChannelCoordinate coord;
	  if (m_cpmMaps->mapping(crate, module, chan, coord)) {
	    double eta = coord.eta();
	    double phi = coord.phi();
	    LVL1::CPMTower* tt = findCpmTower(eta, phi);
	    if ( ! tt ) {   // create new CPM tower
	      std::vector<int> emVec(timeslices);
	      std::vector<int> hadVec(timeslices);
	      std::vector<int> emErrVec(timeslices);
	      std::vector<int> hadErrVec(timeslices);
	      emVec[slice]     = em;
	      hadVec[slice]    = had;
	      emErrVec[slice]  = emErr;
	      hadErrVec[slice] = hadErr;
	      tt = new LVL1::CPMTower(phi, eta, emVec, emErrVec,
	                                        hadVec, hadErrVec, trigCpm);
	      unsigned int key = m_towerKey->ttKey(phi, eta);
	      m_ttMap.insert(std::make_pair(key, tt));
	      m_ttCollection->push_back(tt);
            } else {
	      std::vector<int> emVec(tt->emEnergyVec());
	      std::vector<int> hadVec(tt->hadEnergyVec());
	      std::vector<int> emErrVec(tt->emErrorVec());
	      std::vector<int> hadErrVec(tt->hadErrorVec());
	      emVec[slice]     = em;
	      hadVec[slice]    = had;
	      emErrVec[slice]  = emErr;
	      hadErrVec[slice] = hadErr;
	      tt->fill(emVec, emErrVec, hadVec, hadErrVec, trigCpm);
	    }
          } else {
	    log << MSG::WARNING
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

      unsigned int hits0 = subBlock.hits0(slice);
      unsigned int hits1 = subBlock.hits1(slice);
      if (hits0 || hits1) {
        LVL1::CPMHits* ch = findCpmHits(crate, module);
	if ( ! ch ) {   // create new CPM hits
	  std::vector<unsigned int> hitsVec0(timeslices);
	  std::vector<unsigned int> hitsVec1(timeslices);
	  hitsVec0[slice] = hits0;
	  hitsVec1[slice] = hits1;
	  ch = new LVL1::CPMHits(crate, module, hitsVec0, hitsVec1, trigCpm);
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
                            << crate << "/" << module << "/" << slice
   			    << endreq;
      }
    }
  }
  return StatusCode::SUCCESS;
}

// Find a CPM tower given eta, phi

LVL1::CPMTower* CpByteStreamTool::findCpmTower(double eta, double phi)
{
  LVL1::CPMTower* tt = 0;
  unsigned int key = m_towerKey->ttKey(phi, eta);
  CpmTowerMap::const_iterator mapIter;
  mapIter = m_ttMap.find(key);
  if (mapIter != m_ttMap.end()) tt = mapIter->second;
  return tt;
}

// Find CPM hits for given crate, module

LVL1::CPMHits* CpByteStreamTool::findCpmHits(int crate, int module)
{
  LVL1::CPMHits* hits = 0;
  CpmHitsMap::const_iterator mapIter;
  mapIter = m_hitsMap.find(crate*m_modules + module - 1);
  if (mapIter != m_hitsMap.end()) hits = mapIter->second;
  return hits;
}

// Find CMM-CP hits for given crate, dataID

LVL1::CMMCPHits* CpByteStreamTool::findCmmCpHits(int crate, int dataID)
{
  LVL1::CMMCPHits* hits = 0;
  CmmCpHitsMap::const_iterator mapIter;
  mapIter = m_cmmHitsMap.find(crate*100 + dataID);
  if (mapIter != m_cmmHitsMap.end()) hits = mapIter->second;
  return hits;
}

// Set up CPM tower map

void CpByteStreamTool::setupCpmTowerMap(const CpmTowerCollection* ttCollection)
{
  m_ttMap.clear();
  if (ttCollection) {
    CpmTowerCollection::const_iterator pos  = ttCollection->begin();
    CpmTowerCollection::const_iterator pose = ttCollection->end();
    for (; pos != pose; ++pos) {
      LVL1::CPMTower* tt = *pos;
      unsigned int key = m_towerKey->ttKey(tt->phi(), tt->eta());
      m_ttMap.insert(std::make_pair(key, tt));
    }
  }
}

// Set up CPM hits map

void CpByteStreamTool::setupCpmHitsMap(const CpmHitsCollection* hitCollection)
{
  m_hitsMap.clear();
  if (hitCollection) {
    CpmHitsCollection::const_iterator pos  = hitCollection->begin();
    CpmHitsCollection::const_iterator pose = hitCollection->end();
    for (; pos != pose; ++pos) {
      LVL1::CPMHits* hits = *pos;
      int key = m_modules * hits->crate() + hits->module() - 1;
      m_hitsMap.insert(std::make_pair(key, hits));
    }
  }
}

// Set up CMM-CP hits map

void CpByteStreamTool::setupCmmCpHitsMap(const CmmCpHitsCollection*
                                                                hitCollection)
{
  m_cmmHitsMap.clear();
  if (hitCollection) {
    CmmCpHitsCollection::const_iterator pos  = hitCollection->begin();
    CmmCpHitsCollection::const_iterator pose = hitCollection->end();
    for (; pos != pose; ++pos) {
      LVL1::CMMCPHits* hits = *pos;
      int dataID = hits->dataID();
      int key = hits->crate()*100 + dataID;
      m_cmmHitsMap.insert(std::make_pair(key, hits));
    }
  }
}

// Get number of slices and triggered slice offset for next slink

bool CpByteStreamTool::slinkSlices(int crate, int module,
                        int modulesPerSlink, int& timeslices, int& trigCpm)
{
  int slices = -1;
  int trigC  =  0;
  for (int mod = module; mod < module + modulesPerSlink; ++mod) {
    for (int chan = 0; chan < m_channels; ++chan) {
      ChannelCoordinate coord;
      if ( !m_cpmMaps->mapping(crate, mod, chan, coord)) continue;
      LVL1::CPMTower* tt = findCpmTower(coord.phi(), coord.eta());
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
      int peak = tt->peak();
      for (int i = 0; i < numdat; ++i) {
        if (sums[i] == 0) continue;
        if (slices < 0) {
	  slices = sizes[i];
	  trigC  = peak;
	} else if (slices != sizes[i] || trigC != peak) return false;
      }
    }
    LVL1::CPMHits* hits = findCpmHits(crate, mod);
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
      int peak = hits->peak();
      for (int i = 0; i < numdat; ++i) {
        if (sums[i] == 0) continue;
        if (slices < 0) {
	  slices = sizes[i];
	  trigC  = peak;
        } else if (slices != sizes[i] || trigC != peak) return false;
      }
    }
  }
  if (slices < 0) slices = 1;
  timeslices = slices;
  trigCpm    = trigC;
  return true;
}

// Get number of CMM slices and triggered slice offset for current crate

bool CpByteStreamTool::slinkSlicesCmm(int crate, int& timeslices,
                                                 int& trigCmm)
{
  int slices = -1;
  int trigC  =  0;
  int maxDataID = LVL1::CMMCPHits::MAXID;
  for (int dataID = 0; dataID < maxDataID; ++dataID) {
    const int numdat = 4;
    std::vector<unsigned int> sums(numdat);
    std::vector<int> sizes(numdat);
    LVL1::CMMCPHits* hits = 0;
    hits = findCmmCpHits(crate, dataID);
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
      int peak = hits->peak();
      for (int i = 0; i < 2; ++i) {
        if (sums[i] == 0) continue;
        if (slices < 0) {
	  slices = sizes[i];
	  trigC  = peak;
        } else if (slices != sizes[i] || trigC != peak) return false;
      }
    }
  }
  if (slices < 0) slices = 1;
  timeslices = slices;
  trigCmm    = trigC;
  return true;
}
