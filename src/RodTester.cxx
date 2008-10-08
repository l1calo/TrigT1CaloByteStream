
#include <stdint.h>

#include "GaudiKernel/MsgStream.h"

#include "TrigT1Calo/RODHeader.h"

#include "TrigT1CaloByteStream/RodTester.h"

namespace LVL1BS {

RodTester::RodTester(const std::string& name, ISvcLocator* pSvcLocator)
                     : Algorithm(name, pSvcLocator),
		       m_storeGate("StoreGateSvc", name)
{
  declareProperty("RodHeaderLocation", m_rodHeaderLocation = "RODHeaders");

  m_flags.push_back("");
  m_flags.push_back("PP");
  m_flags.push_back("CP");
  m_flags.push_back("CPRoI");
  m_flags.push_back("JEP");
  m_flags.push_back("JEPRoI");
  m_flags.push_back("CPRoIB");
  m_flags.push_back("JEPRoIB");
}

RodTester::~RodTester()
{
}

// Initialize

StatusCode RodTester::initialize()
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

  return StatusCode::SUCCESS;
}

// Execute

StatusCode RodTester::execute()
{
  MsgStream log( msgSvc(), name() );

  // Loop over all possible collections

  std::vector<std::string>::const_iterator flagIter = m_flags.begin();
  std::vector<std::string>::const_iterator flagEnd  = m_flags.end();
  for (; flagIter != flagEnd; ++flagIter) {
    log << MSG::INFO << "================================================="
        << endreq;
    const std::string location(m_rodHeaderLocation + *flagIter);
    const RodHeaderCollection* rods = 0;
    StatusCode sc = m_storeGate->retrieve(rods, location);
    if (sc.isFailure() || !rods || rods->empty()) {
      log << MSG::INFO << "No ROD headers found for " << location << endreq;
    } else {

      // Print the ROD headers

      log << MSG::INFO << "Number of ROD headers in collection "
                       << location << " is " << rods->size() << endreq;
      log << MSG::INFO << MSG::hex;
      RodHeaderCollection::const_iterator rodIter = rods->begin();
      RodHeaderCollection::const_iterator rodEnd  = rods->end();
      for (; rodIter != rodEnd; ++rodIter) {
        const LVL1::RODHeader* const header = *rodIter;
	log << MSG::INFO << "-------------------------------------------------"
	    << endreq;
	log << MSG::INFO << "version/majorVersion/minorVersion: "
	    << header->version() << "/" << header->majorVersion() << "/"
	    << header->minorVersion() << endreq;
        log << MSG::INFO
	    << "sourceID/subDetectorID/moduleID/crate/sLink/dataType: "
	    << header->sourceID() << "/" << header->subDetectorID() << "/"
	    << header->moduleID() << "/" << header->crate() << "/"
	    << header->sLink() << "/" << header->dataType() << endreq;
        log << MSG::INFO << "run/runType/runNumber: "
	    << header->run() << "/" << header->runType() << "/"
	    << header->runNumber() << endreq;
        log << MSG::INFO << "extendedL1ID/ecrID/l1ID: "
	    << header->extendedL1ID() << "/" << header->ecrID() << "/"
	    << header->l1ID() << endreq;
        log << MSG::INFO << "bunchCrossing/l1TriggerType: "
	    << header->bunchCrossing() << "/" << header->l1TriggerType() << "/"
	    << endreq;
        log << MSG::INFO << "detEventType/orbitCount/stepNumber/stepType: "
	    << header->detEventType() << "/" << header->orbitCount() << "/"
	    << header->stepNumber() << "/" << header->stepType() << endreq;
	log << MSG::INFO << "payloadSize: " << header->payloadSize() << endreq;
	const std::vector<uint32_t>& words(header->statusWords());
        log << MSG::INFO << "statusWords(" << words.size() << "): ";
	std::vector<uint32_t>::const_iterator wordIter = words.begin();
	std::vector<uint32_t>::const_iterator wordEnd  = words.end();
	for (; wordIter != wordEnd; ++wordIter) {
	  log << MSG::INFO << *wordIter << " ";
        }
	log << MSG::INFO << endreq;
	log << MSG::INFO
	    << "bcnMismatch/gLinkTimeout/dataTransportError/rodFifoOverflow: "
	    << header->bcnMismatch() << "/" << header->gLinkTimeout() << "/"
	    << header->dataTransportError() << "/" << header->rodFifoOverflow()
	    << endreq;
        log << MSG::INFO << "lvdsLinkError/cmmParityError/gLinkError: "
	    << header->lvdsLinkError() << "/" << header->cmmParityError() << "/"
	    << header->gLinkError() << endreq;
        log << MSG::INFO << "limitedRoISet/triggerTypeTimeout: "
	    << header->limitedRoISet() << "/" << header->triggerTypeTimeout()
	    << endreq;
      }
      log << MSG::INFO << MSG::dec;
    }
  }

  return StatusCode::SUCCESS;
}

// Finalize

StatusCode RodTester::finalize()
{

  return StatusCode::SUCCESS;
}

} // end namespace
