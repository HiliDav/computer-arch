/* 046267 Computer Architecture - HW #1                                 */
/* This file should hold your implementation of the predictor simulator */

#include "bp_api.h"
#include <vector>
#include <math.h>
#include <iostream>

// ------------------------------------------------------------------------------
// line in the BTB that describes a branch
// ------------------------------------------------------------------------------
class Branch {
    public:
        uint32_t tag;
        uint32_t target_pc;
        bool valid;

        Branch() {
            tag = -1;
            target_pc = -1;
            valid = false;
        }

        ~Branch() {}
};


// ------------------------------------------------------------------------------
// The Branch Predictor
// ------------------------------------------------------------------------------
unsigned branch_count;  // number of branches the predictor took care of.
unsigned flush_count;

unsigned BTB_size;
unsigned hist_size;
unsigned tag_size;
bool is_global_hist;
bool is_global_table;
unsigned fsm_state;     // 0 - SNT, 1 - WNT, 2 - WT, 3 - ST 
int shared;             // 0 - not_using_share
                        // 1 - using_share_lsb
                        // 2 - using_share_mid
std::vector<Branch>           BTB;
std::vector<std::vector<int>> hist_list;
std::vector<std::vector<int>> FSM_table_list;

// function: get_BTB_entry_from_pc
int get_BTB_entry_from_pc(uint32_t pc){
    // mask to find the entry to BTB.
    // example: for 16 entries, mask = 32'b1111 = 15)
    int mask = BTB_size - 1;
    int entry = mask & (pc >> 2);  // 2 least significant bits are 0
    return entry;
}

// function: search_in_BTB
// Description: This function gets pc addr, and returns the iterator in BTB 
//              where one can find the pc (diract mapping).
//              The function doesn't check rather or not the tag of the pc 
//              match the tag in the table.
std::vector<Branch>::iterator search_in_BTB(uint32_t pc){
    uint32_t BTB_entry = get_BTB_entry_from_pc(pc);
    std::vector<Branch>::iterator it = BTB.begin() + BTB_entry;
    return it;
}


// function: get_tag_from_pc
// Description: This funcrion shift the pc addr 2 bits (aligned to 4)
//              and another log2(BTB_size) bits for the BTB entry.
//              To get the tag, mask it with the tag size
uint32_t get_tag_from_pc(uint32_t pc){
    uint32_t pc_shifted = pc >> (2 + (int)log2(BTB_size));     
    uint32_t tag = pc_shifted & (int)(pow(2, tag_size) - 1);
    return tag;
}          


// function: hist_vec_to_number
// Description: Gets the history vector, returns it as a number.
//              If share defined, use the pc address to make xor. 
int hist_vec_to_number(std::vector<int> history, uint32_t pc){
    int history_number = 0;
    int index = hist_size - 1;
    for(int num : history){
        history_number += num * (int)pow(2,index);
        index--;
    }

    // 0. not using share, return history number.    
    
    // 1. using_share_lsb
    int xored_hist_num;
    if(shared == 1 && is_global_table){
        xored_hist_num = history_number ^ (pc >> 2);                         //xor operation
        history_number = xored_hist_num & ((int)pow(2,hist_size) - 1);  //align to history size
    }
    // 2. using_share_mid
    else if(shared == 2 && is_global_table){
        xored_hist_num = history_number ^ (pc >> (16));                      //xor operation
        history_number = xored_hist_num & ((int)pow(2,hist_size) - 1);  //align to history size
    }

    return history_number;
}


// function: update_fsm
// Description: to use it, need to add it's return value to the fsm current state.
int update_fsm(bool taken, unsigned state){
    if(taken && state < 3) return 1;

    else if((!taken) && state > 0) return -1;

    else return 0;
}



int BP_init(unsigned btbSize, unsigned historySize, unsigned tagSize, unsigned fsmState,
			bool isGlobalHist, bool isGlobalTable, int isShare){
    //validity check of inputs
    if(!(btbSize == 1 || btbSize == 2 || btbSize == 4 || btbSize == 8 || btbSize == 16 || btbSize == 32))
        return -1;
    if(historySize > 8 || historySize < 1)
        return -1;
    if((tagSize > 30 - log2(btbSize)) || (tagSize < 0))
        return -1;
    if((fsmState > 3) || (fsmState < 0))
        return -1;
    if((isShare != 0) && (isShare != 1) && (isShare != 2))
        return -1;
    
    // define the predictor
    branch_count = 0;
    flush_count = 0;
    
    BTB_size = btbSize;
    hist_size = historySize;
    tag_size = tagSize;
    is_global_hist = isGlobalHist;
    is_global_table = isGlobalTable;
    fsm_state = fsmState;    
    shared = isShare;             
    
    // initialize empty BTB with size btbSize
    for(unsigned i = 0; i < btbSize; i++){
        Branch empty_branch;
        BTB.push_back(empty_branch);
    }

    // initialize history list
    //  1. initialize history vector filled with 0
    std::vector<int> history0;
    for(unsigned i = 0; i < historySize; i++){
        history0.push_back(0);
    }

    //  2. initialize Global / Local history list
    if(is_global_hist){
        hist_list.push_back(history0);
    }
    else{ //Local history
        for(unsigned i = 0; i < btbSize; i++){
            hist_list.push_back(history0);
        }
    }
    
    // initialize FSMs tables list
    //  1. initialize FSMs table filled with fsmState
    std::vector<int> FSMs_table0;
    for(int i = 0; i <(int)(pow(2,historySize)); i++){
        FSMs_table0.push_back(fsm_state);
    }

    //  2. initialize Global / Local history list
    if(is_global_table){
        FSM_table_list.push_back(FSMs_table0);
    }
    else{   //Local tables
        for(unsigned i = 0; i < BTB_size; i++){
            FSM_table_list.push_back(FSMs_table0);
        }
    }
    
	return 0;
}




bool BP_predict(uint32_t pc, uint32_t *dst){

    std::vector<Branch>::iterator it = search_in_BTB(pc);
    int FSMs_table_entry;
    int state = fsm_state >> 1;

    // The branch doesn't exists in the BTB
    if(it->tag != get_tag_from_pc(pc) || it->valid == false)
    {
        *dst = pc + 4;
        return false;
    }

    // The branch exists in the BTB
    int BTB_entry = get_BTB_entry_from_pc(pc);

    if(is_global_hist)
    {
        FSMs_table_entry = hist_vec_to_number(hist_list[0], pc);

        if(is_global_table)
        {
            // Global history, Global fsm table
            state = FSM_table_list[0][FSMs_table_entry];
        }
        else
        {
            // Global history, Local fsm table
            state = FSM_table_list[BTB_entry][FSMs_table_entry];
        }
    }
    else
    {   
        FSMs_table_entry = hist_vec_to_number(hist_list[BTB_entry], pc);
        if(is_global_table)
        {
            // Local history, Global fsm table
            state = FSM_table_list[0][FSMs_table_entry];
        }
        else
        {
            // Local history, Local fsm table
            state = FSM_table_list[BTB_entry][FSMs_table_entry];
        }
    }

    // Branch not taken
    if(state == 0 || state == 1)
    {
        *dst = pc + 4;
        return false;
    }
    // Branch taken
    else
    {
        *dst = BTB[BTB_entry].target_pc;
        return true;
    }
}



// 1. update BTB
// 2. update fsms
// 3. update hist
void BP_update(uint32_t pc, uint32_t targetPc, bool taken, uint32_t pred_dst){
	
    branch_count++;

    int entry = get_BTB_entry_from_pc(pc);      // entry to BTB, history list, tables list
    uint32_t pc_tag = get_tag_from_pc(pc);      
    std::vector<Branch>::iterator it = search_in_BTB(pc);
    //bool prediction;

    int history_num;
    unsigned state;
    bool new_branch = 0;
  
    std::vector<int> FSMs_table0;
    std::vector<int> history0;



    // 1. BTB update & flush count (in case of miss)


    // The Branch row is empty. no need to initialize history and fsm.
    if(!(it->valid))
    {
        it->valid = true;
        it->tag = pc_tag;
        it->target_pc = targetPc;
    }
    // Branch row is used. need to check if it's the right tag.
    // if it isn't the same tag - need to initialize history and fsms (in case local)
    // if it's the same tag - need to update dst only.
    else if(pc_tag != it->tag)
    {
        new_branch = true;  //flag to initialize history and fsm
        
        //initialize fsms table
        for(int i = 0; i < (int)(pow(2,hist_size)); i++){
            FSMs_table0.push_back(fsm_state);
        }

        // initialize history vector
        for(unsigned i = 0; i < hist_size; i++){
            history0.push_back(0);
        }

        it->tag = pc_tag;
        it->target_pc = targetPc;
    }
    else    //same tag
    {
        it->target_pc = targetPc;
    }


/*
    // Branch exists
    // if the tags doesn't match, update the whole Branch row
    if(pc_tag != it->tag)
    {
        it->tag = pc_tag;
        it->target_pc = targetPc;
    }

    // tags match - there is relevant row in BTB
    else
    {
        // targets doesn't match - the prediction was a miss
        if(targetPc != it->target_pc)
        {
            flush_count++;
            it->target_pc = targetPc;
        }
        //targets match. check if prediction was hit
        else
        {
            uint32_t dummy;
            prediction = BP_predict(pc, &dummy);
            // if prediction was a miss, need to add flush count
            if(prediction != taken)
                flush_count++;
        }
    } 
*/

    // 2. FSMs tables list update
    if(is_global_table)
    {
        if(is_global_hist)
        {
            // Global history, Global FSMs table 
            history_num = hist_vec_to_number(hist_list[0],pc);
            state = FSM_table_list[0][history_num];
            FSM_table_list[0][history_num] += update_fsm(taken, state);
        }
        else
        {
            // Local history, Global FSMs table 
            history_num = new_branch ? hist_vec_to_number(history0,pc) : hist_vec_to_number(hist_list[entry],pc);
            state = FSM_table_list[0][history_num];
            FSM_table_list[0][history_num] += update_fsm(taken, state);
        }
    }
    else if(new_branch) // Local FSMs tables and new branch
    {
        if(is_global_hist)
        {
            // Global history, Local FSMs table 
            history_num = hist_vec_to_number(hist_list[0],pc);
            FSM_table_list[entry] = FSMs_table0;
            FSM_table_list[entry][history_num] += update_fsm(taken, fsm_state);
        }
        else
        {
            // Local history, Local FSMs table 
            history_num = hist_vec_to_number(history0,pc);
            FSM_table_list[entry] = FSMs_table0;
            FSM_table_list[entry][history_num] += update_fsm(taken, fsm_state);
        }

    }
    else // Local FSMs tables and not new branch
    {
        if(is_global_hist)
        {
            // Global history, Local FSMs table 
            history_num = hist_vec_to_number(hist_list[0],pc);  
            state = FSM_table_list[entry][history_num];
            FSM_table_list[entry][history_num] += update_fsm(taken, state);
        }
        else
        {
            // Local history, Local FSMs table 
            history_num = hist_vec_to_number(hist_list[entry],pc);
            state = FSM_table_list[entry][history_num];
            FSM_table_list[entry][history_num] += update_fsm(taken, state);
        }
    }

    // 3. update history list
    if(is_global_hist)
    {
        hist_list[0].erase(hist_list[0].begin());
        hist_list[0].push_back(taken);
    }
    // Local history and new branch
    else if (new_branch)
    {
        hist_list[entry] = history0;
        hist_list[entry].erase(hist_list[entry].begin());
        hist_list[entry].push_back(taken);    
    }
    // Local history and NOT new branch
    else
    {
        hist_list[entry].erase(hist_list[entry].begin());
        hist_list[entry].push_back(taken);
    }

    // flush update
    if (pred_dst != targetPc && taken)
	{
		flush_count++;
	}
	else if (pred_dst != (pc + 4) && !taken)
	{
		flush_count++;
	}

    return;
}
// flush_num: 4, br_num: 9, size: 134b

void BP_GetStats(SIM_stats *curStats){
    curStats->br_num = branch_count;
    curStats->flush_num = flush_count;

    // calculate theoretical memory use
    int BTB_final_size = BTB_size * (tag_size + 30 + 1);    //30 for pc size, 1 for valid bit
    if(is_global_hist)
    {
        BTB_final_size += hist_size;
    }
    else
    {
        BTB_final_size += (hist_size * BTB_size);
    }

    if(is_global_table)
    {
        BTB_final_size += pow(2, hist_size) * 2; //*2 becouse each fsm takes two bits.
    }
    else
    {
        BTB_final_size += (BTB_size * pow(2, hist_size) * 2);  //*2 becouse each fsm takes two bits.
    }
    
    curStats->size = BTB_final_size;

	return;
}

