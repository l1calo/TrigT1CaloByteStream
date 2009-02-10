#ifndef TRIGT1CALOBYTESTREAM_CPREADBYTESTREAMCNV_H
#define TRIGT1CALOBYTESTREAM_CPREADBYTESTREAMCNV_H

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

class CpByteStreamTool;

/** ByteStream converter for CP component containers.
 *
 *  @author Peter Faulkner
 */

template <typename Container>
class CpReadByteStreamCnv: public Converter {

  friend class CnvFactory<CpReadByteStreamCnv<Container> >;

protected:

  CpReadByteStreamCnv(ISvcLocator* svcloc);

public:

  ~CpReadByteStreamCnv();

  virtual StatusCode initialize();
  /// Create Container from ByteStream
  virtual StatusCode createObj(IOpaqueAddress* pAddr, DataObject*& pObj);

  //  Storage type and class ID
  virtual long repSvcType() const { return ByteStream_StorageType;}
  static  long storageType(){ return ByteStream_StorageType; }
  static const CLID& classID();

private:

  /// Tool that does the actual work
  CpByteStreamTool* m_tool;

  IROBDataProviderSvc* m_robDataProvider;
  IByteStreamEventAccess* m_ByteStreamEventAccess;

};

} // end namespace

#include "TrigT1CaloByteStream/CpReadByteStreamCnv.icc"

#endif
