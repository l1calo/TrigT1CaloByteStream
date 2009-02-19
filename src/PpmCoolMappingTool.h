#ifndef TRIGT1CALOBYTESTREAM_PPMCOOLMAPPINGTOOL_H
#define TRIGT1CALOBYTESTREAM_PPMCOOLMAPPINGTOOL_H

#include <map>
#include <utility>
#include <vector>

#include "AthenaBaseComps/AthAlgTool.h"
#include "GaudiKernel/ToolHandle.h"

#include "IL1CaloMappingTool.h"

class CaloLVL1_ID;
class CaloTriggerTowerService;
class IInterface;
class StatusCode;
class TTOnlineID;

namespace LVL1BS {

/** PPM crate/module/channel to eta/phi/layer mappings
 *
 *  Uses COOL channel mapping tools
 *  
 *
 *  @author Peter Faulkner
 */

class PpmCoolMappingTool : virtual public IL1CaloMappingTool,
                                   public AthAlgTool {

 public:

   PpmCoolMappingTool(const std::string& type, const std::string& name,
                                               const IInterface* parent);
   virtual ~PpmCoolMappingTool();

   virtual StatusCode initialize();
   virtual StatusCode finalize();

   /// Return eta, phi and layer mapping for given crate/module/channel
   virtual bool mapping(int crate, int module, int channel,
                        double& eta, double& phi, int& layer);
   /// Return crate, module and channel mapping for given eta/phi/layer
   virtual bool mapping(double eta, double phi, int layer,
                        int& crate, int& module, int& channel);

 private:

   // Tools and helpers
   ToolHandle<CaloTriggerTowerService> m_ttSvc;
   const CaloLVL1_ID*                  m_lvl1Helper;
   const TTOnlineID*                   m_l1ttonlineHelper;

};

} // end namespace

#endif
