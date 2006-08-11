# Test TriggerTower to PPM bytestream conversion
theApp.Dlls += [ "TrigT1CaloByteStream" ]

theApp.TopAlg += [ "PpmTester" ]
Tester = Algorithm( "PpmTester" )
Tester.TriggerTowerLocation ="LVL1TriggerTowers"

ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc.PpmByteStreamTool.DataVersion = 1
ByteStreamCnvSvc.PpmByteStreamTool.DataFormat  = 1
ByteStreamCnvSvc.PpmByteStreamTool.SlinksPerCrate = 4
ByteStreamCnvSvc.PpmByteStreamTool.CompressionVersion = 1
ByteStreamCnvSvc.PpmByteStreamTool.PrintCompStats = 0
ByteStreamCnvSvc.PpmByteStreamTool.DefaultSlicesLUT  = 1
ByteStreamCnvSvc.PpmByteStreamTool.DefaultSlicesFADC = 7
ByteStreamCnvSvc.PpmByteStreamTool.ForceSlicesFADC   = 0
StreamBS = Algorithm( "StreamBS" )
StreamBS.ItemList  += [ "6207#*" ]
