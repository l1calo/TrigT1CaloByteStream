
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

namespace LVL1BS {

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

}

// declare 
DECLARE_NAMESPACE_CONVERTER_FACTORY( LVL1BS, CpByteStreamCnv )
DECLARE_NAMESPACE_CONVERTER_FACTORY( LVL1BS, CpmRoiByteStreamCnv )
DECLARE_NAMESPACE_CONVERTER_FACTORY( LVL1BS, JepByteStreamCnv )
DECLARE_NAMESPACE_CONVERTER_FACTORY( LVL1BS, JepRoiByteStreamCnv )
DECLARE_NAMESPACE_CONVERTER_FACTORY( LVL1BS, PpmByteStreamCnv )
DECLARE_NAMESPACE_CONVERTER_FACTORY( LVL1BS, CpReadCTByteStreamCnvT )
DECLARE_NAMESPACE_CONVERTER_FACTORY( LVL1BS, CpReadCHByteStreamCnvT )
DECLARE_NAMESPACE_CONVERTER_FACTORY( LVL1BS, CpReadCCByteStreamCnvT )
DECLARE_NAMESPACE_CONVERTER_FACTORY( LVL1BS, JepReadJEByteStreamCnvT )
DECLARE_NAMESPACE_CONVERTER_FACTORY( LVL1BS, JepReadJHByteStreamCnvT )
DECLARE_NAMESPACE_CONVERTER_FACTORY( LVL1BS, JepReadESByteStreamCnvT )
DECLARE_NAMESPACE_CONVERTER_FACTORY( LVL1BS, JepReadCJByteStreamCnvT )
DECLARE_NAMESPACE_CONVERTER_FACTORY( LVL1BS, JepReadCEByteStreamCnvT )
DECLARE_NAMESPACE_CONVERTER_FACTORY( LVL1BS, JepRoiReadJRByteStreamCnvT )
DECLARE_NAMESPACE_CONVERTER_FACTORY( LVL1BS, JepRoiReadCRByteStreamCnvT )

DECLARE_NAMESPACE_ALGORITHM_FACTORY( LVL1BS, CpmTester )
DECLARE_NAMESPACE_ALGORITHM_FACTORY( LVL1BS, JemTester )
DECLARE_NAMESPACE_ALGORITHM_FACTORY( LVL1BS, PpmTester )

DECLARE_NAMESPACE_TOOL_FACTORY( LVL1BS, CpByteStreamTool )
DECLARE_NAMESPACE_TOOL_FACTORY( LVL1BS, CpmRoiByteStreamTool )
DECLARE_NAMESPACE_TOOL_FACTORY( LVL1BS, JepByteStreamTool )
DECLARE_NAMESPACE_TOOL_FACTORY( LVL1BS, JepRoiByteStreamTool )
DECLARE_NAMESPACE_TOOL_FACTORY( LVL1BS, PpmByteStreamTool )

DECLARE_FACTORY_ENTRIES( TrigT1CaloByteStream )
{
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, CpByteStreamCnv )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, CpmRoiByteStreamCnv )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, JepByteStreamCnv )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, JepRoiByteStreamCnv )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, PpmByteStreamCnv )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, CpReadCTByteStreamCnvT )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, CpReadCHByteStreamCnvT )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, CpReadCCByteStreamCnvT )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, JepReadJEByteStreamCnvT )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, JepReadJHByteStreamCnvT )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, JepReadESByteStreamCnvT )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, JepReadCJByteStreamCnvT )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, JepReadCEByteStreamCnvT )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, JepRoiReadJRByteStreamCnvT )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, JepRoiReadCRByteStreamCnvT )

  DECLARE_NAMESPACE_ALGORITHM( LVL1BS, CpmTester )
  DECLARE_NAMESPACE_ALGORITHM( LVL1BS, JemTester )
  DECLARE_NAMESPACE_ALGORITHM( LVL1BS, PpmTester )

  DECLARE_NAMESPACE_TOOL( LVL1BS, CpByteStreamTool )
  DECLARE_NAMESPACE_TOOL( LVL1BS, CpmRoiByteStreamTool )
  DECLARE_NAMESPACE_TOOL( LVL1BS, JepByteStreamTool )
  DECLARE_NAMESPACE_TOOL( LVL1BS, JepRoiByteStreamTool )
  DECLARE_NAMESPACE_TOOL( LVL1BS, PpmByteStreamTool )
}
