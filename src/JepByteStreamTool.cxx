
#include <numeric>
#include <utility>

#include "GaudiKernel/IInterface.h"

#include "TrigT1Calo/CMMJetHits.h"
#include "TrigT1Calo/CMMEtSums.h"
#include "TrigT1Calo/JEMHits.h"
#include "TrigT1Calo/JEMEtSums.h"
#include "TrigT1Calo/JEPBSCollection.h"
#include "TrigT1Calo/JetElement.h"
#include "TrigT1Calo/JetElementKey.h"

#include "TrigT1CaloByteStream/ChannelCoordinate.h"
#include "TrigT1CaloByteStream/CmmEnergySubBlock.h"
#include "TrigT1CaloByteStream/CmmJetSubBlock.h"
#include "TrigT1CaloByteStream/CmmSubBlock.h"
#include "TrigT1CaloByteStream/JemCrateMappings.h"
#include "TrigT1CaloByteStream/JemSubBlock.h"
#include "TrigT1CaloByteStream/JepByteStreamTool.h"
#include "TrigT1CaloByteStream/L1CaloSubBlock.h"
#include "TrigT1CaloByteStream/L1CaloUserHeader.h"

// Interface ID

static const InterfaceID IID_IJepByteStreamTool("JepByteStreamTool", 1, 1);

const InterfaceID& JepByteStreamTool::interfaceID()
{
  return IID_IJepByteStreamTool;
}

// Constructor

JepByteStreamTool::JepByteStreamTool(const std::string& type,
                                     const std::string& name,
				     const IInterface*  parent)
                  : AlgTool(type, name, parent),
		    m_srcIdMap(0), m_jemMaps(0), m_elementKey(0)
{
  declareInterface<JepByteStreamTool>(this);

  declareProperty("CrateOffsetHw",  m_crateOffsetHw = 12);
  declareProperty("CrateOffsetSw",  m_crateOffsetSw = 0);

  // Properties for writing bytestream only
  declareProperty("DataVersion",    m_version       = 1);
  declareProperty("DataFormat",     m_dataFormat    = 1);
  declareProperty("SlinksPerCrate", m_slinks        = 4);

}

// Destructor

JepByteStreamTool::~JepByteStreamTool()
{
}

// Initialize

StatusCode JepByteStreamTool::initialize()
{
  m_subDetector = eformat::TDAQ_CALO_JET_PROC_DAQ;
  m_srcIdMap    = new L1CaloSrcIdMap();
  m_jemMaps     = new JemCrateMappings();
  m_crates      = m_jemMaps->crates();
  m_modules     = m_jemMaps->modules();
  m_channels    = m_jemMaps->channels();
  m_elementKey  = new LVL1::JetElementKey();
  return AlgTool::initialize();
}

// Finalize

StatusCode JepByteStreamTool::finalize()
{
  delete m_elementKey;
  delete m_jemMaps;
  delete m_srcIdMap;
  return AlgTool::finalize();
}

// Conversion bytestream to jet elements

StatusCode JepByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::JetElement>* const jeCollection)
{
  m_jeCollection = jeCollection;
  m_jeMap.clear();
  return convertBs(robFrags, JET_ELEMENTS);
}

// Conversion bytestream to jet hits

StatusCode JepByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::JEMHits>* const hitCollection)
{
  m_hitCollection = hitCollection;
  m_hitsMap.clear();
  return convertBs(robFrags, JET_HITS);
}

// Conversion bytestream to energy sums

StatusCode JepByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::JEMEtSums>* const etCollection)
{
  m_etCollection = etCollection;
  m_etMap.clear();
  return convertBs(robFrags, ENERGY_SUMS);
}

// Conversion bytestream to CMM hits

StatusCode JepByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::CMMJetHits>* const hitCollection)
{
  m_cmmHitCollection = hitCollection;
  m_cmmHitsMap.clear();
  return convertBs(robFrags, CMM_HITS);
}

// Conversion bytestream to CMM energy sums

StatusCode JepByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::CMMEtSums>* const etCollection)
{
  m_cmmEtCollection = etCollection;
  m_cmmEtMap.clear();
  return convertBs(robFrags, CMM_SUMS);
}

// Conversion of JEP container to bytestream

StatusCode JepByteStreamTool::convert(const LVL1::JEPBSCollection* const jep,
                                            RawEventWrite* const re)
{
  MsgStream log( msgSvc(), name() );
  const bool debug = msgSvc()->outputLevel(name()) <= MSG::DEBUG;

  // Clear the event assembler

  m_fea.clear();
  const uint16_t minorVersion = 0x1001;
  m_fea.setRodMinorVersion(minorVersion);

  // Pointer to ROD data vector

  FullEventAssembler<L1CaloSrcIdMap>::RODDATA* theROD = 0;

  // Set up the container maps

  setupJeMap(jep->JetElements());
  setupHitsMap(jep->JetHits());
  setupEtMap(jep->EnergySums());
  setupCmmHitsMap(jep->CmmHits());
  setupCmmEtMap(jep->CmmSums());

  // Loop over data

  const bool neutralFormat = m_dataFormat == L1CaloSubBlock::NEUTRAL;
  const int modulesPerSlink = m_modules / m_slinks;
  int timeslices = 0;
  int timeslicesCmm = 0;
  int trigJem = 0;
  int trigCmm = 0;
  for (int crate=0; crate < m_crates; ++crate) {
    const int hwCrate = crate + m_crateOffsetHw;

    // Get CMM number of slices and triggered slice offset for this crate

    if ( ! slinkSlicesCmm(crate, timeslicesCmm, trigCmm)) {
      log << MSG::ERROR << "Inconsistent number of CMM slices or "
          << "triggered slice offsets in data for crate " << hwCrate << endreq;
      return StatusCode::FAILURE;
    }

    for (int module=0; module < m_modules; ++module) {

      // Pack required number of modules per slink

      if (module%modulesPerSlink == 0) {
	const int daqOrRoi = 0;
	const int slink = module/modulesPerSlink;
        if (debug) {
          log << MSG::DEBUG << "Treating crate " << hwCrate
                            << " slink " << slink << endreq;
        }
	// Get number of JEM slices and triggered slice offset
	// for this slink
	if ( ! slinkSlices(crate, module, modulesPerSlink,
	                                  timeslices, trigJem)) {
	  log << MSG::ERROR << "Inconsistent number of slices or "
	      << "triggered slice offsets in data for crate "
	      << hwCrate << " slink " << slink << endreq;
	  return StatusCode::FAILURE;
        }
        if (debug) {
	  log << MSG::DEBUG << "Data Version/Format: " << m_version
	                    << " " << m_dataFormat << endreq;
          log << MSG::DEBUG << "Slices/offset: " << timeslices
                            << " " << trigJem << endreq;
        }
        L1CaloUserHeader userHeader;
        userHeader.setJem(trigJem);
        userHeader.setJepCmm(trigCmm);
	const uint32_t rodIdJem = m_srcIdMap->getRodID(hwCrate, slink, daqOrRoi,
	                                                        m_subDetector);
	theROD = m_fea.getRodData(rodIdJem);
	theROD->push_back(userHeader.header());
      }
      if (debug) {
        log << MSG::DEBUG << "Module " << module << endreq;
      }

      // Create a sub-block for each slice (except Neutral format)

      m_jemBlocks.clear();
      for (int slice = 0; slice < timeslices; ++slice) {
        JemSubBlock* const subBlock = new JemSubBlock();
	subBlock->setJemHeader(m_version, m_dataFormat, slice,
	                       hwCrate, module, timeslices);
        m_jemBlocks.push_back(subBlock);
	if (neutralFormat) break;
      }

      // Find jet elements corresponding to each eta/phi pair and fill
      // sub-blocks

      for (int chan=0; chan < m_channels; ++chan) {
	ChannelCoordinate coord;
	if (m_jemMaps->mapping(crate, module, chan, coord)) {
	  const double eta = coord.eta();
	  const double phi = coord.phi();
          const LVL1::JetElement* const je = findJetElement(eta, phi);
	  if (je ) {
	    std::vector<int> emData(je->emEnergyVec());
	    std::vector<int> hadData(je->hadEnergyVec());
	    std::vector<int> emParity(je->emErrorVec());
	    std::vector<int> hadParity(je->hadErrorVec());
	    std::vector<int> linkError(je->linkErrorVec());
	    // Make sure all vectors are the right size - vectors with only
	    // zeroes are allowed to be different.  This has been checked
	    // already in slinkSlices.
	    emData.resize(timeslices);
	    hadData.resize(timeslices);
	    emParity.resize(timeslices);
	    hadParity.resize(timeslices);
	    linkError.resize(timeslices);
            for (int slice = 0; slice < timeslices; ++slice) {
	      const int index = ( neutralFormat ) ? 0 : slice;
              JemSubBlock* const subBlock = m_jemBlocks[index];
	      const JemJetElement jetEle(chan, emData[slice], hadData[slice],
	                  emParity[slice], hadParity[slice], linkError[slice]);
              subBlock->fillJetElement(slice, jetEle);
	    }
          }
        }
      }

      // Add jet hits and energy subsums

      const LVL1::JEMHits* const hits = findJetHits(crate, module);
      if (hits) {
        std::vector<unsigned int> vec(hits->JetHitsVec());
	vec.resize(timeslices);
        for (int slice = 0; slice < timeslices; ++slice) {
	  const int index = ( neutralFormat ) ? 0 : slice;
	  JemSubBlock* const subBlock = m_jemBlocks[index];
	  subBlock->setJetHits(slice, vec[slice]);
        }
      }
      const LVL1::JEMEtSums* const et = findEnergySums(crate, module);
      if (et) {
        std::vector<unsigned int> exVec(et->ExVec());
        std::vector<unsigned int> eyVec(et->EyVec());
        std::vector<unsigned int> etVec(et->EtVec());
	exVec.resize(timeslices);
	eyVec.resize(timeslices);
	etVec.resize(timeslices);
	for (int slice = 0; slice < timeslices; ++slice) {
	  const int index = ( neutralFormat ) ? 0 : slice;
	  JemSubBlock* const subBlock = m_jemBlocks[index];
	  subBlock->setEnergySubsums(slice, exVec[slice], eyVec[slice],
	                                                  etVec[slice]);
        }
      }
      
      // Pack and write the sub-blocks

      DataVector<JemSubBlock>::const_iterator pos;
      for (pos = m_jemBlocks.begin(); pos != m_jemBlocks.end(); ++pos) {
        JemSubBlock* const subBlock = *pos;
	if ( !subBlock->pack()) {
	  log << MSG::ERROR << "JEM sub-block packing failed" << endreq;
	  return StatusCode::FAILURE;
	}
	if (debug) {
	  log << MSG::DEBUG << "JEM sub-block data words: "
	                    << subBlock->dataWords() << endreq;
	}
	subBlock->write(theROD);
      }
    }

    // Append CMMs to last S-Link of the crate

    // Create a sub-block for each slice (except Neutral format)

    m_cmmEnergyBlocks.clear();
    m_cmmJetBlocks.clear();
    const int summing = (crate == m_crates - 1) ? CmmSubBlock::SYSTEM
                                                : CmmSubBlock::CRATE;
    for (int slice = 0; slice < timeslices; ++slice) {
      CmmEnergySubBlock* const enBlock = new CmmEnergySubBlock();
      enBlock->setCmmHeader(m_version, m_dataFormat, slice, hwCrate,
                            summing, CmmSubBlock::CMM_ENERGY,
			    CmmSubBlock::LEFT, timeslicesCmm);
      m_cmmEnergyBlocks.push_back(enBlock);
      CmmJetSubBlock* const jetBlock = new CmmJetSubBlock();
      jetBlock->setCmmHeader(m_version, m_dataFormat, slice, hwCrate,
                             summing, CmmSubBlock::CMM_JET,
			     CmmSubBlock::RIGHT, timeslicesCmm);
      m_cmmJetBlocks.push_back(jetBlock);
      if (neutralFormat) break;
    }

    // CMM-Energy

    int maxDataID = LVL1::CMMEtSums::MAXID;
    for (int dataID = 0; dataID < maxDataID; ++dataID) {
      int source = dataID;
      if (dataID >= m_modules) {
        if (summing == CmmSubBlock::CRATE && 
	    dataID != LVL1::CMMEtSums::LOCAL) continue;
	switch (dataID) {
	  case LVL1::CMMEtSums::LOCAL:
	    source = CmmEnergySubBlock::LOCAL;
	    break;
	  case LVL1::CMMEtSums::REMOTE:
	    source = CmmEnergySubBlock::REMOTE;
	    break;
	  case LVL1::CMMEtSums::TOTAL:
	    source = CmmEnergySubBlock::TOTAL;
	    break;
	  case LVL1::CMMEtSums::MISSING_ET_MAP:
	  case LVL1::CMMEtSums::SUM_ET_MAP:
	    break;
          default:
	    continue;
        }
      }
      const LVL1::CMMEtSums* const sums = findCmmSums(crate, dataID);
      if ( sums ) {
        std::vector<unsigned int> ex(sums->ExVec());
        std::vector<unsigned int> ey(sums->EyVec());
        std::vector<unsigned int> et(sums->EtVec());
        std::vector<int> exErr(sums->ExErrorVec());
        std::vector<int> eyErr(sums->EyErrorVec());
        std::vector<int> etErr(sums->EtErrorVec());
	ex.resize(timeslicesCmm);
	ey.resize(timeslicesCmm);
	et.resize(timeslicesCmm);
	exErr.resize(timeslicesCmm);
	eyErr.resize(timeslicesCmm);
	etErr.resize(timeslicesCmm);
	for (int slice = 0; slice < timeslicesCmm; ++slice) {
	  const int index = ( neutralFormat ) ? 0 : slice;
	  CmmEnergySubBlock* const subBlock = m_cmmEnergyBlocks[index];
	  if (dataID == LVL1::CMMEtSums::MISSING_ET_MAP) {
	    subBlock->setMissingEtHits(slice, et[slice]); 
          } else if (dataID == LVL1::CMMEtSums::SUM_ET_MAP) {
	    subBlock->setSumEtHits(slice, et[slice]); 
          } else {
	    subBlock->setSubsums(slice, source,
	                         ex[slice], ey[slice], et[slice],
	                         exErr[slice], eyErr[slice], etErr[slice]);
          }
        }
      }
    }
    DataVector<CmmEnergySubBlock>::const_iterator pos;
    pos = m_cmmEnergyBlocks.begin();
    for (; pos != m_cmmEnergyBlocks.end(); ++pos) {
      CmmEnergySubBlock* const subBlock = *pos;
      if ( !subBlock->pack()) {
        log << MSG::ERROR << "CMM-Energy sub-block packing failed" << endreq;
	return StatusCode::FAILURE;
      }
      if (debug) {
	log << MSG::DEBUG << "CMM-Energy sub-block data words: "
	                  << subBlock->dataWords() << endreq;
      }
      subBlock->write(theROD);
    }

    // CMM-Jet

    maxDataID = LVL1::CMMJetHits::MAXID;
    for (int dataID = 0; dataID < maxDataID; ++dataID) {
      int source = dataID;
      if (dataID >= m_modules) {
        if (summing == CmmSubBlock::CRATE && 
	    dataID != LVL1::CMMJetHits::LOCAL_MAIN &&
	    dataID != LVL1::CMMJetHits::LOCAL_FORWARD) continue;
	switch (dataID) {
	  case LVL1::CMMJetHits::LOCAL_MAIN:
	    source = CmmJetSubBlock::LOCAL_MAIN;
	    break;
	  case LVL1::CMMJetHits::REMOTE_MAIN:
	    source = CmmJetSubBlock::REMOTE_MAIN;
	    break;
	  case LVL1::CMMJetHits::TOTAL_MAIN:
	    source = CmmJetSubBlock::TOTAL_MAIN;
	    break;
	  case LVL1::CMMJetHits::LOCAL_FORWARD:
	    source = CmmJetSubBlock::LOCAL_FORWARD;
	    break;
	  case LVL1::CMMJetHits::REMOTE_FORWARD:
	    source = CmmJetSubBlock::REMOTE_FORWARD;
	    break;
	  case LVL1::CMMJetHits::TOTAL_FORWARD:
	    source = CmmJetSubBlock::TOTAL_FORWARD;
	    break;
	  case LVL1::CMMJetHits::ET_MAP:
	    break;
          default:
	    continue;
        }
      }
      const LVL1::CMMJetHits* const ch = findCmmHits(crate, dataID);
      if ( ch ) {
        std::vector<unsigned int> hits(ch->HitsVec());
        std::vector<int> errs(ch->ErrorVec());
	hits.resize(timeslicesCmm);
	errs.resize(timeslicesCmm);
	for (int slice = 0; slice < timeslicesCmm; ++slice) {
	  const int index = ( neutralFormat ) ? 0 : slice;
	  CmmJetSubBlock* const subBlock = m_cmmJetBlocks[index];
	  if (dataID == LVL1::CMMJetHits::ET_MAP) {
	    subBlock->setJetEtMap(slice, hits[slice]);
          } else {
	    subBlock->setJetHits(slice, source, hits[slice], errs[slice]);
          }
        }
      }
    }
    DataVector<CmmJetSubBlock>::const_iterator jos;
    jos = m_cmmJetBlocks.begin();
    for (; jos != m_cmmJetBlocks.end(); ++jos) {
      CmmJetSubBlock* const subBlock = *jos;
      if ( !subBlock->pack()) {
        log << MSG::ERROR << "CMM-Jet sub-block packing failed" << endreq;
	return StatusCode::FAILURE;
      }
      if (debug) {
	log << MSG::DEBUG << "CMM-Jet sub-block data words: "
	                  << subBlock->dataWords() << endreq;
      }
      subBlock->write(theROD);
    }
  }

  // Fill the raw event

  m_fea.fill(re, log);

  return StatusCode::SUCCESS;
}

// Fill a vector with all possible Source Identifiers

void JepByteStreamTool::sourceIDs(std::vector<uint32_t>& vID) const
{
  const int maxCrates = m_crates + m_crateOffsetHw;
  const int maxSlinks = m_srcIdMap->maxSlinks();
  for (int hwCrate = m_crateOffsetHw; hwCrate < maxCrates; ++hwCrate) {
    for (int slink = 0; slink < maxSlinks; ++slink) {
      const int daqOrRoi = 0;
      const uint32_t rodId = m_srcIdMap->getRodID(hwCrate, slink, daqOrRoi,
                                                           m_subDetector);
      const uint32_t robId = m_srcIdMap->getRobID(rodId);
      vID.push_back(robId);
    }
  }
}

// Convert bytestream to given container type

StatusCode JepByteStreamTool::convertBs(
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
    if (m_srcIdMap->subDet(sourceID)   != m_subDetector ||
        m_srcIdMap->daqOrRoi(sourceID) != 0) {
      log << MSG::ERROR << "Wrong source identifier in data" << endreq;
      return StatusCode::FAILURE;
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
    const int trigJem = userHeader.jem();
    const int trigCmm = userHeader.jepCmm();
    if (debug) {
      log << MSG::DEBUG << "JEM triggered slice offset: " << trigJem
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
        // CMMs
	if (CmmSubBlock::cmmType(*payload) == CmmSubBlock::CMM_JET) {
          CmmJetSubBlock subBlock;
          payload = subBlock.read(payload, payloadEnd);
	  if (collection == CMM_HITS) {
	    StatusCode sc = decodeCmmJet(subBlock, trigCmm);
	    if (sc.isFailure()) return sc;
          }
        } else {
	  CmmEnergySubBlock subBlock;
	  payload = subBlock.read(payload, payloadEnd);
	  if (collection == CMM_SUMS) {
	    StatusCode sc = decodeCmmEnergy(subBlock, trigCmm);
	    if (sc.isFailure()) return sc;
          }
	}
      } else {
        // JEM
        JemSubBlock subBlock;
        payload = subBlock.read(payload, payloadEnd);
	if (collection == JET_ELEMENTS || collection == JET_HITS ||
	                                  collection == ENERGY_SUMS) {
	  StatusCode sc = decodeJem(subBlock, trigJem, collection);
	  if (sc.isFailure()) return sc;
        }
      }
    }
  }

  return StatusCode::SUCCESS;
}

// Unpack CMM-Energy sub-block

StatusCode JepByteStreamTool::decodeCmmEnergy(CmmEnergySubBlock& subBlock,
                                                        const int trigCmm)
{
  MsgStream log( msgSvc(), name() );
  const bool debug = msgSvc()->outputLevel(name()) <= MSG::DEBUG;

  const int hwCrate    = subBlock.crate();
  const int module     = subBlock.cmmPosition();
  const int summing    = subBlock.cmmSumming();
  const int timeslices = subBlock.timeslices();
  const int sliceNum   = subBlock.slice();
  if (debug) {
    log << MSG::DEBUG << "CMM-Energy: Crate " << hwCrate
                      << "  Module "          << module
		      << "  Summing "         << summing
                      << "  Total slices "    << timeslices
                      << "  Slice "           << sliceNum    << endreq;
  }
  if (hwCrate < m_crateOffsetHw || hwCrate >= m_crateOffsetHw + m_crates) {
    log << MSG::ERROR << "Unexpected crate number: " << hwCrate << endreq;
    return StatusCode::FAILURE;
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
    log << MSG::ERROR << "CMM-Energy sub-block unpacking failed" << endreq;
    return StatusCode::FAILURE;
  }

  // Retrieve required data

  const bool neutralFormat = subBlock.format() == L1CaloSubBlock::NEUTRAL;
  const int crate    = hwCrate - m_crateOffsetHw;
  const int swCrate  = crate   + m_crateOffsetSw;
  const int maxSid   = CmmEnergySubBlock::MAX_SOURCE_ID;
  const int sliceBeg = ( neutralFormat ) ? 0          : sliceNum;
  const int sliceEnd = ( neutralFormat ) ? timeslices : sliceNum + 1;
  for (int slice = sliceBeg; slice < sliceEnd; ++slice) {

    // Energy sums

    for (int source = 0; source < maxSid; ++source) {
      int dataID = source;
      if (source >= m_modules) {
        if (summing == CmmSubBlock::CRATE &&
	    source != CmmEnergySubBlock::LOCAL) continue;
	switch (source) {
	  case CmmEnergySubBlock::LOCAL:
	    dataID = LVL1::CMMEtSums::LOCAL;
	    break;
          case CmmEnergySubBlock::REMOTE:
	    dataID = LVL1::CMMEtSums::REMOTE;
	    break;
          case CmmEnergySubBlock::TOTAL:
	    dataID = LVL1::CMMEtSums::TOTAL;
	    break;
          default:
	    continue;
        }
      }
      const unsigned int ex = subBlock.ex(slice, source);
      const unsigned int ey = subBlock.ey(slice, source);
      const unsigned int et = subBlock.et(slice, source);
      const int exErr = subBlock.exError(slice, source);
      const int eyErr = subBlock.eyError(slice, source);
      const int etErr = subBlock.etError(slice, source);
      if (ex || ey || et || exErr || eyErr || etErr) {
        LVL1::CMMEtSums* sums = findCmmSums(crate, dataID);
	if ( ! sums ) {   // create new CMM energy sums
	  std::vector<unsigned int> exVec(timeslices);
	  std::vector<unsigned int> eyVec(timeslices);
	  std::vector<unsigned int> etVec(timeslices);
	  std::vector<int> exErrVec(timeslices);
	  std::vector<int> eyErrVec(timeslices);
	  std::vector<int> etErrVec(timeslices);
	  exVec[slice] = ex;
	  eyVec[slice] = ey;
	  etVec[slice] = et;
	  exErrVec[slice] = exErr;
	  eyErrVec[slice] = eyErr;
	  etErrVec[slice] = etErr;
	  sums = new LVL1::CMMEtSums(swCrate, dataID, etVec, exVec, eyVec,
				     etErrVec, exErrVec, eyErrVec, trigCmm);
          const int key = crate*100 + dataID;
	  m_cmmEtMap.insert(std::make_pair(key, sums));
	  m_cmmEtCollection->push_back(sums);
        } else {
	  std::vector<unsigned int> exVec(sums->ExVec());
	  std::vector<unsigned int> eyVec(sums->EyVec());
	  std::vector<unsigned int> etVec(sums->EtVec());
	  std::vector<int> exErrVec(sums->ExErrorVec());
	  std::vector<int> eyErrVec(sums->EyErrorVec());
	  std::vector<int> etErrVec(sums->EtErrorVec());
	  exVec[slice] = ex;
	  eyVec[slice] = ey;
	  etVec[slice] = et;
	  exErrVec[slice] = exErr;
	  eyErrVec[slice] = eyErr;
	  etErrVec[slice] = etErr;
	  sums->addEx(exVec, exErrVec);
	  sums->addEy(eyVec, eyErrVec);
	  sums->addEt(etVec, etErrVec);
        }
      }
    }

    // Hit maps - store as Et with no error

    if (summing == CmmSubBlock::SYSTEM) {
      const unsigned int missEt = subBlock.missingEtHits(slice);
      if ( missEt ) {
        const int dataID = LVL1::CMMEtSums::MISSING_ET_MAP;
        LVL1::CMMEtSums* map = findCmmSums(crate, dataID);
        if ( ! map ) {
          std::vector<unsigned int> missVec(timeslices);
          const std::vector<int> errVec(timeslices);
	  missVec[slice] = missEt;
	  map = new LVL1::CMMEtSums(swCrate, dataID,
	                            missVec, missVec, missVec,
	  			    errVec, errVec, errVec, trigCmm);
          const int key = crate*100 + dataID;
	  m_cmmEtMap.insert(std::make_pair(key, map));
	  m_cmmEtCollection->push_back(map);
        } else {
          std::vector<unsigned int> missVec(map->EtVec());
          const std::vector<int> errVec(map->EtErrorVec());
	  missVec[slice] = missEt;
	  map->addEt(missVec, errVec);
        }
      }
      const unsigned int sumEt = subBlock.sumEtHits(slice);
      if ( sumEt ) {
        const int dataID = LVL1::CMMEtSums::SUM_ET_MAP;
        LVL1::CMMEtSums* map = findCmmSums(crate, dataID);
        if ( ! map ) {
          std::vector<unsigned int> sumVec(timeslices);
          const std::vector<int> errVec(timeslices);
	  sumVec[slice] = sumEt;
	  map = new LVL1::CMMEtSums(swCrate, dataID,
	                            sumVec, sumVec, sumVec,
				    errVec, errVec, errVec, trigCmm);
          const int key = crate*100 + dataID;
	  m_cmmEtMap.insert(std::make_pair(key, map));
	  m_cmmEtCollection->push_back(map);
        } else {
          std::vector<unsigned int> sumVec(map->EtVec());
          const std::vector<int> errVec(map->EtErrorVec());
	  sumVec[slice] = sumEt;
	  map->addEt(sumVec, errVec);
        }
      }
    }
  }
  
  return StatusCode::SUCCESS;
}

// Unpack CMM-Jet sub-block

StatusCode JepByteStreamTool::decodeCmmJet(CmmJetSubBlock& subBlock,
                                                     const int trigCmm)
{
  MsgStream log( msgSvc(), name() );
  const bool debug = msgSvc()->outputLevel(name()) <= MSG::DEBUG;

  const int hwCrate    = subBlock.crate();
  const int module     = subBlock.cmmPosition();
  const int summing    = subBlock.cmmSumming();
  const int timeslices = subBlock.timeslices();
  const int sliceNum   = subBlock.slice();
  if (debug) {
    log << MSG::DEBUG << "CMM-Jet: Crate " << hwCrate
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
    log << MSG::ERROR << "CMM-Jet sub-block unpacking failed" << endreq;
    return StatusCode::FAILURE;
  }

  // Retrieve required data

  const bool neutralFormat = subBlock.format() == L1CaloSubBlock::NEUTRAL;
  const int crate    = hwCrate - m_crateOffsetHw;
  const int swCrate  = crate   + m_crateOffsetSw;
  const int maxSid   = CmmJetSubBlock::MAX_SOURCE_ID;
  const int sliceBeg = ( neutralFormat ) ? 0          : sliceNum;
  const int sliceEnd = ( neutralFormat ) ? timeslices : sliceNum + 1;
  for (int slice = sliceBeg; slice < sliceEnd; ++slice) {

    // Jet hit counts

    for (int source = 0; source < maxSid; ++source) {
      int dataID = source;
      if (source >= m_modules) {
        if (summing == CmmSubBlock::CRATE &&
	    source != CmmJetSubBlock::LOCAL_MAIN     &&
	    source != CmmJetSubBlock::LOCAL_FORWARD) continue;
	switch (source) {
	  case CmmJetSubBlock::LOCAL_MAIN:
	    dataID = LVL1::CMMJetHits::LOCAL_MAIN;
	    break;
	  case CmmJetSubBlock::REMOTE_MAIN:
	    dataID = LVL1::CMMJetHits::REMOTE_MAIN;
	    break;
	  case CmmJetSubBlock::TOTAL_MAIN:
	    dataID = LVL1::CMMJetHits::TOTAL_MAIN;
	    break;
	  case CmmJetSubBlock::LOCAL_FORWARD:
	    dataID = LVL1::CMMJetHits::LOCAL_FORWARD;
	    break;
	  case CmmJetSubBlock::REMOTE_FORWARD:
	    dataID = LVL1::CMMJetHits::REMOTE_FORWARD;
	    break;
	  case CmmJetSubBlock::TOTAL_FORWARD:
	    dataID = LVL1::CMMJetHits::TOTAL_FORWARD;
	    break;
          default:
	    continue;
        }
      }
      const unsigned int hits = subBlock.jetHits(slice, source);
      const int err = subBlock.jetHitsError(slice, source);
      if (hits || err) {
        LVL1::CMMJetHits* jh = findCmmHits(crate, dataID);
	if ( ! jh ) {   // create new CMM hits
	  std::vector<unsigned int> hitsVec(timeslices);
	  std::vector<int> errVec(timeslices);
	  hitsVec[slice] = hits;
	  errVec[slice]  = err;
	  jh = new LVL1::CMMJetHits(swCrate, dataID, hitsVec, errVec, trigCmm);
          const int key = crate*100 + dataID;
	  m_cmmHitsMap.insert(std::make_pair(key, jh));
	  m_cmmHitCollection->push_back(jh);
        } else {
	  std::vector<unsigned int> hitsVec(jh->HitsVec());
	  std::vector<int> errVec(jh->ErrorVec());
	  hitsVec[slice] = hits;
	  errVec[slice]  = err;
	  jh->addHits(hitsVec, errVec);
        }
      }
    }

    // Hit map - store as hits with no error

    if (summing == CmmSubBlock::SYSTEM) {
      const unsigned int etMap = subBlock.jetEtMap(slice);
      if ( etMap ) {
        const int dataID = LVL1::CMMJetHits::ET_MAP;
        LVL1::CMMJetHits* map = findCmmHits(crate, dataID);
        if ( ! map ) {
          std::vector<unsigned int> mapVec(timeslices);
          const std::vector<int> errVec(timeslices);
	  mapVec[slice] = etMap;
	  map = new LVL1::CMMJetHits(swCrate, dataID, mapVec, errVec, trigCmm);
          const int key = crate*100 + dataID;
	  m_cmmHitsMap.insert(std::make_pair(key, map));
	  m_cmmHitCollection->push_back(map);
        } else {
          std::vector<unsigned int> mapVec(map->HitsVec());
          const std::vector<int> errVec(map->ErrorVec());
	  mapVec[slice] = etMap;
	  map->addHits(mapVec, errVec);
        }
      }
    }
  }
  
  return StatusCode::SUCCESS;
}

// Unpack JEM sub-block

StatusCode JepByteStreamTool::decodeJem(JemSubBlock& subBlock,
                                        const int trigJem,
                                        const CollectionType collection)
{
  MsgStream log( msgSvc(), name() );
  const bool debug = msgSvc()->outputLevel(name()) <= MSG::DEBUG;

  const int hwCrate    = subBlock.crate();
  const int module     = subBlock.module();
  const int timeslices = subBlock.timeslices();
  const int sliceNum   = subBlock.slice();
  if (debug) {
    log << MSG::DEBUG << "JEM: Crate "     << hwCrate
                      << "  Module "       << module
                      << "  Total slices " << timeslices
                      << "  Slice "        << sliceNum    << endreq;
  }
  if (hwCrate < m_crateOffsetHw || hwCrate >= m_crateOffsetHw + m_crates) {
    log << MSG::ERROR << "Unexpected crate number: " << hwCrate << endreq;
    return StatusCode::FAILURE;
  }
  if (timeslices <= trigJem) {
    log << MSG::ERROR << "Triggered JEM slice from header "
        << "inconsistent with number of slices: "
        << trigJem << ", " << timeslices << endreq;
    return StatusCode::FAILURE;
  }
  if (timeslices <= sliceNum) {
    log << MSG::ERROR << "Total slices inconsistent with slice number: "
        << timeslices << ", " << sliceNum << endreq;
    return StatusCode::FAILURE;
  }
  // Unpack sub-block
  if ( !subBlock.unpack()) {
    log << MSG::ERROR << "JEM sub-block unpacking failed" << endreq;
    return StatusCode::FAILURE;
  }

  // Retrieve required data

  const bool neutralFormat = subBlock.format() == L1CaloSubBlock::NEUTRAL;
  const int crate    = hwCrate - m_crateOffsetHw;
  const int swCrate  = crate   + m_crateOffsetSw;
  const int sliceBeg = ( neutralFormat ) ? 0          : sliceNum;
  const int sliceEnd = ( neutralFormat ) ? timeslices : sliceNum + 1;
  for (int slice = sliceBeg; slice < sliceEnd; ++slice) {

    if (collection == JET_ELEMENTS) {

      // Loop over jet element channels and fill jet elements

      for (int chan = 0; chan < m_channels; ++chan) {
        const JemJetElement jetEle(subBlock.jetElement(slice, chan));
        if (jetEle.data()) {
	  ChannelCoordinate coord;
	  if (m_jemMaps->mapping(crate, module, chan, coord)) {
	    const double eta = coord.eta();
	    const double phi = coord.phi();
	    LVL1::JetElement* je = findJetElement(eta, phi);
	    if ( ! je ) {   // create new jet element
	      const unsigned int key = m_elementKey->jeKey(phi, eta);
	      const std::vector<int> dummy(timeslices);
	      je = new LVL1::JetElement(phi, eta, dummy, dummy, key,
	                                dummy, dummy, dummy, trigJem);
	      m_jeMap.insert(std::make_pair(key, je));
	      m_jeCollection->push_back(je);
            }
	    je->addSlice(slice, jetEle.emData(), jetEle.hadData(),
	                        jetEle.emParity(), jetEle.hadParity(),
	         	        jetEle.linkError());
          } else {
	    log << MSG::WARNING
	        << "Non-zero data but no channel mapping for channel "
	        << chan << endreq;
          }
        } else if (debug) {
	  log << MSG::VERBOSE << "No jet element data for channel "
	                      << chan << " slice " << slice << endreq;
        }
      }
    } else if (collection == JET_HITS) {

      // Get jet hits

      const unsigned int hits = subBlock.jetHits(slice);
      if (hits) {
        LVL1::JEMHits* jh = findJetHits(crate, module);
	if ( ! jh ) {   // create new jet hits
	  std::vector<unsigned int> hitsVec(timeslices);
	  hitsVec[slice] = hits;
	  jh = new LVL1::JEMHits(swCrate, module, hitsVec, trigJem);
	  m_hitsMap.insert(std::make_pair(crate*m_modules+module, jh));
	  m_hitCollection->push_back(jh);
        } else {
	  std::vector<unsigned int> hitsVec(jh->JetHitsVec());
	  hitsVec[slice] = hits;
	  jh->addJetHits(hitsVec);
        }
      } else if (debug) {
        log << MSG::VERBOSE << "No jet hits data for crate/module/slice "
                            << hwCrate << "/" << module << "/" << slice
   			    << endreq;
      }
    } else if (collection == ENERGY_SUMS) {

      // Get energy subsums

      const unsigned int ex = subBlock.ex(slice);
      const unsigned int ey = subBlock.ey(slice);
      const unsigned int et = subBlock.et(slice);
      if (ex | ey | et) {
	LVL1::JEMEtSums* sums = findEnergySums(crate, module);
	if ( ! sums ) {   // create new energy sums
	  std::vector<unsigned int> exVec(timeslices);
	  std::vector<unsigned int> eyVec(timeslices);
	  std::vector<unsigned int> etVec(timeslices);
	  exVec[slice] = ex;
	  eyVec[slice] = ey;
	  etVec[slice] = et;
	  sums = new LVL1::JEMEtSums(swCrate, module, etVec, exVec, eyVec,
	                                                            trigJem);
          m_etMap.insert(std::make_pair(crate*m_modules+module, sums));
	  m_etCollection->push_back(sums);
        } else {
	  std::vector<unsigned int> exVec(sums->ExVec());
	  std::vector<unsigned int> eyVec(sums->EyVec());
	  std::vector<unsigned int> etVec(sums->EtVec());
	  exVec[slice] = ex;
	  eyVec[slice] = ey;
	  etVec[slice] = et;
	  sums->addEx(exVec);
	  sums->addEy(eyVec);
	  sums->addEt(etVec);
        }
      } else if (debug) {
        log << MSG::VERBOSE << "No energy sums data for crate/module/slice "
                            << hwCrate << "/" << module << "/" << slice
    			    << endreq;
      }
    }
  }
  return StatusCode::SUCCESS;
}

// Find a jet element given eta, phi

LVL1::JetElement* JepByteStreamTool::findJetElement(const double eta,
                                                    const double phi)
{
  LVL1::JetElement* tt = 0;
  const unsigned int key = m_elementKey->jeKey(phi, eta);
  JetElementMap::const_iterator mapIter;
  mapIter = m_jeMap.find(key);
  if (mapIter != m_jeMap.end()) tt = mapIter->second;
  return tt;
}

// Find jet hits for given crate, module

LVL1::JEMHits* JepByteStreamTool::findJetHits(const int crate,
                                              const int module)
{
  LVL1::JEMHits* hits = 0;
  JetHitsMap::const_iterator mapIter;
  mapIter = m_hitsMap.find(crate*m_modules + module);
  if (mapIter != m_hitsMap.end()) hits = mapIter->second;
  return hits;
}

// Find energy sums for given crate, module

LVL1::JEMEtSums* JepByteStreamTool::findEnergySums(const int crate,
                                                   const int module)
{
  LVL1::JEMEtSums* sums = 0;
  EnergySumsMap::const_iterator mapIter;
  mapIter = m_etMap.find(crate*m_modules + module);
  if (mapIter != m_etMap.end()) sums = mapIter->second;
  return sums;
}

// Find CMM hits for given crate, dataID

LVL1::CMMJetHits* JepByteStreamTool::findCmmHits(const int crate,
                                                 const int dataID)
{
  LVL1::CMMJetHits* hits = 0;
  CmmHitsMap::const_iterator mapIter;
  mapIter = m_cmmHitsMap.find(crate*100 + dataID);
  if (mapIter != m_cmmHitsMap.end()) hits = mapIter->second;
  return hits;
}

// Find CMM energy sums for given crate, module, dataID

LVL1::CMMEtSums* JepByteStreamTool::findCmmSums(const int crate,
                                                const int dataID)
{
  LVL1::CMMEtSums* sums = 0;
  CmmSumsMap::const_iterator mapIter;
  mapIter = m_cmmEtMap.find(crate*100 + dataID);
  if (mapIter != m_cmmEtMap.end()) sums = mapIter->second;
  return sums;
}

// Set up jet element map

void JepByteStreamTool::setupJeMap(const JetElementCollection*
                                                        const jeCollection)
{
  m_jeMap.clear();
  if (jeCollection) {
    JetElementCollection::const_iterator pos  = jeCollection->begin();
    JetElementCollection::const_iterator pose = jeCollection->end();
    for (; pos != pose; ++pos) {
      LVL1::JetElement* const je = *pos;
      const unsigned int key = m_elementKey->jeKey(je->phi(), je->eta());
      m_jeMap.insert(std::make_pair(key, je));
    }
  }
}

// Set up jet hits map

void JepByteStreamTool::setupHitsMap(const JetHitsCollection*
                                                        const hitCollection)
{
  m_hitsMap.clear();
  if (hitCollection) {
    JetHitsCollection::const_iterator pos  = hitCollection->begin();
    JetHitsCollection::const_iterator pose = hitCollection->end();
    for (; pos != pose; ++pos) {
      LVL1::JEMHits* const hits = *pos;
      const int crate = hits->crate() - m_crateOffsetSw;
      const int key   = m_modules * crate + hits->module();
      m_hitsMap.insert(std::make_pair(key, hits));
    }
  }
}

// Set up energy sums map

void JepByteStreamTool::setupEtMap(const EnergySumsCollection*
                                                         const etCollection)
{
  m_etMap.clear();
  if (etCollection) {
    EnergySumsCollection::const_iterator pos  = etCollection->begin();
    EnergySumsCollection::const_iterator pose = etCollection->end();
    for (; pos != pose; ++pos) {
      LVL1::JEMEtSums* const sums = *pos;
      const int crate = sums->crate() - m_crateOffsetSw;
      const int key   = m_modules * crate + sums->module();
      m_etMap.insert(std::make_pair(key, sums));
    }
  }
}

// Set up CMM hits map

void JepByteStreamTool::setupCmmHitsMap(const CmmHitsCollection*
                                                         const hitCollection)
{
  m_cmmHitsMap.clear();
  if (hitCollection) {
    CmmHitsCollection::const_iterator pos  = hitCollection->begin();
    CmmHitsCollection::const_iterator pose = hitCollection->end();
    for (; pos != pose; ++pos) {
      LVL1::CMMJetHits* const hits = *pos;
      const int crate = hits->crate() - m_crateOffsetSw;
      const int key   = crate*100 + hits->dataID();
      m_cmmHitsMap.insert(std::make_pair(key, hits));
    }
  }
}

// Set up CMM energy sums map

void JepByteStreamTool::setupCmmEtMap(const CmmSumsCollection*
                                                         const etCollection)
{
  m_cmmEtMap.clear();
  if (etCollection) {
    CmmSumsCollection::const_iterator pos  = etCollection->begin();
    CmmSumsCollection::const_iterator pose = etCollection->end();
    for (; pos != pose; ++pos) {
      LVL1::CMMEtSums* const sums = *pos;
      const int crate = sums->crate() - m_crateOffsetSw;
      const int key   = crate*100 + sums->dataID();
      m_cmmEtMap.insert(std::make_pair(key, sums));
    }
  }
}

// Get number of slices and triggered slice offset for next slink

bool JepByteStreamTool::slinkSlices(const int crate, const int module,
                  const int modulesPerSlink, int& timeslices, int& trigJem)
{
  int slices = -1;
  int trigJ  =  0;
  for (int mod = module; mod < module + modulesPerSlink; ++mod) {
    for (int chan = 0; chan < m_channels; ++chan) {
      ChannelCoordinate coord;
      if ( !m_jemMaps->mapping(crate, mod, chan, coord)) continue;
      const LVL1::JetElement* const je = findJetElement(coord.phi(),
                                                        coord.eta());
      if ( !je ) continue;
      const int numdat = 5;
      std::vector<int> sums(numdat);
      std::vector<int> sizes(numdat);
      sums[0] = std::accumulate((je->emEnergyVec()).begin(),
                                (je->emEnergyVec()).end(), 0);
      sums[1] = std::accumulate((je->hadEnergyVec()).begin(),
                                (je->hadEnergyVec()).end(), 0);
      sums[2] = std::accumulate((je->emErrorVec()).begin(),
                                (je->emErrorVec()).end(), 0);
      sums[3] = std::accumulate((je->hadErrorVec()).begin(),
                                (je->hadErrorVec()).end(), 0);
      sums[4] = std::accumulate((je->linkErrorVec()).begin(),
                                (je->linkErrorVec()).end(), 0);
      sizes[0] = (je->emEnergyVec()).size();
      sizes[1] = (je->hadEnergyVec()).size();
      sizes[2] = (je->emErrorVec()).size();
      sizes[3] = (je->hadErrorVec()).size();
      sizes[4] = (je->linkErrorVec()).size();
      const int peak = je->peak();
      for (int i = 0; i < numdat; ++i) {
        if (sums[i] == 0) continue;
        if (slices < 0) {
	  slices = sizes[i];
	  trigJ  = peak;
	} else if (slices != sizes[i] || trigJ != peak) return false;
      }
    }
    const LVL1::JEMHits* const hits = findJetHits(crate, mod);
    if (hits) {
      const unsigned int sum = std::accumulate((hits->JetHitsVec()).begin(),
                                               (hits->JetHitsVec()).end(), 0);
      if (sum) {
        const int size = (hits->JetHitsVec()).size();
	const int peak = hits->peak();
        if (slices < 0) {
	  slices = size;
	  trigJ  = peak;
        } else if (slices != size || trigJ != peak) return false;
      }
    }
    const LVL1::JEMEtSums* const et = findEnergySums(crate, mod);
    if (et) {
      const int numdat = 3;
      std::vector<unsigned int> sums(numdat);
      std::vector<int> sizes(numdat);
      sums[0] = std::accumulate((et->ExVec()).begin(),
                                (et->ExVec()).end(), 0);
      sums[1] = std::accumulate((et->EyVec()).begin(),
                                (et->EyVec()).end(), 0);
      sums[2] = std::accumulate((et->EtVec()).begin(),
                                (et->EtVec()).end(), 0);
      sizes[0] = (et->ExVec()).size();
      sizes[1] = (et->EyVec()).size();
      sizes[2] = (et->EtVec()).size();
      const int peak = et->peak();
      for (int i = 0; i < numdat; ++i) {
        if (sums[i] == 0) continue;
	if (slices < 0) {
	  slices = sizes[i];
	  trigJ  = peak;
        } else if (slices != sizes[i] || trigJ != peak) return false;
      }
    }
  }
  if (slices < 0) slices = 1;
  timeslices = slices;
  trigJem    = trigJ;
  return true;
}

// Get number of CMM slices and triggered slice offset for current crate

bool JepByteStreamTool::slinkSlicesCmm(const int crate, int& timeslices,
                                                        int& trigCmm)
{
  int slices = -1;
  int trigC  =  0;
  const int maxDataID1 = LVL1::CMMJetHits::MAXID;
  const int maxDataID2 = LVL1::CMMEtSums::MAXID;
  const int maxDataID  = (maxDataID1 > maxDataID2) ? maxDataID1 : maxDataID2;
  for (int dataID = 0; dataID < maxDataID; ++dataID) {
    const int numdat = 6;
    std::vector<unsigned int> sums(numdat);
    std::vector<int> sizes(numdat);
    const LVL1::CMMJetHits* hits = 0;
    if (dataID < maxDataID1) hits = findCmmHits(crate, dataID);
    if (hits) {
      sums[0] = std::accumulate((hits->HitsVec()).begin(),
                                           (hits->HitsVec()).end(), 0);
      sums[1] = std::accumulate((hits->ErrorVec()).begin(),
                                           (hits->ErrorVec()).end(), 0);
      sizes[0] = (hits->HitsVec()).size();
      sizes[1] = (hits->ErrorVec()).size();
      const int peak = hits->peak();
      for (int i = 0; i < 2; ++i) {
        if (sums[i] == 0) continue;
        if (slices < 0) {
	  slices = sizes[i];
	  trigC  = peak;
        } else if (slices != sizes[i] || trigC != peak) return false;
      }
    }
    const LVL1::CMMEtSums* et = 0;
    if (dataID < maxDataID2) et = findCmmSums(crate, dataID);
    if (et) {
      sums[0] = std::accumulate((et->ExVec()).begin(),
				(et->ExVec()).end(), 0);
      sums[1] = std::accumulate((et->EyVec()).begin(),
                                (et->EyVec()).end(), 0);
      sums[2] = std::accumulate((et->EtVec()).begin(),
                                (et->EtVec()).end(), 0);
      sums[3] = std::accumulate((et->ExErrorVec()).begin(),
                                (et->ExErrorVec()).end(), 0);
      sums[4] = std::accumulate((et->EyErrorVec()).begin(),
                                (et->EyErrorVec()).end(), 0);
      sums[5] = std::accumulate((et->EtErrorVec()).begin(),
                                (et->EtErrorVec()).end(), 0);
      sizes[0] = (et->ExVec()).size();
      sizes[1] = (et->EyVec()).size();
      sizes[2] = (et->EtVec()).size();
      sizes[3] = (et->ExErrorVec()).size();
      sizes[4] = (et->EyErrorVec()).size();
      sizes[5] = (et->EtErrorVec()).size();
      const int peak = et->peak();
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
  trigCmm    = trigC;
  return true;
}
