# Test CP Container to bytestream conversion
include ( "TrigT1CaloByteStream/WriteCpBS_jobOptions.py" )
theApp.TopAlg += [ "CpmTester" ]
