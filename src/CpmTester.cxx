
#include <utility>

#include "TrigT1Calo/CMMCPHits.h"
#include "TrigT1Calo/CPMHits.h"
#include "TrigT1Calo/CPMTower.h"
#include "TrigT1Calo/CPMRoI.h"
#include "TrigT1Calo/TriggerTowerKey.h"
#include "TrigT1Interfaces/TrigT1CaloDefs.h"

#include "TrigT1CaloByteStream/CpmTester.h"
#include "TrigT1CaloByteStream/ModifySlices.h"

namespace LVL1BS {

CpmTester::CpmTester(const std::string& name, ISvcLocator* pSvcLocator)
                     : Algorithm(name, pSvcLocator),
		       m_storeGate("StoreGateSvc", name),
		       m_towerKey(0)
{
  declareProperty("CPMTowerLocation",
           m_cpmTowerLocation  = LVL1::TrigT1CaloDefs::CPMTowerLocation);
  declareProperty("CPMHitsLocation",
           m_cpmHitsLocation   = LVL1::TrigT1CaloDefs::CPMHitsLocation);
  declareProperty("CMMCPHitsLocation",
           m_cmmCpHitsLocation = LVL1::TrigT1CaloDefs::CMMCPHitsLocation);
  declareProperty("CPMRoILocation",
           m_cpmRoiLocation    = LVL1::TrigT1CaloDefs::CPMRoILocation);

  declareProperty("ForceSlicesCPM", m_forceSlicesCpm = 0);
  declareProperty("ForceSlicesCMM", m_forceSlicesCmm = 0);

  // By default print everything
  declareProperty("CPMTowerPrint",  m_cpmTowerPrint  = 1);
  declareProperty("CPMHitsPrint",   m_cpmHitsPrint   = 1);
  declareProperty("CMMCPHitsPrint", m_cmmCpHitsPrint = 1);
  declareProperty("CPMRoIPrint",    m_cpmRoiPrint    = 1);
}

CpmTester::~CpmTester()
{
}

// Initialize

StatusCode CpmTester::initialize()
{
  MsgStream log( msgSvc(), name() );

  StatusCode sc = m_storeGate.retrieve();
  if (sc.isFailure()) {
    log << MSG::ERROR << "Couldn't connect to " << m_storeGate.typeAndName()
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

  if (m_cpmRoiPrint) {

    // Find CPM RoIs

    const CpmRoiCollection* roiCollection = 0;
    StatusCode sc = m_storeGate->retrieve(roiCollection, m_cpmRoiLocation);
    if (sc.isFailure() || !roiCollection || roiCollection->empty()) {
      log << MSG::INFO << "No CPM RoIs found" << endreq;
    } else {

      // Order by RoI word

      setupCpmRoiMap(roiCollection);

      // Print the CPM RoIs

      printCpmRois(log, MSG::INFO);
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

void CpmTester::printCpmTowers(MsgStream& log, const MSG::Level level) const
{
  log << level << "Number of CPM towers = " << m_ttMap.size() << endreq;
  CpmTowerMap::const_iterator mapIter = m_ttMap.begin();
  CpmTowerMap::const_iterator mapEnd  = m_ttMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    const LVL1::CPMTower* const tt = mapIter->second;
    int peak   = tt->peak();
    int slices = (tt->emEnergyVec()).size();
    if (m_forceSlicesCpm) {
      peak   = ModifySlices::peak(peak, slices, m_forceSlicesCpm);
      slices = m_forceSlicesCpm;
    }
    log << level
        << "key/eta/phi/peak/em/had/emErr/hadErr: "
        << mapIter->first << "/" << tt->eta() << "/" << tt->phi() << "/"
	<< peak << "/";

    std::vector<int> emEnergy;
    std::vector<int> hadEnergy;
    std::vector<int> emError;
    std::vector<int> hadError;
    ModifySlices::data(tt->emEnergyVec(),  emEnergy,  slices);
    ModifySlices::data(tt->hadEnergyVec(), hadEnergy, slices);
    ModifySlices::data(tt->emErrorVec(),   emError,   slices);
    ModifySlices::data(tt->hadErrorVec(),  hadError,  slices);
    printVec(emEnergy,  log, level);
    printVec(hadEnergy, log, level);
    printVec(emError,   log, level);
    printVec(hadError,  log, level);
    log << level << endreq;
  }
}

// Print the CPM hits

void CpmTester::printCpmHits(MsgStream& log, const MSG::Level level) const
{
  log << level << "Number of CPM Hits = " << m_hitsMap.size() << endreq;
  CpmHitsMap::const_iterator mapIter = m_hitsMap.begin();
  CpmHitsMap::const_iterator mapEnd  = m_hitsMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    const LVL1::CPMHits* const ch = mapIter->second;
    int peak   = ch->peak();
    int slices = (ch->HitsVec0()).size();
    if (m_forceSlicesCpm) {
      peak   = ModifySlices::peak(peak, slices, m_forceSlicesCpm);
      slices = m_forceSlicesCpm;
    }
    log << level
        << "crate/module/peak/hits0/hits1: "
	<< ch->crate() << "/" << ch->module() << "/" << peak << "/";

    std::vector<unsigned int> hits0;
    std::vector<unsigned int> hits1;
    ModifySlices::data(ch->HitsVec0(), hits0, slices);
    ModifySlices::data(ch->HitsVec1(), hits1, slices);
    printVecH(hits0, log, level);
    printVecH(hits1, log, level);
    log << level << endreq;
  }
}

// Print the CMM-CP hits

void CpmTester::printCmmCpHits(MsgStream& log, const MSG::Level level) const
{
  log << level << "Number of CMM-CP Hits = " << m_cmmHitsMap.size() << endreq;
  CmmCpHitsMap::const_iterator mapIter = m_cmmHitsMap.begin();
  CmmCpHitsMap::const_iterator mapEnd  = m_cmmHitsMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    const LVL1::CMMCPHits* const ch = mapIter->second;
    int peak   = ch->peak();
    int slices = (ch->HitsVec0()).size();
    if (m_forceSlicesCmm) {
      peak   = ModifySlices::peak(peak, slices, m_forceSlicesCmm);
      slices = m_forceSlicesCmm;
    }
    log << level
        << "crate/dataID/peak/hits0/hits1/err0/err1: "
	<< ch->crate() << "/" << ch->dataID() << "/" << peak << "/";

    std::vector<unsigned int> hits0;
    std::vector<unsigned int> hits1;
    std::vector<int> err0;
    std::vector<int> err1;
    ModifySlices::data(ch->HitsVec0(), hits0, slices);
    ModifySlices::data(ch->HitsVec1(), hits1, slices);
    ModifySlices::data(ch->ErrorVec0(), err0, slices);
    ModifySlices::data(ch->ErrorVec1(), err1, slices);
    printVecH(hits0, log, level);
    printVecH(hits1, log, level);
    printVec(err0, log, level);
    printVec(err1, log, level);
    log << level << endreq;
  }
}

// Print the CPM RoIs

void CpmTester::printCpmRois(MsgStream& log, const MSG::Level level) const
{
  log << level << "Number of CPM RoIs = " << m_roiMap.size() << endreq;
  CpmRoiMap::const_iterator mapIter = m_roiMap.begin();
  CpmRoiMap::const_iterator mapEnd  = m_roiMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    const LVL1::CPMRoI* const roi = mapIter->second;
    log << level << "crate/cpm/chip/loc/hits/error: "
	<< roi->crate()    << "/" << roi->cpm() << "/" << roi->chip() << "/"
	<< roi->location() << "/";
    MSG::hex(log) << level << roi->hits() << "/" << roi->error() << "/";
    MSG::dec(log) << level << endreq;
  }
}

// Print a vector

void CpmTester::printVec(const std::vector<int>& vec, MsgStream& log,
                                                const MSG::Level level) const
{
  std::vector<int>::const_iterator pos;
  for (pos = vec.begin(); pos != vec.end(); ++pos) {
    if (pos != vec.begin()) log << level << ",";
    log << level << *pos;
  }
  log << level << "/";
}

// Print a vector of hits

void CpmTester::printVecH(const std::vector<unsigned int>& vec,
                          MsgStream& log, const MSG::Level level) const
{
  const int words = 8;
  const int bits  = 3;
  const unsigned int mask = 0x7;
  std::vector<unsigned int>::const_iterator pos;
  std::vector<unsigned int>::const_iterator posb = vec.begin();
  std::vector<unsigned int>::const_iterator pose = vec.end();
  for (pos = posb; pos != pose; ++pos) {
    if (pos != posb) log << level << ",";
    const unsigned int hits = *pos;
    for (int i = 0; i < words; ++i) {
      if (i != 0) log << level << ":";
      const unsigned int thr = (hits >> (bits*i)) & mask;
      log << level << thr;
    }
  }
  log << level << "/";
}

// Set up CPM tower map

void CpmTester::setupCpmTowerMap(const CpmTowerCollection* const ttCollection)
{
  m_ttMap.clear();
  if (ttCollection) {
    CpmTowerCollection::const_iterator pos  = ttCollection->begin();
    CpmTowerCollection::const_iterator pose = ttCollection->end();
    for (; pos != pose; ++pos) {
      const LVL1::CPMTower* const tt = *pos;
      const unsigned int key = m_towerKey->ttKey(tt->phi(), tt->eta());
      m_ttMap.insert(std::make_pair(key, tt));
    }
  }
}

// Set up CPM hits map

void CpmTester::setupCpmHitsMap(const CpmHitsCollection* const hitCollection)
{
  m_hitsMap.clear();
  if (hitCollection) {
    CpmHitsCollection::const_iterator pos  = hitCollection->begin();
    CpmHitsCollection::const_iterator pose = hitCollection->end();
    for (; pos != pose; ++pos) {
      const LVL1::CPMHits* const hits = *pos;
      const int key = hits->crate()*100 + hits->module() - 1;
      m_hitsMap.insert(std::make_pair(key, hits));
    }
  }
}

// Set up CMM-CP hits map

void CpmTester::setupCmmCpHitsMap(const CmmCpHitsCollection*
                                                       const hitCollection)
{
  m_cmmHitsMap.clear();
  if (hitCollection) {
    CmmCpHitsCollection::const_iterator pos  = hitCollection->begin();
    CmmCpHitsCollection::const_iterator pose = hitCollection->end();
    for (; pos != pose; ++pos) {
      const LVL1::CMMCPHits* const hits = *pos;
      const int key = hits->crate()*100 + hits->dataID();
      m_cmmHitsMap.insert(std::make_pair(key, hits));
    }
  }
}

// Set up CPM RoI map

void CpmTester::setupCpmRoiMap(const CpmRoiCollection* const roiCollection)
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
