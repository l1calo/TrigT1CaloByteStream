#include <cmath>

#include "TrigT1CaloByteStream/PpmCrateMappings.h"

// Static constants

const int PpmCrateMappings::s_crates;
const int PpmCrateMappings::s_modules;
const int PpmCrateMappings::s_channels;

PpmCrateMappings::PpmCrateMappings() : m_currentMap(0)
{
  init();
}

PpmCrateMappings::~PpmCrateMappings()
{
  std::vector<CoordinateMap*>::iterator pos;
  for (pos = m_coordMaps.begin(); pos != m_coordMaps.end(); ++pos) {
    delete *pos;
  }
}

// Initialise the mappings

void PpmCrateMappings::init()
{
  // Input to Output channel mappings.
  // Inputs are numbered 1-16 (x4) and outputs 0-63
  // There are four output sets obtained by adding 0,4,8,12 to out

  //    input =    1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16
  int out[16] = { 19,51,35, 3,18,50,34, 2,17,49,33, 1,16,48,32, 0 };

  // Mapping types.
  //
  // Type 1.
  // Eta -2.4 to 2.4.  Inputs for 4x4 block
  //
  //   +----+----+----+----+
  //   | 16 | 15 | 12 | 11 |
  //   +----+----+----+----+
  //   | 13 | 14 |  9 | 10 |
  //   +----+----+----+----+ PI/8
  //   |  4 |  3 |  8 |  7 |
  //   +----+----+----+----+
  //   |  1 |  2 |  5 |  6 |
  //   +----+----+----+----+
  //            0.4

  int in1[16] = { 16,15,12,11,13,14, 9,10, 4, 3, 8, 7, 1, 2, 5, 6 };

  // Type 2.
  // Eta 2.4 to 2.9.  4x1 plus 2x2 block
  //
  //   +----+--------+--------+
  //   | 16 |        |        |
  //   +----+   15   |   12   |
  //   | 13 |        |        |
  //   +----+--------+--------+ PI/8
  //   |  4 |        |        |
  //   +----+    3   |    8   |
  //   |  1 |        |        |
  //   +----+--------+--------+
  //              0.5

  int in2a[4] = { 16,13, 4, 1 };
  int in2b[4] = { 15,12, 3, 8 };

  // Type 3.
  // Eta -2.9 to -2.4.  2x2 plus 4x1 block
  //
  //   +--------+--------+----+
  //   |        |        | 11 |
  //   |   16   |   12   +----+
  //   |        |        | 10 |
  //   +--------+--------+----+ PI/8
  //   |        |        |  7 |
  //   |    4   |    8   +----+
  //   |        |        |  6 |
  //   +--------+--------+----+
  //              0.5

  int in3a[4] = { 16,12, 4, 8 };
  int in3b[4] = { 11,10, 7, 6 };

  // Type 4.
  // Eta 2.9 to 3.2 and -3.2 to -2.9.  4x1 plus 4x1
  //          +                 -
  //   +--------+----+   +----+--------+
  //   |        |    |   |    |        |
  //   |   16   | 15 |   | 16 |   15   |
  //   |        |    |   |    |        |
  //   +--------+----+   +----+--------+
  //   |        |    |   |    |        |
  //   |   12   | 11 |   | 12 |   11   |
  //   |        |    |   |    |        |
  //   +--------+----+   +----+--------+ PI/4
  //   |        |    |   |    |        |
  //   |    8   |  7 |   |  8 |    7   |
  //   |        |    |   |    |        |
  //   +--------+----+   +----+--------+
  //   |        |    |   |    |        |
  //   |    4   |  3 |   |  4 |    3   |
  //   |        |    |   |    |        |
  //   +--------+----+   +----+--------+
  //         0.3               0.3

  int in4a[4]  = { 16,12, 8, 4 };
  int in4b[4]  = { 15,11, 7, 3 };

  // Type 5.
  // Eta 3.2 to 4.9 and -4.9 to -3.2 EM FCAL.  4x4
  //
  //   +----+----+----+----+
  //   |    |    |    |    |
  //   | 16 | 15 | 14 | 13 |
  //   |    |    |    |    |
  //   +----+----+----+----+
  //   |    |    |    |    |
  //   | 12 | 11 | 10 |  9 |
  //   |    |    |    |    |
  //   +----+----+----+----+ PI/2
  //   |    |    |    |    |
  //   |  8 |  7 |  6 |  5 |
  //   |    |    |    |    |
  //   +----+----+----+----+
  //   |    |    |    |    |
  //   |  4 |  3 |  2 |  1 |
  //   |    |    |    |    |
  //   +----+----+----+----+
  //            1.7

  int in5[16]  = { 16,15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };

  // Type 6.
  // Eta 3.2 to 4.9 and -4.9 to -3.2 Had FCAL2 and FCAL3.  4x2 plus 4x2
  // NB. FCAL2 and FCAL3 have the same eta/phi coordinates.
  //          FCAL2                 FCAL3
  //   +--------+--------+   +--------+--------+
  //   |        |        |   |        |        |
  //   |   16   |   14   |   |   15   |   13   |
  //   |        |        |   |        |        |
  //   +--------+--------+   +--------+--------+
  //   |        |        |   |        |        |
  //   |   12   |   10   |   |   11   |    9   |
  //   |        |        |   |        |        |
  //   +--------+--------+   +--------+--------+ PI/2
  //   |        |        |   |        |        |
  //   |    8   |    6   |   |    7   |    5   |
  //   |        |        |   |        |        |
  //   +--------+--------+   +--------+--------+
  //   |        |        |   |        |        |
  //   |    4   |    2   |   |    3   |    1   |
  //   |        |        |   |        |        |
  //   +--------+--------+   +--------+--------+
  //           1.7                   1.7

  int in6a[8]  = { 16,14,12,10, 8, 6, 4, 2 };
  int in6b[8]  = { 15,13,11, 9, 7, 5, 3, 1 };
  
  // Construct coordinate maps for each module type
  // Four blocks of each type make up the complete map

  for (int i = 0; i < 12; ++i) m_coordMaps.push_back(new CoordinateMap);
  for (int block = 0; block < 4; ++block) {
    int incr = block * 4;
    std::vector<CoordinateMap*>::iterator pos = m_coordMaps.begin();

    // Map 0 : Type 1 EM
    addCoords(4,4,0.1,M_PI/32.,0.,block*M_PI/8.,in1,out,incr,
              ChannelCoordinate::EM, *pos);
    ++pos;

    // Map 1 : Type 1 Had
    addCoords(4,4,0.1,M_PI/32.,0.,block*M_PI/8.,in1,out,incr,
              ChannelCoordinate::HAD, *pos);
    ++pos;

    // Map 2 : Type 2 EM
    addCoords(4,1,0.1,M_PI/32.,0.0,block*M_PI/8.,in2a,out,incr,
              ChannelCoordinate::EM, *pos);
    addCoords(2,2,0.2,M_PI/16.,0.1,block*M_PI/8.,in2b,out,incr,
              ChannelCoordinate::EM, *pos);
    ++pos;

    // Map 3 : Type 2 Had
    addCoords(4,1,0.1,M_PI/32.,0.0,block*M_PI/8.,in2a,out,incr,
              ChannelCoordinate::HAD, *pos);
    addCoords(2,2,0.2,M_PI/16.,0.1,block*M_PI/8.,in2b,out,incr,
              ChannelCoordinate::HAD, *pos);
    ++pos;

    // Map 4 : Type 3 EM
    addCoords(2,2,0.2,M_PI/16.,0.0,block*M_PI/8.,in3a,out,incr,
              ChannelCoordinate::EM, *pos);
    addCoords(4,1,0.1,M_PI/32.,0.4,block*M_PI/8.,in3b,out,incr,
              ChannelCoordinate::EM, *pos);
    ++pos;

    // Map 5 : Type 3 Had
    addCoords(2,2,0.2,M_PI/16.,0.0,block*M_PI/8.,in3a,out,incr,
              ChannelCoordinate::HAD, *pos);
    addCoords(4,1,0.1,M_PI/32.,0.4,block*M_PI/8.,in3b,out,incr,
              ChannelCoordinate::HAD, *pos);
    ++pos;

    // Map 6 : Type 4 EM positive eta
    addCoords(4,1,0.2,M_PI/16.,0.0,block*M_PI/4.,in4a,out,incr,
              ChannelCoordinate::EM, *pos);
    addCoords(4,1,0.1,M_PI/16.,0.2,block*M_PI/4.,in4b,out,incr,
              ChannelCoordinate::EM, *pos);
    ++pos;

    // Map 7 : Type 4 Had positive eta
    addCoords(4,1,0.2,M_PI/16.,0.0,block*M_PI/4.,in4a,out,incr,
              ChannelCoordinate::HAD, *pos);
    addCoords(4,1,0.1,M_PI/16.,0.2,block*M_PI/4.,in4b,out,incr,
              ChannelCoordinate::HAD, *pos);
    ++pos;

    // Map 8 : Type 4 EM negative eta
    addCoords(4,1,0.1,M_PI/16.,0.0,block*M_PI/4.,in4a,out,incr,
              ChannelCoordinate::EM, *pos);
    addCoords(4,1,0.2,M_PI/16.,0.1,block*M_PI/4.,in4b,out,incr,
              ChannelCoordinate::EM, *pos);
    ++pos;

    // Map 9 : Type 4 Had negative eta
    addCoords(4,1,0.1,M_PI/16.,0.0,block*M_PI/4.,in4a,out,incr,
              ChannelCoordinate::HAD, *pos);
    addCoords(4,1,0.2,M_PI/16.,0.1,block*M_PI/4.,in4b,out,incr,
              ChannelCoordinate::HAD, *pos);
    ++pos;

    // Map 10 : Type 5 EM FCAL
    addCoords(4,4,0.425,M_PI/8.,0.0,block*M_PI/2.,in5,out,incr,
              ChannelCoordinate::EM, *pos);
    ++pos;

    // Map 11 : Type 6 Had FCAL2 and FCAL3
    addCoords(4,2,0.85,M_PI/8.,0.0,block*M_PI/2.,in6a,out,incr,
              ChannelCoordinate::FCAL2, *pos);
    addCoords(4,2,0.85,M_PI/8.,0.0,block*M_PI/2.,in6b,out,incr,
              ChannelCoordinate::FCAL3, *pos);
  }

  // Fill crate map

  std::vector<CoordinateMap*>::const_iterator pos = m_coordMaps.begin();

  // Map 0 : all of crates 0,1
  //         crate 2 modules 1,2,5,6,9,10,13,14
  //         crate 3 modules 2,3,6,7,10,11,14,15
  addMods(0,0,4,4, 0.0,0.0,0.4,M_PI/2.,*pos);
  addMods(1,0,4,4,-1.6,0.0,0.4,M_PI/2.,*pos);
  addMods(2,1,4,2, 1.6,0.0,0.4,M_PI/2.,*pos);
  addMods(3,2,4,2,-2.4,0.0,0.4,M_PI/2.,*pos);
  ++pos;

  // Map 1 : crate 4 modules 1,2,5,6,9,10,13,14
  //         crate 5 modules 2,3,6,7,10,11,14,15
  //         all of crates 6,7
  addMods(4,1,4,2, 1.6,0.0,0.4,M_PI/2.,*pos);
  addMods(5,2,4,2,-2.4,0.0,0.4,M_PI/2.,*pos);
  addMods(6,0,4,4, 0.0,0.0,0.4,M_PI/2.,*pos);
  addMods(7,0,4,4,-1.6,0.0,0.4,M_PI/2.,*pos);
  ++pos;

  // Map 2 : crate 2 modules 3,7,11,15
  addMods(2,3,4,1, 2.4,0.0,0.5,M_PI/2.,*pos);
  ++pos;

  // Map 3 : crate 4 modules 3,7,11,15
  addMods(4,3,4,1, 2.4,0.0,0.5,M_PI/2.,*pos);
  ++pos;

  // Map 4 : crate 3 modules 1,5,9,13
  addMods(3,1,4,1,-2.9,0.0,0.5,M_PI/2.,*pos);
  ++pos;

  // Map 5 : crate 5 modules 1,5,9,13
  addMods(5,1,4,1,-2.9,0.0,0.5,M_PI/2.,*pos);
  ++pos;

  // Map 6 : crate 2 modules 4,12
  addMods(2, 4,1,1, 2.9,0.0,0.3,M_PI,*pos);
  addMods(2,12,1,1, 2.9,M_PI,0.3,M_PI,*pos);
  ++pos;

  // Map 7 : crate 4 modules 4,12
  addMods(4, 4,1,1, 2.9,0.0,0.3,M_PI,*pos);
  addMods(4,12,1,1, 2.9,M_PI,0.3,M_PI,*pos);
  ++pos;

  // Map 8 : crate 3 modules 4,12
  addMods(3, 4,1,1,-3.2,0.0,0.3,M_PI,*pos);
  addMods(3,12,1,1,-3.2,M_PI,0.3,M_PI,*pos);
  ++pos;

  // Map 9 : crate 5 modules 4,12
  addMods(5, 4,1,1,-3.2,0.0,0.3,M_PI,*pos);
  addMods(5,12,1,1,-3.2,M_PI,0.3,M_PI,*pos);
  ++pos;

  // Map 10 : crate 4 module 0, crate 5 module 0
  addMods(4,0,1,1, 3.2,0.0,1.7,2*M_PI,*pos);
  addMods(5,0,1,1,-4.9,0.0,1.7,2*M_PI,*pos);
  ++pos;

  // Map 11 : crate 4 module 8, crate 5 module 8
  addMods(4,8,1,1, 3.2,0.0,1.7,2*M_PI,*pos);
  addMods(5,8,1,1,-4.9,0.0,1.7,2*M_PI,*pos);
}

// Add entries to a coordinate map

void PpmCrateMappings::addCoords(int nrows, int ncols,
     double etaGran, double phiGran, double etaOffset, double phiOffset,
     const int* in, const int* out, int incr,
     ChannelCoordinate::CaloLayer layer, CoordinateMap* coordMap)
{
  for (int row = 0; row < nrows; ++row) {
    double phi = (double(row) + 0.5) * phiGran + phiOffset;
    for (int col = 0; col < ncols; ++col) {
      double eta = (double(col) + 0.5) * etaGran + etaOffset;
      int channel = out[in[row*ncols+col]-1] + incr;
      coordMap->insert(std::make_pair(channel, 
                       ChannelCoordinate(layer, eta, phi, etaGran, phiGran)));
    }
  }
}

// Add a block of similar modules to a crate

void PpmCrateMappings::addMods(int crate, int modOffset, int nrows, int ncols,
     double etaBase, double phiBase, double etaRange, double phiRange,
     const CoordinateMap* coordMap)
{
  for (int row = 0; row < nrows; ++row) {
    for (int col = 0; col < ncols; ++col) {
      int module = row*4 + col + modOffset;
      double etaOffset = etaRange * double(col) + etaBase;
      double phiOffset = phiRange * double(row) + phiBase;
      Offsets off(etaOffset, phiOffset);
      ModuleInfo modInfo(off, coordMap);
      CrateMap::iterator cpos = m_crateInfo.find(crate);
      if (cpos != m_crateInfo.end()) {
        ModuleMap& modMap(cpos->second);
        modMap.insert(std::make_pair(module, modInfo));
      } else {
        ModuleMap modMap;
        modMap.insert(std::make_pair(module, modInfo));
        m_crateInfo.insert(std::make_pair(crate, modMap));
      }
    }
  }
}
 
// Return eta, phi and layer mapping for given crate/module/channel

bool PpmCrateMappings::mapping(int crate, int module, int channel,
                                          ChannelCoordinate& coord) const
{
  if (crate < 0 || crate >= s_crates || module < 0 || module >= s_modules ||
      channel < 0 || channel >= s_channels) return false;

  if (!m_currentMap || crate != m_currentCrate || module != m_currentModule) {

    // Find the relevant mapping

    CrateMap::const_iterator cpos = m_crateInfo.find(crate);
    if (cpos == m_crateInfo.end()) return false;
    const ModuleMap& modMap(cpos->second);
    ModuleMap::const_iterator mpos = modMap.find(module);
    if (mpos == modMap.end()) return false;
    ModuleInfo modInfo(mpos->second);
    Offsets etaPhiOff = modInfo.first;
    m_currentMap    = modInfo.second;
    m_etaOffset     = etaPhiOff.first;
    m_phiOffset     = etaPhiOff.second;
    m_currentCrate  = crate;
    m_currentModule = module;
  }

  // Set the output

  CoordinateMap::const_iterator pos = m_currentMap->find(channel);
  if (pos == m_currentMap->end()) return false;
  const ChannelCoordinate& relCoord(pos->second);
  coord.setLayer(relCoord.layer());
  coord.setEta(relCoord.eta() + m_etaOffset);
  coord.setPhi(relCoord.phi() + m_phiOffset);
  coord.setEtaGranularity(relCoord.etaGranularity());
  coord.setPhiGranularity(relCoord.phiGranularity());
  return true;
}

int PpmCrateMappings::crates()
{
  return s_crates;
}

int PpmCrateMappings::modules()
{
  return s_modules;
}

int PpmCrateMappings::channels()
{
  return s_channels;
}
