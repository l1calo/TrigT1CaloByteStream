#ifndef TRIGT1CALOBYTESTREAM_CPBYTESTREAMCNV_H
#define TRIGT1CALOBYTESTREAM_CPBYTESTREAMCNV_H

#include <string>

#include "GaudiKernel/ClassID.h"
#include "GaudiKernel/Converter.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/ServiceHandle.h"
#include "GaudiKernel/ToolHandle.h"

class DataObject;
class IByteStreamEventAccess;
class IOpaqueAddress;
class ISvcLocator;
class StatusCode;

template <typename> class CnvFactory;

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

  /// Converter name
  std::string m_name;

  /// Tool that does the actual work
  ToolHandle<LVL1BS::CpByteStreamTool> m_tool;

  /// Service for writing bytestream
  ServiceHandle<IByteStreamEventAccess> m_ByteStreamEventAccess;

  /// Message log
  mutable MsgStream m_log;
  bool m_debug;

};

} // end namespace

#endif
