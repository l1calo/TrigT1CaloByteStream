# Test JEP RoI bytestream to JEMRoI/... conversion
include ( "TrigT1CaloByteStream/ReadJepRoiBS_jobOptions.py" )
theApp.TopAlg += [ "JemTester" ]
