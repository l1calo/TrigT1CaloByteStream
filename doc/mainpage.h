/**
@mainpage TrigT1CaloByteStream Package

@section intro Introduction
This package contains offline bytestream conversion code for the
Level-1 Calorimeter Trigger sub-detectors.<p>

The code is based on the following documents/sources:<p>

ATLAS Level-1 Calorimeter Trigger-Read-out Driver, version 1.06d<br>
Level-1 Calorimeter Trigger: Cable Mappings and Crate Layouts from Analogue
Inputs to Processors, version 1.6<br>
Steve Hillier's online decoder, mainly L1CaloBsDecoderTemplate.h as of
November 2005.<br>
ATLAS L1Calo Pre-processor compressed Slink data formats, D.P.C.Sankey,
Version 1.1, October 17, 2006.<br>
ATLAS Level-1 Calorimeter Trigger: Jet/Energy Processor Module,
Project Specification, version 1.0<br>
Private communications from Steve Hillier.<br>
I could find no helpful documentation on the details of the bytestream
converter so I followed existing implementations, notably
TrigT1ResultByteStream and LArByteStream.<p>

Implemented so far: PPM DAQ neutral, uncompressed and compressed formats.
                    JEM DAQ jet elements, jet hits and Et sums.

@author Peter Faulkner

@ref used_TrigT1CaloByteStream
@ref requirements_TrigT1CaloByteStream

*/

/**
@page used_TrigT1CaloByteStream Used Packages
@htmlinclude used_packages.html
*/

/**
@page requirements_TrigT1CaloByteStream Requirements
@include requirements
*/


