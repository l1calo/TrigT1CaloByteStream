# PPM bytestream to TriggerTower conversion
# Specify the Converters
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::TriggerTower>" ]
ByteStreamCnvSvc.PpmByteStreamTool.PrintCompStats = 0
ByteStreamAddressProviderSvc = Service( "ByteStreamAddressProviderSvc" )
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::TriggerTower>/LVL1TriggerTowers" ]
# DLLs
theApp.Dlls += [ "TrigT1CaloByteStream" ]
