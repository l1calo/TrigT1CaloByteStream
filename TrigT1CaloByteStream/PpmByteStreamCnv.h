#ifndef TRIGT1CALOBYTESTREAM_PPMBYTESTREAMCNV_H
#define TRIGT1CALOBYTESTREAM_PPMBYTESTREAMCNV_H

#include "GaudiKernel/Converter.h"
#include "GaudiKernel/CnvFactory.h"
#include "GaudiKernel/StatusCode.h"

class DataObject;
class IROBDataProviderSvc;
class IByteStreamEventAccess;
class PpmByteStreamTool;

// Externals
extern svcType_t ByteStream_StorageType;

/** ByteStream converters for Pre-processor Module data / TriggerTowers.
 *
 *  @author Peter Faulkner
 */

class PpmByteStreamCnv: public Converter {

  friend class CnvFactory<PpmByteStreamCnv>;

protected:

  PpmByteStreamCnv(ISvcLocator* svcloc);

public:

  ~PpmByteStreamCnv();

  virtual StatusCode initialize();
  /// Create TriggerTowers from ByteStream
  virtual StatusCode createObj(IOpaqueAddress* pAddr, DataObject*& pObj);
  /// Create ByteStream from TriggerTowers
  virtual StatusCode createRep(DataObject* pObj, IOpaqueAddress*& pAddr);

  //  Storage type and class ID
  virtual svcType_t repSvcType() const { return ByteStream_StorageType;}
  static const svcType_t storageType(){ return ByteStream_StorageType; }
  static const CLID& classID();

private:

  /// Tool that does the actual work
  PpmByteStreamTool* m_tool;

  IROBDataProviderSvc* m_robDataProvider;
  IByteStreamEventAccess* m_ByteStreamEventAccess;

};

#endif
