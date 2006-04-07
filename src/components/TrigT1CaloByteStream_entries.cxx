
#include "GaudiKernel/DeclareFactoryEntries.h"

#include "TrigT1CaloByteStream/PpmByteStreamCnv.h"

// declare 
DECLARE_CONVERTER_FACTORY( PpmByteStreamCnv )

DECLARE_FACTORY_ENTRIES( TrigT1CaloByteStream)
{
  DECLARE_CONVERTER( PpmByteStreamCnv )
}
