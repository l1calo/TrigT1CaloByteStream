#ifndef TRIGT1CALOBYTESTREAM_CPBYTESTREAMCNV_H
#define TRIGT1CALOBYTESTREAM_CPBYTESTREAMCNV_H

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

/** ByteStream converter for CP container
 *
 *  @author Peter Faulkner
 */

class CpByteStreamCnv: public Converter {

  friend class CnvFactory<CpByteStreamCnv>;

protected:

  CpByteStreamCnv(ISvcLocator* svcloc);

public:

  ~CpByteStreamCnv();

  virtual StatusCode initialize();
  /// Create ByteStream from Cp Container
  virtual StatusCode createRep(DataObject* pObj, IOpaqueAddress*& pAddr);

  //  Storage type and class ID
  virtual long repSvcType() const { return ByteStream_StorageType;}
  static  long storageType(){ return ByteStream_StorageType; }
  static const CLID& classID();

private:

  //  Tool that does the actual work
  CpByteStreamTool* m_tool;

  IROBDataProviderSvc* m_robDataProvider;
  IByteStreamEventAccess* m_ByteStreamEventAccess;

};

} // end namespace

#endif
