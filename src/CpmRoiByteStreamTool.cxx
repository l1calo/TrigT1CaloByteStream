
#include <numeric>
#include <utility>

#include "GaudiKernel/IInterface.h"

#include "TrigT1Calo/CPMRoI.h"

#include "TrigT1CaloByteStream/CpmCrateMappings.h"
#include "TrigT1CaloByteStream/CpmRoiByteStreamTool.h"
//#include "TrigT1CaloByteStream/L1CaloRodStatus.h"
#include "TrigT1CaloByteStream/L1CaloUserHeader.h"

namespace LVL1BS {

// Interface ID

static const InterfaceID IID_ICpmRoiByteStreamTool("CpmRoiByteStreamTool",
                                                                        1, 1);

const InterfaceID& CpmRoiByteStreamTool::interfaceID()
{
  return IID_ICpmRoiByteStreamTool;
}

// Constructor

CpmRoiByteStreamTool::CpmRoiByteStreamTool(const std::string& type,
                                           const std::string& name,
                                           const IInterface*  parent)
                      : AlgTool(type, name, parent),
                        m_srcIdMap(0), m_rodStatus(0)
{
  declareInterface<CpmRoiByteStreamTool>(this);

  declareProperty("CrateOffsetHw",  m_crateOffsetHw = 8,
                  "Offset of CP crate numbers in bytestream");
  declareProperty("CrateOffsetSw",  m_crateOffsetSw = 0,
                  "Offset of CP crate numbers in RDOs");

  // Properties for reading bytestream only
  declareProperty("ROBSourceIDs",       m_sourceIDs,
                  "ROB fragment source identifiers");
  declareProperty("ROBSourceIDsRoIB",   m_sourceIDsRoIB,
                  "ROB fragment source identifiers for RoIBs");

  // Properties for writing bytestream only
  declareProperty("DataVersion",    m_version       = 1,
                  "Format version number in sub-block header");
  declareProperty("DataFormat",     m_dataFormat    = 1,
                  "Format identifier (0-1) in sub-block header");
  declareProperty("SlinksPerCrate", m_slinks        = 1,
                  "The number of S-Links per crate");

}

// Destructor

CpmRoiByteStreamTool::~CpmRoiByteStreamTool()
{
}

// Initialize

StatusCode CpmRoiByteStreamTool::initialize()
{
  m_subDetector = eformat::TDAQ_CALO_CLUSTER_PROC_ROI;
  m_srcIdMap    = new L1CaloSrcIdMap();
  m_crates      = CpmCrateMappings::crates();
  m_modules     = CpmCrateMappings::modules();
  m_rodStatus   = new std::vector<uint32_t>(2);
  return AlgTool::initialize();
}

// Finalize

StatusCode CpmRoiByteStreamTool::finalize()
{
  delete m_rodStatus;
  delete m_srcIdMap;
  return AlgTool::finalize();
}

// Convert ROB fragments to CPM RoIs

StatusCode CpmRoiByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::CPMRoI>* const roiCollection)
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
          m_srcIdMap->daqOrRoi(sourceID) != 1) {
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
        m_subBlock.clear();
        payload = m_subBlock.read(payload, payloadEnd);
        if (debug) {
          log << MSG::DEBUG << "CPM RoI sub-block: Crate " << m_subBlock.crate()
                            << "  Module " << m_subBlock.module() << endreq;
        }
        // Unpack sub-block
        if (m_subBlock.dataWords() && !m_subBlock.unpack()) {
          log << MSG::DEBUG << "CPM RoI sub-block unpacking failed" << endreq;
          //return StatusCode::FAILURE;
        }
	const int numChips = 8;
	const int numLocs  = 2;
	for (int chip = 0; chip < numChips; ++chip) {
	  for (int loc = 0; loc < numLocs; ++loc) {
	    const LVL1::CPMRoI roi = m_subBlock.roi(chip, loc);
            if (roi.hits() || roi.error()) {
              roiCollection->push_back(new LVL1::CPMRoI(roi));
            }
          }
        }
      } else {
        // Just RoI word
	LVL1::CPMRoI roi;
	if (roi.setRoiWord(*payload)) {
	  roiCollection->push_back(new LVL1::CPMRoI(*payload));
	} else {
	  log << MSG::DEBUG << "Invalid RoI word "
	      << MSG::hex << *payload << MSG::dec << endreq;
        }
	++payload;
      }
    }
  }

  return StatusCode::SUCCESS;
}

// Convert CPM RoI to bytestream

StatusCode CpmRoiByteStreamTool::convert(
           const DataVector<LVL1::CPMRoI>* const roiCollection,
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

  // Set up the container map

  setupCpmRoiMap(roiCollection);
  CpmRoiMap::const_iterator mapIter    = m_roiMap.begin();
  CpmRoiMap::const_iterator mapIterEnd = m_roiMap.end();

  // Loop over data

  const bool neutralFormat = m_dataFormat == L1CaloSubBlock::NEUTRAL;
  const int  modulesPerSlink = m_modules / m_slinks;
  for (int crate=0; crate < m_crates; ++crate) {
    const int hwCrate = crate + m_crateOffsetHw;

    // CPM modules are numbered 1 to m_modules
    for (int module=1; module <= m_modules; ++module) {
      const int mod = module - 1;

      // Pack required number of modules per slink

      if (mod%modulesPerSlink == 0) {
	const int daqOrRoi = 1;
	const int slink = (m_slinks == 2) ? 2*(mod/modulesPerSlink)
	                                  : mod/modulesPerSlink;
        if (debug) {
          log << MSG::DEBUG << "Treating crate " << hwCrate
                            << " slink " << slink << endreq;
	  log << MSG::DEBUG << "Data Version/Format: " << m_version
	                    << " " << m_dataFormat << endreq;
        }
	const uint32_t rodIdCpm = m_srcIdMap->getRodID(hwCrate, slink, daqOrRoi,
	                                                        m_subDetector);
	theROD = m_fea.getRodData(rodIdCpm);
	if (neutralFormat) {
          const L1CaloUserHeader userHeader;
	  theROD->push_back(userHeader.header());
        }
	m_rodStatusMap.insert(make_pair(rodIdCpm, m_rodStatus));
      }
      if (debug) {
        log << MSG::DEBUG << "Module " << module << endreq;
      }

      // Create a sub-block (Neutral format only)

      if (neutralFormat) {
        m_subBlock.clear();
	m_subBlock.setRoiHeader(m_version, hwCrate, module);
      }

      // Find CPM RoIs for this module

      for (; mapIter != mapIterEnd; ++mapIter) {
        const LVL1::CPMRoI* const roi = mapIter->second;
	if (roi->crate() < crate)  continue;
	if (roi->crate() > crate)  break;
	if (roi->cpm()   < module) continue;
	if (roi->cpm()   > module) break;
	if (roi->hits() || roi->error()) {
	  if (neutralFormat) m_subBlock.fillRoi(*roi);
          else theROD->push_back(roi->roiWord());
        }
      }
      
      // Pack and write the sub-block

      if (neutralFormat) {
        if ( !m_subBlock.pack()) {
          log << MSG::ERROR << "CPMRoI sub-block packing failed" << endreq;
 	  return StatusCode::FAILURE;
        }
	if (debug) {
	  log << MSG::DEBUG << "CPMRoI sub-block data words: "
	                    << m_subBlock.dataWords() << endreq;
        }
        m_subBlock.write(theROD);
      }
    }
  }

  // Fill the raw event

  m_fea.fill(re, log);

  // Set ROD status words

  //L1CaloRodStatus::setStatus(re, m_rodStatusMap, m_srcIdMap);

  return StatusCode::SUCCESS;
}

// Return reference to vector with all possible Source Identifiers

const std::vector<uint32_t>& CpmRoiByteStreamTool::sourceIDs(
                                                      const std::string& sgKey)
{
  const std::string flag("RoIB");
  const std::string::size_type pos = sgKey.find(flag);
  const bool roiDaq =
           (pos == std::string::npos || pos != sgKey.length() - flag.length());
  const bool empty  = (roiDaq) ? m_sourceIDs.empty() : m_sourceIDsRoIB.empty();
  if (empty) {
    const int maxCrates = m_crates + m_crateOffsetHw;
    const int maxSlinks = m_srcIdMap->maxSlinks();
    for (int hwCrate = m_crateOffsetHw; hwCrate < maxCrates; ++hwCrate) {
      for (int slink = 0; slink < maxSlinks; ++slink) {
        const int daqOrRoi = 1;
        const uint32_t rodId = m_srcIdMap->getRodID(hwCrate, slink, daqOrRoi,
                                                             m_subDetector);
        const uint32_t robId = m_srcIdMap->getRobID(rodId);
	if (roiDaq) {
	  if (slink < 2) m_sourceIDs.push_back(robId);
	} else if (slink >= 2) m_sourceIDsRoIB.push_back(robId);
      }
    }
  }
  return (roiDaq) ? m_sourceIDs : m_sourceIDsRoIB;
}

// Set up CPM RoI map

void CpmRoiByteStreamTool::setupCpmRoiMap(const CpmRoiCollection*
                                                          const roiCollection)
{
  m_roiMap.clear();
  if (roiCollection) {
    CpmRoiCollection::const_iterator pos  = roiCollection->begin();
    CpmRoiCollection::const_iterator pose = roiCollection->end();
    for (; pos != pose; ++pos) {
      const LVL1::CPMRoI* const roi = *pos;
      const uint32_t key = roi->roiWord();
      m_roiMap.insert(std::make_pair(key, roi));
    }
  }
}

} // end namespace
