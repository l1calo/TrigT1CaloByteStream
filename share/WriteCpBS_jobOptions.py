# CP container to bytestream conversion
theApp.Dlls += [ "TrigT1CaloByteStream" ]
StreamBS = Algorithm( "StreamBS" )
StreamBS.ItemList  += [ "1272124447#*" ]
