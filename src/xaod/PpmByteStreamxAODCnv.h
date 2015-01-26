#ifndef TRIGT1CALOBYTESTREAM_PPMBYTESTREAMXAODCNV_H
#define TRIGT1CALOBYTESTREAM_PPMBYTESTREAMXAODCNV_H

#include <string>

#include "GaudiKernel/ClassID.h"
#include "GaudiKernel/Converter.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/ServiceHandle.h"
#include "GaudiKernel/ToolHandle.h"

#include "AthenaBaseComps/AthMessaging.h"

class DataObject;
class IByteStreamEventAccess;
class IOpaqueAddress;
class IROBDataProviderSvc;
class ISvcLocator;
class StatusCode;

template <typename> class CnvFactory;
class StoreGateSvc;


// Externals
extern long ByteStream_StorageType;




namespace LVL1BS {
class PpmByteStreamxAODReadTool;

/** ByteStream converter for Pre-processor Module DAQ data / TriggerTowers.
 *
 *  @author alexander.mazurov@cern.ch
 */

class PpmByteStreamxAODCnv: public Converter, public ::AthMessaging {

  friend class CnvFactory<PpmByteStreamxAODCnv>;

protected:

  PpmByteStreamxAODCnv(ISvcLocator* svcloc);

public:

  virtual ~PpmByteStreamxAODCnv(){};

  virtual StatusCode initialize();
  /// Create TriggerTowers from ByteStream
  virtual StatusCode createObj(IOpaqueAddress* pAddr, DataObject*& pObj);
  /// Create ByteStream from TriggerTowers
  virtual StatusCode createRep(DataObject* pObj, IOpaqueAddress*& pAddr);

  //  Storage type and class ID
  virtual long repSvcType() const { return ByteStream_StorageType;}
  static  long storageType(){ return ByteStream_StorageType; }

  static const CLID& classID();

private:

  /// Converter name
  std::string m_name;

  /// ServiceHandle to the data store service to store aux objects
  ServiceHandle<StoreGateSvc> m_storeSvc;

  /// Service for reading bytestream
  ServiceHandle<IROBDataProviderSvc> m_robDataProvider;
  /// Service for writing bytestream
  ServiceHandle<IByteStreamEventAccess> m_ByteStreamEventAccess;

  /// Do the main job - retrieve xAOD TriggerTowers from robs
  ToolHandle<PpmByteStreamxAODReadTool> m_readTool;
};

} // end namespace

#endif