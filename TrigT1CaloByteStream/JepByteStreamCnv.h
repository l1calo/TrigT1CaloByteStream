#ifndef TRIGT1CALOBYTESTREAM_JEPBYTESTREAMCNV_H
#define TRIGT1CALOBYTESTREAM_JEPBYTESTREAMCNV_H

#include "GaudiKernel/ClassID.h"
#include "GaudiKernel/CnvFactory.h"
#include "GaudiKernel/Converter.h"
#include "GaudiKernel/StatusCode.h"

class DataObject;
class IByteStreamEventAccess;
class IOpaqueAddress;
class IROBDataProviderSvc;
class ISvcLocator;
class JepByteStreamTool;

// Externals
extern long ByteStream_StorageType;

/** ByteStream converter for JEP container
 *
 *  @author Peter Faulkner
 */

class JepByteStreamCnv: public Converter {

  friend class CnvFactory<JepByteStreamCnv>;

protected:

  JepByteStreamCnv(ISvcLocator* svcloc);

public:

  ~JepByteStreamCnv();

  virtual StatusCode initialize();
  /// Create ByteStream from JepContainer
  virtual StatusCode createRep(DataObject* pObj, IOpaqueAddress*& pAddr);

  //  Storage type and class ID
  virtual long repSvcType() const { return ByteStream_StorageType;}
  static const long storageType(){ return ByteStream_StorageType; }
  static const CLID& classID();

private:

  //  Tool that does the actual work
  JepByteStreamTool* m_tool;

  IROBDataProviderSvc* m_robDataProvider;
  IByteStreamEventAccess* m_ByteStreamEventAccess;

};

#endif
