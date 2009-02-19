#ifndef TRIGT1CALOBYTESTREAM_TRIGT1CALODATAACCESS_H
#define TRIGT1CALOBYTESTREAM_TRIGT1CALODATAACCESS_H

#include <string>
#include <vector>

#include "AthenaBaseComps/AthAlgTool.h"
#include "ByteStreamData/RawEvent.h"
#include "DataModel/DataVector.h"
#include "GaudiKernel/ServiceHandle.h"
#include "GaudiKernel/ToolHandle.h"

#include "TrigT1CaloByteStream/ITrigT1CaloDataAccess.h"

class IInterface;
class IROBDataProviderSvc;
class StatusCode;

namespace LVL1 {
  class TriggerTower;
}

namespace LVL1BS {

class IPpmByteStreamSubsetTool;
class ITriggerTowerSelectionTool;

/** Tool to retrieve TriggerTowers corresponding to a given eta/phi range
 *  from bytestream.
 *
 *  @author Peter Faulkner
 */

class TrigT1CaloDataAccess : virtual public ITrigT1CaloDataAccess,
                                     public AthAlgTool {

 public:
   TrigT1CaloDataAccess(const std::string& type, const std::string& name,
                        const IInterface* parent);
   virtual ~TrigT1CaloDataAccess();

   virtual StatusCode initialize();
   virtual StatusCode finalize();

   /// Return iterators to required trigger towers
   virtual StatusCode loadCollection(
                      DataVector<LVL1::TriggerTower>::const_iterator& beg,
		      DataVector<LVL1::TriggerTower>::const_iterator& end,
		      double etaMin, double etaMax,
		      double phiMin, double phiMax);

 private:

   /// Data fetching service
   ServiceHandle<IROBDataProviderSvc>             m_robDataProvider;
   /// Tool for selections
   ToolHandle<LVL1BS::ITriggerTowerSelectionTool> m_selectionTool;
   /// Tool for bytestream conversion
   ToolHandle<LVL1BS::IPpmByteStreamSubsetTool>   m_ppmBSConverter;

   /// ROB fragment pointers
   std::vector<const OFFLINE_FRAGMENTS_NAMESPACE::ROBFragment*> m_robFrags;
   /// Current TriggerTower sub-collection
   DataVector<LVL1::TriggerTower>* m_ttCol;

};

} // end namespace

#endif
