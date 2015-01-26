#include <vector>
#include <stdint.h>

#include "ByteStreamCnvSvcBase/ByteStreamAddress.h"
#include "ByteStreamCnvSvcBase/IByteStreamEventAccess.h"
#include "ByteStreamCnvSvcBase/IROBDataProviderSvc.h"

#include "ByteStreamData/RawEvent.h"
#include "ByteStreamData/ROBData.h"


#include "AthenaKernel/errorcheck.h"
#include "GaudiKernel/CnvFactory.h"
#include "GaudiKernel/DataObject.h"
#include "GaudiKernel/IOpaqueAddress.h"
#include "GaudiKernel/IRegistry.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/StatusCode.h"

#include "SGTools/ClassID_traits.h"
#include "SGTools/StorableConversions.h"
#include "StoreGate/StoreGateSvc.h"

#include "xAODTrigL1Calo/TriggerTower.h"
#include "xAODTrigL1Calo/TriggerTowerContainer.h"
#include "xAODTrigL1Calo/TriggerTowerAuxContainer.h"

#include "PpmByteStreamxAODCnv.h"
#include "PpmByteStreamxAODReadTool.h"

#include "../ToString.h"

namespace LVL1BS {

PpmByteStreamxAODCnv::PpmByteStreamxAODCnv(ISvcLocator* svcloc) :
	Converter(ByteStream_StorageType, classID(), svcloc),
	AthMessaging(svcloc != 0 ? msgSvc() : 0, "PpmByteStreamxAODCnv"),
	m_name("PpmByteStreamxAODCnv"),
	m_storeSvc("StoreGateSvc", m_name),
	m_robDataProvider("ROBDataProviderSvc", m_name),
	m_ByteStreamEventAccess("ByteStreamCnvSvc", m_name),
	m_readTool("LVL1BS::PpmByteStreamxAODReadTool/PpmByteStreamxAODReadTool")
{

}

// CLID

const CLID& PpmByteStreamxAODCnv::classID() {
	return ClassID_traits < xAOD::TriggerTowerContainer > ::ID();
}

//  Init method gets all necessary services etc.

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

StatusCode PpmByteStreamxAODCnv::initialize() {
	ATH_MSG_DEBUG(
			"Initializing " << m_name << " - package version "
					<< PACKAGE_VERSION
	);

	CHECK(Converter::initialize());
	//Get ByteStreamCnvSvc
	CHECK(m_ByteStreamEventAccess.retrieve());
	CHECK(m_readTool.retrieve());

	// Get ROBDataProvider
	StatusCode sc = m_robDataProvider.retrieve();
	if (sc.isFailure()) {
		ATH_MSG_WARNING("Failed to retrieve service " << m_robDataProvider);
		// return is disabled for Write BS which does not require ROBDataProviderSvc
		// return sc ;
	} else {
		ATH_MSG_DEBUG("Retrieved service " << m_robDataProvider);
	}

	sc = m_storeSvc.retrieve();
	if (sc.isFailure()) {
		ATH_MSG_WARNING("Failed to retrieve service " << m_storeSvc);
		// return is disabled for Write BS which does not require m_storeSvc
		// return sc ;
	} else {
		ATH_MSG_DEBUG("Retrieved service " << m_storeSvc);
	}

	return StatusCode::SUCCESS;
}

// createObj should create the RDO from bytestream.

StatusCode PpmByteStreamxAODCnv::createObj(IOpaqueAddress* pAddr,
		DataObject*& pObj) {
	ATH_MSG_DEBUG("createObj() called");
	// -------------------------------------------------------------------------
	ByteStreamAddress *pBS_Addr = dynamic_cast<ByteStreamAddress *>(pAddr);
	CHECK(pBS_Addr != nullptr);
	// -------------------------------------------------------------------------
	const std::string nm = *(pBS_Addr->par());
	ATH_MSG_DEBUG("Creating Objects " << nm);
	// -------------------------------------------------------------------------
	// // get SourceIDs
	const std::vector<uint32_t>& vID(m_readTool->sourceIDs(nm));
	// // get ROB fragments
	IROBDataProviderSvc::VROBFRAG robFrags;
	m_robDataProvider->getROBData(vID, robFrags, "PpmByteStreamxAODCnv");
	// -------------------------------------------------------------------------
	// size check
	xAOD::TriggerTowerAuxContainer* aux = new xAOD::TriggerTowerAuxContainer();
	xAOD::TriggerTowerContainer* const ttCollection =
			new xAOD::TriggerTowerContainer();
	ttCollection->setStore(aux);

	ATH_MSG_DEBUG("Number of ROB fragments is " << robFrags.size());
	CHECK(m_readTool->convert(robFrags, ttCollection));
	ATH_MSG_DEBUG(ToString(*ttCollection));
	if (!robFrags.size()) {
		pObj = SG::asStorable(ttCollection);
		return StatusCode::SUCCESS;
	}
	return StatusCode::SUCCESS;
}

// createRep should create the bytestream from RDOs.

StatusCode PpmByteStreamxAODCnv::createRep(DataObject* /*pObj*/,
		IOpaqueAddress*& /*pAddr*/) {
	return StatusCode::FAILURE;
}

} // end namespace
