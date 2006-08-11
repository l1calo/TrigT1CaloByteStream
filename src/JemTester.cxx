
#include <utility>

#include "StoreGate/StoreGateSvc.h"

#include "TrigT1Calo/JetElement.h"
#include "TrigT1Calo/JetElementKey.h"
#include "TrigT1Calo/JEMHits.h"
#include "TrigT1Calo/JEMEtSums.h"
#include "TrigT1Interfaces/TrigT1CaloDefs.h"

#include "TrigT1CaloByteStream/JemCrateMappings.h"
#include "TrigT1CaloByteStream/JemTester.h"


JemTester::JemTester(const std::string& name, ISvcLocator* pSvcLocator)
                     : Algorithm(name, pSvcLocator), m_elementKey(0)
{
  declareProperty("JetElementLocation",
           m_jetElementLocation = LVL1::TrigT1CaloDefs::JetElementLocation);
  declareProperty("JEMHitsLocation",
           m_jemHitsLocation    = LVL1::TrigT1CaloDefs::JEMHitsLocation);
  declareProperty("JEMEtSumsLocation",
           m_jemEtSumsLocation  = LVL1::TrigT1CaloDefs::JEMEtSumsLocation);

  // By default print everything
  declareProperty("JetElementPrint", m_jetElementPrint = 1);
  declareProperty("JEMHitsPrint",    m_jemHitsPrint    = 1);
  declareProperty("JEMEtSumsPrint",  m_jemEtSumsPrint  = 1);

  m_modules = JemCrateMappings::modules();
}

JemTester::~JemTester()
{
}

// Initialize

StatusCode JemTester::initialize()
{
  MsgStream log( msgSvc(), name() );

  StatusCode sc = service("StoreGateSvc", m_storeGate);
  if (sc.isFailure()) {
    log << MSG::ERROR << "Unable to get pointer to StoreGate service"
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

  return StatusCode::SUCCESS;
}

// Finalize

StatusCode JemTester::finalize()
{
  delete m_elementKey;

  return StatusCode::SUCCESS;
}

// Print the jet elements

void JemTester::printJetElements(MsgStream& log, MSG::Level level)
{
  log << level << "Number of Jet Elements = " << m_jeMap.size() << endreq;
  JetElementMap::const_iterator mapIter = m_jeMap.begin();
  JetElementMap::const_iterator mapEnd  = m_jeMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    LVL1::JetElement* je = mapIter->second;
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

void JemTester::printJetHits(MsgStream& log, MSG::Level level)
{
  log << level << "Number of Jet Hits = " << m_hitsMap.size() << endreq;
  JetHitsMap::const_iterator mapIter = m_hitsMap.begin();
  JetHitsMap::const_iterator mapEnd  = m_hitsMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    LVL1::JEMHits* jh = mapIter->second;
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
      unsigned int hits = *pos;
      for (int i = 0; i < words; ++i) {
        if (i != 0) log << level << ":";
	unsigned int thr = (hits >> bits*i) & mask;
        log << level << thr;
      }
    }
    log << level << "/" << endreq;
  }
}

// Print energy sums

void JemTester::printEnergySums(MsgStream& log, MSG::Level level)
{
  log << level << "Number of Energy Sums = " << m_etMap.size() << endreq;
  EnergySumsMap::const_iterator mapIter = m_etMap.begin();
  EnergySumsMap::const_iterator mapEnd  = m_etMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    LVL1::JEMEtSums* et = mapIter->second;
    log << level
        << "crate/module/peak/Ex/Ey/Et: "
	<< et->crate() << "/" << et->module() << "/" << et->peak() << "/";
    printVecU(et->ExVec(), log, level);
    printVecU(et->EyVec(), log, level);
    printVecU(et->EtVec(), log, level);
    log << level << endreq;
  }
}

// Print a vector

void JemTester::printVec(const std::vector<int>& vec, MsgStream& log,
                                                      MSG::Level level)
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
                                                           MSG::Level level)
{
  std::vector<unsigned int>::const_iterator pos;
  for (pos = vec.begin(); pos != vec.end(); ++pos) {
    if (pos != vec.begin()) log << level << ",";
    log << level << *pos;
  }
  log << level << "/";
}

// Set up jet element map

void JemTester::setupJeMap(const JetElementCollection* jeCollection)
{
  m_jeMap.clear();
  if (jeCollection) {
    JetElementCollection::const_iterator pos = jeCollection->begin();
    JetElementCollection::const_iterator pose = jeCollection->end();
    for (; pos != pose; ++pos) {
      LVL1::JetElement* je = *pos;
      unsigned int key = m_elementKey->jeKey(je->phi(), je->eta());
      m_jeMap.insert(std::make_pair(key, je));
    }
  }
}

// Set up jet hits map

void JemTester::setupHitsMap(const JetHitsCollection* hitCollection)
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

void JemTester::setupEtMap(const EnergySumsCollection* etCollection)
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
