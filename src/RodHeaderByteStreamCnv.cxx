
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

#include "TrigT1Calo/RODHeader.h"

#include "TrigT1CaloByteStream/RodHeaderByteStreamCnv.h"
#include "TrigT1CaloByteStream/RodHeaderByteStreamTool.h"

namespace LVL1BS {

RodHeaderByteStreamCnv::RodHeaderByteStreamCnv( ISvcLocator* svcloc )
    : Converter( ByteStream_StorageType, classID(), svcloc )
{
}

RodHeaderByteStreamCnv::~RodHeaderByteStreamCnv()
{
}

// CLID

const CLID& RodHeaderByteStreamCnv::classID()
{
  return ClassID_traits<DataVector<LVL1::RODHeader> >::ID();
}

//  Init method gets all necessary services etc.

StatusCode RodHeaderByteStreamCnv::initialize()
{
  StatusCode sc = Converter::initialize();
  if ( sc.isFailure() )
    return sc;

  MsgStream log( msgSvc(), "RodHeaderByteStreamCnv" );
  log << MSG::DEBUG << " RodHeaderByteStreamCnv in initialize() " << endreq;

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
  const std::string toolType =
                    "LVL1BS::RodHeaderByteStreamTool/RodHeaderByteStreamTool";
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
    return sc ;
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

StatusCode RodHeaderByteStreamCnv::createObj( IOpaqueAddress* pAddr,
                                        DataObject*& pObj )
{
  MsgStream log( msgSvc(), "RodHeaderByteStreamCnv" );
  const bool debug =
             msgSvc()->outputLevel("RodHeaderByteStreamCnv") <= MSG::DEBUG;

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
  const std::vector<uint32_t>& vID(m_tool->sourceIDs(nm));

  // get ROB fragments
  IROBDataProviderSvc::VROBFRAG robFrags;
  m_robDataProvider->getROBData( vID, robFrags );

  // size check
  DataVector<LVL1::RODHeader>* const rhCollection =
                                           new DataVector<LVL1::RODHeader>;
  if (debug) {
    log << MSG::DEBUG << " Number of ROB fragments is " << robFrags.size()
        << endreq;
  }
  if (robFrags.size() == 0) {
    pObj = SG::asStorable(rhCollection) ;
    return StatusCode::SUCCESS;
  }

  StatusCode sc = m_tool->convert(robFrags, rhCollection);
  if ( sc.isFailure() ) {
    log << MSG::ERROR << " Failed to create Objects   " << nm << endreq;
    delete rhCollection;
    return sc;
  }

  pObj = SG::asStorable(rhCollection);

  return sc;
}

} // end namespace
