# Test CPMRoI Container to bytestream conversion
include ( "TrigT1CaloByteStream/WriteCpmRoiBS_jobOptions.py" )
theApp.TopAlg += [ "CpmTester" ]
