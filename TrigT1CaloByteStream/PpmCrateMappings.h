#ifndef TRIGT1CALOBYTESTREAM_PPMCRATEMAPPINGS_H
#define TRIGT1CALOBYTESTREAM_PPMCRATEMAPPINGS_H

#include <map>
#include <utility>
#include <vector>

#include "TrigT1CaloByteStream/ChannelCoordinate.h"

namespace LVL1BS {

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
   ~PpmCrateMappings();

   /// Return eta, phi and layer mapping for given crate/module/channel
   bool mapping(int crate, int module, int channel,
                                       ChannelCoordinate& coord) const;
   static int crates();
   static int modules();
   static int channels();

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
   std::vector<CoordinateMap*> m_coordMaps;
   /// Current Coordinate map
   mutable const CoordinateMap* m_currentMap;
   /// Current module eta offset
   mutable double m_etaOffset;
   /// Current module phi offset
   mutable double m_phiOffset;
   /// Current crate
   mutable int m_currentCrate;
   /// Current module
   mutable int m_currentModule;

   static const int s_crates   = 8;
   static const int s_modules  = 16;
   static const int s_channels = 64;

};

} // end namespace

#endif
