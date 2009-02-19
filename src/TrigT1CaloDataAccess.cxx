
#include <stdint.h>

#include "GaudiKernel/IInterface.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/StatusCode.h"

#include "ByteStreamCnvSvcBase/IROBDataProviderSvc.h"
#include "TrigT1CaloEvent/TriggerTower.h"

#include "IPpmByteStreamSubsetTool.h"
#include "ITriggerTowerSelectionTool.h"

#include "TrigT1CaloDataAccess.h"

namespace LVL1BS {

// Constructor

TrigT1CaloDataAccess::TrigT1CaloDataAccess(const std::string& type,
                      const std::string& name, const IInterface*  parent)
                    : AthAlgTool(type, name, parent),
 m_robDataProvider("ROBDataProviderSvc/ROBDataProviderSvc", name),
 m_selectionTool("LVL1BS::TriggerTowerSelectionTool/TriggerTowerSelectionTool"),
 m_ppmBSConverter("LVL1BS::PpmByteStreamSubsetTool/PpmByteStreamSubsetTool"),
 m_ttCol(0)
{
  declareInterface<ITrigT1CaloDataAccess>(this);

  declareProperty("ROBDataProviderSvc",        m_robDataProvider);
  declareProperty("TriggerTowerSelectionTool", m_selectionTool);
  declareProperty("PpmByteStreamSubsetTool",   m_ppmBSConverter);

}

// Destructor

TrigT1CaloDataAccess::~TrigT1CaloDataAccess()
{
}

// Initialize

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

StatusCode TrigT1CaloDataAccess::initialize()
{
  msg(MSG::INFO) << "Initializing " << name() << " - package version "
                 << PACKAGE_VERSION << endreq;

  // Retrieve data provider service

  StatusCode sc = m_robDataProvider.retrieve();
  if ( sc.isFailure() ) {
    msg(MSG::ERROR) << "Failed to retrieve service " << m_robDataProvider
                    << endreq;
    return sc;
  } else msg(MSG::INFO) << "Retrieved service " << m_robDataProvider << endreq;

  // Retrieve selection tool

  sc = m_selectionTool.retrieve();
  if ( sc.isFailure() ) {
    msg(MSG::ERROR) << "Failed to retrieve tool " << m_selectionTool << endreq;
    return sc;
  } else msg(MSG::INFO) << "Retrieved tool " << m_selectionTool << endreq;

  // Retrieve PPM converter tool

  sc = m_ppmBSConverter.retrieve();
  if ( sc.isFailure() ) {
    msg(MSG::ERROR) << "Failed to retrieve tool " << m_ppmBSConverter << endreq;
    return sc;
  } else msg(MSG::INFO) << "Retrieved tool " << m_ppmBSConverter << endreq;

  m_ttCol = new DataVector<LVL1::TriggerTower>;

  return StatusCode::SUCCESS;
}

// Finalize

StatusCode TrigT1CaloDataAccess::finalize()
{
  delete m_ttCol;
  return StatusCode::SUCCESS;
}

// Return iterators to required trigger towers

StatusCode TrigT1CaloDataAccess::loadCollection(
                     DataVector<LVL1::TriggerTower>::const_iterator& beg,
	             DataVector<LVL1::TriggerTower>::const_iterator& end,
	             const double etaMin, const double etaMax,
	             const double phiMin, const double phiMax)
{

  // Get PPM sub-block channel IDs for wanted TriggerTowers
  std::vector<unsigned int> chanIds;
  m_selectionTool->channelIDs(etaMin, etaMax, phiMin, phiMax, chanIds);

  // Get ROB IDs for wanted TriggerTowers
  std::vector<uint32_t> robs;
  m_selectionTool->robIDs(chanIds, robs);

  // Get data
  m_robFrags.clear();
  m_robDataProvider->getROBData(robs, m_robFrags);

  // Convert to TriggerTowers
  m_ttCol->clear();
  StatusCode sc = m_ppmBSConverter->convert(m_robFrags, m_ttCol, chanIds);
  if (sc.isFailure() ) {
    msg(MSG::ERROR) << "PPM bytestream conversion failed" << endreq;
    m_ttCol->clear();
  }
  beg = m_ttCol->begin();
  end = m_ttCol->end();

  return sc;
}


} // end namespace
