
#include <utility>

#include "StoreGate/StoreGateSvc.h"

#include "TrigT1Calo/CMMCPHits.h"
#include "TrigT1Calo/CPMHits.h"
#include "TrigT1Calo/CPMTower.h"
#include "TrigT1Calo/TriggerTowerKey.h"
#include "TrigT1Interfaces/TrigT1CaloDefs.h"

#include "TrigT1CaloByteStream/CpmCrateMappings.h"
#include "TrigT1CaloByteStream/CpmTester.h"


CpmTester::CpmTester(const std::string& name, ISvcLocator* pSvcLocator)
                     : Algorithm(name, pSvcLocator), m_towerKey(0)
{
  declareProperty("CPMTowerLocation",
           m_cpmTowerLocation  = LVL1::TrigT1CaloDefs::CPMTowerLocation);
  declareProperty("CPMHitsLocation",
           m_cpmHitsLocation   = LVL1::TrigT1CaloDefs::CPMHitsLocation);
  declareProperty("CMMCPHitsLocation",
           m_cmmCpHitsLocation = LVL1::TrigT1CaloDefs::CMMCPHitsLocation);

  // By default print everything
  declareProperty("CPMTowerPrint",  m_cpmTowerPrint  = 1);
  declareProperty("CPMHitsPrint",   m_cpmHitsPrint   = 1);
  declareProperty("CMMCPHitsPrint", m_cmmCpHitsPrint = 1);

  m_modules = CpmCrateMappings::modules();
}

CpmTester::~CpmTester()
{
}

// Initialize

StatusCode CpmTester::initialize()
{
  MsgStream log( msgSvc(), name() );

  StatusCode sc = service("StoreGateSvc", m_storeGate);
  if (sc.isFailure()) {
    log << MSG::ERROR << "Unable to get pointer to StoreGate service"
                      << endreq;
    return sc;
  }
  m_towerKey = new LVL1::TriggerTowerKey();

  return StatusCode::SUCCESS;
}

// Execute

StatusCode CpmTester::execute()
{
  MsgStream log( msgSvc(), name() );

  if (m_cpmTowerPrint) {

    // Find CPM towers

    const CpmTowerCollection* ttCollection = 0;
    StatusCode sc = m_storeGate->retrieve(ttCollection, m_cpmTowerLocation);
    if (sc.isFailure() || !ttCollection || ttCollection->empty()) {
      log << MSG::INFO << "No CPM towers found" << endreq;
    } else {

      // Order by eta, phi

      setupCpmTowerMap(ttCollection);

      // Print the CPM towers

      printCpmTowers(log, MSG::INFO);
    }
  }

  if (m_cpmHitsPrint) {

    // Find CPM hits

    const CpmHitsCollection* hitCollection = 0;
    StatusCode sc = m_storeGate->retrieve(hitCollection, m_cpmHitsLocation);
    if (sc.isFailure() || !hitCollection || hitCollection->empty()) {
      log << MSG::INFO << "No CPM Hits found" << endreq;
    } else {

      // Order by crate, module

      setupCpmHitsMap(hitCollection);

      // Print the CPM hits

      printCpmHits(log, MSG::INFO);
    }
  }

  if (m_cmmCpHitsPrint) {

    // Find CMM-CP hits

    const CmmCpHitsCollection* hitCollection = 0;
    StatusCode sc = m_storeGate->retrieve(hitCollection, m_cmmCpHitsLocation);
    if (sc.isFailure() || !hitCollection || hitCollection->empty()) {
      log << MSG::INFO << "No CMM-CP Hits found" << endreq;
    } else {

      // Order by crate, dataID

      setupCmmCpHitsMap(hitCollection);

      // Print the CMM-CP hits

      printCmmCpHits(log, MSG::INFO);
    }
  }

  return StatusCode::SUCCESS;
}

// Finalize

StatusCode CpmTester::finalize()
{
  delete m_towerKey;

  return StatusCode::SUCCESS;
}

// Print the CPM towers

void CpmTester::printCpmTowers(MsgStream& log, MSG::Level level)
{
  log << level << "Number of CPM towers = " << m_ttMap.size() << endreq;
  CpmTowerMap::const_iterator mapIter = m_ttMap.begin();
  CpmTowerMap::const_iterator mapEnd  = m_ttMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    LVL1::CPMTower* tt = mapIter->second;
    log << level
        //<< "key/eta/phi/peak/em/had/emErr/hadErr/linkErr: "
        << "key/eta/phi/peak/em/had/emErr/hadErr: "
        << mapIter->first << "/" << tt->eta() << "/" << tt->phi() << "/"
	<< tt->peak() << "/";

    printVec(tt->emEnergyVec(),  log, level);
    printVec(tt->hadEnergyVec(), log, level);
    printVec(tt->emErrorVec(),   log, level);
    printVec(tt->hadErrorVec(),  log, level);
    //printVec(tt->linkErrorVec(), log, level);
    log << level << endreq;
  }
}

// Print the CPM hits

void CpmTester::printCpmHits(MsgStream& log, MSG::Level level)
{
  log << level << "Number of CPM Hits = " << m_hitsMap.size() << endreq;
  CpmHitsMap::const_iterator mapIter = m_hitsMap.begin();
  CpmHitsMap::const_iterator mapEnd  = m_hitsMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    LVL1::CPMHits* ch = mapIter->second;
    log << level
        << "crate/module/peak/hits0/hits1: "
	<< ch->crate() << "/" << ch->module() << "/" << ch->peak() << "/";

    printVecH(ch->HitsVec0(), log, level);
    printVecH(ch->HitsVec1(), log, level);
    log << level << endreq;
  }
}

// Print the CMM-CP hits

void CpmTester::printCmmCpHits(MsgStream& log, MSG::Level level)
{
  log << level << "Number of CMM-CP Hits = " << m_cmmHitsMap.size() << endreq;
  CmmCpHitsMap::const_iterator mapIter = m_cmmHitsMap.begin();
  CmmCpHitsMap::const_iterator mapEnd  = m_cmmHitsMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    LVL1::CMMCPHits* ch = mapIter->second;
    log << level
        << "crate/dataID/peak/hits0/hits1/err0/err1: "
	<< ch->crate() << "/" << ch->dataID() << "/" << ch->peak() << "/";

    printVecH(ch->HitsVec0(), log, level);
    printVecH(ch->HitsVec1(), log, level);
    printVec(ch->ErrorVec0(), log, level);
    printVec(ch->ErrorVec1(), log, level);
    log << level << endreq;
  }
}

// Print a vector

void CpmTester::printVec(const std::vector<int>& vec, MsgStream& log,
                                                      MSG::Level level)
{
  std::vector<int>::const_iterator pos;
  for (pos = vec.begin(); pos != vec.end(); ++pos) {
    if (pos != vec.begin()) log << level << ",";
    log << level << *pos;
  }
  log << level << "/";
}

// Print a vector of hits

void CpmTester::printVecH(const std::vector<unsigned int>& vec, MsgStream& log,
                                                           MSG::Level level)
{
  const int words = 8;
  const int bits  = 3;
  const unsigned int mask = 0x7;
  std::vector<unsigned int>::const_iterator pos;
  std::vector<unsigned int>::const_iterator posb = vec.begin();
  std::vector<unsigned int>::const_iterator pose = vec.end();
  for (pos = posb; pos != pose; ++pos) {
    if (pos != posb) log << level << ",";
    unsigned int hits = *pos;
    for (int i = 0; i < words; ++i) {
      if (i != 0) log << level << ":";
      unsigned int thr = (hits >> (bits*i)) & mask;
      log << level << thr;
    }
  }
  log << level << "/";
}

// Set up CPM tower map

void CpmTester::setupCpmTowerMap(const CpmTowerCollection* ttCollection)
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

void CpmTester::setupCpmHitsMap(const CpmHitsCollection* hitCollection)
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

void CpmTester::setupCmmCpHitsMap(const CmmCpHitsCollection* hitCollection)
{
  m_cmmHitsMap.clear();
  if (hitCollection) {
    CmmCpHitsCollection::const_iterator pos  = hitCollection->begin();
    CmmCpHitsCollection::const_iterator pose = hitCollection->end();
    for (; pos != pose; ++pos) {
      LVL1::CMMCPHits* hits = *pos;
      int key = hits->crate()*100 + hits->dataID();
      m_cmmHitsMap.insert(std::make_pair(key, hits));
    }
  }
}
