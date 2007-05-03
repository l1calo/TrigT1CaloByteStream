# Test CPMRoI Container to bytestream conversion
include ( "TrigT1CaloByteStream/WriteCpmRoiBS_jobOptions.py" )
theApp.TopAlg += [ "LVL1BS::CpmTester/CpmTester" ]
