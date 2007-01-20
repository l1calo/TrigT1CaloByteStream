
#include "DataModel/DataVector.h"
#include "GaudiKernel/DeclareFactoryEntries.h"

#include "TrigT1Calo/JetElement.h"
#include "TrigT1Calo/JEMHits.h"
#include "TrigT1Calo/JEMEtSums.h"

#include "TrigT1CaloByteStream/JemTester.h"
#include "TrigT1CaloByteStream/JepContainerMaker.h"
#include "TrigT1CaloByteStream/PpmTester.h"

#include "TrigT1CaloByteStream/JepByteStreamCnv.h"
#include "TrigT1CaloByteStream/JepReadByteStreamCnv.h"
#include "TrigT1CaloByteStream/PpmByteStreamCnv.h"

#include "TrigT1CaloByteStream/JepByteStreamTool.h"
#include "TrigT1CaloByteStream/PpmByteStreamTool.h"

typedef DataVector<LVL1::JetElement> JetElementCollection;
typedef DataVector<LVL1::JEMHits>    JetHitsCollection;
typedef DataVector<LVL1::JEMEtSums>  EnergySumsCollection;
typedef JepReadByteStreamCnv<JetElementCollection> JepReadJEByteStreamCnvT;
typedef JepReadByteStreamCnv<JetHitsCollection>    JepReadJHByteStreamCnvT;
typedef JepReadByteStreamCnv<EnergySumsCollection> JepReadESByteStreamCnvT;

// declare 
DECLARE_CONVERTER_FACTORY( PpmByteStreamCnv )
DECLARE_CONVERTER_FACTORY( JepByteStreamCnv )
DECLARE_CONVERTER_FACTORY( JepReadJEByteStreamCnvT )
DECLARE_CONVERTER_FACTORY( JepReadJHByteStreamCnvT )
DECLARE_CONVERTER_FACTORY( JepReadESByteStreamCnvT )
DECLARE_ALGORITHM_FACTORY( PpmTester )
DECLARE_ALGORITHM_FACTORY( JemTester )
DECLARE_ALGORITHM_FACTORY( JepContainerMaker )

DECLARE_TOOL_FACTORY( JepByteStreamTool )
DECLARE_TOOL_FACTORY( PpmByteStreamTool )

DECLARE_FACTORY_ENTRIES( TrigT1CaloByteStream )
{
  DECLARE_CONVERTER( PpmByteStreamCnv )
  DECLARE_CONVERTER( JepByteStreamCnv )
  DECLARE_CONVERTER( JepReadJEByteStreamCnvT )
  DECLARE_CONVERTER( JepReadJHByteStreamCnvT )
  DECLARE_CONVERTER( JepReadESByteStreamCnvT )
  DECLARE_ALGORITHM( PpmTester )
  DECLARE_ALGORITHM( JemTester )
  DECLARE_ALGORITHM( JepContainerMaker )
}
