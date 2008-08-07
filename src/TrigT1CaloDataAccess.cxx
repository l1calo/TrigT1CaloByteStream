
#include <stdint.h>

#include "TrigT1Calo/TriggerTower.h"

#include "TrigT1CaloByteStream/TrigT1CaloDataAccess.h"

namespace LVL1BS {

// Constructor

TrigT1CaloDataAccess::TrigT1CaloDataAccess(const std::string& type,
                      const std::string& name, const IInterface*  parent)
                    : AlgTool(type, name, parent),
 m_robDataProvider("ROBDataProviderSvc/ROBDataProviderSvc", name),
 m_selectionTool("LVL1BS::TriggerTowerSelectionTool/TriggerTowerSelectionTool"),
 m_ppmBSConverter("LVL1BS::PpmByteStreamSubsetTool/PpmByteStreamSubsetTool"),
 m_log(msgSvc(), name), m_ttCol(0)
{
  declareInterface<ITrigT1CaloDataAccess>(this);

}

// Destructor

TrigT1CaloDataAccess::~TrigT1CaloDataAccess()
{
}

// Initialize

StatusCode TrigT1CaloDataAccess::initialize()
{
  m_log.setLevel(outputLevel());
  m_debug = outputLevel() <= MSG::DEBUG;

  StatusCode sc = AlgTool::initialize();
  if (sc.isFailure()) {
    m_log << MSG::ERROR << "Problem initializing AlgTool " <<  endreq;
    return sc;
  }

  // Retrieve data provider service

  sc = m_robDataProvider.retrieve();
  if ( sc.isFailure() ) {
    m_log << MSG::ERROR << "Couldn't retrieve ROBDataProviderSvc" << endreq;
    return sc;
  }

  // Retrieve selection tool

  sc = m_selectionTool.retrieve();
  if ( sc.isFailure() ) {
    m_log << MSG::ERROR << "Couldn't retrieve TriggerTowerSelectionTool"
          << endreq;
    return sc;
  }

  // Retrieve PPM converter tool

  sc = m_ppmBSConverter.retrieve();
  if ( sc.isFailure() ) {
    m_log << MSG::ERROR << "Couldn't retrieve PpmByteStreamSubsetTool"
          << endreq;
    return sc;
  }

  m_ttCol = new DataVector<LVL1::TriggerTower>;

  m_log << MSG::INFO << "Initialization completed" << endreq;

  return StatusCode::SUCCESS;
}

// Finalize

StatusCode TrigT1CaloDataAccess::finalize()
{
  delete m_ttCol;
  return AlgTool::finalize();
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
    m_log << MSG::ERROR << "PPM bytestream conversion failed" << endreq;
    m_ttCol->clear();
  }
  beg = m_ttCol->begin();
  end = m_ttCol->end();

  return sc;
}


} // end namespace
