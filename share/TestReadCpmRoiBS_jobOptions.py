# Test CP RoI bytestream to CPMRoI conversion
include ( "TrigT1CaloByteStream/ReadCpmRoiBS_jobOptions.py" )
theApp.TopAlg += [ "LVL1BS::CpmTester/CpmTester" ]
