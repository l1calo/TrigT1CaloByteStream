# Bytestream to RODHeader conversion
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__RodHeaderByteStreamTool
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc += LVL1BS__RodHeaderByteStreamTool("RodHeaderByteStreamTool")
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::RODHeader>" ]
ByteStreamAddressProviderSvc = Service( "ByteStreamAddressProviderSvc" )
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::RODHeader>/RODHeaders" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::RODHeader>/RODHeadersPP" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::RODHeader>/RODHeadersCP" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::RODHeader>/RODHeadersCPRoI" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::RODHeader>/RODHeadersJEP" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::RODHeader>/RODHeadersJEPRoI" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::RODHeader>/RODHeadersCPRoIB" ]
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::RODHeader>/RODHeadersJEPRoIB" ]
