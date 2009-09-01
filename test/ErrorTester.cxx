
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/StatusCode.h"
#include "StoreGate/StoreGateSvc.h"
#include "SGTools/StlVectorClids.h"

#include "ErrorTester.h"

namespace LVL1BS {

ErrorTester::ErrorTester(const std::string& name, ISvcLocator* pSvcLocator)
                     : AthAlgorithm(name, pSvcLocator)
{
  declareProperty("L1CaloErrorLocation",
                   m_errorVectorLocation = "L1CaloUnpackingErrors");
}

ErrorTester::~ErrorTester()
{
}

// Initialize

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

StatusCode ErrorTester::initialize()
{
  msg(MSG::INFO) << "Initializing " << name() << " - package version "
                 << /* version() */ PACKAGE_VERSION << endreq;

  return StatusCode::SUCCESS;
}

// Execute

StatusCode ErrorTester::execute()
{
  if ( !msgLvl(MSG::INFO) ) return StatusCode::SUCCESS;
  msg(MSG::INFO);

  const ErrorCollection* errVec = 0;
  StatusCode sc = evtStore()->retrieve(errVec, m_errorVectorLocation);
  if (sc.isFailure() || !errVec) {
    msg() << "No error vector found for " << m_errorVectorLocation << endreq;
  } else {
    if (errVec->empty()) {
      msg() << "Error vector empty" << endreq;
    } else {
      ErrorCollection::const_iterator iter  = errVec->begin();
      ErrorCollection::const_iterator iterE = errVec->end();
      unsigned int size1 = *iter;
      ++iter;
      msg() << "Number of ROB status errors: " << size1 << endreq;
      while (iter != iterE) {
	msg() << "ROB ID: " << MSG::hex << *iter << MSG::dec;
	++iter;
	if (iter != iterE) {
	  if (size1 > 0) {
	    msg() << "  status error: " << MSG::hex << *iter << MSG::dec
	          << endreq;
	    size1--;
	  } else msg() << "  unpacking error: " << *iter << endreq;
	  ++iter;
        } else {
	  msg() << "  vector truncated, no error value" << endreq;
        }
      }
    }
  }
  return StatusCode::SUCCESS;
}

// Finalize

StatusCode ErrorTester::finalize()
{

  return StatusCode::SUCCESS;
}

} // end namespace
