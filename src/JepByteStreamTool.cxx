
#include <numeric>
#include <utility>

#include "GaudiKernel/IInterface.h"

#include "TrigT1Calo/CMMJetHits.h"
#include "TrigT1Calo/CMMEtSums.h"
#include "TrigT1Calo/DataError.h"
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
#include "TrigT1CaloByteStream/L1CaloRodStatus.h"
#include "TrigT1CaloByteStream/L1CaloSubBlock.h"
#include "TrigT1CaloByteStream/L1CaloUserHeader.h"
#include "TrigT1CaloByteStream/ModifySlices.h"

namespace LVL1BS {

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
		    m_coreOverlap(1),
		    m_srcIdMap(0), m_jemMaps(0), m_elementKey(0),
		    m_rodStatus(0)
{
  declareInterface<JepByteStreamTool>(this);

  declareProperty("CrateOffsetHw",  m_crateOffsetHw  = 12,
                  "Offset of JEP crate numbers in bytestream");
  declareProperty("CrateOffsetSw",  m_crateOffsetSw  = 0,
                  "Offset of JEP crate numbers in RDOs");

  // Properties for reading bytestream only
  declareProperty("ROBSourceIDs",       m_sourceIDs,
                  "ROB fragment source identifiers");

  // Properties for writing bytestream only
  declareProperty("DataVersion",    m_version        = 1,
                  "Format version number in sub-block header");
  declareProperty("DataFormat",     m_dataFormat     = 1,
                  "Format identifier (0-1) in sub-block header");
  declareProperty("SlinksPerCrate", m_slinks         = 4,
                  "The number of S-Links per crate");
  declareProperty("SimulSlicesJEM", m_dfltSlicesJem  = 1,
                  "The number of JEM slices in the simulation");
  declareProperty("SimulSlicesCMM", m_dfltSlicesCmm  = 1,
                  "The number of CMM slices in the simulation");
  declareProperty("ForceSlicesJEM", m_forceSlicesJem = 0,
                  "If >0, the number of JEM slices in bytestream");
  declareProperty("ForceSlicesCMM", m_forceSlicesCmm = 0,
                  "If >0, the number of CMM slices in bytestream");

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
  m_rodStatus   = new std::vector<uint32_t>(2);
  return AlgTool::initialize();
}

// Finalize

StatusCode JepByteStreamTool::finalize()
{
  delete m_rodStatus;
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
  m_rodStatusMap.clear();

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
  int timeslices       = 1;
  int timeslicesCmm    = 1;
  int trigJem          = 0;
  int trigCmm          = 0;
  int timeslicesNew    = 1;
  int timeslicesCmmNew = 1;
  int trigJemNew       = 0;
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
	timeslicesNew = (m_forceSlicesJem) ? m_forceSlicesJem : timeslices;
	trigJemNew    = ModifySlices::peak(trigJem, timeslices, timeslicesNew);
        if (debug) {
	  log << MSG::DEBUG << "Data Version/Format: " << m_version
	                    << " " << m_dataFormat << endreq;
          log << MSG::DEBUG << "Slices/offset: " << timeslices
                            << " " << trigJem;
	  if (timeslices != timeslicesNew) {
	    log << MSG::DEBUG << " modified to " << timeslicesNew
	                      << " " << trigJemNew;
          }
	  log << MSG::DEBUG << endreq;
        }
        L1CaloUserHeader userHeader;
        userHeader.setJem(trigJemNew);
        userHeader.setJepCmm(trigCmmNew);
	const uint32_t rodIdJem = m_srcIdMap->getRodID(hwCrate, slink, daqOrRoi,
	                                                        m_subDetector);
	theROD = m_fea.getRodData(rodIdJem);
	theROD->push_back(userHeader.header());
	m_rodStatusMap.insert(make_pair(rodIdJem, m_rodStatus));
      }
      if (debug) {
        log << MSG::DEBUG << "Module " << module << endreq;
      }

      // Create a sub-block for each slice (except Neutral format)

      m_jemBlocks.clear();
      for (int slice = 0; slice < timeslicesNew; ++slice) {
        JemSubBlock* const subBlock = new JemSubBlock();
	subBlock->setJemHeader(m_version, m_dataFormat, slice,
	                       hwCrate, module, timeslicesNew);
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
	    std::vector<int> emData;
	    std::vector<int> hadData;
	    std::vector<int> emErrors;
	    std::vector<int> hadErrors;
	    ModifySlices::data(je->emEnergyVec(),  emData,    timeslicesNew);
	    ModifySlices::data(je->hadEnergyVec(), hadData,   timeslicesNew);
	    ModifySlices::data(je->emErrorVec(),   emErrors,  timeslicesNew);
	    ModifySlices::data(je->hadErrorVec(),  hadErrors, timeslicesNew);
            for (int slice = 0; slice < timeslicesNew; ++slice) {
	      const LVL1::DataError emErrBits(emErrors[slice]);
	      const LVL1::DataError hadErrBits(hadErrors[slice]);
	      const int index = ( neutralFormat ) ? 0 : slice;
              JemSubBlock* const subBlock = m_jemBlocks[index];
	      const JemJetElement jetEle(chan, emData[slice], hadData[slice],
	                  emErrBits.get(LVL1::DataError::Parity),
	                  hadErrBits.get(LVL1::DataError::Parity),
			  emErrBits.get(LVL1::DataError::LinkDown) +
			 (hadErrBits.get(LVL1::DataError::LinkDown) << 1));
              subBlock->fillJetElement(slice, jetEle);
	    }
          }
        }
      }

      // Add jet hits and energy subsums

      const LVL1::JEMHits* const hits = findJetHits(crate, module);
      if (hits) {
        std::vector<unsigned int> vec;
	ModifySlices::data(hits->JetHitsVec(), vec, timeslicesNew);
        for (int slice = 0; slice < timeslicesNew; ++slice) {
	  const int index = ( neutralFormat ) ? 0 : slice;
	  JemSubBlock* const subBlock = m_jemBlocks[index];
	  subBlock->setJetHits(slice, vec[slice]);
        }
      }
      const LVL1::JEMEtSums* const et = findEnergySums(crate, module);
      if (et) {
        std::vector<unsigned int> exVec;
        std::vector<unsigned int> eyVec;
        std::vector<unsigned int> etVec;
	ModifySlices::data(et->ExVec(), exVec, timeslicesNew);
	ModifySlices::data(et->EyVec(), eyVec, timeslicesNew);
	ModifySlices::data(et->EtVec(), etVec, timeslicesNew);
	for (int slice = 0; slice < timeslicesNew; ++slice) {
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
    for (int slice = 0; slice < timeslicesCmmNew; ++slice) {
      CmmEnergySubBlock* const enBlock = new CmmEnergySubBlock();
      enBlock->setCmmHeader(m_version, m_dataFormat, slice, hwCrate,
                            summing, CmmSubBlock::CMM_ENERGY,
			    CmmSubBlock::LEFT, timeslicesCmmNew);
      m_cmmEnergyBlocks.push_back(enBlock);
      CmmJetSubBlock* const jetBlock = new CmmJetSubBlock();
      jetBlock->setCmmHeader(m_version, m_dataFormat, slice, hwCrate,
                             summing, CmmSubBlock::CMM_JET,
			     CmmSubBlock::RIGHT, timeslicesCmmNew);
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
        std::vector<unsigned int> ex;
        std::vector<unsigned int> ey;
        std::vector<unsigned int> et;
        std::vector<int> exErr;
        std::vector<int> eyErr;
        std::vector<int> etErr;
	ModifySlices::data(sums->ExVec(), ex, timeslicesCmmNew);
	ModifySlices::data(sums->EyVec(), ey, timeslicesCmmNew);
	ModifySlices::data(sums->EtVec(), et, timeslicesCmmNew);
	ModifySlices::data(sums->ExErrorVec(), exErr, timeslicesCmmNew);
	ModifySlices::data(sums->EyErrorVec(), eyErr, timeslicesCmmNew);
	ModifySlices::data(sums->EtErrorVec(), etErr, timeslicesCmmNew);
	for (int slice = 0; slice < timeslicesCmmNew; ++slice) {
	  const LVL1::DataError exErrBits(exErr[slice]);
	  const LVL1::DataError eyErrBits(eyErr[slice]);
	  const LVL1::DataError etErrBits(etErr[slice]);
	  int exError = exErrBits.get(LVL1::DataError::Parity);
	  int eyError = eyErrBits.get(LVL1::DataError::Parity);
	  int etError = etErrBits.get(LVL1::DataError::Parity);
	  if (dataID == LVL1::CMMEtSums::LOCAL ||
	      dataID == LVL1::CMMEtSums::REMOTE ||
	      dataID == LVL1::CMMEtSums::TOTAL) {
	    exError = (exError << 1) + exErrBits.get(LVL1::DataError::Overflow);
	    eyError = (eyError << 1) + eyErrBits.get(LVL1::DataError::Overflow);
	    etError = (etError << 1) + etErrBits.get(LVL1::DataError::Overflow);
	  }
	  const int index = ( neutralFormat ) ? 0 : slice;
	  CmmEnergySubBlock* const subBlock = m_cmmEnergyBlocks[index];
	  if (dataID == LVL1::CMMEtSums::MISSING_ET_MAP) {
	    subBlock->setMissingEtHits(slice, et[slice]); 
          } else if (dataID == LVL1::CMMEtSums::SUM_ET_MAP) {
	    subBlock->setSumEtHits(slice, et[slice]); 
          } else {
	    subBlock->setSubsums(slice, source,
	                         ex[slice], ey[slice], et[slice],
	                         exError, eyError, etError);
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
        std::vector<unsigned int> hits;
        std::vector<int> errs;
	ModifySlices::data(ch->HitsVec(),  hits, timeslicesCmmNew);
	ModifySlices::data(ch->ErrorVec(), errs, timeslicesCmmNew);
	for (int slice = 0; slice < timeslicesCmmNew; ++slice) {
	  const LVL1::DataError errBits(errs[slice]);
	  const int index = ( neutralFormat ) ? 0 : slice;
	  CmmJetSubBlock* const subBlock = m_cmmJetBlocks[index];
	  if (dataID == LVL1::CMMJetHits::ET_MAP) {
	    subBlock->setJetEtMap(slice, hits[slice]);
          } else {
	    subBlock->setJetHits(slice, source, hits[slice],
	                         errBits.get(LVL1::DataError::Parity));
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

  // Set ROD status words

  L1CaloRodStatus::setStatus(re, m_rodStatusMap, m_srcIdMap);

  return StatusCode::SUCCESS;
}

// Return reference to vector with all possible Source Identifiers

const std::vector<uint32_t>& JepByteStreamTool::sourceIDs(
                                                const std::string& sgKey)
{
  // Check if overlap jet element channels wanted
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
	    //if (sc.isFailure()) return sc;
          }
        } else {
	  CmmEnergySubBlock subBlock;
	  payload = subBlock.read(payload, payloadEnd);
	  if (collection == CMM_SUMS) {
	    StatusCode sc = decodeCmmEnergy(subBlock, trigCmm);
	    //if (sc.isFailure()) return sc;
          }
	}
      } else {
        // JEM
        JemSubBlock subBlock;
        payload = subBlock.read(payload, payloadEnd);
	if (collection == JET_ELEMENTS || collection == JET_HITS ||
	                                  collection == ENERGY_SUMS) {
	  StatusCode sc = decodeJem(subBlock, trigJem, collection);
	  //if (sc.isFailure()) return sc;
        }
      }
    }
  }

  return StatusCode::SUCCESS;
}

// Unpack CMM-Energy sub-block

StatusCode JepByteStreamTool::decodeCmmEnergy(CmmEnergySubBlock& subBlock,
                                                              int trigCmm)
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
    log << MSG::DEBUG << "CMM-Energy sub-block unpacking failed" << endreq;
    //return StatusCode::FAILURE;
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
      int exErr = subBlock.exError(slice, source);
      int eyErr = subBlock.eyError(slice, source);
      int etErr = subBlock.etError(slice, source);
      LVL1::DataError exErrBits;
      LVL1::DataError eyErrBits;
      LVL1::DataError etErrBits;
      if (dataID == LVL1::CMMEtSums::LOCAL ||
          dataID == LVL1::CMMEtSums::REMOTE ||
	  dataID == LVL1::CMMEtSums::TOTAL) {
        exErrBits.set(LVL1::DataError::Overflow, exErr);
	exErrBits.set(LVL1::DataError::Parity,   exErr >> 1);
        eyErrBits.set(LVL1::DataError::Overflow, eyErr);
	eyErrBits.set(LVL1::DataError::Parity,   eyErr >> 1);
        etErrBits.set(LVL1::DataError::Overflow, etErr);
	etErrBits.set(LVL1::DataError::Parity,   etErr >> 1);
      } else {
        exErrBits.set(LVL1::DataError::Parity, exErr);
        eyErrBits.set(LVL1::DataError::Parity, eyErr);
        etErrBits.set(LVL1::DataError::Parity, etErr);
      }
      exErrBits.set(LVL1::DataError::SubStatusWord, subBlock.subStatus());
      eyErrBits.set(LVL1::DataError::SubStatusWord, subBlock.subStatus());
      etErrBits.set(LVL1::DataError::SubStatusWord, subBlock.subStatus());
      exErr = exErrBits.error();
      eyErr = eyErrBits.error();
      etErr = etErrBits.error();
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

    // Hit maps - store as Et

    if (summing == CmmSubBlock::SYSTEM) {
      LVL1::DataError errBits;
      errBits.set(LVL1::DataError::SubStatusWord, subBlock.subStatus());
      const int err = errBits.error();
      const unsigned int missEt = subBlock.missingEtHits(slice);
      if ( missEt || err ) {
        const int dataID = LVL1::CMMEtSums::MISSING_ET_MAP;
        LVL1::CMMEtSums* map = findCmmSums(crate, dataID);
        if ( ! map ) {
          std::vector<unsigned int> missVec(timeslices);
          std::vector<int> errVec(timeslices);
	  missVec[slice] = missEt;
	  errVec[slice]  = err;
	  map = new LVL1::CMMEtSums(swCrate, dataID,
	                            missVec, missVec, missVec,
	  			    errVec, errVec, errVec, trigCmm);
          const int key = crate*100 + dataID;
	  m_cmmEtMap.insert(std::make_pair(key, map));
	  m_cmmEtCollection->push_back(map);
        } else {
          std::vector<unsigned int> missVec(map->EtVec());
          std::vector<int> errVec(map->EtErrorVec());
	  missVec[slice] = missEt;
	  errVec[slice]  = err;
	  map->addEx(missVec, errVec);
	  map->addEy(missVec, errVec);
	  map->addEt(missVec, errVec);
        }
      }
      const unsigned int sumEt = subBlock.sumEtHits(slice);
      if ( sumEt || err ) {
        const int dataID = LVL1::CMMEtSums::SUM_ET_MAP;
        LVL1::CMMEtSums* map = findCmmSums(crate, dataID);
        if ( ! map ) {
          std::vector<unsigned int> sumVec(timeslices);
          std::vector<int> errVec(timeslices);
	  sumVec[slice] = sumEt;
	  errVec[slice] = err;
	  map = new LVL1::CMMEtSums(swCrate, dataID,
	                            sumVec, sumVec, sumVec,
				    errVec, errVec, errVec, trigCmm);
          const int key = crate*100 + dataID;
	  m_cmmEtMap.insert(std::make_pair(key, map));
	  m_cmmEtCollection->push_back(map);
        } else {
          std::vector<unsigned int> sumVec(map->EtVec());
          std::vector<int> errVec(map->EtErrorVec());
	  sumVec[slice] = sumEt;
	  errVec[slice] = err;
	  map->addEx(sumVec, errVec);
	  map->addEy(sumVec, errVec);
	  map->addEt(sumVec, errVec);
        }
      }
    }
  }
  
  return StatusCode::SUCCESS;
}

// Unpack CMM-Jet sub-block

StatusCode JepByteStreamTool::decodeCmmJet(CmmJetSubBlock& subBlock,
                                                                  int trigCmm)
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
    log << MSG::DEBUG << "CMM-Jet sub-block unpacking failed" << endreq;
    //return StatusCode::FAILURE;
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
      LVL1::DataError errBits;
      errBits.set(LVL1::DataError::Parity,
                        subBlock.jetHitsError(slice, source));
      errBits.set(LVL1::DataError::SubStatusWord, subBlock.subStatus());
      const int err = errBits.error();
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

    // Hit map - store as hits

    if (summing == CmmSubBlock::SYSTEM) {
      LVL1::DataError errBits;
      errBits.set(LVL1::DataError::SubStatusWord, subBlock.subStatus());
      const int err = errBits.error();
      const unsigned int etMap = subBlock.jetEtMap(slice);
      if ( etMap || err ) {
        const int dataID = LVL1::CMMJetHits::ET_MAP;
        LVL1::CMMJetHits* map = findCmmHits(crate, dataID);
        if ( ! map ) {
          std::vector<unsigned int> mapVec(timeslices);
          std::vector<int> errVec(timeslices);
	  mapVec[slice] = etMap;
	  errVec[slice] = err;
	  map = new LVL1::CMMJetHits(swCrate, dataID, mapVec, errVec, trigCmm);
          const int key = crate*100 + dataID;
	  m_cmmHitsMap.insert(std::make_pair(key, map));
	  m_cmmHitCollection->push_back(map);
        } else {
          std::vector<unsigned int> mapVec(map->HitsVec());
          std::vector<int> errVec(map->ErrorVec());
	  mapVec[slice] = etMap;
	  errVec[slice] = err;
	  map->addHits(mapVec, errVec);
        }
      }
    }
  }
  
  return StatusCode::SUCCESS;
}

// Unpack JEM sub-block

StatusCode JepByteStreamTool::decodeJem(JemSubBlock& subBlock, int trigJem,
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
    log << MSG::DEBUG << "Triggered JEM slice from header "
        << "inconsistent with number of slices: "
        << trigJem << ", " << timeslices << ", reset to 0" << endreq;
    trigJem = 0;
    //return StatusCode::FAILURE;
  }
  if (timeslices <= sliceNum) {
    log << MSG::ERROR << "Total slices inconsistent with slice number: "
        << timeslices << ", " << sliceNum << endreq;
    return StatusCode::FAILURE;
  }
  // Unpack sub-block
  if (subBlock.dataWords() && !subBlock.unpack()) {
    log << MSG::DEBUG << "JEM sub-block unpacking failed" << endreq;
    //return StatusCode::FAILURE;
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
	LVL1::DataError emErrBits;
	LVL1::DataError hadErrBits;
	emErrBits.set(LVL1::DataError::SubStatusWord, subBlock.subStatus());
	hadErrBits.set(LVL1::DataError::SubStatusWord, subBlock.subStatus());
        if (jetEle.data() || emErrBits.error()) {
	  ChannelCoordinate coord;
	  int coreOrOverlap = m_jemMaps->mapping(crate, module, chan, coord);
	  if (coreOrOverlap == m_coreOverlap) {
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
	    emErrBits.set(LVL1::DataError::Parity, jetEle.emParity());
	    emErrBits.set(LVL1::DataError::LinkDown, jetEle.linkError());
	    hadErrBits.set(LVL1::DataError::Parity, jetEle.hadParity());
	    hadErrBits.set(LVL1::DataError::LinkDown, jetEle.linkError() >> 1);
	    je->addSlice(slice, jetEle.emData(), jetEle.hadData(),
	                        emErrBits.error(), hadErrBits.error(),
	         	        jetEle.linkError());
          } else if (debug && jetEle.data() && !coreOrOverlap) {
	    log << MSG::VERBOSE
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
  int trigJ  = m_dfltSlicesJem/2;
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
  if (slices < 0) slices = m_dfltSlicesJem;
  timeslices = slices;
  trigJem    = trigJ;
  return true;
}

// Get number of CMM slices and triggered slice offset for current crate

bool JepByteStreamTool::slinkSlicesCmm(const int crate, int& timeslices,
                                                        int& trigCmm)
{
  int slices = -1;
  int trigC  = m_dfltSlicesCmm/2;
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
  if (slices < 0) slices = m_dfltSlicesCmm;
  timeslices = slices;
  trigCmm    = trigC;
  return true;
}

} // end namespace
