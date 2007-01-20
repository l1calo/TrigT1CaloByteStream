
#include <numeric>
#include <utility>

#include "GaudiKernel/IInterface.h"

#include "TrigT1Calo/JetElement.h"
#include "TrigT1Calo/JetElementKey.h"
#include "TrigT1Calo/JEMHits.h"
#include "TrigT1Calo/JEMEtSums.h"

#include "TrigT1CaloByteStream/ChannelCoordinate.h"
#include "TrigT1CaloByteStream/CmmSubBlock.h"
#include "TrigT1CaloByteStream/L1CaloSubBlock.h"
#include "TrigT1CaloByteStream/L1CaloUserHeader.h"
#include "TrigT1CaloByteStream/JemCrateMappings.h"
#include "TrigT1CaloByteStream/JemSubBlock.h"
#include "TrigT1CaloByteStream/JepByteStreamTool.h"
#include "TrigT1CaloByteStream/JepContainer.h"

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

  // Properties for writing bytestream only
  declareProperty("DataVersion",    m_version    = 1);
  declareProperty("DataFormat",     m_dataFormat = 1);
  declareProperty("SlinksPerCrate", m_slinks     = 4);

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
                            DataVector<LVL1::JetElement>* jeCollection)
{
  m_jeCollection = jeCollection;
  m_jeMap.clear();
  return convertBs(robFrags, JET_ELEMENTS);
}

// Conversion bytestream to jet hits

StatusCode JepByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::JEMHits>* hitCollection)
{
  m_hitCollection = hitCollection;
  m_hitsMap.clear();
  return convertBs(robFrags, JET_HITS);
}

// Conversion bytestream to energy sums

StatusCode JepByteStreamTool::convert(
                            const IROBDataProviderSvc::VROBFRAG& robFrags,
                            DataVector<LVL1::JEMEtSums>* etCollection)
{
  m_etCollection = etCollection;
  m_etMap.clear();
  return convertBs(robFrags, ENERGY_SUMS);
}

// Convert bytestream to given container type

StatusCode JepByteStreamTool::convertBs(
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
    if (headerWords != 1) {
      log << MSG::ERROR << "Unexpected number of user header words: "
          << headerWords << endreq;
      return StatusCode::FAILURE;
    }
    // triggered slice offsets
    int trigJem = userHeader.jem();
    int trigCmm = userHeader.jepCmm();
    if (debug) {
      log << MSG::DEBUG << "JEM triggered slice offset: " << trigJem
                        << endreq;
      log << MSG::DEBUG << "CMM triggered slice offset: " << trigCmm
                        << endreq;
    }
    ++payload;

    // Loop over JEMs

    JemSubBlock subBlock;
    while (payload != payloadEnd) {
      
      if (L1CaloSubBlock::wordType(*payload) != L1CaloSubBlock::HEADER) {
        log << MSG::ERROR << "Unexpected data sequence" << endreq;
        return StatusCode::FAILURE;
      }
      // Skip CMMs
      if (CmmSubBlock::cmmBlock(*payload)) {
	if (debug) {
	  if (CmmSubBlock::cmmType(*payload) == CmmSubBlock::CMM_JET) {
	    log << MSG::DEBUG << "CMM-JET block skipped" << endreq;
          } else log << MSG::DEBUG << "CMM-ENERGY block skipped" << endreq;
	}
        L1CaloSubBlock cmmBlock;
	payload == cmmBlock.read(payload, payloadEnd);
	continue;
      }
      // Fill sub-block
      subBlock.clear();
      payload = subBlock.read(payload, payloadEnd);
      int crate      = subBlock.crate();
      int module     = subBlock.module();
      int timeslices = subBlock.timeslices();
      int sliceNum   = subBlock.slice();
      if (debug) {
        log << MSG::DEBUG << "Crate "          << crate
	                  << "  Module "       << module
	                  << "  Total slices " << timeslices
			  << "  Slice "        << sliceNum    << endreq;
      }
      if (crate != rodCrate) {
        log << MSG::ERROR << "Inconsistent crate number in ROD source ID"
            << endreq;
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

      int sliceBeg = sliceNum;
      int sliceEnd = sliceNum + 1;
      if (subBlock.format() == L1CaloSubBlock::NEUTRAL) {
        sliceBeg = 0;
	sliceEnd = timeslices;
      }
      for (int slice = sliceBeg; slice < sliceEnd; ++slice) {

        if (collection == JET_ELEMENTS) {

          // Loop over jet element channels and fill jet elements

          for (int chan = 0; chan < m_channels; ++chan) {
            JemJetElement jetEle(subBlock.jetElement(slice, chan));
	    if (jetEle.data()) {
	      ChannelCoordinate coord;
	      if (m_jemMaps->mapping(crate, module, chan, coord)) {
	        double eta = coord.eta();
	        double phi = coord.phi();
	        LVL1::JetElement* je = findJetElement(eta, phi);
	        if ( ! je ) {   // create new jet element
	          unsigned int key = m_elementKey->jeKey(phi, eta);
	          std::vector<int> dummy(timeslices);
	          je = new LVL1::JetElement(phi, eta, dummy, dummy, key,
	                                    dummy, dummy, dummy, trigJem);
	          m_jeMap.insert(std::make_pair(key, je));
	          m_jeCollection->push_back(je);
                }
	        je->addSlice(slice, jetEle.emData(), jetEle.hadData(),
	                            jetEle.emParity(), jetEle.hadParity(),
	  	    	            jetEle.linkError());
	        if (debug) {
	          log << MSG::VERBOSE << "Slice=" << slice << "/";
	          printJeData(chan, coord,
		                    std::vector<int>(1,jetEle.emData()),
	                            std::vector<int>(1,jetEle.hadData()),
		           	    std::vector<int>(1,jetEle.emParity()),
				    std::vector<int>(1,jetEle.hadParity()),
				    std::vector<int>(1,jetEle.linkError()),
				    log, MSG::VERBOSE);
                }
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

          unsigned int hits = subBlock.jetHits(slice);
	  if (hits) {
	    LVL1::JEMHits* jh = findJetHits(crate, module);
	    if ( ! jh ) {   // create new jet hits
	      std::vector<unsigned int> hitsVec(timeslices);
	      hitsVec[slice] = hits;
	      jh = new LVL1::JEMHits(crate, module, hitsVec, trigJem);
	      m_hitsMap.insert(std::make_pair(crate*m_modules+module, jh));
	      m_hitCollection->push_back(jh);
            } else {
	      std::vector<unsigned int> hitsVec(jh->JetHitsVec());
	      hitsVec[slice] = hits;
	      jh->addJetHits(hitsVec);
            }
          } else if (debug) {
	    log << MSG::VERBOSE << "No jet hits data for crate/module/slice "
	                        << crate << "/" << module << "/" << slice
				<< endreq;
          }
        } else if (collection == ENERGY_SUMS) {

          // Get energy subsums

	  unsigned int ex = subBlock.ex(slice);
	  unsigned int ey = subBlock.ey(slice);
	  unsigned int et = subBlock.et(slice);
	  if (ex | ey | et) {
	    LVL1::JEMEtSums* sums = findEnergySums(crate, module);
	    if ( ! sums ) {   // create new energy sums
	      std::vector<unsigned int> exVec(timeslices);
	      std::vector<unsigned int> eyVec(timeslices);
	      std::vector<unsigned int> etVec(timeslices);
	      exVec[slice] = ex;
	      eyVec[slice] = ey;
	      etVec[slice] = et;
	      sums = new LVL1::JEMEtSums(crate, module, etVec, exVec, eyVec,
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
	                        << crate << "/" << module << "/" << slice
				<< endreq;
	  }
        }
      }
    }
  }

  return StatusCode::SUCCESS;
}

// Conversion of JEP container to bytestream

StatusCode JepByteStreamTool::convert(const JepContainer* jep,
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

  // Set up the container maps

  setupJeMap(jep->JetElements());
  setupHitsMap(jep->JetHits());
  setupEtMap(jep->EnergySums());

  // Loop over data

  int modulesPerSlink = m_modules / m_slinks;
  int timeslices = 0;
  int trigJem = 0;
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
	// Get number of slices and triggered slice offset
	// for this slink
	if ( ! slinkSlices(crate, module, modulesPerSlink,
	                                  timeslices, trigJem)) {
	  log << MSG::ERROR << "Inconsistent number of slices or "
	      << "triggered slice offsets in data for crate "
	      << crate << " slink " << slink << endreq;
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
	uint32_t rodIdJem = m_srcIdMap->getRodID(crate, slink, daqOrRoi,
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
        JemSubBlock* subBlock = new JemSubBlock();
	subBlock->setJemHeader(m_version, m_dataFormat, slice,
	                       crate, module, timeslices);
        m_jemBlocks.push_back(subBlock);
	if (m_dataFormat == L1CaloSubBlock::NEUTRAL) break;
      }

      // Find jet elements corresponding to each eta/phi pair and fill
      // sub-blocks

      for (int chan=0; chan < m_channels; ++chan) {
	ChannelCoordinate coord;
	if (m_jemMaps->mapping(crate, module, chan, coord)) {
	  double eta = coord.eta();
	  double phi = coord.phi();
          LVL1::JetElement* je = findJetElement(eta, phi);
	  if (je ) {
	    std::vector<int> emData(je->emEnergyVec());
	    std::vector<int> hadData(je->hadEnergyVec());
	    std::vector<int> emParity(je->emErrorVec());
	    std::vector<int> hadParity(je->hadErrorVec());
	    std::vector<int> linkError(je->linkErrorVec());
	    if (debug) printJeData(chan, coord, emData, hadData, emParity,
	                           hadParity, linkError, log, MSG::VERBOSE);
	    // Make sure all vectors are the right size - vectors with only
	    // zeroes are allowed to be different.  This has been checked
	    // already in slinkSlices.
	    emData.resize(timeslices);
	    hadData.resize(timeslices);
	    emParity.resize(timeslices);
	    hadParity.resize(timeslices);
	    linkError.resize(timeslices);
            for (int slice = 0; slice < timeslices; ++slice) {
	      int index = slice;
	      if (m_dataFormat == L1CaloSubBlock::NEUTRAL) index = 0;
              JemSubBlock* subBlock = m_jemBlocks[index];
	      JemJetElement jetEle(chan, emData[slice], hadData[slice],
	                  emParity[slice], hadParity[slice], linkError[slice]);
              subBlock->fillJetElement(slice, jetEle);
	    }
          }
        }
      }

      // Add jet hits and energy subsums

      LVL1::JEMHits* hits = findJetHits(crate, module);
      if (hits) {
        std::vector<unsigned int> vec(hits->JetHitsVec());
	vec.resize(timeslices);
        for (int slice = 0; slice < timeslices; ++slice) {
	  int index = slice;
	  if (m_dataFormat == L1CaloSubBlock::NEUTRAL) index = 0;
	  JemSubBlock* subBlock = m_jemBlocks[index];
	  subBlock->setJetHits(slice, vec[slice]);
        }
      }
      LVL1::JEMEtSums* et = findEnergySums(crate, module);
      if (et) {
        std::vector<unsigned int> exVec(et->ExVec());
        std::vector<unsigned int> eyVec(et->EyVec());
        std::vector<unsigned int> etVec(et->EtVec());
	exVec.resize(timeslices);
	eyVec.resize(timeslices);
	etVec.resize(timeslices);
	for (int slice = 0; slice < timeslices; ++slice) {
	  int index = slice;
	  if (m_dataFormat == L1CaloSubBlock::NEUTRAL) index = 0;
	  JemSubBlock* subBlock = m_jemBlocks[index];
	  subBlock->setEnergySubsums(slice, exVec[slice], eyVec[slice],
	                                                  etVec[slice]);
        }
      }
      
      // Pack and write the sub-blocks

      DataVector<JemSubBlock>::const_iterator pos;
      for (pos = m_jemBlocks.begin(); pos != m_jemBlocks.end(); ++pos) {
        JemSubBlock* subBlock = *pos;
	if ( !subBlock->pack()) {
	  log << MSG::ERROR << "JEM sub-block packing failed" << endreq;
	  return StatusCode::FAILURE;
	}
	subBlock->write(theROD);
      }

      // Append CMMs to last S-Link

      if (crate == m_crates - 1 && module == m_modules - 1) {
      }
    }
  }

  // Fill the raw event

  m_fea.fill(re, log);

  return StatusCode::SUCCESS;
}

// Print jet element data values

void JepByteStreamTool::printJeData(int channel,
                                    const ChannelCoordinate& coord,
                                    const std::vector<int>& emData, 
				    const std::vector<int>& hadData,
				    const std::vector<int>& emParity,
				    const std::vector<int>& hadParity,
				    const std::vector<int>& linkError,
				    MsgStream& log, MSG::Level level)
{
  double eta = coord.eta();
  double phi = coord.phi();
  unsigned int key = m_elementKey->jeKey(phi, eta);

  log << level
      << "key/channel/eta/phi/em/had/emErr/hadErr/linkErr: "
      << key << "/" << channel << "/" << eta << "/" << phi << "/";

  printVec(emData,    log, level);
  printVec(hadData,   log, level);
  printVec(emParity,  log, level);
  printVec(hadParity, log, level);
  printVec(linkError, log, level);
  log << level << endreq;
}

// Print vector values

void JepByteStreamTool::printVec(const std::vector<int>& vec, MsgStream& log,
                                                              MSG::Level level)
{
  std::vector<int>::const_iterator pos;
  for (pos = vec.begin(); pos != vec.end(); ++pos) {
    if (pos != vec.begin()) log << level << ",";
    log << level << *pos;
  }
  log << level << "/";
}

// Find a jet element given eta, phi

LVL1::JetElement* JepByteStreamTool::findJetElement(double eta, double phi)
{
  LVL1::JetElement* tt = 0;
  unsigned int key = m_elementKey->jeKey(phi, eta);
  JetElementMap::const_iterator mapIter;
  mapIter = m_jeMap.find(key);
  if (mapIter != m_jeMap.end()) tt = mapIter->second;
  return tt;
}

// Find jet hits for given crate, module

LVL1::JEMHits* JepByteStreamTool::findJetHits(int crate, int module)
{
  LVL1::JEMHits* hits = 0;
  JetHitsMap::const_iterator mapIter;
  mapIter = m_hitsMap.find(crate*m_modules + module);
  if (mapIter != m_hitsMap.end()) hits = mapIter->second;
  return hits;
}

// Find energy sums for given crate, module

LVL1::JEMEtSums* JepByteStreamTool::findEnergySums(int crate, int module)
{
  LVL1::JEMEtSums* sums = 0;
  EnergySumsMap::const_iterator mapIter;
  mapIter = m_etMap.find(crate*m_modules + module);
  if (mapIter != m_etMap.end()) sums = mapIter->second;
  return sums;
}

// Set up jet element map

void JepByteStreamTool::setupJeMap(const JetElementCollection* jeCollection)
{
  m_jeMap.clear();
  if (jeCollection) {
    JetElementCollection::const_iterator pos  = jeCollection->begin();
    JetElementCollection::const_iterator pose = jeCollection->end();
    for (; pos != pose; ++pos) {
      LVL1::JetElement* je = *pos;
      unsigned int key = m_elementKey->jeKey(je->phi(), je->eta());
      m_jeMap.insert(std::make_pair(key, je));
    }
  }
}

// Set up jet hits map

void JepByteStreamTool::setupHitsMap(const JetHitsCollection* hitCollection)
{
  m_hitsMap.clear();
  if (hitCollection) {
    JetHitsCollection::const_iterator pos  = hitCollection->begin();
    JetHitsCollection::const_iterator pose = hitCollection->end();
    for (; pos != pose; ++pos) {
      LVL1::JEMHits* hits = *pos;
      int key = m_modules * hits->crate() + hits->module();
      m_hitsMap.insert(std::make_pair(key, hits));
    }
  }
}

// Set up energy sums map

void JepByteStreamTool::setupEtMap(const EnergySumsCollection* etCollection)
{
  m_etMap.clear();
  if (etCollection) {
    EnergySumsCollection::const_iterator pos  = etCollection->begin();
    EnergySumsCollection::const_iterator pose = etCollection->end();
    for (; pos != pose; ++pos) {
      LVL1::JEMEtSums* sums = *pos;
      int key = m_modules * sums->crate() + sums->module();
      m_etMap.insert(std::make_pair(key, sums));
    }
  }
}

// Get number of slices and triggered slice offset for next slink

bool JepByteStreamTool::slinkSlices(int crate, int module,
                        int modulesPerSlink, int& timeslices, int& trigJem)
{
  int slices = -1;
  int trigJ  =  0;
  for (int mod = module; mod < module + modulesPerSlink; ++mod) {
    for (int chan = 0; chan < m_channels; ++chan) {
      ChannelCoordinate coord;
      if ( !m_jemMaps->mapping(crate, mod, chan, coord)) continue;
      LVL1::JetElement* je = findJetElement(coord.phi(), coord.eta());
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
      int peak = je->peak();
      for (int i = 0; i < numdat; ++i) {
        if (sums[i] == 0) continue;
        if (slices < 0) {
	  slices = sizes[i];
	  trigJ  = peak;
	} else if (slices != sizes[i] || trigJ != peak) return false;
      }
    }
    LVL1::JEMHits* hits = findJetHits(crate, mod);
    if (hits) {
      unsigned int sum = std::accumulate((hits->JetHitsVec()).begin(),
                                         (hits->JetHitsVec()).end(), 0);
      if (sum) {
        int size = (hits->JetHitsVec()).size();
	int peak = hits->peak();
        if (slices < 0) {
	  slices = size;
	  trigJ  = peak;
        } else if (slices != size || trigJ != peak) return false;
      }
    }
    LVL1::JEMEtSums* et = findEnergySums(crate, mod);
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
      int peak = et->peak();
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

// Fill a vector with all possible Source Identifiers

void JepByteStreamTool::sourceIDs(std::vector<uint32_t>& vID) const
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
