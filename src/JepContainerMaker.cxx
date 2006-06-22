
#include "GaudiKernel/MsgStream.h"
#include "StoreGate/StoreGateSvc.h"

#include "TrigT1Calo/JetElement.h"
#include "TrigT1CaloByteStream/JepContainer.h"
#include "TrigT1CaloByteStream/JepContainerMaker.h"


JepContainerMaker::JepContainerMaker(const std::string& name,
                                     ISvcLocator* pSvcLocator)
                                    : Algorithm(name, pSvcLocator)
{
  declareProperty("JetElementLocation",
                                m_jetElementLocation = "LVL1JetElements");
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

  // Find ...

  // Create JEP container

  JepContainer* jep = new JepContainer(jeCollection);
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
