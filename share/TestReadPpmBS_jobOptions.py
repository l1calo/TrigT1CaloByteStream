# Test PPM bytestream to TriggerTower conversion
# Specify the Converters
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc.InitCnvs += [ "DataVector<LVL1::TriggerTower>" ]
ByteStreamAddressProviderSvc = Service( "ByteStreamAddressProviderSvc" )
ByteStreamAddressProviderSvc.TypeNames += [ "DataVector<LVL1::TriggerTower>/LVL1TriggerTowers" ]
# DLLs
theApp.Dlls += [ "TrigT1CaloByteStream" ]
theApp.Dlls += [ "TrigT1Calo" ]
# Algs
theApp.TopAlg += [ "LVL1::Tester" ]
Tester = Algorithm( "LVL1::Tester" )
Tester.TriggerTowerLocation ="LVL1TriggerTowers"
Tester.Mode = 1
