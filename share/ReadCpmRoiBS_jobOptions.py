# CP RoI bytestream to CPMRoI conversion
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__CpmRoiByteStreamTool
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc += LVL1BS__CpmRoiByteStreamTool("CpmRoiByteStreamTool")
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::CPMRoI>" ]
ByteStreamAddressProviderSvc = Service( "ByteStreamAddressProviderSvc" )
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::CPMRoI>/CPMRoIs" ]
