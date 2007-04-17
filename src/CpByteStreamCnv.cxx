
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

#include "TrigT1Calo/CPBSCollection.h"

#include "TrigT1CaloByteStream/CpByteStreamCnv.h"
#include "TrigT1CaloByteStream/CpByteStreamTool.h"

CpByteStreamCnv::CpByteStreamCnv( ISvcLocator* svcloc )
    : Converter( ByteStream_StorageType, classID(), svcloc )
{
}

CpByteStreamCnv::~CpByteStreamCnv()
{
}

// CLID

const CLID& CpByteStreamCnv::classID()
{
  return ClassID_traits<LVL1::CPBSCollection>::ID();
}

//  Init method gets all necessary services etc.

StatusCode CpByteStreamCnv::initialize()
{
  StatusCode sc = Converter::initialize();
  if ( sc.isFailure() )
    return sc;

  MsgStream log( msgSvc(), "CpByteStreamCnv" );
  log << MSG::DEBUG << " CpByteStreamCnv in initialize() " << endreq;

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
  const std::string toolType = "CpByteStreamTool" ;
  sc = toolSvc->retrieveTool( toolType, m_tool, m_ByteStreamEventAccess);
  if ( sc.isFailure() ) {
    log << MSG::ERROR << " Can't get ByteStreamTool of type "
        << toolType << endreq;
    return StatusCode::FAILURE;
  }

  return StatusCode::SUCCESS;
}

// createRep should create the bytestream from RDOs.

StatusCode CpByteStreamCnv::createRep( DataObject* pObj,
                                        IOpaqueAddress*& pAddr )
{
  MsgStream log( msgSvc(), "CpByteStreamCnv" );
  const bool debug = msgSvc()->outputLevel("CpByteStreamCnv") <= MSG::DEBUG;

  if (debug) log << MSG::DEBUG << "createRep() called" << endreq;

  RawEventWrite* re = m_ByteStreamEventAccess->getRawEvent();

  LVL1::CPBSCollection* cp = 0;
  if( !SG::fromStorable( pObj, cp ) ) {
    log << MSG::ERROR << " Cannot cast to CPBSCollection" << endreq;
    return StatusCode::FAILURE;
  }

  const std::string nm = pObj->registry()->name();

  ByteStreamAddress* addr = new ByteStreamAddress( classID(), nm, "" );

  pAddr = addr;

  // Convert to ByteStream
  return m_tool->convert( cp, re );
}
