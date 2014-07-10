
#include <vector>

#include "ByteStreamCnvSvcBase/ByteStreamAddress.h"
#include "ByteStreamCnvSvcBase/IByteStreamEventAccess.h"

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

#include "TrigT1CaloEvent/JEPRoIBSCollectionV1.h"

#include "JepRoiByteStreamV1Cnv.h"
#include "JepRoiByteStreamV1Tool.h"

namespace LVL1BS {

JepRoiByteStreamV1Cnv::JepRoiByteStreamV1Cnv( ISvcLocator* svcloc )
    : Converter( ByteStream_StorageType, classID(), svcloc ),
      m_name("JepRoiByteStreamV1Cnv"),
      m_tool("LVL1BS::JepRoiByteStreamV1Tool/JepRoiByteStreamV1Tool"),
      m_ByteStreamEventAccess("ByteStreamCnvSvc", m_name),
      m_log(msgSvc(), m_name), m_debug(false)
{
}

JepRoiByteStreamV1Cnv::~JepRoiByteStreamV1Cnv()
{
}

// CLID

const CLID& JepRoiByteStreamV1Cnv::classID()
{
  return ClassID_traits<LVL1::JEPRoIBSCollectionV1>::ID();
}

//  Init method gets all necessary services etc.

#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "unknown"
#endif

StatusCode JepRoiByteStreamV1Cnv::initialize()
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
    return StatusCode::FAILURE;
  } else m_log << MSG::DEBUG << "Retrieved tool " << m_tool << endreq;

  return StatusCode::SUCCESS;
}

// createRep should create the bytestream from RDOs.

StatusCode JepRoiByteStreamV1Cnv::createRep( DataObject* pObj,
                                             IOpaqueAddress*& pAddr )
{
  if (m_debug) m_log << MSG::DEBUG << "createRep() called" << endreq;

  RawEventWrite* re = m_ByteStreamEventAccess->getRawEvent();

  LVL1::JEPRoIBSCollectionV1* jep = 0;
  if( !SG::fromStorable( pObj, jep ) ) {
    m_log << MSG::ERROR << " Cannot cast to JEPRoIBSCollectionV1" << endreq;
    return StatusCode::FAILURE;
  }

  const std::string nm = pObj->registry()->name();

  ByteStreamAddress* addr = new ByteStreamAddress( classID(), nm, "" );

  pAddr = addr;

  // Convert to ByteStream
  return m_tool->convert( jep, re );
}

} // end namespace
