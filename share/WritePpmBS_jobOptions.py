# TriggerTower to PPM bytestream conversion
ByteStreamCnvSvc = Service( "ByteStreamCnvSvc" )
ByteStreamCnvSvc.PpmByteStreamTool.DataVersion = 1
ByteStreamCnvSvc.PpmByteStreamTool.DataFormat  = 1
ByteStreamCnvSvc.PpmByteStreamTool.SlinksPerCrate = 4
ByteStreamCnvSvc.PpmByteStreamTool.CompressionVersion = 1
ByteStreamCnvSvc.PpmByteStreamTool.PrintCompStats = 0
ByteStreamCnvSvc.PpmByteStreamTool.DefaultSlicesLUT  = 1
ByteStreamCnvSvc.PpmByteStreamTool.DefaultSlicesFADC = 7
ByteStreamCnvSvc.PpmByteStreamTool.ForceSlicesFADC   = 0

theApp.Dlls += [ "TrigT1CaloByteStream" ]
StreamBS = Algorithm( "StreamBS" )
StreamBS.ItemList  += [ "6207#*" ]
