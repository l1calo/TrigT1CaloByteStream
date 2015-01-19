#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/StatusCode.h"

#include "PpmByteStreamV2Tool.h"
#include "ByteStreamCnvSvcBase/IROBDataProviderSvc.h"
#include "TrigT1CaloDataAccessV2.h"

#include "ToString.h"
#include "xAODTrigL1Calo/TriggerTowerAuxContainer.h"

namespace LVL1BS {

// Constructor

TrigT1CaloDataAccessV2::TrigT1CaloDataAccessV2(const std::string& name="TrigT1CaloDataAccessV2")
                    : AsgTool(name),
 m_tool("LVL1BS::PpmByteStreamV2Tool/PpmByteStreamV2Tool"),
 m_robDataProvider("ROBDataProviderSvc/ROBDataProviderSvc", name)
{

}


// Initialize

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

StatusCode TrigT1CaloDataAccessV2::initialize()
{
	ATH_MSG_INFO("Initializing " << name() << " - package version "
                 << PACKAGE_VERSION);
	// Retrieve Tool
	CHECK(m_tool.retrieve().isSuccess());
	CHECK(m_robDataProvider.retrieve().isSuccess());


  return StatusCode::SUCCESS;
}



// Return iterators to required trigger towers

StatusCode TrigT1CaloDataAccessV2::loadTriggerTowers(xAOD::TriggerTowerContainer&  container)
{
	auto sourceIds = m_tool->sourceIDs("TriggerTowers");

	IROBDataProviderSvc::VROBFRAG robFrags;
	m_robDataProvider->getROBData(sourceIds, robFrags);
	CHECK((m_tool->convert(robFrags, &container)).isSuccess());

	return StatusCode::SUCCESS;
}

StatusCode TrigT1CaloDataAccessV2::PrintTriggerTowers()
{
  xAOD::TriggerTowerContainer ttCollection;
  xAOD::TriggerTowerAuxContainer aux;

  ttCollection.setStore(&aux);
  CHECK(loadTriggerTowers(ttCollection));
  ATH_MSG_ALWAYS(ToString(ttCollection));
  return StatusCode::SUCCESS;
}

} // end namespace
