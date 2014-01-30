// SCFeatureExtractor.cpp : Defines the exported functions for the DLL application.
//
#include <assert.h>
#include <limits>
#include "SCFeatureExtractor.h"
using namespace BWAPI;

// this is how often (in terms of frames) that we should dump a feature vector
// increase for less examples, decrease for more
const int dump_frame_period = 240;

// this is for determining if a worker's destination is near a base
// this is how close a worker's target needs to be to be considered close
const int closeness_threshold = 400;


void SCFeatureExtractor::onStart()
{
	//Broodwar->printf("Hello World!");
	if (Broodwar->isReplay()){
		// make sure the replay is viewed quickly
		//Broodwar->setGUI(false);
		//Broodwar->setLocalSpeed(0);

		// next open a file to write the vectors for this game
		// use this line to put the new file in the same folder as the .rep file
		//std::string filepath = Broodwar->mapPathName().substr(0,Broodwar->mapPathName().length()-4) + "-feature-vectors.txt";

		// if you want a custom directory, modify and use these lines
		std::string folderpath = "C:\\Users\\Graham\\Desktop\\PvT-feature-vector-files\\";
		std::string filepath = folderpath + Broodwar->mapFileName().substr(0,Broodwar->mapFileName().length()-4) + "-feature-vectors.txt";

		writeData.open(filepath.c_str());

		// intializations happen here
		// construct a map for each player for mapping unit types to counts
		/*
		for(std::set<Player*>::iterator p=Broodwar->getPlayers().begin();p!=Broodwar->getPlayers().end();p++){
			// account for the neutral player
			if ((*p)->getID()!=-1){
				std::map<std::string,int> unit_counts;
				unit_count_maps[(*p)->getID()] = unit_counts;

				// now we need to set up the maps, i.e. make an entry for each possible unit type
				// in order to do that we need to know which faction the player is using
				if ((*p)->getRace().getName()=="Protoss"){
					for(int i=0;i<protoss_units_length;i++){
						unit_counts[protoss_units[i]]=0;
					}
				}
				else if ((*p)->getRace().getName()=="Terran"){
					for(int i=0;i<terran_units_length;i++){
						unit_counts[terran_units[i]]=0;
					}
				}
				else if ((*p)->getRace().getName()=="Zerg"){
					for(int i=0;i<zerg_units_length;i++){
						unit_counts[zerg_units[i]]=0;
					}
				}
				else {
					// if this happens we are including raceless players, which is unexpected
					assert(false);
				}

			}
		}
		*/

		//initialize maps for economic features

		for(std::set<Player*>::iterator p=Broodwar->getPlayers().begin();p!=Broodwar->getPlayers().end();p++){
			// account for the neutral player
			if ((*p)->getID()!=-1  && !(*p)->isObserver()){
				avg_unspent_mineral_total[(*p)->getID()]=0;
				avg_unspent_gas_total[(*p)->getID()]=0;
				numOrders[(*p)->getID()]=0;
				idleWorkerCount[(*p)->getID()]=0;
			}
		}
		
		avg_unspent_count=0;

		// intialize the vector of vectors which represents map coverage
		// what this should be is like a 2-d array where the first dimension
		// is corresponds to the x coordinate and the second dimension corresponds to y
		// also if super_tile_size!=2 there might be problems (I've assumed map sizes are even)
		unit_coverage_map.resize((Broodwar->mapWidth()/super_tile_size));
		for(int i=0;i<(Broodwar->mapWidth()/super_tile_size);i++){
			std::vector<int> height_vector(Broodwar->mapHeight()/super_tile_size,0);
			unit_coverage_map[i]=height_vector;
		}
		building_coverage_map.resize((Broodwar->mapWidth()/super_tile_size));
		for(int i=0;i<(Broodwar->mapWidth()/super_tile_size);i++){
			std::vector<int> height_vector(Broodwar->mapHeight()/super_tile_size,0);
			building_coverage_map[i]=height_vector;
		}
		all_coverage_map.resize((Broodwar->mapWidth()/super_tile_size));
		for(int i=0;i<(Broodwar->mapWidth()/super_tile_size);i++){
			std::vector<int> height_vector(Broodwar->mapHeight()/super_tile_size,0);
			all_coverage_map[i]=height_vector;
		}
		coverage_map_counter=0;

		// we also want to figure out how many super tiles units could possibly be in
		// and same for buildings
		// for the buildings we could borrow the building_coverage_map for this task 
		// it won't be blank anymore but that is okay
		coverage_map_counter++;
		int is_buildable_counter = 0;
		int is_walkable_counter = 0;
		for (int x=0;x<Broodwar->mapWidth();x++){
			for (int y=0;y<Broodwar->mapWidth();y++){
				
				int x_index = (int) x / super_tile_size;
				int y_index = (int) y / super_tile_size;

				// this is for checking if the tile can be built on
				if (Broodwar->isBuildable(x,y)){	
					if (building_coverage_map[x_index][y_index]!=coverage_map_counter){
						building_coverage_map[x_index][y_index]=coverage_map_counter;
						is_buildable_counter++;
					}
				}
			
				// now we also want to see if the tile is walkable at all
				// now, isWalkable uses walk tile (of which that are 4X4 for every buildTile)
				// so to see if this tile is walkable at all, we need to convert to walk tile and loop through
				// all the possible walktile in that tile... if it is walkable at all we can say that the tile is walkable
				bool myIsWalkable = false;
				for (int walk_x=(x*4);walk_x<((x+1)*4);walk_x++){
					for (int walk_y=(y*4);walk_y<((y+1)*4);walk_y++){
						if(Broodwar->isWalkable(walk_x,walk_y)){
							myIsWalkable=true;
						}
					}
				}

				// now at this point we can do what we did for the isBuildable, just borrow one of the other coverage maps
				if (myIsWalkable) {
					if (unit_coverage_map[x_index][y_index]!=coverage_map_counter){
						unit_coverage_map[x_index][y_index]=coverage_map_counter;
						is_walkable_counter++;
					}

				}


			}
		}
		build_tile_count = is_buildable_counter;
		walk_tile_count = is_walkable_counter;
	}

}

void SCFeatureExtractor::onFrame()
{

	// first do any state updates that need to be done
	for(std::set<Player*>::iterator p=Broodwar->getPlayers().begin();p!=Broodwar->getPlayers().end();p++){
		// account for the neutral player and observers
		if ((*p)->getID()!=-1 && !(*p)->isObserver()){
			avg_unspent_mineral_total[(*p)->getID()]+=(*p)->minerals();
			avg_unspent_gas_total[(*p)->getID()]+=(*p)->gas();

			// let us also look at the unit orders..
			// we can get an idea of how many orders are being executed by each player
			for (std::set<BWAPI::Unit*>::const_iterator u=(*p)->getUnits().begin();u!=(*p)->getUnits().end();u++){
			
				// getting the number of idle workers on each frame
				if ((*u)->getType().isWorker() && (*u)->isIdle() && (*u)->isCompleted() && !(*u)->isGatheringMinerals() && !(*u)->isCarryingMinerals() && 
					!(*u)->isGatheringGas() && !(*u)->isCarryingGas() && !(*u)->isConstructing() && !(*u)->isMorphing() && !(*u)->isRepairing() &&
					!(*u)->isAttacking() && !(*u)->isBurrowed()){
					/*writeData << (*p)->getID() << ",";
					writeData << (*u)->getID() << ",";
					writeData << (*u)->isMoving() << ",";
					writeData << (*u)->isCarryingMinerals() << ",";
					writeData << Broodwar->getFrameCount() << ",";
					writeData << "\n";*/

					idleWorkerCount[(*p)->getID()]++;
				}


				// this if statment is borrowed from gabriel synnaeve, and is used to make sure that we are avoiding system level orders and
				// are just looking at orders that are given by the player
				if ((!((*u)->getType().isWorker() && ((*u)->isGatheringMinerals() || (*u)->isGatheringGas())) || ((*u)->isGatheringMinerals() && (*u)->getOrder() != BWAPI::Orders::WaitForMinerals && (*u)->getOrder() != BWAPI::Orders::MiningMinerals && (*u)->getOrder() != BWAPI::Orders::ReturnMinerals) ||
				((*u)->isGatheringGas() && (*u)->getOrder() != BWAPI::Orders::WaitForGas && (*u)->getOrder() != BWAPI::Orders::HarvestGas && (*u)->getOrder() != BWAPI::Orders::ReturnGas)) &&
				((*u)->getOrder() != BWAPI::Orders::ResetCollision) &&
				((*u)->getOrder() != BWAPI::Orders::Larva) &&
				((*u)->getOrder() != BWAPI::Orders::None && (*u)->getOrder() != BWAPI::Orders::Nothing && (*u)->getOrder() != BWAPI::Orders::Nothing3 && (*u)->getOrder() != BWAPI::Orders::Neutral)){
				
					// if we have seen the unit before, check if the current order is the last order seen, if it is not, or if the order has never been seen,
					// we update the last order to be seen and count this as one order for this player
					bool newOrder=false;
					if (unitOrders.count(*u)!=0){
						if(this->unitOrders[*u] != (*u)->getOrder() || this->unitOrdersTargets[*u] != (*u)->getOrderTarget() || this->unitOrdersTargetPositions[*u] != (*u)->getOrderTargetPosition()){
							newOrder=true;
						}
					}
					else {
						newOrder=true;
					}
					
					// update
					if (newOrder){
						unitOrders[*u] = (*u)->getOrder();
						unitOrdersTargets[*u] = (*u)->getOrderTarget();
						unitOrdersTargetPositions[*u] = (*u)->getOrderTargetPosition();

						// and check that this is one more order for that player
						numOrders[(*p)->getID()]+=1;
						
						// just checking
						/*
						writeData << (*p)->getID();
						writeData << ",";
						writeData << (*u)->getOrder().getName();
						writeData << "\n";*/
						
					}


				}

			}


		}
	}
		
	avg_unspent_count+=1;


	//for(std::set<Player*>::iterator p=Broodwar->getPlayers().begin();p!=Broodwar->getPlayers().end();p++){
	//	Broodwar->setReplayVision((*p),false);
	//}


	// testing visibility code here
	/*for(std::set<Player*>::iterator p=Broodwar->getPlayers().begin();p!=Broodwar->getPlayers().end();p++){
		if ((*p)->getID()!=-1 && !(*p)->isObserver()){
			int cur_visible_count=0;
			for (int x=0;x<Broodwar->mapWidth();x++){
				for (int y=0;y<Broodwar->mapHeight();y++){
					if (Broodwar->isVisible(x,y)){
						cur_visible_count+=1;
					}
				}
			}
			Broodwar->printf("pl_id: %d, cur vis: %d",(*p)->getID(),cur_visible_count);
			//for (std::set<Unit*>::const_iterator u=(*p)->getUnits().begin();u!=(*p)->getUnits().end();u++){
				//(*u)->
			//	(*p)->sightRange((*u)->getType());
			//}
		
		}
	}
	*/




	if (Broodwar->getFrameCount()%dump_frame_period==0 && Broodwar->getFrameCount()!=0){
		
		// compute the current state of the unit maps for each player
		// first we need to reset the counts
		// note: I am not doing an incremental approach (although it would be faster) because that is what Synnaeve did
		// and there appears to be bugs in his implementation
		/*
		for(std::set<Player*>::iterator p=Broodwar->getPlayers().begin();p!=Broodwar->getPlayers().end();p++){
			// account for the neutral player
			if ((*p)->getID()!=-1){
				
				for (std::map<std::string,int>::iterator it=unit_count_maps[(*p)->getID()].begin(); it!=unit_count_maps[(*p)->getID()].end(); ++it){
					unit_count_maps[(*p)->getID()][it->first] = 0;
				}
			}
		}*/

		/*
		for(std::set<Player*>::iterator p=Broodwar->getPlayers().begin();p!=Broodwar->getPlayers().end();p++){
			// account for the neutral player
			if ((*p)->getID()!=-1){
				
			}
		}
		*/

		// then do the dump if it is time
		this->doDump();

	}

}

/***
* This function computes a score for the given player
* the score is the number of 'super tiles' that the player has a unit/building in
* scores are returned in a triple (a boost tuple), the order being
* (unit score,building score, all score)
**/
boost::tuple<int,int,int> SCFeatureExtractor::compute_map_coverage_score(Player* cur_player){
	
	// before we start, increment the counter
	// this value is now 'true' for this context
	coverage_map_counter++;

	assert(coverage_map_counter!=std::numeric_limits<int>::max());

	// and keep track of how many were switched on
	int unit_score=0;
	int building_score=0;
	int all_score=0;

	for (std::set<Unit*>::const_iterator u=cur_player->getUnits().begin();u!=cur_player->getUnits().end();u++){
		// use integer division to get the indices
		int x_index = (int) (*u)->getTilePosition().x()/ super_tile_size;
		int y_index = (int) (*u)->getTilePosition().y()/ super_tile_size;


		if (!(*u)->getType().isBuilding()){
			// assume that not building means unit
			if (unit_coverage_map[x_index][y_index]!=coverage_map_counter){
				unit_score++;
				unit_coverage_map[x_index][y_index]=coverage_map_counter;
			}
		}
		else {
			// if it is a building
			if (building_coverage_map[x_index][y_index]!=coverage_map_counter){
				building_score++;
				building_coverage_map[x_index][y_index]=coverage_map_counter;
			}
		}

		// and then do the all case all the time
		if (all_coverage_map[x_index][y_index]!=coverage_map_counter){
			all_score++;
			all_coverage_map[x_index][y_index]=coverage_map_counter;
		}
	}
	
	return boost::tuple<int,int,int>(unit_score,building_score,all_score);
}

/**
*  doDump takes care of dumping the current state to file as a feature vector
*  it will write a single line for the feature vector
**/
void SCFeatureExtractor::doDump(){
	Broodwar->printf("Doing the dump!");

	for(std::set<Player*>::iterator p=Broodwar->getPlayers().begin();p!=Broodwar->getPlayers().end();p++){
		// account for the neutral player, as in don't dump for that player
		// also don't dump for observers, as they are not actually playing
		if ((*p)->getID()!=-1 && !(*p)->isObserver()){
				// now we can start printing stuff, format is each attribute is separated by a comma and it goes attr_label:attr_value 
				// time stamp first
				writeData << "timestamp:";
				writeData << Broodwar->getFrameCount();
				writeData << ",";
				writeData << "player_id:";
				writeData << (*p)->getID();
				writeData << ",";

				// now let's get into unit counts
				// do this based on faction, also printing the faction wouldn't hurt either
				writeData << "race:";
				writeData << (*p)->getRace().getName();
				writeData << ",";

				// economic features, these are all being added just so something isn't missed, using all of them would possibly be redundant
				//current counts
				writeData << "current_minerals:";
				writeData << (*p)->minerals();
				writeData << ",";
				writeData << "current_gas:";
				writeData << (*p)->gas();
				writeData << ",";
				//cummulative
				writeData << "total_minerals:";
				writeData << (*p)->gatheredMinerals();
				writeData << ",";
				writeData << "total_gas:";
				writeData << (*p)->gatheredGas();
				writeData << ",";
				//spent resources, not sure if this is useful as it should be dependant of what has been made
				writeData << "spent_minerals:";
				writeData << (*p)->spentMinerals();
				writeData << ",";
				writeData << "spent_gas:";
				writeData << (*p)->spentGas();
				writeData << ",";
				// resources specifically spent on repairing, might give a sense of what sort of damage the player has dealt with
				writeData << "repair_minerals:";
				writeData << (*p)->repairedMinerals();
				writeData << ",";
				writeData << "repair_gas:";
				writeData << (*p)->repairedGas();
				writeData << ",";
				//avg unspent resources
				writeData << "avg_unspent_minerals:";
				writeData <<  ((float)avg_unspent_mineral_total[(*p)->getID()])/((float) avg_unspent_count);
				writeData << ",";
				writeData << "avg_unspent_gas:";
				writeData <<  ((float)avg_unspent_gas_total[(*p)->getID()])/((float) avg_unspent_count);
				writeData << ",";
				// and income, which is just a rate
				// so we'll try doing total gathered / time passed ( in seconds, though it doesn't really matter I suppose)
				writeData << "minerals_per_second:";
				writeData << ((float)(*p)->gatheredMinerals())/(((float)Broodwar->getFrameCount())/24.0);
				writeData << ",";
				writeData << "gas_per_second:";
				writeData << ((float)(*p)->gatheredGas())/(((float)Broodwar->getFrameCount())/24.0);
				writeData << ",";



				// so now unit count, do these based on race
				// we'll have three columns for each unit type,
				// one for completed, incomplete, and dead units
				if ((*p)->getRace()==BWAPI::Races::Protoss){
					for (int i=0;i<protoss_units_length;i++){
						writeData << protoss_units[i].getName();
						writeData << ":";
						writeData << (*p)->completedUnitCount(protoss_units[i]);
						writeData << ",";
						writeData << protoss_units[i].getName();
						writeData << "-incomplete:";
						writeData << (*p)->incompleteUnitCount(protoss_units[i]);
						writeData << ",";
						writeData << protoss_units[i].getName();
						writeData << "-dead:";
						writeData << (*p)->deadUnitCount(protoss_units[i]);
						writeData << ",";
					}
				}
				else if ((*p)->getRace()==BWAPI::Races::Terran) {
					for (int i=0;i<terran_units_length;i++){
						writeData << terran_units[i].getName();
						writeData << ":";
						writeData << (*p)->completedUnitCount(terran_units[i]);
						writeData << ",";
						writeData << terran_units[i].getName();
						writeData << "-incomplete:";
						writeData << (*p)->incompleteUnitCount(terran_units[i]);
						writeData << ",";
						writeData << terran_units[i].getName();
						writeData << "-dead:";
						writeData << (*p)->deadUnitCount(terran_units[i]);
						writeData << ",";
					}
				}
				else if ((*p)->getRace()==BWAPI::Races::Zerg){
					for (int i=0;i<zerg_units_length;i++){
						writeData << zerg_units[i].getName();
						writeData << ":";
						writeData << (*p)->completedUnitCount(zerg_units[i]);
						writeData << ",";
						writeData << zerg_units[i].getName();
						writeData << "-incomplete:";
						writeData << (*p)->incompleteUnitCount(zerg_units[i]);
						writeData << ",";
						writeData << zerg_units[i].getName();
						writeData << "-dead:";
						writeData << (*p)->deadUnitCount(zerg_units[i]);
						writeData << ",";
					}
				}

				// next I want features for what each player has killed/razed
				// we can use killedUnitCount() to get the numbers, but we need to know the opponent's faction... how do we get that?
				// well we could just have a column for EVERY unit type, and just clean it up later (will be sparse-ish)
				for (int i=0;i<protoss_units_length;i++){
					writeData << protoss_units[i].getName();
					writeData << "-killed:";
					writeData << (*p)->killedUnitCount(protoss_units[i]);
					writeData << ",";
				}
				for (int i=0;i<terran_units_length;i++){
					writeData << terran_units[i].getName();
					writeData << "-killed:";
					writeData << (*p)->killedUnitCount(terran_units[i]);
					writeData << ",";
				}
				for (int i=0;i<zerg_units_length;i++){
					writeData << zerg_units[i].getName();
					writeData << "-killed:";
					writeData << (*p)->killedUnitCount(zerg_units[i]);
					writeData << ",";
				}
				
				// here we have research (tech), these features are binary, they just show if the player has researched or not
				for(std::set<TechType>::const_iterator t=BWAPI::TechTypes::allTechTypes().begin();t!=BWAPI::TechTypes::allTechTypes().end();t++){
					// only use techs that are for the current player's race
					if ((*t).getRace()==(*p)->getRace() && (*t).whatResearches()!=UnitTypes::None){
						writeData << (*t).getName();
						writeData << ":";
						writeData << (*p)->hasResearched((*t));
						writeData << ",";
					}
				}
				
				// there are also upgrades, which have levels associated with them
				// these might not be great features because how do you tell what upgrades an enemy has?
				// some might be obvious but stuff like infantry armour would be tough
				for(std::set<UpgradeType>::const_iterator t=BWAPI::UpgradeTypes::allUpgradeTypes().begin();t!=BWAPI::UpgradeTypes::allUpgradeTypes().end();t++){
					if ((*t).getRace()==(*p)->getRace()){
					
						writeData << (*t).getName();
						writeData << ":";
						writeData << (*p)->getUpgradeLevel((*t));
						writeData << ",";

					}
				}

				// there are a few 'simple' map features we could get
				// the question is how do we get the vision/explored for each player separately
				// that proved... difficult, so let's use an approximate map coverage feature instead
				boost::tuple<int,int,int> score_triple = compute_map_coverage_score((*p));

				// and write the three scores
				writeData << "unit_coverage_score:";
				writeData << score_triple.get<0>();
				writeData << ",";
				writeData << "building_coverage_score:";
				writeData << score_triple.get<1>();
				writeData << ",";
				writeData << "all_coverage_score:";
				writeData << score_triple.get<2>();
				writeData << ",";

				// it will also be useful to know what that score is out of
				writeData << "max_coverage_score:";
				writeData << ((int) Broodwar->mapWidth()/super_tile_size)*((int) Broodwar->mapHeight()/super_tile_size);
				writeData << ",";

				// max coverage is just the area in terms of super tiles
				// this isn't the best upper limit, as there are tiles that just can't be built or walked upon
				writeData << "max_buildable_tiles_score:";
				writeData << build_tile_count;
				writeData << ",";

				writeData << "max_walkable_tiles_score:";
				writeData << walk_tile_count;
				writeData << ",";

				// we could alos use to have more information about workers
				// like not just how many are there, but how many are doing what
				int num_workers_on_minerals = 0;
				int num_workers_on_gas = 0;
				int num_workers_constructing =0;
				int num_workers_repairing=0;
				int num_workers_attacking = 0;
				int num_workers_burrowed = 0;
				int num_workers_moving=0;
				int num_workers_other=0;
				int num_workers_who_knows=0;
				int num_workers_scouting=0;
				for (std::set<Unit*>::const_iterator u=(*p)->getUnits().begin();u!=(*p)->getUnits().end();u++){
					// we're looking at workers, and we don't want to count incomplete units, those are already shown in another feature
					if((*u)->getType().isWorker() && (*u)->isCompleted()){
						// minerals
						if ((*u)->isGatheringMinerals() || (*u)->isCarryingMinerals()){
							num_workers_on_minerals++;
						}
						// gas
						else if ((*u)->isGatheringGas() || (*u)->isCarryingGas()){
							num_workers_on_gas++;
						}
						// building, is constructing makes more sense for terran, but will count workers who are starting a construction for the other races
						// the isMorphing should catch zerg drones building as well
						else if ((*u)->isConstructing() || (*u)->isMorphing()){
							num_workers_constructing++;
						}
						// repairing, only counts for terran
						else if ((*u)->isRepairing()){
							num_workers_repairing++;
						}
						// is the worker being involved in a fight?
						else if ((*u)->isAttacking()){
							num_workers_attacking++;
						}
						// this is just for zerg, in case the player has them burrowed
						else if ((*u)->isBurrowed()){
							num_workers_burrowed++;
						}
						else {
							// okay, one idea for this is if we get here, just figure out if the worker is being used as a scout or not
							BWAPI::Position dest_posit = (*u)->getOrderTargetPosition();
							//Broodwar->printf("%s: %d %d",(*p)->getRace().getName().c_str(),dest_posit.x(),dest_posit.y());
							bool isScout=false;

							// if they are both zero let's say that is null basically
							if (dest_posit.x()!=0 || dest_posit.y()!=0){
								// so if they aren't, let us consider that for being scouting
								// and we will say a worker is scouting if it is being sent near an enemy base
								// there are really two cases here though, because you could be scouting but not near a base (i.e. scouting unsuccessfully)
								// so first let us just look for successful scouts

								// so to look through bases, we need to do another loop through players and then units when we have found an enemy (expensive!)
								for(std::set<Player*>::iterator p2=Broodwar->getPlayers().begin();p2!=Broodwar->getPlayers().end();p2++){
									if ((*p2)->getID()!=(*p)->getID() && (*p)->getID()!=-1 && !(*p)->isObserver()){
										// enemy player
										// now look for enemy bases
										for (std::set<Unit*>::const_iterator u2=(*p2)->getUnits().begin();u2!=(*p2)->getUnits().end();u2++){
											if ((*u2)->getType() == BWAPI::UnitTypes::Protoss_Nexus || (*u2)->getType() == BWAPI::UnitTypes::Terran_Command_Center ||
												(*u2)->getType() == BWAPI::UnitTypes::Zerg_Hatchery || (*u2)->getType() == BWAPI::UnitTypes::Zerg_Hive ||
												(*u2)->getType() == BWAPI::UnitTypes::Zerg_Lair){
												
												// now check if the location is 'close' to that of the worker
												// we'll have constant to determine closeness
												if ((*u2)->getDistance(dest_posit)<=closeness_threshold){
													isScout=true;
												}
											
											}
										}
									}

								}
							}

							if (isScout)
								num_workers_scouting++;
							else
								num_workers_other++;

						}

					}
				}

				// then write those stats to the file
				writeData << "num_workers_on_minerals:";
				writeData << num_workers_on_minerals;
				writeData << ",";
				writeData << "num_workers_on_gas:";
				writeData << num_workers_on_gas;
				writeData << ",";
				writeData << "num_workers_constructing:";
				writeData << num_workers_constructing;
				writeData << ",";
				writeData << "num_workers_repairing:";
				writeData << num_workers_repairing;
				writeData << ",";
				writeData << "num_workers_attacking:";
				writeData << num_workers_attacking;
				writeData << ",";
				writeData << "num_workers_burrowed:";
				writeData << num_workers_burrowed;
				writeData << ",";
				writeData << "num_workers_scouting:";
				writeData << num_workers_scouting;
				writeData << ",";
				writeData << "num_workers_other:";
				writeData << num_workers_other;
				writeData << ",";

				// cummulative idle
				writeData << "tot_frames_workers_idle:";
				writeData << idleWorkerCount[(*p)->getID()];
				writeData << ",";

				// here we can put apm related features
				// note that this isn't actually apm, it is more like unit orders per minute
				writeData << "num_orders_total:";
				writeData << numOrders[(*p)->getID()];
				writeData << ",";

				writeData << "orders_per_frame:";
				writeData << (float)numOrders[(*p)->getID()] / (float)Broodwar->getFrameCount();
				writeData << ",";

				writeData << "orders_per_second:";
				writeData << (float)numOrders[(*p)->getID()] / ((float)Broodwar->getFrameCount()/24.0);
				writeData << ",";

				writeData << "orders_per_minute:";
				writeData << (float)numOrders[(*p)->getID()] / (((float)Broodwar->getFrameCount()/24.0)/60.0);
				writeData << ",";

				writeData << "orders_per_minute_norm:";
				writeData << ((float)numOrders[(*p)->getID()] / (((float)Broodwar->getFrameCount()/24.0)/60.0))/(float)Broodwar->getFrameCount();
				writeData << ",";

				writeData << "\n";


			}
		}

}

void SCFeatureExtractor::onEnd(){
	writeData.close();
}


