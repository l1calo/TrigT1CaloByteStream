#include <vector>
#include <stdint.h>

#include "ByteStreamCnvSvcBase/ByteStreamAddress.h"
#include "ByteStreamCnvSvcBase/IByteStreamEventAccess.h"
#include "ByteStreamCnvSvcBase/IROBDataProviderSvc.h"

#include "ByteStreamData/RawEvent.h"
#include "ByteStreamData/ROBData.h"

#include "DataModel/DataVector.h"

#include "GaudiKernel/CnvFactory.h"
#include "GaudiKernel/DataObject.h"
#include "GaudiKernel/IOpaqueAddress.h"
#include "GaudiKernel/IRegistry.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/StatusCode.h"

#include "SGTools/ClassID_traits.h"
#include "SGTools/StorableConversions.h"

#include "xAODTrigL1Calo/TriggerTower.h"
#include "xAODTrigL1Calo/TriggerTowerContainer.h"
#include "xAODTrigL1Calo/TriggerTowerAuxContainer.h"

#include "PpmByteStreamV2Cnv.h"
#include "PpmByteStreamV2Tool.h"

namespace LVL1BS {

PpmByteStreamV2Cnv::PpmByteStreamV2Cnv( ISvcLocator* svcloc )
    : Converter( ByteStream_StorageType, classID(), svcloc ),
      m_name("PpmByteStreamV2Cnv"),
      m_tool("LVL1BS::PpmByteStreamV2Tool/PpmByteStreamV2Tool"),
      m_robDataProvider("ROBDataProviderSvc", m_name),
      m_ByteStreamEventAccess("ByteStreamCnvSvc", m_name),
      m_log(msgSvc(), m_name), m_debug(false)
{
}

PpmByteStreamV2Cnv::~PpmByteStreamV2Cnv()
{
}

// CLID

const CLID& PpmByteStreamV2Cnv::classID()
{
  return ClassID_traits<xAOD::TriggerTowerContainer>::ID();
}

//  Init method gets all necessary services etc.

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

StatusCode PpmByteStreamV2Cnv::initialize()
{
  m_debug = msgSvc()->outputLevel(m_name) <= MSG::DEBUG;
  m_log << MSG::DEBUG << "Initializing " << m_name << " - package version "
                      << PACKAGE_VERSION << endreq;

  StatusCode sc = Converter::initialize();
  if ( sc.isFailure() )
    return sc;

  //Get ByteStreamCnvSvc
  sc = m_ByteStreamEventAccess.retrieve();
  if ( sc.isFailure() ) {
    m_log << MSG::ERROR << "Failed to retrieve service "
          << m_ByteStreamEventAccess << endreq;
    return sc;
  } else {
    m_log << MSG::DEBUG << "Retrieved service "
          << m_ByteStreamEventAccess << endreq;
  }

  // Retrieve Tool
  sc = m_tool.retrieve();
  if ( sc.isFailure() ) {
    m_log << MSG::ERROR << "Failed to retrieve tool " << m_tool << endreq;
    return sc;
  } else m_log << MSG::DEBUG << "Retrieved tool " << m_tool << endreq;

  // Get ROBDataProvider
  sc = m_robDataProvider.retrieve();
  if ( sc.isFailure() ) {
    m_log << MSG::WARNING << "Failed to retrieve service "
          << m_robDataProvider << endreq;
    // return is disabled for Write BS which does not require ROBDataProviderSvc
    // return sc ;
  } else {
    m_log << MSG::DEBUG << "Retrieved service "
          << m_robDataProvider << endreq;
  }

  return StatusCode::SUCCESS;
}

// createObj should create the RDO from bytestream.

StatusCode PpmByteStreamV2Cnv::createObj( IOpaqueAddress* pAddr,
                                        DataObject*& pObj )
{
  if (m_debug) m_log << MSG::DEBUG << "createObj() called" << endreq;
  // -------------------------------------------------------------------------
  ByteStreamAddress *pBS_Addr;
  pBS_Addr = dynamic_cast<ByteStreamAddress *>( pAddr );
  if ( !pBS_Addr ) {
    m_log << MSG::ERROR << " Can not cast to ByteStreamAddress " << endreq;
    return StatusCode::FAILURE;
  }
  // -------------------------------------------------------------------------
  const std::string nm = *( pBS_Addr->par() );
  if (m_debug) m_log << MSG::DEBUG << " Creating Objects " << nm << endreq;
  // -------------------------------------------------------------------------
  // // get SourceIDs
  const std::vector<uint32_t>& vID(m_tool->sourceIDs(nm));
  // // get ROB fragments
  IROBDataProviderSvc::VROBFRAG robFrags;
  m_robDataProvider->getROBData(vID, robFrags);
  // -------------------------------------------------------------------------
  // size check
  xAOD::TriggerTowerContainer* const ttCollection =
                      new xAOD::TriggerTowerContainer(SG::VIEW_ELEMENTS);
  
  if (m_debug) {
     m_log << MSG::DEBUG << " Number of ROB fragments is " << robFrags.size()
           << endreq;
  }
  
  if (!robFrags.size()) {
    pObj = SG::asStorable(ttCollection) ;
    return StatusCode::SUCCESS;
  }
  // -------------------------------------------------------------------------
  StatusCode sc = m_tool->convert(robFrags, ttCollection);
  if ( sc.isFailure() ) {
     m_log << MSG::ERROR << " Failed to create Objects   " << endreq;
     delete ttCollection;
     return sc;
  }

  pObj = SG::asStorable(ttCollection);
  return sc;
}

// createRep should create the bytestream from RDOs.

StatusCode PpmByteStreamV2Cnv::createRep( DataObject* /*pObj*/,
                                        IOpaqueAddress*& /*pAddr*/)
{
  //TODO: (Sasha) implement createRep
  if (m_debug) m_log << MSG::DEBUG << "createRep() called" << endreq;

  // RawEventWrite* re = m_ByteStreamEventAccess->getRawEvent();

  // DataVector<LVL1::TriggerTower2>* ttCollection = 0;
  // if( !SG::fromStorable( pObj, ttCollection ) ) {
  //   m_log << MSG::ERROR << " Cannot cast to DataVector<TriggerTower2>" << endreq;
  //   return StatusCode::FAILURE;
  // }

  // const std::string nm = pObj->registry()->name();

  // ByteStreamAddress* addr = new ByteStreamAddress( classID(), nm, "" );

  // pAddr = addr;

  // // Convert to ByteStream
  // return m_tool->convert( ttCollection, re );
  return StatusCode::FAILURE;
}

} // end namespace
