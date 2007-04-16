#ifndef TRIGT1CALOBYTESTREAM_JEPROIREADBYTESTREAMCNV_H
#define TRIGT1CALOBYTESTREAM_JEPROIREADBYTESTREAMCNV_H

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

/** ByteStream converter for JEP component containers.
 *
 *  @author Peter Faulkner
 */

template <typename Container>
class JepRoiReadByteStreamCnv: public Converter {

  friend class CnvFactory<JepRoiReadByteStreamCnv<Container> >;

protected:

  JepRoiReadByteStreamCnv(ISvcLocator* svcloc);

public:

  ~JepRoiReadByteStreamCnv();

  virtual StatusCode initialize();
  /// Create Container from ByteStream
  virtual StatusCode createObj(IOpaqueAddress* pAddr, DataObject*& pObj);

  //  Storage type and class ID
  virtual long repSvcType() const { return ByteStream_StorageType;}
  static const long storageType(){ return ByteStream_StorageType; }
  static const CLID& classID();

private:

  /// Tool that does the actual work
  JepRoiByteStreamTool* m_tool;

  IROBDataProviderSvc* m_robDataProvider;
  IByteStreamEventAccess* m_ByteStreamEventAccess;

};

#include "TrigT1CaloByteStream/JepRoiReadByteStreamCnv.icc"

#endif
