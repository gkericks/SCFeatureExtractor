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
		std::string battle_filepath = folderpath + Broodwar->mapFileName().substr(0,Broodwar->mapFileName().length()-4) + "-battle-info.txt";

		writeData.open(filepath.c_str());
		writeBattleData.open(battle_filepath.c_str());

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
				supplyUsedUpFrameCount[(*p)->getID()]=0;
				idleProdFacTotal[(*p)->getID()] = 0;
				queuedUnitsTotal[(*p)->getID()] = 0;
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

		battlesIDCount=0;
	}

}

void SCFeatureExtractor::onUnitCreate(Unit* unit){
	//if (unit->getID()==187){
	//	Broodwar->printf("unit 187 is created");
	//}
}

void SCFeatureExtractor::onUnitDestroy(Unit* unit){
	
	
	/*
	if (unit->getID()==187){
		Broodwar->printf("unit 187 died at: %d %d", unit->getPosition().x(), unit->getPosition().y());
		Broodwar->printf(unit->getOrder().getName().c_str());
		Broodwar->printf("is completed: %d",unit->isCompleted());
	}
	else{
		Broodwar->printf("on destroy unit %d is completed: %d",unit->getID(),unit->isCompleted());
	}*/

	// don't care about friendly fire
	if (unit->getLastAttackingPlayer()!=NULL && unit->getLastAttackingPlayer()->getID()==unit->getPlayer()->getID())
		return;

	// make sure units from non players/observers don't get counted
	if (unit->getPlayer()->isObserver() || unit->getPlayer()->getID()==-1)
		return;

	// if the unit wasn't completed yet (i.e. it was cancelled) we don't want to count that
	if (!unit->isCompleted())
		return;

	// we don't want things like spider mines starting a new battle
	// only this if statement might also avoid a lot of zerg units...
	//if (!unit->getType().isBuilding() && !unit->getType().whatBuilds().first.isBuilding())
	//	return;

	// so let's just explicitly put what we want to avoid
	//if (unit->getType()==UnitTypes::Terran_Vulture_Spider_Mine || unit->getType()==UnitTypes::Protoss_Scarab || unit->getType()==UnitTypes::Protoss_Interceptor)
	//	return;

	// should spells start battles?  Scanner sweep definitely should not, not sure about the other two
	if (unit->getType()==UnitTypes::Spell_Dark_Swarm || unit->getType()==UnitTypes::Spell_Disruption_Web || unit->getType()==UnitTypes::Spell_Scanner_Sweep)
		return;

#ifdef DEBUG_BATTLES_OUTPUT
	writeBattleData << "Destroyed:" << unit->getType().getName() << ":" << unit->getID() << ":" << Broodwar->getFrameCount() << ":" << unit->getPlayer()->getID() << "\n";
#endif

	// so first we check if the unit is in the range of another battle or not
	// if it is, we just let onFrame up date it, so return from here
	for(std::list<battle>::iterator i=trackedBattles.begin();i!=trackedBattles.end();i++){
		if (unit->getPosition().getDistance(i->battleCenter)<=i->radius){
#ifdef DEBUG_BATTLES_OUTPUT
			writeBattleData << "Unit: " << unit->getID() << " is in battle range: " << i->id << "\n";
#endif
			return;
		}
	}

	
	
	// if we haven't return yet that means this unit is not in range of another battle,
	// which means we need to start up a new battle
	battle newBattle;
	newBattle.startTime=Broodwar->getFrameCount();
	newBattle.curTime=Broodwar->getFrameCount();

	// so let's put a default radius around this unit's position, and use that to grab what unit we want in initially
	std::list<Unit*> includedUnits;
	int numNonBuildings=0;
	int totX=0;
	int totY=0;

	// for keeping track of if both sides have fighting units in the battle
	int numPlayersWithNonBuildingsNonAmmo=0;
	// the only problem is that the unit that dies isn't included and it might affect which players are added
	bool isCurPlayerCounted=false;

	// also we probably want to add the destroyed unit (whose death started the battle) to our data structures
	includedUnits.push_back(unit);
	

	for (std::set<Player*>::iterator p=Broodwar->getPlayers().begin();p!=Broodwar->getPlayers().end();p++){
		if (!(*p)->isObserver() && (*p)->getID()!=-1){
			std::map<Unit*,int> unitTimeMap;
			newBattle.enteredUnits[(*p)->getID()] = unitTimeMap;

			// for checking if this player added anything
			bool hasPlayerUnits=false;

			for (std::set<Unit*>::const_iterator u=(*p)->getUnits().begin();u!=(*p)->getUnits().end();u++){
				
				// don't include incomplete units or units that really are just ammo
				//if ((*u)->isCompleted() && (*u)->getType()!=UnitTypes::Terran_Vulture_Spider_Mine && (*u)->getType()!=UnitTypes::Protoss_Scarab && (*u)->getType()!=UnitTypes::Protoss_Interceptor){
				if ((*u)->isCompleted()){
					// now lets just add units within the default range of the dead unit
					if ((*u)->getPosition().getDistance(unit->getPosition())<=DEFAULT_RADIUS){
						includedUnits.push_back((*u));
						newBattle.enteredUnits[(*p)->getID()][(*u)]=Broodwar->getFrameCount();
						newBattle.trackedUnitTypes[(*u)->getID()] = (*u)->getType();
						// count military units, basically units and cannons
						if (!(*u)->getType().isBuilding() || (*u)->getType()==UnitTypes::Protoss_Photon_Cannon || (*u)->getType()==UnitTypes::Terran_Missile_Turret || 
							(*u)->getType()==UnitTypes::Terran_Bunker || (*u)->getType()==UnitTypes::Zerg_Sunken_Colony && ((*u)->getType()!=UnitTypes::Terran_Vulture_Spider_Mine && (*u)->getType()!=UnitTypes::Protoss_Scarab && (*u)->getType()!=UnitTypes::Protoss_Interceptor)){
							numNonBuildings+=1;
							totX+=(*u)->getPosition().x();
							totY+=(*u)->getPosition().y();
						}

						// now its only a battle if both sides have something besides mines
						if ((*u)->getType()!=UnitTypes::Terran_Vulture_Spider_Mine && (*u)->getType()!=UnitTypes::Protoss_Scarab && (*u)->getType()!=UnitTypes::Protoss_Interceptor){
							hasPlayerUnits=true;
							if ((*p)->getID()==unit->getPlayer()->getID())
								isCurPlayerCounted=true;
						}

					}
				}
			}
			if (hasPlayerUnits)
				numPlayersWithNonBuildingsNonAmmo+=1;

		}
	}

	newBattle.enteredUnits[unit->getPlayer()->getID()][unit] = Broodwar->getFrameCount();
	newBattle.trackedUnitTypes[unit->getID()] = unit->getType();

	if (!isCurPlayerCounted && unit->getType()!=UnitTypes::Terran_Vulture_Spider_Mine && unit->getType()!=UnitTypes::Protoss_Scarab && unit->getType()!=UnitTypes::Protoss_Interceptor)
		numPlayersWithNonBuildingsNonAmmo+=1;

	//  Watch out for when numNonBuildings is zero
	if (numNonBuildings==0)
		return;

	// also if only one player has units (that aren't buildings or mines/scarabs/interceptors), that isn't a battle
	if (numPlayersWithNonBuildingsNonAmmo<2)
		return;

	// now to get the position of the match, it is the average position of the potentially fighting units we grabbed
	newBattle.battleCenter = Position(totX/numNonBuildings,totY/numNonBuildings);
	// and we need the radius
	int cur_radius=DEFAULT_RADIUS;
	for (std::list<Unit*>::const_iterator u=includedUnits.begin();u!=includedUnits.end();u++){
		int tmpDis = (*u)->getDistance(newBattle.battleCenter) + std::max((*u)->getType().groundWeapon().maxRange(),(*u)->getType().airWeapon().maxRange());
		if (tmpDis>cur_radius){
			cur_radius=tmpDis;
		}
	}

	newBattle.radius=cur_radius;

#ifdef DEBUG_BATTLES_OUTPUT
	writeBattleData << "Destroyed and starting a battle:" << unit->getType().getName() << ":" << unit->getID() << ":" << Broodwar->getFrameCount() << ":" << unit->getPlayer()->getID() << "\n";
#endif

	// give battle an id
	newBattle.id=battlesIDCount;
	battlesIDCount++;

	// make sure we know that these units are all apart of a battle now
	for(std::map<int, std::map<Unit*, int>>::iterator it=newBattle.enteredUnits.begin();it!=newBattle.enteredUnits.end();it++){
		for (std::map<Unit*, int>::iterator u_it=it->second.begin();u_it!=it->second.end();u_it++){
			inBattleIds.insert(u_it->first->getID());
		}
	}

	// then start tracking the new battle
	trackedBattles.push_back(newBattle);

}

void SCFeatureExtractor::endBattle(battle c, int end_time){
	// so here we are dumping the contents of the battle,
	//  start time is important
	battle* b = &c;
	writeBattleData << "battleID:" << b->id << ",";
	writeBattleData << "startTime:" << b->startTime << ",";
	writeBattleData << "startUnits:";

	// then just dump each unit with the time they entered the battle, we will deal with deaths etc after
	for(std::map<int, std::map<Unit*, int>>::iterator it=b->enteredUnits.begin();it!=b->enteredUnits.end();it++){
		writeBattleData << it->first << ": {";
		// now iterate through the unit map
		for (std::map<Unit*, int>::iterator u_it=it->second.begin();u_it!=it->second.end();u_it++){
			
			if (u_it->first->getType()==UnitTypes::Unknown)
				writeBattleData << b->trackedUnitTypes[u_it->first->getID()].getName() << ":";
			else
				writeBattleData << u_it->first->getType().getName() << ":";
			//writeBattleData << u_it->first->getHitPoints() << ":";
			writeBattleData << u_it->first->getID() << ":";
			writeBattleData << u_it->second << ",";

			// also make sure the unit is now longer being considered in a battle
			inBattleIds.erase(u_it->first->getID());

		}

		writeBattleData << "}, ";
	
	}

	// lets make the end time the last updated time
	writeBattleData << "endTime:" << end_time << ",";
	writeBattleData << "exitUnits:";

	// now we want to tell who is alive at the end, so we'll go back through the units
	// and if they are not dead, we will include them
	for(std::map<int, std::map<Unit*, int>>::iterator it=b->enteredUnits.begin();it!=b->enteredUnits.end();it++){
		writeBattleData << it->first << ": {";
		for (std::map<Unit*, int>::iterator u_it=it->second.begin();u_it!=it->second.end();u_it++){
			// not sure if this is the correct way to check if the unit is alive or not
			//if (u_it->first->getHitPoints()>0){
			
			if (u_it->first->exists() && !(u_it->first->getPlayer()->isDefeated()) && !(u_it->first->getPlayer()->leftGame())){
				writeBattleData << u_it->first->getType().getName() << ":";
				//writeBattleData << u_it->first->getHitPoints() << ":";
				writeBattleData << u_it->first->getID() << ",";

			}

		}
		writeBattleData << "}, ";
	}

	writeBattleData << "\n";

}

void SCFeatureExtractor::updateBattles(){

	// iterate through battles, update all of them as neccessary
	std::list<battle>::iterator b=trackedBattles.begin();
	while (b!=trackedBattles.end()){
		std::set<Unit*> applicableUnits = Broodwar->getUnitsInRadius(b->battleCenter,b->radius);
		
		// if there are no units in the battle area, end the battle, otherwise update the battle
		if (applicableUnits.size()==0){
#ifdef DEBUG_BATTLES_OUTPUT
			writeBattleData << "no units at all in area\n";
#endif
			//endBattle(&(*b));
			endBattle((*b),Broodwar->getFrameCount());
			trackedBattles.erase(b++);
		}
		else {
			// now we look at the applicable units and add the ones that haven't already been added to the battle
			// we can also grab the new center position here
			int xTotPos = 0;
			int yTotPos = 0;
			int totUnitsCounted=0;
			//  are there units actively attacking/beingattacked?
			bool isAttack=false; 

			// first pass checks if new units should be added
			for (std::set<Unit*>::iterator u=applicableUnits.begin();u!=applicableUnits.end();u++){
				//if (!(*u)->getPlayer()->isObserver() && (*u)->getPlayer()->getID()!=-1 && (*u)->isCompleted() &&
				//	(*u)->getType()!=UnitTypes::Terran_Vulture_Spider_Mine && (*u)->getType()!=UnitTypes::Protoss_Scarab && (*u)->getType()!=UnitTypes::Protoss_Interceptor){
				if (!(*u)->getPlayer()->isObserver() && (*u)->getPlayer()->getID()!=-1 && (*u)->isCompleted()){

					int curPlId = (*u)->getPlayer()->getID();
			
					//xTotPos+=(*u)->getPosition().x();
					//yTotPos+=(*u)->getPosition().y();
					//totUnitsCounted++;

					bool okayToConsider=true;
					// has this unit entered the battle already? If not then add it
					if (b->enteredUnits[curPlId].count((*u))==0 && ((*u)->isAttacking() || (*u)->isStartingAttack() || (*u)->isUnderAttack() || (*u)->isAttackFrame())){
						
						// so this is the is this unit in a battle already check
						if (inBattleIds.count((*u)->getID())==0){
							b->enteredUnits[curPlId][(*u)]=Broodwar->getFrameCount();
							b->trackedUnitTypes[(*u)->getID()]=(*u)->getType();
							inBattleIds.insert((*u)->getID());
						}
						else{
							// so this happens when a unit is not in this battle yet, but is already in another battle, we do not
							// want to be considering it for this battle
							okayToConsider=false;
						}
					}

					// is unit attacking or being attacked?
					if (okayToConsider && ((*u)->isAttacking() || (*u)->isStartingAttack() || (*u)->isUnderAttack() || (*u)->isAttackFrame())){
						isAttack=true;
					}
				}
			}

			std::map<int,int> plUnitsCounted;
			// second pass computes new totUnits and positions, as well as counts for the players
			// only counts units that are both in the range and in our battle (so have attacked/been attacked before)
			for (std::set<Unit*>::iterator u=applicableUnits.begin();u!=applicableUnits.end();u++){
				
				// so i've included mines in the battles, but not as part of the calculations....
				if  (!(*u)->getPlayer()->isObserver() && (*u)->getPlayer()->getID()!=-1 && b->enteredUnits[(*u)->getPlayer()->getID()].count((*u))!=0 && (*u)->isCompleted() &&
					(*u)->getType()!=UnitTypes::Terran_Vulture_Spider_Mine && (*u)->getType()!=UnitTypes::Protoss_Scarab && (*u)->getType()!=UnitTypes::Protoss_Interceptor){
					
					xTotPos+=(*u)->getPosition().x();
					yTotPos+=(*u)->getPosition().y();
					totUnitsCounted++;

					if (plUnitsCounted.count((*u)->getPlayer()->getID())==0)
						plUnitsCounted[(*u)->getPlayer()->getID()]=1;
					else
						plUnitsCounted[(*u)->getPlayer()->getID()]+=1;
				}
			}

			bool isOneSideDone=false;
			int isDoneCount=0;
			int isNotDoneCount=0;
			// now we can use plUnitsCounted to see if one player is done (no units in the battle)
			for (std::map<int,std::map<Unit*,int>>::const_iterator p = b->enteredUnits.begin();p!=b->enteredUnits.end();p++){
				if (plUnitsCounted.count((*p).first)==0){
					isOneSideDone=true;
					isDoneCount++;
				}
				else{
					isNotDoneCount++;
				}
			}


			// so after we do this, totUnitsCounted could be zero,
			// if there were unit in applicableUnits like mineral patches that we won't count, but no unit we do count
			// so use this as a battle termination condition as well
			if (totUnitsCounted==0 || (isOneSideDone && isNotDoneCount!=2)){
#ifdef DEBUG_BATTLES_OUTPUT
				if (totUnitsCounted==0)
					writeBattleData << "no units that we count\n";
				if (isOneSideDone)
					writeBattleData << "at least one side is out of units\n";
#endif
				//endBattle(&(*b));
				endBattle((*b),Broodwar->getFrameCount());
				trackedBattles.erase(b++);
			}
			else{
				// update the battle position, then get the new radius
				b->battleCenter = Position(xTotPos/totUnitsCounted,yTotPos/totUnitsCounted);
				int cur_radius = DEFAULT_RADIUS;
				for (std::set<Unit*>::iterator u=applicableUnits.begin();u!=applicableUnits.end();u++){
					if (!(*u)->getPlayer()->isObserver() && (*u)->getPlayer()->getID()!=-1 && b->enteredUnits[(*u)->getPlayer()->getID()].count((*u))!=0){
						int tmpDis = (*u)->getDistance(b->battleCenter) + std::max((*u)->getType().groundWeapon().maxRange(),(*u)->getType().airWeapon().maxRange());
						if (tmpDis>cur_radius){
							cur_radius=tmpDis;
						}
					}
				}

				b->radius=cur_radius;

				if (isAttack){
					b->curTime=Broodwar->getFrameCount();
				}


				// we need another case where battles can finish/be removed
				// probably a time out of sorts
				if ((Broodwar->getFrameCount()-(b->curTime))>=TIME_LIMIT_SINCE_ACTIVITY){
#ifdef DEBUG_BATTLES_OUTPUT
					writeBattleData << "time out\n";
#endif
					//endBattle(&(*b));
					endBattle((*b),b->curTime);
					trackedBattles.erase(b++);
				}
				else
					++b;
			}
		}
	}


}



void SCFeatureExtractor::onFrame()
{
	//Broodwar->printf("is zealot built by a building: %d",BWAPI::UnitTypes::Protoss_Zealot.whatBuilds().first.isBuilding());

	// first do any state updates that need to be done
	for(std::set<Player*>::iterator p=Broodwar->getPlayers().begin();p!=Broodwar->getPlayers().end();p++){
		// account for the neutral player and observers
		if ((*p)->getID()!=-1 && !(*p)->isObserver()){
			avg_unspent_mineral_total[(*p)->getID()]+=(*p)->minerals();
			avg_unspent_gas_total[(*p)->getID()]+=(*p)->gas();

			// check if supply is all used up
			if ((*p)->supplyTotal()==(*p)->supplyUsed()){
				supplyUsedUpFrameCount[(*p)->getID()]++;
			}

			int tmp_count_queued_units=0;
			int tmp_count_idle_proc_fac=0;

			// let us also look at the unit orders..
			// we can get an idea of how many orders are being executed by each player
			for (std::set<BWAPI::Unit*>::const_iterator u=(*p)->getUnits().begin();u!=(*p)->getUnits().end();u++){
			
				if ((*u)->getType().isBuilding() && (*u)->getType().canProduce()){
					
					if (!(*u)->isTraining())
						tmp_count_idle_proc_fac++;
					else{
						// -1 because training queue includes the currently trained unit
						tmp_count_queued_units+=((*u)->getTrainingQueue().size()-1);
					}
				}

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

			idleProdFacTotal[(*p)->getID()] += tmp_count_idle_proc_fac;
			queuedUnitsTotal[(*p)->getID()] += tmp_count_queued_units;
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



	updateBattles();
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
	//Broodwar->printf("Doing the dump!");

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
				// resources that were refunded, gives a sense of how much cancelling a player has done
				writeData << "refunded_minerals:" << (*p)->refundedMinerals() << ",";
				writeData << "refunded_gas:" << (*p)->refundedGas() << ",";
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

				//how many units are incomplete i.e. being queued
				int incomplete_unit_count=0;

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

						// looking at units that are built by production facilities
						if (!protoss_units[i].isBuilding() && protoss_units[i].whatBuilds().first.isBuilding()){
							incomplete_unit_count+=(*p)->incompleteUnitCount(protoss_units[i]);
						}
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

						// looking at units that are built by production facilities
						if (!terran_units[i].isBuilding() && terran_units[i].whatBuilds().first.isBuilding()){
							incomplete_unit_count+=(*p)->incompleteUnitCount(terran_units[i]);
						}
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

						// looking at units that are built by production facilities
						if (!zerg_units[i].isBuilding() && zerg_units[i].whatBuilds().first.isBuilding()){
							incomplete_unit_count+=(*p)->incompleteUnitCount(zerg_units[i]);
						}
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
				int num_production_buildings=0;
				int num_idle_production_buildings=0;
				int num_queued_units=0;
				for (std::set<Unit*>::const_iterator u=(*p)->getUnits().begin();u!=(*p)->getUnits().end();u++){
					
					// how about checking number of production facilities, then comparing that to the incomplete unit counts
					if ((*u)->getType().isBuilding() && (*u)->getType().canProduce()){
						num_production_buildings++;

						if (!(*u)->isTraining())
							num_idle_production_buildings++;
						else{
							// -1 because training queue includes the currently trained unit
							num_queued_units+=((*u)->getTrainingQueue().size()-1);
						}
					}
					
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

				// supply features
				writeData << "available_supply:";
				writeData << (*p)->supplyTotal();
				writeData << ",";

				writeData << "currently_used_supply:" << (*p)->supplyUsed() << ",";

				writeData << "current_unused_supply:" << (*p)->supplyTotal()-(*p)->supplyUsed() << ",";

				// having to do with not being able to produce units
				writeData << "frames_that_supply_is_maxed_out:" << supplyUsedUpFrameCount[(*p)->getID()] << ",";

				// using incomplete unit count and number of production facilities we can say how many units are being queued and how many production facitilites are idle
				int queueCount=0;
				int idleProductionFacCount=0;
				if (incomplete_unit_count>num_production_buildings)
					queueCount=incomplete_unit_count-num_production_buildings;
				if (incomplete_unit_count<num_production_buildings)
					idleProductionFacCount=num_production_buildings-incomplete_unit_count;

				// this is confusing, but this is the number of queued units more than there are production facilities
				writeData << "num_units_queued_more_than_fac:" << queueCount << ",";
				writeData << "num_idle_prod_fac_less_than_queued:" << idleProductionFacCount << ",";

				writeData << "num_queued_units:" << num_queued_units << ",";
				writeData << "num_idle_production_facilities:" << num_idle_production_buildings << ",";

				writeData << "avg_queued_units:" << (float)queuedUnitsTotal[(*p)->getID()]/(float)avg_unspent_count << ",";
				writeData << "avg_idle_prod_facilities:" << (float)idleProdFacTotal[(*p)->getID()]/(float)avg_unspent_count << ",";


				// finally, lets add in the game score features, these are for a baseline
				writeData << "unit_score:" << (*p)->getUnitScore() << ",";
				writeData << "kill_score:" << (*p)->getKillScore() << ",";
				writeData << "building_score:" << (*p)->getBuildingScore() << ",";
				writeData << "razing_score:" << (*p)->getRazingScore();

				// the resource features were already added

				writeData << "\n";

				
			}
		}

}

void SCFeatureExtractor::onEnd(bool isWinner){
	writeBattleData << "endtime:" << Broodwar->getFrameCount() << "\n";
	if (trackedBattles.empty())
		writeBattleData << "no open battles\n";
	else{
		writeBattleData << "open battles\n";
		
		for(std::list<battle>::iterator i=trackedBattles.begin();i!=trackedBattles.end();i++){
			endBattle((*i),Broodwar->getFrameCount());
		}

	}
	
	writeData.close();
	writeBattleData.close();
}


