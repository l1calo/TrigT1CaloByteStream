#ifndef TRIGT1CALOBYTESTREAM_PPMCRATEMAPPINGS_H
#define TRIGT1CALOBYTESTREAM_PPMCRATEMAPPINGS_H

#include <map>
#include <utility>

#include "DataModel/DataVector.h"

#include "TrigT1CaloByteStream/ChannelCoordinate.h"

/** PPM crate/module/channel to eta/phi/layer mappings
 *
 *  The inputs and crate layouts come from "Level-1 Calorimeter Trigger:
 *  Cable Mappings and Crate Layouts from Analogue Inputs to Processors"
 *  version 1.6.
 *  The input-output mapping comes from Steve Hillier's online decoder.
 *
 *  @author Peter Faulkner
 */

class PpmCrateMappings {

 public:
   PpmCrateMappings();

   /// Return channel to eta, phi and layer mappings for given crate/module
   void mappings(int crate, int module,
                 std::map<int, ChannelCoordinate>& etaPhiMap) const;

   int crates() const;
   int modules() const;
   int channels() const;

 private:
   //  Crate/module map constituents
   typedef std::pair< double, double >                Offsets;
   typedef std::map< int, ChannelCoordinate >         CoordinateMap;
   typedef std::pair< Offsets, const CoordinateMap* > ModuleInfo;
   typedef std::map< int, ModuleInfo >                ModuleMap;
   typedef std::map< int, ModuleMap >                 CrateMap;

   /// Add entries to a coordinate map
   void addCoords(int nrows, int ncols, double etaGran, double phiGran,
        double etaOffset, double phiOffset,
	const int* in, const int* out, int incr,
	ChannelCoordinate::CaloLayer layer, CoordinateMap* coordMap);
   /// Add a block of similar modules to a crate
   void addMods(int crate, int modOffset, int nrows, int ncols,
        double etaBase, double phiBase, double etaRange, double phiRange,
	const CoordinateMap* coordMap);
   /// Initialise the mappings
   void init();

   /// Crate/module map
   CrateMap m_crateInfo;
   /// Vector of CoordinateMaps
   DataVector<CoordinateMap> m_coordMaps;

   static const int s_crates   = 8;
   static const int s_modules  = 16;
   static const int s_channels = 64;

};

inline int PpmCrateMappings::crates() const
{
  return s_crates;
}

inline int PpmCrateMappings::modules() const
{
  return s_modules;
}

inline int PpmCrateMappings::channels() const
{
  return s_channels;
}

#endif
