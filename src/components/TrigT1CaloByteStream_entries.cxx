
#include "DataModel/DataVector.h"
#include "GaudiKernel/DeclareFactoryEntries.h"

#include "TrigT1CaloEvent/CMMCPHits.h"
#include "TrigT1CaloEvent/CMMEtSums.h"
#include "TrigT1CaloEvent/CMMJetHits.h"
#include "TrigT1CaloEvent/CMMRoI.h"
#include "TrigT1CaloEvent/CPMHits.h"
#include "TrigT1CaloEvent/CPMTower.h"
#include "TrigT1CaloEvent/JEMEtSums.h"
#include "TrigT1CaloEvent/JEMHits.h"
#include "TrigT1CaloEvent/JEMRoI.h"
#include "TrigT1CaloEvent/JetElement.h"

#include "../CpByteStreamCnv.h"
#include "../CpmRoiByteStreamCnv.h"
#include "../CpReadByteStreamCnv.h"
#include "../JepByteStreamCnv.h"
#include "../JepReadByteStreamCnv.h"
#include "../JepRoiByteStreamCnv.h"
#include "../JepRoiReadByteStreamCnv.h"
#include "../PpmByteStreamCnv.h"
#include "../RodHeaderByteStreamCnv.h"

#include "../CpByteStreamTool.h"
#include "../CpmRoiByteStreamTool.h"
#include "../JepByteStreamTool.h"
#include "../JepRoiByteStreamTool.h"
#include "../PpmByteStreamTool.h"
#include "../RodHeaderByteStreamTool.h"

#include "../PpmByteStreamSubsetTool.h"
#include "../TriggerTowerSelectionTool.h"
#include "../TrigT1CaloDataAccess.h"

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
DECLARE_NAMESPACE_CONVERTER_FACTORY( LVL1BS, RodHeaderByteStreamCnv )
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

DECLARE_NAMESPACE_TOOL_FACTORY( LVL1BS, CpByteStreamTool )
DECLARE_NAMESPACE_TOOL_FACTORY( LVL1BS, CpmRoiByteStreamTool )
DECLARE_NAMESPACE_TOOL_FACTORY( LVL1BS, JepByteStreamTool )
DECLARE_NAMESPACE_TOOL_FACTORY( LVL1BS, JepRoiByteStreamTool )
DECLARE_NAMESPACE_TOOL_FACTORY( LVL1BS, PpmByteStreamTool )
DECLARE_NAMESPACE_TOOL_FACTORY( LVL1BS, RodHeaderByteStreamTool )

DECLARE_NAMESPACE_TOOL_FACTORY( LVL1BS, PpmByteStreamSubsetTool )
DECLARE_NAMESPACE_TOOL_FACTORY( LVL1BS, TriggerTowerSelectionTool )
DECLARE_NAMESPACE_TOOL_FACTORY( LVL1BS, TrigT1CaloDataAccess )

DECLARE_FACTORY_ENTRIES( TrigT1CaloByteStream )
{
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, CpByteStreamCnv )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, CpmRoiByteStreamCnv )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, JepByteStreamCnv )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, JepRoiByteStreamCnv )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, PpmByteStreamCnv )
  DECLARE_NAMESPACE_CONVERTER( LVL1BS, RodHeaderByteStreamCnv )
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

  DECLARE_NAMESPACE_TOOL( LVL1BS, CpByteStreamTool )
  DECLARE_NAMESPACE_TOOL( LVL1BS, CpmRoiByteStreamTool )
  DECLARE_NAMESPACE_TOOL( LVL1BS, JepByteStreamTool )
  DECLARE_NAMESPACE_TOOL( LVL1BS, JepRoiByteStreamTool )
  DECLARE_NAMESPACE_TOOL( LVL1BS, PpmByteStreamTool )
  DECLARE_NAMESPACE_TOOL( LVL1BS, RodHeaderByteStreamTool )

  DECLARE_NAMESPACE_TOOL( LVL1BS, PpmByteStreamSubsetTool )
  DECLARE_NAMESPACE_TOOL( LVL1BS, TriggerTowerSelectionTool )
  DECLARE_NAMESPACE_TOOL( LVL1BS, TrigT1CaloDataAccess )
}
