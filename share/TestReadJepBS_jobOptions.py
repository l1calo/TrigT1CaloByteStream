# Test JEP bytestream to JetElement/... conversion
include ( "TrigT1CaloByteStream/ReadJepBS_jobOptions.py" )
theApp.TopAlg += [ "LVL1BS::JemTester/JemTester" ]
JemTester = Algorithm( "JemTester" )
JemTester.JetElementLocation = "LVL1JetElements"
