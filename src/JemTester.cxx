
#include <utility>

#include "StoreGate/StoreGateSvc.h"

#include "TrigT1Calo/JetElement.h"
#include "TrigT1Calo/JetElementKey.h"

#include "TrigT1CaloByteStream/JemTester.h"


JemTester::JemTester(const std::string& name, ISvcLocator* pSvcLocator)
                     : Algorithm(name, pSvcLocator)
{
  declareProperty("JetElementLocation",
                                m_jetElementLocation = "LVL1JetElements");
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

  // Find jet elements

  const JetElementCollection* jeCollection = 0;
  StatusCode sc = m_storeGate->retrieve(jeCollection, m_jetElementLocation);
  if (sc.isFailure() || !jeCollection || jeCollection->empty()) {
    log << MSG::DEBUG << "No Jet Elements found" << endreq;
    return StatusCode::SUCCESS;
  }

  // Order by key

  setupJeMap(jeCollection);

  // Print the jet elements

  printJetElements(log, MSG::INFO);

  return StatusCode::SUCCESS;
}

// Finalize

StatusCode JemTester::finalize()
{
  delete m_elementKey;

  return StatusCode::SUCCESS;
}

// Find a jet element given eta, phi

LVL1::JetElement* JemTester::findJetElement(double eta, double phi)
{
  LVL1::JetElement* je = 0;
  unsigned int key = m_elementKey->jeKey(phi, eta);
  JetElementMap::const_iterator mapIter;
  mapIter = m_jeMap.find(key);
  if (mapIter != m_jeMap.end()) je = mapIter->second;
  return je;
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

// Set up jet element map

void JemTester::setupJeMap(const JetElementCollection* jeCollection)
{
  m_jeMap.clear();
  JetElementCollection::const_iterator pos = jeCollection->begin();
  JetElementCollection::const_iterator pose = jeCollection->end();
  for (; pos != pose; ++pos) {
    LVL1::JetElement* je = *pos;
    unsigned int key = m_elementKey->jeKey(je->phi(), je->eta());
    m_jeMap.insert(std::make_pair(key, je));
  }
}
