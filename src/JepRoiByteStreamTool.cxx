
#include <numeric>
#include <utility>

#include "GaudiKernel/IInterface.h"

#include "TrigT1Calo/CMMJetHits.h"
#include "TrigT1Calo/CMMEtSums.h"
#include "TrigT1Calo/CMMRoI.h"
#include "TrigT1Calo/JEMRoI.h"
#include "TrigT1Calo/JEPRoIBSCollection.h"

#include "TrigT1CaloByteStream/CmmEnergySubBlock.h"
#include "TrigT1CaloByteStream/CmmJetSubBlock.h"
#include "TrigT1CaloByteStream/CmmSubBlock.h"
#include "TrigT1CaloByteStream/JemCrateMappings.h"
#include "TrigT1CaloByteStream/JepRoiByteStreamTool.h"
#include "TrigT1CaloByteStream/L1CaloSubBlock.h"
#include "TrigT1CaloByteStream/L1CaloUserHeader.h"

// Interface ID

static const InterfaceID IID_IJepRoiByteStreamTool("JepRoiByteStreamTool",
                                                                        1, 1);

const InterfaceID& JepRoiByteStreamTool::interfaceID()
{
  return IID_IJepRoiByteStreamTool;
}

// Constructor

JepRoiByteStreamTool::JepRoiByteStreamTool(const std::string& type,
                                           const std::string& name,
				           const IInterface*  parent)
                     : AlgTool(type, name, parent),
		       m_srcIdMap(0)
{
  declareInterface<JepRoiByteStreamTool>(this);

  // Properties for writing bytestream only
  declareProperty("DataVersion",    m_version    = 1);
  declareProperty("DataFormat",     m_dataFormat = 1);
  declareProperty("SlinksPerCrate", m_slinks     = 1);

}

// Destructor

JepRoiByteStreamTool::~JepRoiByteStreamTool()
{
}

// Initialize

StatusCode JepRoiByteStreamTool::initialize()
{
  m_subDetector = eformat::TDAQ_CALO_JET_PROC_ROI;
  m_srcIdMap    = new L1CaloSrcIdMap();
  m_crates      = JemCrateMappings::crates();
  m_modules     = JemCrateMappings::modules();
  return AlgTool::initialize();
}

// Finalize

StatusCode JepRoiByteStreamTool::finalize()
{
  delete m_srcIdMap;
  return AlgTool::finalize();
}

// Conversion bytestream to JEM RoI

StatusCode JepRoiByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::JEMRoI>* const jeCollection)
{
  m_jeCollection = jeCollection;
  return convertBs(robFrags, JEM_ROI);
}

// Conversion bytestream to CMM RoI

StatusCode JepRoiByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            LVL1::CMMRoI* const cmCollection)
{
  m_cmCollection = cmCollection;
  return convertBs(robFrags, CMM_ROI);
}

// Conversion of JEP container to bytestream

StatusCode JepRoiByteStreamTool::convert(
                                 const LVL1::JEPRoIBSCollection* const jep,
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

  const bool neutralFormat = m_dataFormat == L1CaloSubBlock::NEUTRAL;
  setupJemRoiMap(jep->JemRoi());
  JemRoiMap::const_iterator mapIter    = m_roiMap.begin();
  JemRoiMap::const_iterator mapIterEnd = m_roiMap.end();
  if (neutralFormat) {
    setupCmmHitsMap(jep->CmmHits());
    setupCmmEtMap(jep->CmmSums());
  }

  // Loop over JEM RoI data

  const int modulesPerSlink = m_modules / m_slinks;
  for (int crate=0; crate < m_crates; ++crate) {

    for (int module=0; module < m_modules; ++module) {

      // Pack required number of modules per slink

      if (module%modulesPerSlink == 0) {
	const int daqOrRoi = 0;  // 1 is for RoIB
	const int slink = module/modulesPerSlink;
        if (debug) {
          log << MSG::DEBUG << "Treating crate " << crate
                            << " slink " << slink << endreq;
	  log << MSG::DEBUG << "Data Version/Format: " << m_version
	                    << " " << m_dataFormat << endreq;
        }
	const uint32_t rodIdJem = m_srcIdMap->getRodID(crate, slink, daqOrRoi,
	                                                       m_subDetector);
	theROD = m_fea.getRodData(rodIdJem);
        if (neutralFormat) {
          const L1CaloUserHeader userHeader;
	  theROD->push_back(userHeader.header());
        }
      }
      if (debug) {
        log << MSG::DEBUG << "JEM Module " << module << endreq;
      }

      // Create a sub-block (Neutral format only)

      if (neutralFormat) {
        m_subBlock.clear();
	m_subBlock.setRoiHeader(m_version, crate, module);
      }

      // Find JEM RoIs for this module

      for (; mapIter != mapIterEnd; ++mapIter) {
        const LVL1::JEMRoI* const roi = mapIter->second;
	if (roi->crate() < crate)  continue;
	if (roi->crate() > crate)  break;
	if (roi->jem()   < module) continue;
	if (roi->jem()   > module) break;
	if (roi->hits() || roi->error()) {
	  if (neutralFormat) m_subBlock.fillRoi(*roi);
	  else theROD->push_back(roi->roiWord());
        }
      }

      // Pack and write the sub-block

      if (neutralFormat) {
        if ( !m_subBlock.pack()) {
	  log << MSG::ERROR << "JEM RoI sub-block packing failed" << endreq;
	  return StatusCode::FAILURE;
        }
	m_subBlock.write(theROD);
      }
    }

    // Append CMM RoIs to last S-Link of the system crate

    if (crate != m_crates - 1) continue;

    // Create sub-blocks for Neutral format

    if (neutralFormat) {
      const int timeslices = 1;
      const int slice = 0;

      // CMM-Energy

      CmmEnergySubBlock enBlock;
      enBlock.setCmmHeader(m_version, m_dataFormat, slice, crate,
                           CmmSubBlock::SYSTEM, CmmSubBlock::CMM_ENERGY,
			   CmmSubBlock::LEFT, timeslices);
      int maxDataID = LVL1::CMMEtSums::MAXID;
      for (int dataID = 0; dataID < maxDataID; ++dataID) {
        int source = dataID;
        if (dataID >= m_modules) {
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
          const unsigned int ex = sums->Ex();
          const unsigned int ey = sums->Ey();
          const unsigned int et = sums->Et();
          const int exErr = sums->ExError();
          const int eyErr = sums->EyError();
          const int etErr = sums->EtError();
	  if (dataID == LVL1::CMMEtSums::MISSING_ET_MAP) {
	    enBlock.setMissingEtHits(slice, et); 
          } else if (dataID == LVL1::CMMEtSums::SUM_ET_MAP) {
	    enBlock.setSumEtHits(slice, et); 
          } else {
	    enBlock.setSubsums(slice, source, ex, ey, et, exErr, eyErr, etErr);
          }
        }
      }
      if ( !enBlock.pack()) {
        log << MSG::ERROR << "CMM-Energy sub-block packing failed" << endreq;
	return StatusCode::FAILURE;
      }
      enBlock.write(theROD);

      // CMM-Jet

      CmmJetSubBlock jetBlock;
      jetBlock.setCmmHeader(m_version, m_dataFormat, slice, crate,
                            CmmSubBlock::SYSTEM, CmmSubBlock::CMM_JET,
			    CmmSubBlock::RIGHT, timeslices);
      maxDataID = LVL1::CMMJetHits::MAXID;
      for (int dataID = 0; dataID < maxDataID; ++dataID) {
        int source = dataID;
        if (dataID >= m_modules) {
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
          const unsigned int hits = ch->Hits();
          const int          errs = ch->Error();
	  if (dataID == LVL1::CMMJetHits::ET_MAP) {
	    jetBlock.setJetEtMap(slice, hits);
          } else {
	    jetBlock.setJetHits(slice, source, hits, errs);
          }
        }
      }
      if ( !jetBlock.pack()) {
        log << MSG::ERROR << "CMM-Jet sub-block packing failed" << endreq;
	return StatusCode::FAILURE;
      }
      jetBlock.write(theROD);

    } else {

      // Standard format

      const LVL1::CMMRoI* const roi = jep->CmmRoi();
      if ( roi ) {
	// Make sure word IDs are correct
        const LVL1::CMMRoI roid(roi->jetEtHits(), roi->sumEtHits(),
	            roi->missingEtHits(), roi->ex(), roi->ey(), roi->et(),
		    roi->jetEtError(), roi->sumEtError(),
		    roi->missingEtError(), roi->exError(), roi->eyError(),
		    roi->etError());
        if (roid.jetEtHits() || roid.jetEtError()) {
          theROD->push_back(roid.jetEtRoiWord());
        }
        // CMM-Energy RoIs are not zero-supressed
        theROD->push_back(roid.energyRoiWord0());
        theROD->push_back(roid.energyRoiWord1());
        theROD->push_back(roid.energyRoiWord2());
      } else {
        const LVL1::CMMRoI roid(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        theROD->push_back(roid.energyRoiWord0());
        theROD->push_back(roid.energyRoiWord1());
        theROD->push_back(roid.energyRoiWord2());
      }
    }
  }

  // Fill the raw event

  m_fea.fill(re, log);

  return StatusCode::SUCCESS;
}

// Fill a vector with all possible Source Identifiers

void JepRoiByteStreamTool::sourceIDs(std::vector<uint32_t>& vID) const
{
  const int maxlinks = m_srcIdMap->maxSlinks();
  for (int crate = 0; crate < m_crates; ++crate) {
    for (int slink = 0; slink < maxlinks; ++slink) {
      const int daqOrRoi = 0;
      const uint32_t rodId = m_srcIdMap->getRodID(crate, slink, daqOrRoi,
                                                          m_subDetector);
      const uint32_t robId = m_srcIdMap->getRobID(rodId);
      vID.push_back(robId);
    }
  }
}

// Convert bytestream to given container type

StatusCode JepRoiByteStreamTool::convertBs(
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

    // First word may be User Header
    if (L1CaloUserHeader::isValid(*payload)) {
      const L1CaloUserHeader userHeader(*payload);
      const int headerWords = userHeader.words();
      if (headerWords != 1 && debug) {
        log << MSG::DEBUG << "Unexpected number of user header words: "
	    << headerWords << endreq;
      }
      for (int i = 0; i < headerWords; ++i) ++payload;
    }

    // Loop over sub-blocks if there are any

    while (payload != payloadEnd) {
      
      if (L1CaloSubBlock::wordType(*payload) == L1CaloSubBlock::HEADER) {
	const int slice = 0;
        if (CmmSubBlock::cmmBlock(*payload)) {
          // CMMs
	  if (CmmSubBlock::cmmType(*payload) == CmmSubBlock::CMM_JET) {
            CmmJetSubBlock subBlock;
            payload = subBlock.read(payload, payloadEnd);
	    if (collection == CMM_ROI) {
	      if ( !subBlock.unpack()) {
	        log << MSG::ERROR << "CMM-Jet sub-block unpacking failed"
		    << endreq;
		return StatusCode::FAILURE;
              }
	      const LVL1::CMMRoI roi(subBlock.jetEtMap(slice),
	                             0,0,0,0,0,0,0,0,0,0,0);
	      m_cmCollection->setRoiWord(roi.jetEtRoiWord());
            }
          } else {
	    CmmEnergySubBlock subBlock;
	    payload = subBlock.read(payload, payloadEnd);
	    if (collection == CMM_ROI) {
	      if ( !subBlock.unpack()) {
	        log << MSG::ERROR << "CMM-Energy sub-block unpacking failed"
		    << endreq;
		return StatusCode::FAILURE;
              }
	      const int mask = 0x3fff; // 14 bits
	      const int sign = 0x4000; // sign bit
	      const int exp = subBlock.ex(slice, CmmEnergySubBlock::TOTAL);
	      int ex = exp & mask;
	      if (exp & sign) ex = -ex;
	      const int eyp = subBlock.ey(slice, CmmEnergySubBlock::TOTAL);
	      int ey = eyp & mask;
	      if (eyp & sign) ey = -ey;
	      const LVL1::CMMRoI roi(0, subBlock.sumEtHits(slice),
	                   subBlock.missingEtHits(slice), ex, ey,
			   subBlock.et(slice, CmmEnergySubBlock::TOTAL),
			   0, 0, 0,
			   subBlock.exError(slice, CmmEnergySubBlock::TOTAL),
			   subBlock.eyError(slice, CmmEnergySubBlock::TOTAL),
			   subBlock.etError(slice, CmmEnergySubBlock::TOTAL));
	      m_cmCollection->setRoiWord(roi.energyRoiWord0());
	      m_cmCollection->setRoiWord(roi.energyRoiWord1());
	      m_cmCollection->setRoiWord(roi.energyRoiWord2());
            }
	  }
        } else {
          // JEM RoI
          JemRoiSubBlock subBlock;
          payload = subBlock.read(payload, payloadEnd);
	  if (collection == JEM_ROI) {
	    if ( !subBlock.unpack()) {
	      log << MSG::ERROR << "JEM RoI sub-block unpacking failed"
	          << endreq;
              return StatusCode::FAILURE;
            }
	    for (int frame = 0; frame < 8; ++frame) {
	      for (int forward = 0; forward < 2; ++forward) {
	        const LVL1::JEMRoI roi = subBlock.roi(frame, forward);
		if (roi.hits() || roi.error()) {
		  m_jeCollection->push_back(new LVL1::JEMRoI(roi));
	        }
	      }
	    }
          }
        }
      } else {
        // Just RoI word
	LVL1::JEMRoI jroi;
	LVL1::CMMRoI croi;
	if (jroi.setRoiWord(*payload)) {
	  if (collection == JEM_ROI) {
	    m_jeCollection->push_back(new LVL1::JEMRoI(*payload));
	  }
        } else if (croi.setRoiWord(*payload)) {
	  if (collection == CMM_ROI) {
	    m_cmCollection->setRoiWord(*payload);
	  }
        } else {
	  log << MSG::ERROR << "Invalid RoI word ";
	  MSG::hex(log) << MSG::ERROR << *payload;
	  MSG::dec(log) << MSG::ERROR << endreq;
        }
	++payload;
      }
    }
  }

  return StatusCode::SUCCESS;
}

// Find CMM hits for given crate, dataID

const LVL1::CMMJetHits* JepRoiByteStreamTool::findCmmHits(const int crate,
                                                          const int dataID)
{
  const LVL1::CMMJetHits* hits = 0;
  CmmHitsMap::const_iterator mapIter;
  mapIter = m_cmmHitsMap.find(crate*100 + dataID);
  if (mapIter != m_cmmHitsMap.end()) hits = mapIter->second;
  return hits;
}

// Find CMM energy sums for given crate, module, dataID

const LVL1::CMMEtSums* JepRoiByteStreamTool::findCmmSums(const int crate,
                                                         const int dataID)
{
  const LVL1::CMMEtSums* sums = 0;
  CmmSumsMap::const_iterator mapIter;
  mapIter = m_cmmEtMap.find(crate*100 + dataID);
  if (mapIter != m_cmmEtMap.end()) sums = mapIter->second;
  return sums;
}

// Set up JEM RoIs map

void JepRoiByteStreamTool::setupJemRoiMap(const JemRoiCollection*
                                                            const jeCollection)
{
  m_roiMap.clear();
  if (jeCollection) {
    JemRoiCollection::const_iterator pos  = jeCollection->begin();
    JemRoiCollection::const_iterator pose = jeCollection->end();
    for (; pos != pose; ++pos) {
      const LVL1::JEMRoI* const roi = *pos;
      const uint32_t key = roi->roiWord();
      m_roiMap.insert(std::make_pair(key, roi));
    }
  }
}

// Set up CMM hits map

void JepRoiByteStreamTool::setupCmmHitsMap(const CmmHitsCollection*
                                                           const hitCollection)
{
  m_cmmHitsMap.clear();
  if (hitCollection) {
    CmmHitsCollection::const_iterator pos  = hitCollection->begin();
    CmmHitsCollection::const_iterator pose = hitCollection->end();
    for (; pos != pose; ++pos) {
      const LVL1::CMMJetHits* const hits = *pos;
      const int key = hits->crate()*100 + hits->dataID();
      m_cmmHitsMap.insert(std::make_pair(key, hits));
    }
  }
}

// Set up CMM energy sums map

void JepRoiByteStreamTool::setupCmmEtMap(const CmmSumsCollection*
                                                            const etCollection)
{
  m_cmmEtMap.clear();
  if (etCollection) {
    CmmSumsCollection::const_iterator pos  = etCollection->begin();
    CmmSumsCollection::const_iterator pose = etCollection->end();
    for (; pos != pose; ++pos) {
      const LVL1::CMMEtSums* const sums = *pos;
      const int key = sums->crate()*100 + sums->dataID();
      m_cmmEtMap.insert(std::make_pair(key, sums));
    }
  }
}