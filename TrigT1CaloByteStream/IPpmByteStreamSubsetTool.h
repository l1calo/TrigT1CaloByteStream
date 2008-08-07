#ifndef TRIGT1CALOBYTESTREAM_IPPMBYTESTREAMSUBSETTOOL_H
#define TRIGT1CALOBYTESTREAM_IPPMBYTESTREAMSUBSETTOOL_H

#include <vector>

#include "GaudiKernel/IAlgTool.h"

#include "ByteStreamCnvSvcBase/ROBDataProviderSvc.h"
#include "DataModel/DataVector.h"

namespace LVL1 {
  class TriggerTower;
}

namespace LVL1BS {

static const InterfaceID IID_IPpmByteStreamSubsetTool("LVL1BS::IPpmByteStreamSubsetTool", 1, 0);

class IPpmByteStreamSubsetTool : virtual public IAlgTool {

 public:
   static const InterfaceID& interfaceID();

   virtual StatusCode convert(const IROBDataProviderSvc::VROBFRAG& robFrags,
                              DataVector<LVL1::TriggerTower>* ttCollection,
			      const std::vector<unsigned int> chanIds) = 0;
      
};

inline const InterfaceID& IPpmByteStreamSubsetTool::interfaceID()
{ 
  return IID_IPpmByteStreamSubsetTool;
}

} // end of namespace

#endif 
