
#include <string>
#include <vector>

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

#include "TrigT1Calo/JEPRoIBSCollection.h"

#include "TrigT1CaloByteStream/JepRoiByteStreamCnv.h"
#include "TrigT1CaloByteStream/JepRoiByteStreamTool.h"

JepRoiByteStreamCnv::JepRoiByteStreamCnv( ISvcLocator* svcloc )
    : Converter( ByteStream_StorageType, classID(), svcloc )
{
}

JepRoiByteStreamCnv::~JepRoiByteStreamCnv()
{
}

// CLID

const CLID& JepRoiByteStreamCnv::classID()
{
  return ClassID_traits<LVL1::JEPRoIBSCollection>::ID();
}

//  Init method gets all necessary services etc.

StatusCode JepRoiByteStreamCnv::initialize()
{
  StatusCode sc = Converter::initialize();
  if ( sc.isFailure() )
    return sc;

  MsgStream log( msgSvc(), "JepRoiByteStreamCnv" );
  log << MSG::DEBUG << " JepRoiByteStreamCnv in initialize() " << endreq;

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
  const std::string toolType = "JepRoiByteStreamTool" ;
  sc = toolSvc->retrieveTool( toolType, m_tool, m_ByteStreamEventAccess);
  if ( sc.isFailure() ) {
    log << MSG::ERROR << " Can't get ByteStreamTool of type "
        << toolType << endreq;
    return StatusCode::FAILURE;
  }

  return StatusCode::SUCCESS;
}

// createRep should create the bytestream from RDOs.

StatusCode JepRoiByteStreamCnv::createRep( DataObject* pObj,
                                        IOpaqueAddress*& pAddr )
{
  MsgStream log( msgSvc(), "JepRoiByteStreamCnv" );
  const bool debug = msgSvc()->outputLevel("JepRoiByteStreamCnv") <= MSG::DEBUG;

  if (debug) log << MSG::DEBUG << "createRep() called" << endreq;

  RawEventWrite* re = m_ByteStreamEventAccess->getRawEvent();

  const LVL1::JEPRoIBSCollection* jep;
  if( !SG::fromStorable( pObj, jep ) ) {
    log << MSG::ERROR << " Cannot cast to JEPRoIBSCollection" << endreq;
    return StatusCode::FAILURE;
  }

  const std::string nm = pObj->registry()->name();

  ByteStreamAddress* addr = new ByteStreamAddress( classID(), nm, "" );

  pAddr = addr;

  // Convert to ByteStream
  return m_tool->convert( jep, re );
}
