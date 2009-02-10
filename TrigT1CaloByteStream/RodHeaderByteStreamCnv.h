#ifndef TRIGT1CALOBYTESTREAM_RODHEADERBYTESTREAMCNV_H
#define TRIGT1CALOBYTESTREAM_RODHEADERBYTESTREAMCNV_H

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

class RodHeaderByteStreamTool;

/** ByteStream converter for L1Calo ROD header info
 *
 *  @author Peter Faulkner
 */

class RodHeaderByteStreamCnv: public Converter {

  friend class CnvFactory<RodHeaderByteStreamCnv>;

protected:

  RodHeaderByteStreamCnv(ISvcLocator* svcloc);

public:

  ~RodHeaderByteStreamCnv();

  virtual StatusCode initialize();
  /// Create RodHeaders from ByteStream
  virtual StatusCode createObj(IOpaqueAddress* pAddr, DataObject*& pObj);

  //  Storage type and class ID
  virtual long repSvcType() const { return ByteStream_StorageType;}
  static  long storageType(){ return ByteStream_StorageType; }
  static const CLID& classID();

private:

  /// Tool that does the actual work
  RodHeaderByteStreamTool* m_tool;

  IROBDataProviderSvc* m_robDataProvider;
  IByteStreamEventAccess* m_ByteStreamEventAccess;

};

} // end namespace

#endif
