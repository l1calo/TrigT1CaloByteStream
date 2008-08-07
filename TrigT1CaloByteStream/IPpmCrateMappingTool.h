#ifndef TRIGT1CALOBYTESTREAM_IPPMCRATEMAPPINGTOOL_H
#define TRIGT1CALOBYTESTREAM_IPPMCRATEMAPPINGTOOL_H

#include "GaudiKernel/IAlgTool.h"

#include "TrigT1CaloByteStream/ChannelCoordinate.h"

namespace LVL1BS {

static const InterfaceID IID_IPpmCrateMappingTool("LVL1BS::IPpmCrateMappingTool", 1, 0);

class IPpmCrateMappingTool : virtual public IAlgTool {

 public:
   static const InterfaceID& interfaceID();

   virtual bool mapping(int crate, int module, int channel,
                                       ChannelCoordinate& coord) const = 0;
   virtual bool mapping(const ChannelCoordinate& coord, int& crate,
                                      int& module, int& channel) = 0;
      
};

inline const InterfaceID& IPpmCrateMappingTool::interfaceID()
{ 
  return IID_IPpmCrateMappingTool;
}

} // end of namespace

#endif 
