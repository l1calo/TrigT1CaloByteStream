#ifndef TRIGT1CALOBYTESTREAM_JEPREADBYTESTREAMCNV_H
#define TRIGT1CALOBYTESTREAM_JEPREADBYTESTREAMCNV_H

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
class JepByteStreamTool;

// Externals
extern svcType_t ByteStream_StorageType;

/** ByteStream converter for JEP component containers.
 *
 *  @author Peter Faulkner
 */

template <typename Container>
class JepReadByteStreamCnv: public Converter {

  friend class CnvFactory<JepReadByteStreamCnv<Container> >;

protected:

  JepReadByteStreamCnv(ISvcLocator* svcloc);

public:

  ~JepReadByteStreamCnv();

  virtual StatusCode initialize();
  /// Create JetElements from ByteStream
  virtual StatusCode createObj(IOpaqueAddress* pAddr, DataObject*& pObj);

  //  Storage type and class ID
  virtual svcType_t repSvcType() const { return ByteStream_StorageType;}
  static const svcType_t storageType(){ return ByteStream_StorageType; }
  static const CLID& classID();

private:

  /// Tool that does the actual work
  JepByteStreamTool* m_tool;

  IROBDataProviderSvc* m_robDataProvider;
  IByteStreamEventAccess* m_ByteStreamEventAccess;

};

#include "TrigT1CaloByteStream/JepReadByteStreamCnv.icc"

#endif
