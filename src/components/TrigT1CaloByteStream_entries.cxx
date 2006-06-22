
#include "DataModel/DataVector.h"
#include "GaudiKernel/DeclareFactoryEntries.h"

#include "TrigT1Calo/JetElement.h"

#include "TrigT1CaloByteStream/JemTester.h"
#include "TrigT1CaloByteStream/JepContainerMaker.h"
#include "TrigT1CaloByteStream/PpmTester.h"

#include "TrigT1CaloByteStream/JepByteStreamCnv.h"
#include "TrigT1CaloByteStream/JepReadByteStreamCnv.h"
#include "TrigT1CaloByteStream/PpmByteStreamCnv.h"

typedef DataVector<LVL1::JetElement> JetElementCollection;
typedef JepReadByteStreamCnv<JetElementCollection> JepReadByteStreamCnvT;

// declare 
DECLARE_CONVERTER_FACTORY( PpmByteStreamCnv )
DECLARE_CONVERTER_FACTORY( JepByteStreamCnv )
DECLARE_CONVERTER_FACTORY( JepReadByteStreamCnvT )
DECLARE_ALGORITHM_FACTORY( PpmTester )
DECLARE_ALGORITHM_FACTORY( JemTester )
DECLARE_ALGORITHM_FACTORY( JepContainerMaker )

DECLARE_FACTORY_ENTRIES( TrigT1CaloByteStream )
{
  DECLARE_CONVERTER( PpmByteStreamCnv )
  DECLARE_CONVERTER( JepByteStreamCnv )
  DECLARE_CONVERTER( JepReadByteStreamCnvT )
  DECLARE_ALGORITHM( PpmTester )
  DECLARE_ALGORITHM( JemTester )
  DECLARE_ALGORITHM( JepContainerMaker )
}
