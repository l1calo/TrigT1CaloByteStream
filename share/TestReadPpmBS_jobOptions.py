# Test PPM bytestream to TriggerTower conversion
# Specify the Converters
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::TriggerTower>" ]
ByteStreamAddressProviderSvc = Service( "ByteStreamAddressProviderSvc" )
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::TriggerTower>/LVL1TriggerTowers" ]
# DLLs
theApp.Dlls += [ "TrigT1CaloByteStream" ]
# Algs
theApp.TopAlg += [ "PpmTester" ]
Tester = Algorithm( "PpmTester" )
Tester.TriggerTowerLocation ="LVL1TriggerTowers"
