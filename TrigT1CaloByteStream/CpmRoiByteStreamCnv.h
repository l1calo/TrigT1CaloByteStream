#ifndef TRIGT1CALOBYTESTREAM_CPMROIBYTESTREAMCNV_H
#define TRIGT1CALOBYTESTREAM_CPMROIBYTESTREAMCNV_H

#include "GaudiKernel/ClassID.h"
#include "GaudiKernel/CnvFactory.h"
#include "GaudiKernel/Converter.h"
#include "GaudiKernel/StatusCode.h"

class DataObject;
class IByteStreamEventAccess;
class IOpaqueAddress;
class IROBDataProviderSvc;
class ISvcLocator;

// Externals
extern long ByteStream_StorageType;

namespace LVL1BS {

class CpmRoiByteStreamTool;

/** ByteStream converter for Cluster Processor Module RoIs.
 *
 *  @author Peter Faulkner
 */

class CpmRoiByteStreamCnv: public Converter {

  friend class CnvFactory<CpmRoiByteStreamCnv>;

protected:

  CpmRoiByteStreamCnv(ISvcLocator* svcloc);

public:

  ~CpmRoiByteStreamCnv();

  virtual StatusCode initialize();
  /// Create CPM RoIs from ByteStream
  virtual StatusCode createObj(IOpaqueAddress* pAddr, DataObject*& pObj);
  /// Create ByteStream from CPM RoIs
  virtual StatusCode createRep(DataObject* pObj, IOpaqueAddress*& pAddr);

  //  Storage type and class ID
  virtual long repSvcType() const { return ByteStream_StorageType;}
  static const long storageType(){ return ByteStream_StorageType; }
  static const CLID& classID();

private:

  /// Tool that does the actual work
  CpmRoiByteStreamTool* m_tool;

  IROBDataProviderSvc* m_robDataProvider;
  IByteStreamEventAccess* m_ByteStreamEventAccess;

};

} // end namespace

#endif
