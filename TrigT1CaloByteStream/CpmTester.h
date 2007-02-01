#ifndef TRIGT1CALOBYTESTREAM_CPMTESTER_H
#define TRIGT1CALOBYTESTREAM_CPMTESTER_H

#include <map>
#include <string>

#include "DataModel/DataVector.h"
#include "GaudiKernel/Algorithm.h"
#include "GaudiKernel/MsgStream.h"

class StoreGateSvc;

namespace LVL1 {
  class CMMCPHits;
  class CPMHits;
  class CPMTower;
  class TriggerTowerKey;
}

/** Algorithm to test CPM component bytestream conversions.
 *
 *  Just prints out the contents of the CPMTower objects
 *  and CPMHits objects.
 *  Also includes CMMCPHits.
 *
 *  Run before writing bytestream and after reading it and compare.
 *
 *  @author Peter Faulkner
 */

class CpmTester : public Algorithm {

 public:
   CpmTester(const std::string& name, ISvcLocator* pSvcLocator);
   virtual ~CpmTester();

   virtual StatusCode initialize();
   virtual StatusCode execute();
   virtual StatusCode finalize();

 private:
   typedef DataVector<LVL1::CPMTower>              CpmTowerCollection;
   typedef DataVector<LVL1::CPMHits>               CpmHitsCollection;
   typedef DataVector<LVL1::CMMCPHits>             CmmCpHitsCollection;
   typedef std::map<unsigned int, LVL1::CPMTower*> CpmTowerMap;
   typedef std::map<int, LVL1::CPMHits*>           CpmHitsMap;
   typedef std::map<int, LVL1::CMMCPHits*>         CmmCpHitsMap;

   /// Print the CPM towers
   void printCpmTowers(MsgStream& log, MSG::Level level);
   /// Print the CPM hits
   void printCpmHits(MsgStream& log, MSG::Level level);
   /// Print the CMM-CP hits
   void printCmmCpHits(MsgStream& log, MSG::Level level);

   /// Print a vector
   void printVec(const std::vector<int>& vec, MsgStream& log,
                                              MSG::Level level);
   /// Print a vector of hits
   void printVecH(const std::vector<unsigned int>& vec, MsgStream& log,
                                                   MSG::Level level);

   /// Set up CPM tower map
   void setupCpmTowerMap(const CpmTowerCollection* ttCollection);
   /// Set up CPM hits map
   void setupCpmHitsMap(const CpmHitsCollection* hitCollection);
   /// Set up CMM-CP hits map
   void setupCmmCpHitsMap(const CmmCpHitsCollection* hitCollection);

   /// CPM tower key provider
   LVL1::TriggerTowerKey* m_towerKey;
   /// StoreGate service
   StoreGateSvc* m_storeGate;
   /// CPM tower container StoreGate key
   std::string m_cpmTowerLocation;
   /// CPM hits container StoreGate key
   std::string m_cpmHitsLocation;
   /// CMM-CP hits container StoreGate key
   std::string m_cmmCpHitsLocation;
   /// CPM tower print flag
   int m_cpmTowerPrint;
   /// CPM hits print flag
   int m_cpmHitsPrint;
   /// CMM-CP hits print flag
   int m_cmmCpHitsPrint;
   /// Number of CPM modules per crate
   int m_modules;

   /// CPM tower map
   CpmTowerMap  m_ttMap;
   /// CPM hits map
   CpmHitsMap   m_hitsMap;
   /// CMM-CP hits map
   CmmCpHitsMap m_cmmHitsMap;

};

#endif
