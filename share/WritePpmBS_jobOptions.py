# TriggerTower to PPM bytestream conversion
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc.PpmByteStreamTool.DataVersion = 1
ByteStreamCnvSvc.PpmByteStreamTool.DataFormat = 1
ByteStreamCnvSvc.PpmByteStreamTool.ChannelsPerSubBlock = 16
ByteStreamCnvSvc.PpmByteStreamTool.SlinksPerCrate = 4
theApp.Dlls += [ "TrigT1CaloByteStream" ]
StreamBS = Algorithm( "StreamBS" )
StreamBS.ItemList  += [ "6207#*" ]
