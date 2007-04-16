
#include "DataModel/DataVector.h"
#include "GaudiKernel/DeclareFactoryEntries.h"

#include "TrigT1Calo/CMMCPHits.h"
#include "TrigT1Calo/CMMEtSums.h"
#include "TrigT1Calo/CMMJetHits.h"
#include "TrigT1Calo/CMMRoI.h"
#include "TrigT1Calo/CPMHits.h"
#include "TrigT1Calo/CPMTower.h"
#include "TrigT1Calo/JEMEtSums.h"
#include "TrigT1Calo/JEMHits.h"
#include "TrigT1Calo/JEMRoI.h"
#include "TrigT1Calo/JetElement.h"

#include "TrigT1CaloByteStream/CpmTester.h"
#include "TrigT1CaloByteStream/JemTester.h"
#include "TrigT1CaloByteStream/PpmTester.h"

#include "TrigT1CaloByteStream/CpByteStreamCnv.h"
#include "TrigT1CaloByteStream/CpmRoiByteStreamCnv.h"
#include "TrigT1CaloByteStream/CpReadByteStreamCnv.h"
#include "TrigT1CaloByteStream/JepByteStreamCnv.h"
#include "TrigT1CaloByteStream/JepReadByteStreamCnv.h"
#include "TrigT1CaloByteStream/JepRoiByteStreamCnv.h"
#include "TrigT1CaloByteStream/JepRoiReadByteStreamCnv.h"
#include "TrigT1CaloByteStream/PpmByteStreamCnv.h"

#include "TrigT1CaloByteStream/CpByteStreamTool.h"
#include "TrigT1CaloByteStream/CpmRoiByteStreamTool.h"
#include "TrigT1CaloByteStream/JepByteStreamTool.h"
#include "TrigT1CaloByteStream/JepRoiByteStreamTool.h"
#include "TrigT1CaloByteStream/PpmByteStreamTool.h"

typedef DataVector<LVL1::CPMTower>   CPMTowerCollection;
typedef DataVector<LVL1::CPMHits>    CPMHitsCollection;
typedef DataVector<LVL1::CMMCPHits>  CMMCPHitsCollection;
typedef DataVector<LVL1::JetElement> JetElementCollection;
typedef DataVector<LVL1::JEMHits>    JEMHitsCollection;
typedef DataVector<LVL1::JEMEtSums>  JEMEtSumsCollection;
typedef DataVector<LVL1::CMMJetHits> CMMJetHitsCollection;
typedef DataVector<LVL1::CMMEtSums>  CMMEtSumsCollection;
typedef DataVector<LVL1::JEMRoI>     JEMRoICollection;
typedef CpReadByteStreamCnv<CPMTowerCollection>    CpReadCTByteStreamCnvT;
typedef CpReadByteStreamCnv<CPMHitsCollection>     CpReadCHByteStreamCnvT;
typedef CpReadByteStreamCnv<CMMCPHitsCollection>   CpReadCCByteStreamCnvT;
typedef JepReadByteStreamCnv<JetElementCollection> JepReadJEByteStreamCnvT;
typedef JepReadByteStreamCnv<JEMHitsCollection>    JepReadJHByteStreamCnvT;
typedef JepReadByteStreamCnv<JEMEtSumsCollection>  JepReadESByteStreamCnvT;
typedef JepReadByteStreamCnv<CMMJetHitsCollection> JepReadCJByteStreamCnvT;
typedef JepReadByteStreamCnv<CMMEtSumsCollection>  JepReadCEByteStreamCnvT;
typedef JepRoiReadByteStreamCnv<JEMRoICollection>  JepRoiReadJRByteStreamCnvT;
typedef JepRoiReadByteStreamCnv<LVL1::CMMRoI>      JepRoiReadCRByteStreamCnvT;

// declare 
DECLARE_CONVERTER_FACTORY( CpByteStreamCnv )
DECLARE_CONVERTER_FACTORY( CpmRoiByteStreamCnv )
DECLARE_CONVERTER_FACTORY( JepByteStreamCnv )
DECLARE_CONVERTER_FACTORY( JepRoiByteStreamCnv )
DECLARE_CONVERTER_FACTORY( PpmByteStreamCnv )
DECLARE_CONVERTER_FACTORY( CpReadCTByteStreamCnvT )
DECLARE_CONVERTER_FACTORY( CpReadCHByteStreamCnvT )
DECLARE_CONVERTER_FACTORY( CpReadCCByteStreamCnvT )
DECLARE_CONVERTER_FACTORY( JepReadJEByteStreamCnvT )
DECLARE_CONVERTER_FACTORY( JepReadJHByteStreamCnvT )
DECLARE_CONVERTER_FACTORY( JepReadESByteStreamCnvT )
DECLARE_CONVERTER_FACTORY( JepReadCJByteStreamCnvT )
DECLARE_CONVERTER_FACTORY( JepReadCEByteStreamCnvT )
DECLARE_CONVERTER_FACTORY( JepRoiReadJRByteStreamCnvT )
DECLARE_CONVERTER_FACTORY( JepRoiReadCRByteStreamCnvT )

DECLARE_ALGORITHM_FACTORY( CpmTester )
DECLARE_ALGORITHM_FACTORY( JemTester )
DECLARE_ALGORITHM_FACTORY( PpmTester )

DECLARE_TOOL_FACTORY( CpByteStreamTool )
DECLARE_TOOL_FACTORY( CpmRoiByteStreamTool )
DECLARE_TOOL_FACTORY( JepByteStreamTool )
DECLARE_TOOL_FACTORY( JepRoiByteStreamTool )
DECLARE_TOOL_FACTORY( PpmByteStreamTool )

DECLARE_FACTORY_ENTRIES( TrigT1CaloByteStream )
{
  DECLARE_CONVERTER( CpByteStreamCnv )
  DECLARE_CONVERTER( CpmRoiByteStreamCnv )
  DECLARE_CONVERTER( JepByteStreamCnv )
  DECLARE_CONVERTER( JepRoiByteStreamCnv )
  DECLARE_CONVERTER( PpmByteStreamCnv )
  DECLARE_CONVERTER( CpReadCTByteStreamCnvT )
  DECLARE_CONVERTER( CpReadCHByteStreamCnvT )
  DECLARE_CONVERTER( CpReadCCByteStreamCnvT )
  DECLARE_CONVERTER( JepReadJEByteStreamCnvT )
  DECLARE_CONVERTER( JepReadJHByteStreamCnvT )
  DECLARE_CONVERTER( JepReadESByteStreamCnvT )
  DECLARE_CONVERTER( JepReadCJByteStreamCnvT )
  DECLARE_CONVERTER( JepReadCEByteStreamCnvT )
  DECLARE_CONVERTER( JepRoiReadJRByteStreamCnvT )
  DECLARE_CONVERTER( JepRoiReadCRByteStreamCnvT )
  DECLARE_ALGORITHM( CpmTester )
  DECLARE_ALGORITHM( JemTester )
  DECLARE_ALGORITHM( PpmTester )
}
