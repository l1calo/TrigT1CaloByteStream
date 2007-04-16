#ifndef TRIGT1CALOBYTESTREAM_JEPROIBYTESTREAMCNV_H
#define TRIGT1CALOBYTESTREAM_JEPROIBYTESTREAMCNV_H

#include "GaudiKernel/ClassID.h"
#include "GaudiKernel/CnvFactory.h"
#include "GaudiKernel/Converter.h"
#include "GaudiKernel/StatusCode.h"

class DataObject;
class IByteStreamEventAccess;
class IOpaqueAddress;
class IROBDataProviderSvc;
class ISvcLocator;
class JepRoiByteStreamTool;

// Externals
extern long ByteStream_StorageType;

/** ByteStream converter for JEP RoI container
 *
 *  @author Peter Faulkner
 */

class JepRoiByteStreamCnv: public Converter {

  friend class CnvFactory<JepRoiByteStreamCnv>;

protected:

  JepRoiByteStreamCnv(ISvcLocator* svcloc);

public:

  ~JepRoiByteStreamCnv();

  virtual StatusCode initialize();
  /// Create ByteStream from JEP Container
  virtual StatusCode createRep(DataObject* pObj, IOpaqueAddress*& pAddr);

  //  Storage type and class ID
  virtual long repSvcType() const { return ByteStream_StorageType;}
  static const long storageType(){ return ByteStream_StorageType; }
  static const CLID& classID();

private:

  //  Tool that does the actual work
  JepRoiByteStreamTool* m_tool;

  IROBDataProviderSvc* m_robDataProvider;
  IByteStreamEventAccess* m_ByteStreamEventAccess;

};

#endif
