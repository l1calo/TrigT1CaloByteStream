#ifndef TRIGT1CALOBYTESTREAM_PPMBYTESTREAMCNV_H
#define TRIGT1CALOBYTESTREAM_PPMBYTESTREAMCNV_H

#include "GaudiKernel/ClassID.h"
#include "GaudiKernel/CnvFactory.h"
#include "GaudiKernel/Converter.h"
#include "GaudiKernel/StatusCode.h"
#include "GaudiKernel/svcType_t.h"

class DataObject;
class IByteStreamEventAccess;
class IOpaqueAddress;
class IROBDataProviderSvc;
class ISvcLocator;
class PpmByteStreamTool;

// Externals
extern svcType_t ByteStream_StorageType;

/** ByteStream converter for Pre-processor Module DAQ data / TriggerTowers.
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
