
#include <utility>

#include "StoreGate/StoreGateSvc.h"

#include "TrigT1Calo/TriggerTower.h"
#include "TrigT1Calo/TriggerTowerKey.h"
#include "TrigT1Interfaces/TrigT1CaloDefs.h"

#include "TrigT1CaloByteStream/PpmTester.h"


PpmTester::PpmTester(const std::string& name, ISvcLocator* pSvcLocator)
                     : Algorithm(name, pSvcLocator), m_towerKey(0)
{
  declareProperty("TriggerTowerLocation",
         m_triggerTowerLocation = LVL1::TrigT1CaloDefs::TriggerTowerLocation);
}

PpmTester::~PpmTester()
{
}

// Initialize

StatusCode PpmTester::initialize()
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

StatusCode PpmTester::execute()
{
  MsgStream log( msgSvc(), name() );

  // Find trigger towers

  const TriggerTowerCollection* ttCollection = 0;
  StatusCode sc = m_storeGate->retrieve(ttCollection, m_triggerTowerLocation);
  if (sc.isFailure() || !ttCollection || ttCollection->empty()) {
    log << MSG::DEBUG << "No Trigger Towers found" << endreq;
    return StatusCode::SUCCESS;
  }

  // Order by key

  setupTTMap(ttCollection);

  // Print the trigger towers

  printTriggerTowers(log, MSG::INFO);

  return StatusCode::SUCCESS;
}

// Finalize

StatusCode PpmTester::finalize()
{
  delete m_towerKey;

  return StatusCode::SUCCESS;
}

// Print the trigger towers

void PpmTester::printTriggerTowers(MsgStream& log, MSG::Level level)
{
  log << level << "Number of Trigger Towers = " << m_ttMap.size() << endreq;
  TriggerTowerMap::const_iterator mapIter = m_ttMap.begin();
  TriggerTowerMap::const_iterator mapEnd  = m_ttMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    LVL1::TriggerTower* tt = mapIter->second;
    log << level
        << " EM:key/eta/phi/LUTpeak/FADCpeak/LUT/FADC/bcidLUT/bcidFADC/error: "
        << mapIter->first << "/" << tt->eta() << "/" << tt->phi() << "/"
	<< tt->emPeak() << "/" << tt->emADCPeak() << "/";
    printVec(tt->emLUT(),     log, level);
    printVec(tt->emADC(),     log, level);
    printVec(tt->emBCIDvec(), log, level);
    printVec(tt->emBCIDext(), log, level);
    log << level << tt->emError() << "/" << endreq;
    log << level
        << "HAD:key/eta/phi/LUTpeak/FADCpeak/LUT/FADC/bcidLUT/bcidFADC/error: "
        << mapIter->first << "/" << tt->eta() << "/" << tt->phi() << "/"
	<< tt->hadPeak() << "/" << tt->hadADCPeak() << "/";
    printVec(tt->hadLUT(),     log, level);
    printVec(tt->hadADC(),     log, level);
    printVec(tt->hadBCIDvec(), log, level);
    printVec(tt->hadBCIDext(), log, level);
    log << level << tt->hadError() << "/" << endreq;
  }
}

// Print a vector

void PpmTester::printVec(const std::vector<int>& vec, MsgStream& log,
                                                      MSG::Level level)
{
  std::vector<int>::const_iterator pos;
  for (pos = vec.begin(); pos != vec.end(); ++pos) {
    if (pos != vec.begin()) log << level << ",";
    log << level << *pos;
  }
  log << level << "/";
}

// Set up trigger tower map

void PpmTester::setupTTMap(const TriggerTowerCollection* ttCollection)
{
  m_ttMap.clear();
  TriggerTowerCollection::const_iterator pos  = ttCollection->begin();
  TriggerTowerCollection::const_iterator pose = ttCollection->end();
  for (; pos != pose; ++pos) {
    LVL1::TriggerTower* tt = *pos;
    unsigned int key = m_towerKey->ttKey(tt->phi(), tt->eta());
    m_ttMap.insert(std::make_pair(key, tt));
  }
}
