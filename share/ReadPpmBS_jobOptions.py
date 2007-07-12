# PPM bytestream to TriggerTower conversion
from TrigT1CaloByteStream.TrigT1CaloByteStreamConf import LVL1BS__PpmByteStreamTool
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc += LVL1BS__PpmByteStreamTool("PpmByteStreamTool")
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::TriggerTower>" ]
ByteStreamAddressProviderSvc = Service( "ByteStreamAddressProviderSvc" )
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::TriggerTower>/TriggerTowers" ]
