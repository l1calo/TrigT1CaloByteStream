
#include <string>
#include <vector>
#include <stdint.h>

#include "ByteStreamCnvSvcBase/ByteStreamAddress.h"
#include "ByteStreamCnvSvcBase/ByteStreamCnvSvcBase.h"
#include "ByteStreamCnvSvcBase/IByteStreamEventAccess.h"
#include "ByteStreamCnvSvcBase/ROBDataProviderSvc.h"

#include "ByteStreamData/RawEvent.h"
#include "ByteStreamData/ROBData.h"

#include "CLIDSvc/tools/ClassID_traits.h"

#include "DataModel/DataVector.h"

#include "GaudiKernel/DataObject.h"
#include "GaudiKernel/IOpaqueAddress.h"
#include "GaudiKernel/IRegistry.h"
#include "GaudiKernel/IService.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/IToolSvc.h"
#include "GaudiKernel/MsgStream.h"

#include "SGTools/StorableConversions.h"

#include "TrigT1Calo/TriggerTower.h"

#include "TrigT1CaloByteStream/PpmByteStreamCnv.h"
#include "TrigT1CaloByteStream/PpmByteStreamTool.h"

namespace LVL1BS {

PpmByteStreamCnv::PpmByteStreamCnv( ISvcLocator* svcloc )
    : Converter( ByteStream_StorageType, classID(), svcloc )
{
}

PpmByteStreamCnv::~PpmByteStreamCnv()
{
}

// CLID

const CLID& PpmByteStreamCnv::classID()
{
  return ClassID_traits<DataVector<LVL1::TriggerTower> >::ID();
}

//  Init method gets all necessary services etc.

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

StatusCode PpmByteStreamCnv::initialize()
{
  MsgStream log( msgSvc(), "PpmByteStreamCnv" );
  log << MSG::DEBUG << "Initializing PpmByteStreamCnv - package version "
                    << PACKAGE_VERSION << endreq;

  StatusCode sc = Converter::initialize();
  if ( sc.isFailure() )
    return sc;

  //Get ByteStreamCnvSvc
  sc = service( "ByteStreamCnvSvc", m_ByteStreamEventAccess );
  if ( sc.isFailure() ) {
    log << MSG::ERROR << " Can't get ByteStreamEventAccess interface "
        << endreq;
    return sc;
  }

  // Retrieve Tool
  IToolSvc* toolSvc;
  sc = service( "ToolSvc", toolSvc );
  if ( sc.isFailure() ) {
    log << MSG::ERROR << " Can't get ToolSvc " << endreq;
    return sc;
  }

  // make it a private tool by giving the ByteStreamCnvSvc as parent
  const std::string toolType = "LVL1BS::PpmByteStreamTool/PpmByteStreamTool" ;
  sc = toolSvc->retrieveTool( toolType, m_tool, m_ByteStreamEventAccess);
  if ( sc.isFailure() ) {
    log << MSG::ERROR << " Can't get ByteStreamTool of type "
        << toolType << endreq;
    return StatusCode::FAILURE;
  }

  // Get ROBDataProvider
  IService* robSvc ;
  sc = serviceLocator() ->getService( "ROBDataProviderSvc", robSvc );
  if ( sc.isFailure() ) {
    log << MSG::WARNING << " Cant get ROBDataProviderSvc" << endreq;

    // return is disabled for Write BS which does not require ROBDataProviderSvc
    // return sc ;
  } else {
    m_robDataProvider = dynamic_cast<IROBDataProviderSvc *> ( robSvc );
    if ( m_robDataProvider == 0 ) {
      log << MSG::ERROR << " Cannot cast to ROBDataProviderSvc " << endreq;
      return StatusCode::FAILURE;
    }
  }

  return StatusCode::SUCCESS;
}

// createObj should create the RDO from bytestream.

StatusCode PpmByteStreamCnv::createObj( IOpaqueAddress* pAddr,
                                        DataObject*& pObj )
{
  MsgStream log( msgSvc(), "PpmByteStreamCnv" );
  const bool debug = msgSvc()->outputLevel("PpmByteStreamCnv") <= MSG::DEBUG;

  if (debug) log << MSG::DEBUG << "createObj() called" << endreq;

  ByteStreamAddress *pBS_Addr;
  pBS_Addr = dynamic_cast<ByteStreamAddress *>( pAddr );
  if ( !pBS_Addr ) {
    log << MSG::ERROR << " Can not cast to ByteStreamAddress " << endreq;
    return StatusCode::FAILURE;
  }

  const std::string nm = *( pBS_Addr->par() );

  if (debug) log << MSG::DEBUG << " Creating Objects " << nm << endreq;

  // get SourceIDs
  const std::vector<uint32_t>& vID(m_tool->sourceIDs());

  // get ROB fragments
  IROBDataProviderSvc::VROBFRAG robFrags;
  m_robDataProvider->getROBData( vID, robFrags );

  // size check
  DataVector<LVL1::TriggerTower>* const ttCollection =
                                           new DataVector<LVL1::TriggerTower>;
  if (debug) {
    log << MSG::DEBUG << " Number of ROB fragments is " << robFrags.size()
        << endreq;
  }
  if (robFrags.size() == 0) {
    pObj = SG::asStorable(ttCollection) ;
    return StatusCode::SUCCESS;
  }

  StatusCode sc = m_tool->convert(robFrags, ttCollection);
  if ( sc.isFailure() ) {
    log << MSG::ERROR << " Failed to create Objects   " << nm << endreq;
    delete ttCollection;
    return sc;
  }

  pObj = SG::asStorable(ttCollection);

  return sc;
}

// createRep should create the bytestream from RDOs.

StatusCode PpmByteStreamCnv::createRep( DataObject* pObj,
                                        IOpaqueAddress*& pAddr )
{
  MsgStream log( msgSvc(), "PpmByteStreamCnv" );
  const bool debug = msgSvc()->outputLevel("PpmByteStreamCnv") <= MSG::DEBUG;

  if (debug) log << MSG::DEBUG << "createRep() called" << endreq;

  RawEventWrite* re = m_ByteStreamEventAccess->getRawEvent();

  DataVector<LVL1::TriggerTower>* ttCollection = 0;
  if( !SG::fromStorable( pObj, ttCollection ) ) {
    log << MSG::ERROR << " Cannot cast to DataVector<TriggerTower>" << endreq;
    return StatusCode::FAILURE;
  }

  const std::string nm = pObj->registry()->name();

  ByteStreamAddress* addr = new ByteStreamAddress( classID(), nm, "" );

  pAddr = addr;

  // Convert to ByteStream
  return m_tool->convert( ttCollection, re );
}

} // end namespace
