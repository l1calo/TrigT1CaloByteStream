
#include "GaudiKernel/MsgStream.h"
#include "StoreGate/StoreGateSvc.h"

#include "TrigT1Calo/JetElement.h"
#include "TrigT1Calo/JEMHits.h"
#include "TrigT1Calo/JEMEtSums.h"
#include "TrigT1Interfaces/TrigT1CaloDefs.h"

#include "TrigT1CaloByteStream/JepContainer.h"
#include "TrigT1CaloByteStream/JepContainerMaker.h"


JepContainerMaker::JepContainerMaker(const std::string& name,
                                     ISvcLocator* pSvcLocator)
                                    : Algorithm(name, pSvcLocator)
{
  declareProperty("JetElementLocation",
         m_jetElementLocation   = LVL1::TrigT1CaloDefs::JetElementLocation);
  declareProperty("JEMHitsLocation",
         m_jemHitsLocation      = LVL1::TrigT1CaloDefs::JEMHitsLocation);
  declareProperty("JEMEtSumsLocation",
         m_jemEtSumsLocation    = LVL1::TrigT1CaloDefs::JEMEtSumsLocation);
  declareProperty("JepContainerLocation",
         m_jepContainerLocation = "JepContainer");
}

JepContainerMaker::~JepContainerMaker()
{
}

// Initialize

StatusCode JepContainerMaker::initialize()
{
  MsgStream log( msgSvc(), name() );

  StatusCode sc = service("StoreGateSvc", m_storeGate);
  if (sc.isFailure()) {
    log << MSG::ERROR << "Unable to get pointer to StoreGate service"
                      << endreq;
    return sc;
  }
  return StatusCode::SUCCESS;
}

// Execute

StatusCode JepContainerMaker::execute()
{
  MsgStream log( msgSvc(), name() );

  // Find jet elements

  const JetElementCollection* jeCollection = 0;
  StatusCode sc = m_storeGate->retrieve(jeCollection, m_jetElementLocation);
  if (sc.isFailure() || !jeCollection || jeCollection->empty()) {
    log << MSG::DEBUG << "No Jet Elements found" << endreq;
    jeCollection = 0;
  }

  // Find jet hits

  const JetHitsCollection* hitCollection = 0;
  sc = m_storeGate->retrieve(hitCollection, m_jemHitsLocation);
  if (sc.isFailure() || !hitCollection || hitCollection->empty()) {
    log << MSG::DEBUG << "No Jet Hits found" << endreq;
    hitCollection = 0;
  }

  // Find Energy Sums

  const EnergySumsCollection* etCollection = 0;
  sc = m_storeGate->retrieve(etCollection, m_jemEtSumsLocation);
  if (sc.isFailure() || !etCollection || etCollection->empty()) {
    log << MSG::DEBUG << "No Energy Sums found" << endreq;
    etCollection = 0;
  }

  // Create JEP container

  JepContainer* jep = new JepContainer(jeCollection, hitCollection,
                                                     etCollection);
  sc = m_storeGate->record(jep, m_jepContainerLocation);
  if (sc != StatusCode::SUCCESS) {
    log << MSG::ERROR << "Error recording JEP container in TDS " << endreq;
    return sc;
  }

  return StatusCode::SUCCESS;
}

// Finalize

StatusCode JepContainerMaker::finalize()
{

  return StatusCode::SUCCESS;
}
