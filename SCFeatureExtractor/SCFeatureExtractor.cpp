// SCFeatureExtractor.cpp : Defines the exported functions for the DLL application.
//
#include <assert.h>
#include "SCFeatureExtractor.h"
using namespace BWAPI;

// this is how often (in terms of frames) that we should dump a feature vector
// increase for less examples, decrease for more
const int dump_frame_period = 240; 

void SCFeatureExtractor::onStart()
{
	//Broodwar->printf("Hello World!");
	if (Broodwar->isReplay()){
		// make sure the replay is viewed quickly
		//Broodwar->setGUI(false);
		Broodwar->setLocalSpeed(0);

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
			}
		}
		
		avg_unspent_count=0;
	}

}

void SCFeatureExtractor::onFrame()
{
	// first do any state updates that need to be done
	for(std::set<Player*>::iterator p=Broodwar->getPlayers().begin();p!=Broodwar->getPlayers().end();p++){
		// account for the neutral player
		if ((*p)->getID()!=-1){
			avg_unspent_mineral_total[(*p)->getID()]+=(*p)->minerals();
			avg_unspent_gas_total[(*p)->getID()]+=(*p)->gas();
		}
	}
		
	avg_unspent_count+=1;

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

				writeData << "\n";
			}
		}

}

void SCFeatureExtractor::onEnd(){
	writeData.close();
}


