#ifndef TRIGT1CALOBYTESTREAM_PPMTESTER_H
#define TRIGT1CALOBYTESTREAM_PPMTESTER_H

#include <map>
#include <string>

#include "DataModel/DataVector.h"
#include "GaudiKernel/Algorithm.h"
#include "GaudiKernel/MsgStream.h"

class StoreGateSvc;

namespace LVL1 {
  class TriggerTower;
  class TriggerTowerKey;
}

/** Algorithm to test Trigger tower bytestream conversions.
 *
 *  Just prints out the contents of the TriggerTower objects.
 *  Run before writing bytestream and after reading it and compare.
 *
 *  @author Peter Faulkner
 */

class PpmTester : public Algorithm {

 public:
   PpmTester(const std::string& name, ISvcLocator* pSvcLocator);
   virtual ~PpmTester();

   virtual StatusCode initialize();
   virtual StatusCode execute();
   virtual StatusCode finalize();

 private:
   /// Trigger tower container
   typedef DataVector<LVL1::TriggerTower> TriggerTowerCollection;
   /// Trigger tower map
   typedef std::map<unsigned int, LVL1::TriggerTower*> TriggerTowerMap;

   /// Print the trigger towers
   void printTriggerTowers(MsgStream& log, MSG::Level level);

   /// Print a vector
   void printVec(const std::vector<int>& vec, MsgStream& log,
                                              MSG::Level level);

   /// Set up trigger tower map
   void setupTTMap(const TriggerTowerCollection* jeCollection);

   /// Trigger tower key provider
   LVL1::TriggerTowerKey* m_towerKey;
   /// StoreGate service
   StoreGateSvc* m_storeGate;
   /// Trigger tower container StoreGate key
   std::string m_triggerTowerLocation;

   /// Trigger tower map
   TriggerTowerMap m_ttMap;

};

#endif
