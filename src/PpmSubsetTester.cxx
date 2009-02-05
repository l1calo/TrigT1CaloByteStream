
#include <cmath>
#include <utility>

#include "TrigT1CaloEvent/TriggerTower.h"
#include "TrigT1CaloUtils/TriggerTowerKey.h"
#include "TrigT1Interfaces/TrigT1CaloDefs.h"

#include "TrigT1CaloByteStream/ModifySlices.h"
#include "TrigT1CaloByteStream/PpmSubsetTester.h"

namespace LVL1BS {

PpmSubsetTester::PpmSubsetTester(const std::string& name,
                                 ISvcLocator* pSvcLocator)
         : Algorithm(name, pSvcLocator),
	   m_storeGate("StoreGateSvc", name),
	   m_dataAccess("LVL1BS::TrigT1CaloDataAccess/TrigT1CaloDataAccess"),
	   m_towerKey(0)
{
  declareProperty("TriggerTowerLocation",
         m_triggerTowerLocation = LVL1::TrigT1CaloDefs::TriggerTowerLocation);

  declareProperty("ForceSlicesLUT",  m_forceSlicesLut  = 0);
  declareProperty("ForceSlicesFADC", m_forceSlicesFadc = 0);
}

PpmSubsetTester::~PpmSubsetTester()
{
}

// Initialize

StatusCode PpmSubsetTester::initialize()
{
  MsgStream log( msgSvc(), name() );

  log << MSG::INFO << "Initializing " << name() << " - package version "
                   << version() << endreq;

  StatusCode sc = m_storeGate.retrieve();
  if (sc.isFailure()) {
    log << MSG::ERROR << "Couldn't connect to " << m_storeGate.typeAndName()
                      << endreq;
    return sc;
  }
  sc = m_dataAccess.retrieve();
  if (sc.isFailure()) {
    log << MSG::ERROR << "Couldn't retrieve TrigT1CaloDataAccess tool"
                      << endreq;
    return sc;
  }
  m_towerKey = new LVL1::TriggerTowerKey();

  return StatusCode::SUCCESS;
}

// Execute

StatusCode PpmSubsetTester::execute()
{
  MsgStream log( msgSvc(), name() );

  const double etaMin = -4.9;
  const double etaMax = 4.9;
  const double phiMin = 0.;
  const double phiMax = M_PI*2.;

  // Find trigger towers

  const TriggerTowerCollection* ttCollection = 0;
  StatusCode sc = m_storeGate->retrieve(ttCollection, m_triggerTowerLocation);
  if (sc.isFailure() || !ttCollection || ttCollection->empty()) {
    log << MSG::DEBUG << "No Trigger Towers found" << endreq;
    return StatusCode::SUCCESS;
  }

  // Order by key

  setupTTMap(ttCollection, etaMin, etaMax, phiMin, phiMax);

  // Print the trigger towers

  printTriggerTowers(log, MSG::INFO);

  // Now use subset tool

  TriggerTowerCollection::const_iterator beg;
  TriggerTowerCollection::const_iterator end;
  sc = m_dataAccess->loadCollection(beg, end, etaMin, etaMax, phiMin, phiMax);
  if (sc.isFailure()) {
    log << MSG::ERROR << "Error accessing data" << endreq;
    return sc;
  }

  // Order by key

  setupTTMap(beg, end);

  // Print the trigger towers

  printTriggerTowers(log, MSG::INFO);

  return StatusCode::SUCCESS;
}

// Finalize

StatusCode PpmSubsetTester::finalize()
{
  delete m_towerKey;

  return StatusCode::SUCCESS;
}

// Print the trigger towers

void PpmSubsetTester::printTriggerTowers(MsgStream& log,
                                              const MSG::Level level) const
{
  log << level << "Number of Trigger Towers = " << m_ttMap.size() << endreq;
  TriggerTowerMap::const_iterator mapIter = m_ttMap.begin();
  TriggerTowerMap::const_iterator mapEnd  = m_ttMap.end();
  for (; mapIter != mapEnd; ++mapIter) {
    const LVL1::TriggerTower* const tt = mapIter->second;
    int emPeak       = tt->emPeak();
    int hadPeak      = tt->hadPeak();
    int emADCPeak    = tt->emADCPeak();
    int hadADCPeak   = tt->hadADCPeak();
    int emLUTSlices  = (tt->emLUT()).size();
    int emADCSlices  = (tt->emADC()).size();
    int hadLUTSlices = (tt->hadLUT()).size();
    int hadADCSlices = (tt->hadADC()).size();
    if (m_forceSlicesLut) {
      emPeak  = ModifySlices::peak(emPeak, emLUTSlices, m_forceSlicesLut);
      hadPeak = ModifySlices::peak(hadPeak, hadLUTSlices, m_forceSlicesLut);
      emLUTSlices  = m_forceSlicesLut;
      hadLUTSlices = m_forceSlicesLut;
    }
    if (m_forceSlicesFadc) {
      emADCPeak  = ModifySlices::peak(emADCPeak, emADCSlices,
                                                            m_forceSlicesFadc);
      hadADCPeak = ModifySlices::peak(hadADCPeak, hadADCSlices,
                                                            m_forceSlicesFadc);
      emADCSlices  = m_forceSlicesFadc;
      hadADCSlices = m_forceSlicesFadc;
    }
    log << level
        << " EM:key/eta/phi/LUTpeak/FADCpeak/LUT/FADC/bcidLUT/bcidFADC/error: "
        << mapIter->first << "/" << tt->eta() << "/" << tt->phi() << "/"
	<< emPeak << "/" << emADCPeak << "/";
    std::vector<int> lut;
    std::vector<int> fadc;
    std::vector<int> bcidLut;
    std::vector<int> bcidFadc;
    ModifySlices::data(tt->emLUT(),     lut,      emLUTSlices);
    ModifySlices::data(tt->emADC(),     fadc,     emADCSlices);
    ModifySlices::data(tt->emBCIDvec(), bcidLut,  emLUTSlices);
    ModifySlices::data(tt->emBCIDext(), bcidFadc, emADCSlices);
    printVec(lut,      log, level);
    printVec(fadc,     log, level);
    printVec(bcidLut,  log, level);
    printVec(bcidFadc, log, level);
    log << level << MSG::hex << tt->emError() << MSG::dec << "/" << endreq;
    log << level
        << "HAD:key/eta/phi/LUTpeak/FADCpeak/LUT/FADC/bcidLUT/bcidFADC/error: "
        << mapIter->first << "/" << tt->eta() << "/" << tt->phi() << "/"
	<< hadPeak << "/" << hadADCPeak << "/";
    ModifySlices::data(tt->hadLUT(),     lut,      hadLUTSlices);
    ModifySlices::data(tt->hadADC(),     fadc,     hadADCSlices);
    ModifySlices::data(tt->hadBCIDvec(), bcidLut,  hadLUTSlices);
    ModifySlices::data(tt->hadBCIDext(), bcidFadc, hadADCSlices);
    printVec(lut,      log, level);
    printVec(fadc,     log, level);
    printVec(bcidLut,  log, level);
    printVec(bcidFadc, log, level);
    log << level << MSG::hex << tt->hadError() << MSG::dec << "/" << endreq;
  }
}

// Print a vector

void PpmSubsetTester::printVec(const std::vector<int>& vec, MsgStream& log,
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

void PpmSubsetTester::setupTTMap(const TriggerTowerCollection*
                                 const ttCollection,
                                 const double etaMin, const double etaMax,
				 const double phiMin, const double phiMax)
{
  m_ttMap.clear();
  TriggerTowerCollection::const_iterator pos  = ttCollection->begin();
  TriggerTowerCollection::const_iterator pose = ttCollection->end();
  for (; pos != pose; ++pos) {
    const LVL1::TriggerTower* const tt = *pos;
    const double eta = tt->eta();
    if (eta < etaMin || eta > etaMax) continue;
    const double phi = tt->phi();
    if (phi < phiMin || phi > phiMax) continue;
    const unsigned int key = m_towerKey->ttKey(phi, eta);
    m_ttMap.insert(std::make_pair(key, tt));
  }
}

// Set up trigger tower map

void PpmSubsetTester::setupTTMap(TriggerTowerCollection::const_iterator& beg,
                                 TriggerTowerCollection::const_iterator& end)
{
  m_ttMap.clear();
  TriggerTowerCollection::const_iterator pos  = beg;
  TriggerTowerCollection::const_iterator pose = end;
  for (; pos != pose; ++pos) {
    const LVL1::TriggerTower* const tt = *pos;
    const unsigned int key = m_towerKey->ttKey(tt->phi(), tt->eta());
    m_ttMap.insert(std::make_pair(key, tt));
  }
}

} // end namespace
