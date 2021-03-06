#include <BWAPI.h>
#include <fstream>
#include <map>
#include <vector>
#include <boost\tuple\tuple.hpp>

using namespace BWAPI;

// uncomment to include extra information in the battle info file, for debugging
//#define DEBUG_BATTLES_OUTPUT

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

// this is how many tiles are in one 'super tile'
const int super_tile_size=2;

// do we ignore battles where no units are lost or not
const bool ignore_no_death_battles=true;

// so we can't always just use unit pointers because some values (hp, location etc) are updated
// by starcraft, so we use battleUnit as a way to log info at a particular time (namely, when a unit enters the battle)
struct battleUnit
{
	int hp;
	int shields;
	int x;
	int y;

};

// okay, now we are concerned with extracting "battles" from the replays
struct battle
{
	// map player id to lists of units,timestamps, tells when each unit entered the battle
	//std::map<int,std::list<std::pair<BWAPI::Unit*,int>>> enteredUnits;
	std::map<int,std::map<Unit*,int>> enteredUnits;

	// when a unit dies the unittype of the unit object gets switched to unknown
	// so let's keep track of what unit-ids had what type before they died
	std::map<int, UnitType> trackedUnitTypes;

	// framecount of when we started tracking this battle
	int startTime;

	// framecount of when battle was last updated
	int curTime;

	// current middle of battle
	Position battleCenter;

	// current battle radius
	int radius;

	// for identifying which battle is which, mostly for debugging purposes
	int id;

	// for quick computing, from player ids to unit count
	//std::map<int, int> numAliveMilitaryUnits;
	
	// for storing the unit records, maps unit ids to battleUnits
	std::map<int,battleUnit> unitRecords;
	

	// the unit whose death started the battle
	Unit* startUnit;
};


// constants that have to do with battles

// time in frame counts that we allow 
const int TIME_LIMIT_SINCE_ACTIVITY=10*24;

// defalut radius, based on the max build tile range of 12
const int DEFAULT_RADIUS=13*TILE_SIZE;


class SCFeatureExtractor : public BWAPI::AIModule
{
public:
  virtual void onStart();
  virtual void onFrame();
  virtual void onEnd(bool isWinner);
  //virtual void onUnitDestroy(Unit* unit);
  virtual void onUnitCreate(Unit* unit);
  virtual void onPlayerLeft(Player* player);
  virtual void onSaveGame(std::string gameName);
  std::map<int,int> playerLeftTime;

  // logic to actually dump the vectors to file
  void doDump();

  // updates the states of tracked battles as necessary
  void updateBattles();

  // possibly starts a battle, unit is the unit who fired to start the battle
  void startBattle(Unit* unit);

  // ends a tracked battle and writes the battle info to a file
  //void endBattle(battle *b);
  // the end_time parameter is the timestamp to be used as the end of the battle
  void endBattle(battle b, int end_time);

  // returns a battleUnit, which is just a record of some attributes on unit
  battleUnit makeBattleUnit(Unit* unit);

  // function for computing the map coverage score for a player
  boost::tuple<int,int,int> compute_map_coverage_score(Player* cur_player);

  // this just dumps some info that we will use to figure out who won, it is to be called when the game has finished
  void endGameInfo();

  // the file handle we will be writing to
  std::ofstream writeData;

  // file handle for writing info just about battles to 
  std::ofstream writeBattleData;

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

  // this vector of vectors is used to keep track of unit placment
  // it is a major approximation for the vision coverage feature
  std::vector<std::vector<int>> unit_coverage_map;

  // same as unit_coverage_map but for buildings
  std::vector<std::vector<int>> building_coverage_map;

  // same as unit_coverage_map and building_coverage_map but for units and buildings
  std::vector<std::vector<int>> all_coverage_map;

  // use this variable so that you don't have to 'flush' the coverage_map each iteration
  int coverage_map_counter;

  // just used to store how many super_tiles can actually be built in
  int build_tile_count;

  // just used to store how many super tiles can actually be walked through (so can have units on)
  int walk_tile_count;

  // now to approximate apm we want to look at the orders players are making 
  // to do that we need to know when an order is a new order, so we can use these maps to
  // manage that, this is borrowed from Gabriel Synnaeve's BWRepDump project
  std::map<BWAPI::Unit*, BWAPI::Order> unitOrders;
  std::map<BWAPI::Unit*, BWAPI::Unit*> unitOrdersTargets;
  std::map<BWAPI::Unit*, BWAPI::Position> unitOrdersTargetPositions;

  // and a map for telling how many orders each player has executed
  std::map<int, int> numOrders;

  // for keeping track of number of frames workers have been idle for
  // this is a measure of skill, since good players don't usually let thier workers sit around all the time
  std::map<int, int> idleWorkerCount;

  // count frames that supply is maxed out for
  std::map<int, int> supplyUsedUpFrameCount;

  // count number of queued units each frame, then we can take the average... this is for the total
  std::map<int,int> queuedUnitsTotal;

  // count number of idle production facilities each frame, then we can take the average... this is for the total
  std::map<int, int> idleProdFacTotal;

  // battles we are monitoring
  std::list<battle> trackedBattles;

  // for giving battles unique ids as the game goes on
  int battlesIDCount;

  // keep track of who is already in a battle, quick look-up, stores unit ids
  std::set<int> inBattleIds;

};