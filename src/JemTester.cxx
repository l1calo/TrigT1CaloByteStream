
#include <utility>

#include "TrigT1CaloByteStream/JemTester.h"
#include "TrigT1Calo/CMMEtSums.h"
#include "TrigT1Calo/CMMJetHits.h"
#include "TrigT1Calo/CMMRoI.h"
#include "TrigT1Calo/JEMEtSums.h"
#include "TrigT1Calo/JEMHits.h"
#include "TrigT1Calo/JEMRoI.h"
#include "TrigT1Calo/JetElement.h"
#include "TrigT1Calo/JetElementKey.h"
#include "TrigT1Interfaces/TrigT1CaloDefs.h"


JemTester::JemTester(const std::string& name, ISvcLocator* pSvcLocator)
                     : Algorithm(name, pSvcLocator), 
                       m_storeGate("StoreGateSvc", name),
                       m_elementKey(0)
{
  declareProperty("JetElementLocation",
           m_jetElementLocation = LVL1::TrigT1CaloDefs::JetElementLocation);
  declareProperty("JEMHitsLocation",
           m_jemHitsLocation    = LVL1::TrigT1CaloDefs::JEMHitsLocation);
  declareProperty("JEMEtSumsLocation",
           m_jemEtSumsLocation  = LVL1::TrigT1CaloDefs::JEMEtSumsLocation);
  declareProperty("CMMJetHitsLocation",
           m_cmmJetLocation     = LVL1::TrigT1CaloDefs::CMMJetHitsLocation);
  declareProperty("CMMEtSumsLocation",
           m_cmmEnergyLocation  = LVL1::TrigT1CaloDefs::CMMEtSumsLocation);
  declareProperty("JEMRoILocation",
           m_jemRoiLocation     = LVL1::TrigT1CaloDefs::JEMRoILocation);
  declareProperty("CMMRoILocation",
           m_cmmRoiLocation     = LVL1::TrigT1CaloDefs::CMMRoILocation);

  // By default print everything
  declareProperty("JetElementPrint", m_jetElementPrint = 1);
  declareProperty("JEMHitsPrint",    m_jemHitsPrint    = 1);
  declareProperty("JEMEtSumsPrint",  m_jemEtSumsPrint  = 1);
  declareProperty("CMMJetHitsPrint", m_cmmHitsPrint    = 1);
  declareProperty("CMMEtSumsPrint",  m_cmmEtSumsPrint  = 1);
  declareProperty("JEMRoIPrint",     m_jemRoiPrint     = 1);
  declareProperty("CMMRoIPrint",     m_cmmRoiPrint     = 1);
}

JemTester::~JemTester()
{
}

// Initialize

StatusCode JemTester::initialize()
{
  MsgStream log( msgSvc(), name() );

  StatusCode sc = m_storeGate.retrieve();
  if ( sc.isFailure() ) {
    log << MSG::ERROR << "Couldn't connect to " << m_storeGate.typeAndName() 
        << endreq;
    return sc;
  }
  m_elementKey = new LVL1::JetElementKey();

  return StatusCode::SUCCESS;
}

// Execute

StatusCode JemTester::execute()
{
  MsgStream log( msgSvc(), name() );

  if (m_jetElementPrint) {

    // Find jet elements

    const JetElementCollection* jeCollection = 0;
    StatusCode sc = m_storeGate->retrieve(jeCollection, m_jetElementLocation);
    if (sc.isFailure() || !jeCollection || jeCollection->empty()) {
      log << MSG::INFO << "No Jet Elements found" << endreq;
    } else {

      // Order by eta, phi

      setupJeMap(jeCollection);

      // Print the jet elements

      printJetElements(log, MSG::INFO);
    }
  }

  if (m_jemHitsPrint) {

    // Find jet hits

    const JetHitsCollection* hitCollection = 0;
    StatusCode sc = m_storeGate->retrieve(hitCollection, m_jemHitsLocation);
    if (sc.isFailure() || !hitCollection || hitCollection->empty()) {
      log << MSG::INFO << "No Jet Hits found" << endreq;
    } else {

      // Order by crate, module

      setupHitsMap(hitCollection);

      // Print the jet hits

      printJetHits(log, MSG::INFO);
    }
  }

  if (m_jemEtSumsPrint) {

    // Find energy sums

    const EnergySumsCollection* etCollection = 0;
    StatusCode sc = m_storeGate->retrieve(etCollection, m_jemEtSumsLocation);
    if (sc.isFailure() || !etCollection || etCollection->empty()) {
      log << MSG::INFO << "No Energy Sums found" << endreq;
    } else {

      // Order by crate, module

      setupEtMap(etCollection);

      // Print the energy sums

      printEnergySums(log, MSG::INFO);
    }
  }

  if (m_cmmHitsPrint) {

    // Find CMM hits

    const CmmJetCollection* hitCollection = 0;
    StatusCode sc = m_storeGate->retrieve(hitCollection, m_cmmJetLocation);
    if (sc.isFailure() || !hitCollection || hitCollection->empty()) {
      log << MSG::INFO << "No CMM Hits found" << endreq;
    } else {

      // Order by crate, dataID

      setupCmmHitsMap(hitCollection);

      // Print the CMM hits

      printCmmHits(log, MSG::INFO);
    }
  }

  if (m_cmmEtSumsPrint) {

    // Find CMM energy sums

    const CmmEnergyCollection* etCollection = 0;
    StatusCode sc = m_storeGate->retrieve(etCollection, m_cmmEnergyLocation);
    if (sc.isFailure() || !etCollection || etCollection->empty()) {
      log << MSG::INFO << "No CMM Energy Sums found" << endreq;
    } else {

      // Order by crate, dataID

      setupCmmEtMap(etCollection);

      // Print the CMM energy sums

      printCmmSums(log, MSG::INFO);
    }
  }

  if (m_jemRoiPrint) {

    // Find JEM RoIs

    const JemRoiCollection* jrCollection = 0;
    StatusCode sc = m_storeGate->retrieve(jrCollection, m_jemRoiLocation);
    if (sc.isFailure() || !jrCollection || jrCollection->empty()) {
      log << MSG::INFO << "No JEM RoIs found" << endreq;
    } else {

      // Order by RoI word

      setupJemRoiMap(jrCollection);

      // Print the JEM RoIs

      printJemRois(log, MSG::INFO);
    }
  }

  if (m_cmmRoiPrint) {

    // Find CMM RoIs

    const LVL1::CMMRoI* crCollection = 0;
    StatusCode sc = m_storeGate->retrieve(crCollection, m_cmmRoiLocation);
    if (sc.isFailure() || !crCollection) {
      log << MSG::INFO << "No CMM RoIs found" << endreq;
    } else {

      // Print the CMM RoIs

      printCmmRois(crCollection, log, MSG::INFO);
    }
  }

  return StatusCode::SUCCESS;
}

// Finalize

StatusCode JemTester::finalize()
{
  delete m_elementKey;

  return StatusCode::SUCCESS;
}

// Print the jet elements

void JemTester::printJetElements(MsgStream& log, const MSG::Level level) const
{
  log << level << "Number of Jet Elements = " << m_jeMap.size() << endreq;
  JetElementMap::const_iterator mapIter = m_jeMap.begin();
  JetElementMap::const_iterator mapEnd  = m_jeMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    const LVL1::JetElement* const je = mapIter->second;
    log << level
        << "key/eta/phi/peak/em/had/emErr/hadErr/linkErr: "
        << mapIter->first << "/" << je->eta() << "/" << je->phi() << "/"
	<< je->peak() << "/";

    printVec(je->emEnergyVec(),  log, level);
    printVec(je->hadEnergyVec(), log, level);
    printVec(je->emErrorVec(),   log, level);
    printVec(je->hadErrorVec(),  log, level);
    printVec(je->linkErrorVec(), log, level);
    log << level << endreq;
  }
}

// Print the jet hits

void JemTester::printJetHits(MsgStream& log, const MSG::Level level) const
{
  log << level << "Number of Jet Hits = " << m_hitsMap.size() << endreq;
  JetHitsMap::const_iterator mapIter = m_hitsMap.begin();
  JetHitsMap::const_iterator mapEnd  = m_hitsMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    const LVL1::JEMHits* const jh = mapIter->second;
    log << level
        << "crate/module/peak/hits: "
	<< jh->crate() << "/" << jh->module() << "/" << jh->peak() << "/";
    int words = 8;
    int bits  = 3;
    unsigned int mask = 0x7;
    if (jh->forward()) {
      words = 12;
      bits  = 2;
      mask  = 0x3;
    }
    std::vector<unsigned int>::const_iterator pos;
    std::vector<unsigned int>::const_iterator posb = (jh->JetHitsVec()).begin();
    std::vector<unsigned int>::const_iterator pose = (jh->JetHitsVec()).end();
    for (pos = posb; pos != pose; ++pos) {
      if (pos != posb) log << level << ",";
      const unsigned int hits = *pos;
      for (int i = 0; i < words; ++i) {
        if (i != 0) log << level << ":";
	const unsigned int thr = (hits >> bits*i) & mask;
        log << level << thr;
      }
    }
    log << level << "/" << endreq;
  }
}

// Print energy sums

void JemTester::printEnergySums(MsgStream& log, const MSG::Level level) const
{
  log << level << "Number of Energy Sums = " << m_etMap.size() << endreq;
  EnergySumsMap::const_iterator mapIter = m_etMap.begin();
  EnergySumsMap::const_iterator mapEnd  = m_etMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    const LVL1::JEMEtSums* const et = mapIter->second;
    log << level
        << "crate/module/peak/Ex/Ey/Et: "
	<< et->crate() << "/" << et->module() << "/" << et->peak() << "/";
    printVecU(et->ExVec(), log, level);
    printVecU(et->EyVec(), log, level);
    printVecU(et->EtVec(), log, level);
    log << level << endreq;
  }
}

// Print the CMM hits

void JemTester::printCmmHits(MsgStream& log, const MSG::Level level) const
{
  log << level << "Number of CMM Hits = " << m_cmmHitsMap.size() << endreq;
  CmmHitsMap::const_iterator mapIter = m_cmmHitsMap.begin();
  CmmHitsMap::const_iterator mapEnd  = m_cmmHitsMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    const LVL1::CMMJetHits* const jh = mapIter->second;
    const int dataID = jh->dataID();
    log << level
        << "crate/dataID/peak/hits/error: "
	<< jh->crate() << "/" << dataID << "/" << jh->peak() << "/";
    int words = 8;
    int bits  = 3;
    unsigned int mask = 0x7;
    if (dataID == 0 || dataID == 7 || dataID == 8 || dataID == 15) {
      words = 12;
      bits  = 2;
      mask  = 0x3;
    } else if (dataID == LVL1::CMMJetHits::REMOTE_FORWARD ||
               dataID == LVL1::CMMJetHits::LOCAL_FORWARD  ||
	       dataID == LVL1::CMMJetHits::TOTAL_FORWARD) {
      bits  = 2;
      mask  = 0x3;
    } else if (dataID == LVL1::CMMJetHits::ET_MAP) {
      words = 1;
      bits  = 4;
      mask  = 0xf;
    }
    std::vector<unsigned int>::const_iterator pos;
    std::vector<unsigned int>::const_iterator posb = (jh->HitsVec()).begin();
    std::vector<unsigned int>::const_iterator pose = (jh->HitsVec()).end();
    for (pos = posb; pos != pose; ++pos) {
      if (pos != posb) log << level << ",";
      const unsigned int hits = *pos;
      for (int i = 0; i < words; ++i) {
        if (i != 0) log << level << ":";
	const unsigned int thr = (hits >> bits*i) & mask;
        log << level << thr;
      }
    }
    log << level << "/";
    printVec(jh->ErrorVec(), log, level);
    log << level << endreq;
  }
}

// Print CMM energy sums

void JemTester::printCmmSums(MsgStream& log, const MSG::Level level) const
{
  log << level << "Number of CMM Energy Sums = " << m_cmmEtMap.size() << endreq;
  CmmSumsMap::const_iterator mapIter = m_cmmEtMap.begin();
  CmmSumsMap::const_iterator mapEnd  = m_cmmEtMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    const LVL1::CMMEtSums* const et = mapIter->second;
    log << level << "crate/dataID/peak/Ex/Ey/Et/ExErr/EyErr/EtErr: "
	<< et->crate() << "/" << et->dataID() << "/" << et->peak() << "/";
    printVecU(et->ExVec(), log, level);
    printVecU(et->EyVec(), log, level);
    printVecU(et->EtVec(), log, level);
    printVec(et->ExErrorVec(), log, level);
    printVec(et->EyErrorVec(), log, level);
    printVec(et->EtErrorVec(), log, level);
    log << level << endreq;
  }
}

// Print the JEM RoIs

void JemTester::printJemRois(MsgStream& log, const MSG::Level level) const
{
  log << level << "Number of JEM RoIs = " << m_roiMap.size() << endreq;
  JemRoiMap::const_iterator mapIter = m_roiMap.begin();
  JemRoiMap::const_iterator mapEnd  = m_roiMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    const LVL1::JEMRoI* const roi = mapIter->second;
    log << level << "crate/jem/frame/loc/fwd/hits/error: "
        << roi->crate() << "/" << roi->jem() << "/" << roi->frame() << "/"
	<< roi->location() << "/" << roi->forward() << "/";
    MSG::hex(log) << level << roi->hits() << "/" << roi->error() << "/";
    MSG::dec(log) << level << endreq;
  }
}

// Print the CMM RoIs

void JemTester::printCmmRois(const LVL1::CMMRoI* const roi, MsgStream& log,
                                                 const MSG::Level level) const
{
  log << level << "jetEtHits/sumEtHits/missingEtHits/Ex/Ey/Et;error: ";
  MSG::hex(log) << level << roi->jetEtHits() << ";" << roi->jetEtError() << "/"
                << roi->sumEtHits() << ";" << roi->sumEtError() << "/"
		<< roi->missingEtHits() << ";" << roi->missingEtError() << "/";
  MSG::dec(log) << level << roi->ex() << ";" << roi->exError() << "/"
                         << roi->ey() << ";" << roi->eyError() << "/"
                         << roi->et() << ";" << roi->etError() << "/" << endreq;
}

// Print a vector

void JemTester::printVec(const std::vector<int>& vec, MsgStream& log,
                                                const MSG::Level level) const
{
  std::vector<int>::const_iterator pos;
  for (pos = vec.begin(); pos != vec.end(); ++pos) {
    if (pos != vec.begin()) log << level << ",";
    log << level << *pos;
  }
  log << level << "/";
}

// Print a vector (unsigned)

void JemTester::printVecU(const std::vector<unsigned int>& vec, MsgStream& log,
                                                  const MSG::Level level) const
{
  std::vector<unsigned int>::const_iterator pos;
  for (pos = vec.begin(); pos != vec.end(); ++pos) {
    if (pos != vec.begin()) log << level << ",";
    log << level << *pos;
  }
  log << level << "/";
}

// Set up jet element map

void JemTester::setupJeMap(const JetElementCollection* const jeCollection)
{
  m_jeMap.clear();
  if (jeCollection) {
    JetElementCollection::const_iterator pos = jeCollection->begin();
    JetElementCollection::const_iterator pose = jeCollection->end();
    for (; pos != pose; ++pos) {
      const LVL1::JetElement* const je = *pos;
      const unsigned int key = m_elementKey->jeKey(je->phi(), je->eta());
      m_jeMap.insert(std::make_pair(key, je));
    }
  }
}

// Set up jet hits map

void JemTester::setupHitsMap(const JetHitsCollection* const hitCollection)
{
  m_hitsMap.clear();
  if (hitCollection) {
    JetHitsCollection::const_iterator pos  = hitCollection->begin();
    JetHitsCollection::const_iterator pose = hitCollection->end();
    for (; pos != pose; ++pos) {
      const LVL1::JEMHits* const hits = *pos;
      const int key = hits->crate()*100 + hits->module();
      m_hitsMap.insert(std::make_pair(key, hits));
    }
  }
}

// Set up energy sums map

void JemTester::setupEtMap(const EnergySumsCollection* const etCollection)
{
  m_etMap.clear();
  if (etCollection) {
    EnergySumsCollection::const_iterator pos  = etCollection->begin();
    EnergySumsCollection::const_iterator pose = etCollection->end();
    for (; pos != pose; ++pos) {
      const LVL1::JEMEtSums* const sums = *pos;
      const int key = sums->crate()*100 + sums->module();
      m_etMap.insert(std::make_pair(key, sums));
    }
  }
}

// Set up CMM hits map

void JemTester::setupCmmHitsMap(const CmmJetCollection* const hitCollection)
{
  m_cmmHitsMap.clear();
  if (hitCollection) {
    CmmJetCollection::const_iterator pos  = hitCollection->begin();
    CmmJetCollection::const_iterator pose = hitCollection->end();
    for (; pos != pose; ++pos) {
      const LVL1::CMMJetHits* const hits = *pos;
      const int key = hits->crate()*100 + hits->dataID();
      m_cmmHitsMap.insert(std::make_pair(key, hits));
    }
  }
}

// Set up CMM energy sums map

void JemTester::setupCmmEtMap(const CmmEnergyCollection* const etCollection)
{
  m_cmmEtMap.clear();
  if (etCollection) {
    CmmEnergyCollection::const_iterator pos  = etCollection->begin();
    CmmEnergyCollection::const_iterator pose = etCollection->end();
    for (; pos != pose; ++pos) {
      const LVL1::CMMEtSums* const sums = *pos;
      const int key = sums->crate()*100 + sums->dataID();
      m_cmmEtMap.insert(std::make_pair(key, sums));
    }
  }
}

// Set up JEM RoI map

void JemTester::setupJemRoiMap(const JemRoiCollection* const jrCollection)
{
  m_roiMap.clear();
  if (jrCollection) {
    JemRoiCollection::const_iterator pos  = jrCollection->begin();
    JemRoiCollection::const_iterator pose = jrCollection->end();
    for (; pos != pose; ++pos) {
      const LVL1::JEMRoI* const roi = *pos;
      const uint32_t key = roi->roiWord();
      m_roiMap.insert(std::make_pair(key, roi));
    }
  }
}
