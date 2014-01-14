#include <BWAPI.h>
#include <fstream>
#include <map>

using namespace BWAPI;

// these are unit types, they include units, buildings workers and drops
/*
const std::string protoss_units[] = {"Protoss Observer", "Protoss Dragoon", "Protoss Zealot", "Protoss Archon", "Protoss Reaver", "Protoss High Templar", 
	"Protoss Arbiter", "Protoss Carrier", "Protoss Scout", "Protoss Dark Archon", "Protoss Corsair", "Protoss Dark Templar","Protoss Shuttle","Protoss Probe",
	"Protoss Nexus", "Protoss Robotics Facility", "Protoss Pylon", "Protoss Assimilator", "Protoss Observatory", "Protoss Gateway", "Protoss Photon Cannon", 
	"Protoss Citadel of Adun", "Protoss Cybernetics Core", "Protoss Templar Archives","Protoss Forge", "Protoss Stargate", "Protoss Fleet Beacon", 
	"Protoss Arbiter Tribunal", "Protoss Robotics Support Bay", "Protoss Shield Battery"};
const int protoss_units_length = 30; // the number of different units

const std::string terran_units[] = {"Terran Marine", "Terran Ghost", "Terran Vulture", "Terran Vulture Spider Mine", "Terran Goliath", "Terran Siege Tank Tank Mode", 
	"Terran Wraith", "Terran Science Vessel", "Terran Battlecruiser", "Terran Siege Tank Siege Mode", "Terran Firebat", "Terran Medic", "Terran Valkyrie",
	"Terran Dropship", "Terran SCV", "Terran Command Center", "Terran Comsat Station", "Terran Nuclear Silo", "Terran Supply Depot", "Terran Refinery", "Terran Barracks", 
	"Terran Academy", "Terran Factory", "Terran Starport", "Terran Control Tower", "Terran Science Facility", "Terran Covert Ops", "Terran Physics Lab", "Terran Machine Shop", 
	"Terran Engineering Bay", "Terran Armory", "Terran Missile Turret", "Terran Bunker"};
const int terran_units_length = 33;

const std::string zerg_units[] = {"Zerg Overlord","Zerg Drone","Zerg Zergling", "Zerg Devourer", "Zerg Guardian", "Zerg Ultralisk", "Zerg Queen", "Zerg Hydralisk", "Zerg Mutalisk", 
	"Zerg Scourge", "Zerg Lurker", "Zerg Defiler","Zerg Infested Command Center", "Zerg Hatchery", "Zerg Lair", "Zerg Hive", "Zerg Nydus Canal", "Zerg Hydralisk Den", 
	"Zerg Defiler Mound", "Zerg Greater Spire", "Zerg Queens Nest", "Zerg Evolution Chamber", "Zerg Ultralisk Cavern", "Zerg Spire", "Zerg Spawning Pool", "Zerg Creep Colony",
	"Zerg Spore Colony", "Zerg Sunken Colony", "Zerg Extractor"};
const int zerg_units_length = 29;
*/

const UnitType protoss_units[] = {UnitTypes::Protoss_Arbiter,UnitTypes::Protoss_Arbiter_Tribunal,UnitTypes::Protoss_Archon,UnitTypes::Protoss_Assimilator,UnitTypes::Protoss_Carrier,
	UnitTypes::Protoss_Citadel_of_Adun,UnitTypes::Protoss_Corsair,UnitTypes::Protoss_Cybernetics_Core,UnitTypes::Protoss_Dark_Archon,UnitTypes::Protoss_Dark_Templar,
	UnitTypes::Protoss_Dragoon,UnitTypes::Protoss_Fleet_Beacon,UnitTypes::Protoss_Forge,UnitTypes::Protoss_Gateway,UnitTypes::Protoss_High_Templar,
	UnitTypes::Protoss_Interceptor,UnitTypes::Protoss_Nexus,UnitTypes::Protoss_Observatory,UnitTypes::Protoss_Observer,UnitTypes::Protoss_Photon_Cannon,
	UnitTypes::Protoss_Probe,UnitTypes::Protoss_Pylon,UnitTypes::Protoss_Reaver,UnitTypes::Protoss_Robotics_Facility,UnitTypes::Protoss_Robotics_Support_Bay,
	UnitTypes::Protoss_Scarab,UnitTypes::Protoss_Scout,UnitTypes::Protoss_Shield_Battery,UnitTypes::Protoss_Shuttle,UnitTypes::Protoss_Stargate,
	UnitTypes::Protoss_Templar_Archives,UnitTypes::Protoss_Zealot};
const int protoss_units_length = 32;

const UnitType terran_units[] = {UnitTypes::Terran_Academy,UnitTypes::Terran_Armory,UnitTypes::Terran_Barracks,UnitTypes::Terran_Battlecruiser,UnitTypes::Terran_Bunker,
	UnitTypes::Terran_Civilian,UnitTypes::Terran_Command_Center,UnitTypes::Terran_Comsat_Station,UnitTypes::Terran_Control_Tower,UnitTypes::Terran_Covert_Ops,
	UnitTypes::Terran_Dropship,UnitTypes::Terran_Engineering_Bay,UnitTypes::Terran_Factory,UnitTypes::Terran_Firebat,UnitTypes::Terran_Ghost,UnitTypes::Terran_Goliath,
	UnitTypes::Terran_Machine_Shop,UnitTypes::Terran_Marine,UnitTypes::Terran_Medic,UnitTypes::Terran_Missile_Turret,UnitTypes::Terran_Nuclear_Missile,
	UnitTypes::Terran_Nuclear_Silo,UnitTypes::Terran_Physics_Lab,UnitTypes::Terran_Refinery,UnitTypes::Terran_Science_Facility,UnitTypes::Terran_Science_Vessel,
	UnitTypes::Terran_SCV,UnitTypes::Terran_Siege_Tank_Siege_Mode,UnitTypes::Terran_Siege_Tank_Tank_Mode,UnitTypes::Terran_Starport,UnitTypes::Terran_Supply_Depot,
	UnitTypes::Terran_Valkyrie,UnitTypes::Terran_Vulture,UnitTypes::Terran_Vulture_Spider_Mine,UnitTypes::Terran_Wraith};
const int terran_units_length = 35;

const UnitType zerg_units[] = {UnitTypes::Zerg_Broodling,UnitTypes::Zerg_Cocoon,UnitTypes::Zerg_Creep_Colony,UnitTypes::Zerg_Defiler,UnitTypes::Zerg_Defiler_Mound,
	UnitTypes::Zerg_Devourer,UnitTypes::Zerg_Drone,UnitTypes::Zerg_Egg,UnitTypes::Zerg_Evolution_Chamber,UnitTypes::Zerg_Extractor,UnitTypes::Zerg_Greater_Spire,
	UnitTypes::Zerg_Guardian,UnitTypes::Zerg_Hatchery,UnitTypes::Zerg_Hive,UnitTypes::Zerg_Hydralisk,UnitTypes::Zerg_Hydralisk_Den,UnitTypes::Zerg_Infested_Command_Center,
	UnitTypes::Zerg_Infested_Terran,UnitTypes::Zerg_Lair,UnitTypes::Zerg_Larva,UnitTypes::Zerg_Lurker,UnitTypes::Zerg_Lurker_Egg,UnitTypes::Zerg_Mutalisk,
	UnitTypes::Zerg_Nydus_Canal,UnitTypes::Zerg_Overlord,UnitTypes::Zerg_Queen,UnitTypes::Zerg_Queens_Nest,UnitTypes::Zerg_Scourge,UnitTypes::Zerg_Spawning_Pool,
	UnitTypes::Zerg_Spire,UnitTypes::Zerg_Spore_Colony,UnitTypes::Zerg_Sunken_Colony,UnitTypes::Zerg_Ultralisk,UnitTypes::Zerg_Ultralisk_Cavern,UnitTypes::Zerg_Zergling};
const int zerg_units_length = 35;


class SCFeatureExtractor : public BWAPI::AIModule
{
public:
  virtual void onStart();
  virtual void onFrame();
  virtual void onEnd();

  // logic to actually dump the vectors to file
  void doDump();

  // the file handle we will be writing to
  std::ofstream writeData;

  // maps from player ids (ints) to maps of unit_types to counts, one for each player,
  // I am using maps for the player ids instead of arrays in case player ids are not
  // exhaustive of [0,number of players]
  std::map<int,std::map<std::string,int>> unit_count_maps;

  // these are to keep track of average unspent minerals/gas

  // maps from player ids (ints) to a minerals count (total)
  std::map<int,int> avg_unspent_mineral_total;

  // maps from player ids (ints) to a gas count (total)
  std::map<int,int> avg_unspent_gas_total;

  // this is a count of how many times the current mineral/gas count has been added,
  // used to find the avg 
  int avg_unspent_count;

};