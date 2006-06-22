# TriggerTower to PPM bytestream conversion
theApp.Dlls += [ "TrigT1CaloByteStream" ]
theApp.TopAlg += [ "PpmTester" ]
Tester = Algorithm( "PpmTester" )
Tester.TriggerTowerLocation ="LVL1TriggerTowers"

ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc.PpmByteStreamTool.DataVersion = 1
ByteStreamCnvSvc.PpmByteStreamTool.DataFormat = 1
ByteStreamCnvSvc.PpmByteStreamTool.SlinksPerCrate = 4
StreamBS = Algorithm( "StreamBS" )
StreamBS.ItemList  += [ "6207#*" ]
