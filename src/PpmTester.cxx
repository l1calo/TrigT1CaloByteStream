
#include <utility>

#include "TrigT1CaloByteStream/PpmTester.h"

#include "TrigT1Calo/TriggerTower.h"
#include "TrigT1Calo/TriggerTowerKey.h"
#include "TrigT1Interfaces/TrigT1CaloDefs.h"


PpmTester::PpmTester(const std::string& name, ISvcLocator* pSvcLocator)
                     : Algorithm(name, pSvcLocator), 
                       m_storeGate("StoreGateSvc", name),
                       m_towerKey(0)
{
  declareProperty("TriggerTowerLocation",
         m_triggerTowerLocation = LVL1::TrigT1CaloDefs::TriggerTowerLocation);
  declareProperty("ForceSlicesFADC", m_forceSlicesFadc = 0);
}

PpmTester::~PpmTester()
{
}

// Initialize

StatusCode PpmTester::initialize()
{
  MsgStream log( msgSvc(), name() );

  StatusCode sc = m_storeGate.retrieve();
  if ( sc.isFailure() ) {
    log << MSG::ERROR << "Couldn't connect to " << m_storeGate.typeAndName() 
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

void PpmTester::printTriggerTowers(MsgStream& log,
                                              const MSG::Level level) const
{
  log << level << "Number of Trigger Towers = " << m_ttMap.size() << endreq;
  TriggerTowerMap::const_iterator mapIter = m_ttMap.begin();
  TriggerTowerMap::const_iterator mapEnd  = m_ttMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    const LVL1::TriggerTower* const tt = mapIter->second;
    int emADCPeak = tt->emADCPeak();
    int hadADCPeak = tt->hadADCPeak();
    const int emADCSlices = (tt->emADC()).size();
    const int hadADCSlices = (tt->hadADC()).size();
    if (m_forceSlicesFadc && m_forceSlicesFadc < emADCSlices) {
      emADCPeak = m_forceSlicesFadc / 2;
    }
    if (m_forceSlicesFadc && m_forceSlicesFadc < hadADCSlices) {
      hadADCPeak = m_forceSlicesFadc / 2;
    }
    log << level
        << " EM:key/eta/phi/LUTpeak/FADCpeak/LUT/FADC/bcidLUT/bcidFADC/error: "
        << mapIter->first << "/" << tt->eta() << "/" << tt->phi() << "/"
	<< tt->emPeak() << "/" << emADCPeak << "/";
    printVec(tt->emLUT(),     log, level);
    printAdc(tt->emADC(),     log, level);
    printVec(tt->emBCIDvec(), log, level);
    printAdc(tt->emBCIDext(), log, level);
    log << level << tt->emError() << "/" << endreq;
    log << level
        << "HAD:key/eta/phi/LUTpeak/FADCpeak/LUT/FADC/bcidLUT/bcidFADC/error: "
        << mapIter->first << "/" << tt->eta() << "/" << tt->phi() << "/"
	<< tt->hadPeak() << "/" << hadADCPeak << "/";
    printVec(tt->hadLUT(),     log, level);
    printAdc(tt->hadADC(),     log, level);
    printVec(tt->hadBCIDvec(), log, level);
    printAdc(tt->hadBCIDext(), log, level);
    log << level << tt->hadError() << "/" << endreq;
  }
}

// Print FADC vector

void PpmTester::printAdc(const std::vector<int>& vec, MsgStream& log,
                                                const MSG::Level level) const
{
  const int slices = vec.size();
  if (m_forceSlicesFadc && m_forceSlicesFadc < slices) {
    const int offset = (slices - m_forceSlicesFadc) / 2;
    std::vector<int> newVec;
    for (int sl = 0; sl < m_forceSlicesFadc; ++sl) {
      newVec.push_back(vec[sl + offset]);
    }
    printVec(newVec, log, level);
  } else printVec(vec, log, level);
}

// Print a vector

void PpmTester::printVec(const std::vector<int>& vec, MsgStream& log,
                                                const MSG::Level level) const
{
  std::vector<int>::const_iterator pos;
  for (pos = vec.begin(); pos != vec.end(); ++pos) {
    if (pos != vec.begin()) log << level << ",";
    log << level << *pos;
  }
  log << level << "/";
}

// Set up trigger tower map

void PpmTester::setupTTMap(const TriggerTowerCollection* const ttCollection)
{
  m_ttMap.clear();
  TriggerTowerCollection::const_iterator pos  = ttCollection->begin();
  TriggerTowerCollection::const_iterator pose = ttCollection->end();
  for (; pos != pose; ++pos) {
    const LVL1::TriggerTower* const tt = *pos;
    const unsigned int key = m_towerKey->ttKey(tt->phi(), tt->eta());
    m_ttMap.insert(std::make_pair(key, tt));
  }
}
