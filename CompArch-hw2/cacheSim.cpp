/* 046267 Computer Architecture - Spring 25 - HW #2 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <math.h>

using std::FILE;
using std::string;
using std::cout;
using std::endl;
using std::cerr;
using std::ifstream;
using std::stringstream;
using std::vector;

class cache_block {
	public:
	unsigned long int addr;
	int tag;
	int data_block;
	int counter;		// used for LRU policy
	bool dirty_bit;
	bool valid;

	cache_block(){
		counter = 0;
		dirty_bit = 0;
		valid = 0;
	}
};


//		L1 Cache: vec < vec <cache_clock> >  
//                  ___________________________________________
//				   |way\set| 000 | 001 | ...                   |
//			       | way 0 |     |     |                       |
//				   | way 1 |     |     |                       |
//				   |   :   |  :  |   : |                       |
//			       |_______|_____|_____|_______________________|

class cache {
	public:
	unsigned mem_cyc;		// Memory access cycles
	unsigned block_size;	// Block size in bytes
	unsigned write_allocate;	//1 - write allocate, 0 - no write allocate

	unsigned L1_size;		// log2 (L1 cache size in bytes)
	unsigned L1_assoc;		// log2 (L1 cache associativity) (number of ways)
	unsigned L1_cycles;		// log2 (L1 cache access cycles)

	unsigned L2_size;		// log2 (L2 cache size in bytes)
	unsigned L2_assoc;		// log2 (L2 cache associativity) (number of ways)
	unsigned L2_cycles;		// log2 (L2 cache access cycles)

	unsigned L1_sets;		// log2 (Number of sets in L1 cache)
	unsigned L2_sets;		// log2 (Number of sets in L2 cache)
	unsigned L1_num_sets;	// Number of sets in L1 cache (2^L1_sets)
	unsigned L2_num_sets;	// Number of sets in L2 cache (2^L2_sets)

	unsigned L1_hits;		// Number of hits in L1 cache
	unsigned L1_total;		// Total number of accesses to L1 cache
	unsigned L2_hits;		// Number of hits in L2 cache
	unsigned L2_total;		// Total number of accesses to L2 cache
	unsigned mem_total;		// Total number of accesses to memory

	vector <vector <cache_block> > L1;
	vector <vector <cache_block> > L2;

	// ----------------------------------------
	// Function: cache
	// ----------------------------------------
	// Description: Constructor to initialize the cache
	cache(unsigned mem_cycles, unsigned b_size, unsigned w_allocate, unsigned L1_s, unsigned L1_asso, unsigned L1_cyc,
			unsigned L2_s, unsigned L2_asso, unsigned L2_cyc) {
		
		mem_cyc = mem_cycles;
		block_size = b_size;
		write_allocate = w_allocate;

		//---------------------------------
		// Initialize L1 cache
		//---------------------------------
		L1_size = L1_s;
		L1_assoc = L1_asso;
		L1_cycles = L1_cyc;
		L1_sets = L1_size - block_size - L1_assoc;	
		L1_num_sets = (unsigned) pow(2, L1_sets);	// Number of sets (number of blocks can fit in one way)

		unsigned L1_num_ways = (unsigned) pow(2, L1_assoc); // Number of ways in L1 cache
		
		L1 = vector <vector <cache_block> > (L1_num_ways);
		for (int i = 0; i < L1_num_ways; i++)
		{
			L1[i] = vector<cache_block>(L1_num_sets);
		}
		
		//---------------------------------
		// Initialize L2 cache
		//---------------------------------
		L2_size = L2_s;
		L2_assoc = L2_asso;
		L2_cycles = L2_cyc;
		L2_sets = L2_size - block_size - L2_assoc;
		L2_num_sets = (unsigned) pow(2, L2_sets);

		unsigned L2_num_ways = (unsigned) pow(2, L2_assoc); // Number of ways in L2 cache

		L2 = vector <vector <cache_block> > (L2_num_ways);
		for (int i = 0; i < L2_num_ways; i++)
		{
			L2[i] = vector<cache_block>(L2_num_sets);
		}

		//---------------------------------
		// Initialize counters
		//---------------------------------
		L1_hits = 0;
		L1_total = 0;
		L2_hits = 0;
		L2_total = 0;
		mem_total = 0;
	}
	
	// ----------------------------------------
	// Function: find_in_cache
	// ----------------------------------------
	// Description: This function will be used to check if a block is in the cache.
	// 				It will return the way nember if the block is found, or -1 if not found.
	// 				It will also update the LRU counter for the blocks in the set.
	int find_in_cache(vector <vector <cache_block> > &cache, unsigned long int tag, unsigned long int set_index, unsigned assoc) {
		int way_index = -1; // -1 means not found
		for (unsigned way = 0; way < (unsigned) pow(2, assoc); way++) {
			
			if (cache[way][set_index].valid && cache[way][set_index].tag == tag) {
				cache[way][set_index].counter = 0; //  Block found, Reset counter for LRU policy
				way_index = way;	
			}
			else {
				cache[way][set_index].counter++; // Increment counter for LRU policy
			}
		}
		return way_index; 
	}

	// ----------------------------------------
	// Function: add_to_cache
	// ----------------------------------------
	// Description: This function will add a block to the cache using LRU policy.
	void add_to_cache(vector <vector <cache_block> > &cache, unsigned long int new_addr, unsigned int tag, unsigned long int set_index, unsigned assoc,
			bool is_dirty, unsigned long int &removed_addr, bool &was_valid, bool &was_dirty) {
		// Find the way with the highest counter (oldest block in the set)
		int oldest_way = 0;
		int oldest_counter = 0;
		for (unsigned way = 0; way < (unsigned) pow(2, assoc); way++) {
			if (cache[way][set_index].counter > oldest_counter) {
				oldest_counter = cache[way][set_index].counter;
				oldest_way = way;
			}
		}

		// in case of replacement, save the removed block's address, valid and dirty status
		removed_addr = cache[oldest_way][set_index].addr;
		was_valid = cache[oldest_way][set_index].valid;
		was_dirty = cache[oldest_way][set_index].dirty_bit;

		// Replace the block in the oldest way
		cache[oldest_way][set_index].addr = new_addr; 		// Update the address
		cache[oldest_way][set_index].tag = tag;
		cache[oldest_way][set_index].counter = 0; 			// Reset counter for LRU policy
		cache[oldest_way][set_index].dirty_bit = is_dirty; 
		cache[oldest_way][set_index].valid = true; 		
	}


	// ----------------------------------------
	// Function: update_L1_to_L2_cache
	// ----------------------------------------
	// Description: This function update the lower cache (L2 in this case) with the removed block's address from L1.
	// 				It also mark the block as dirty and valid.
	// 				It is guaranteed that the removed block is in the L2 cache since the cache is Inclusive.
	void update_L1_to_L2_cache(unsigned long int removed_address, bool was_dirty = true) {
		// Update the lower cache (L2) with the removed block's address
		unsigned long int rm_tag = (removed_address >> block_size) >> L2_sets;
		unsigned long int rm_set = (removed_address >> block_size) % (L2_num_sets);
		
		for (unsigned way = 0; way < (unsigned) pow(2, L2_assoc); way++) {
			if (L2[way][rm_set].tag == rm_tag && L2[way][rm_set].valid) {
				L2[way][rm_set].counter = 0; 			// Reset counter for LRU policy
				L2[way][rm_set].dirty_bit = was_dirty; 	// Mark as dirty in default
			} else {
				L2[way][rm_set].counter++; 			// Increment counter for LRU policy
			}
		}
	}

	// ----------------------------------------
	// Function: evict_and_update_L1
	// ----------------------------------------
	// Description: This function will evict a block from L1 cache and update it with the new block.
	//				The exicted block is the block evicted from L2, and the new block will be a block that was added to L2 cache instead.
	//				It return True in case the removed address was found in L1 cache.
	bool evict_and_update_L1(unsigned long int removed_address, unsigned long int new_tag, unsigned long int new_address, int is_dirty){
		unsigned long int rm_tag = (removed_address >> block_size) >> L1_sets;
		unsigned long int rm_set = (removed_address >> block_size) % (L1_num_sets);

		for (unsigned way = 0; way < (unsigned) pow(2, L1_assoc); way++) {
			if (L1[way][rm_set].tag == rm_tag && L1[way][rm_set].valid) {	//addr was found in L1 cache
				L1[way][rm_set].counter = 0; 			// Reset counter for LRU policy
				L1[way][rm_set].tag = new_tag; 			// Update the tag
				L1[way][rm_set].valid = true; 			// Mark as valid
				L1[way][rm_set].dirty_bit = is_dirty; 	// Mark as dirty if wtire operation
				L1[way][rm_set].addr = new_address; 	// Update the address
				return true; // Success
			}
		}
		return false; // Not found in L1 cache
	}



	// ----------------------------------------
	// Function: read
	// ----------------------------------------
	// 		address built of:
	// 			1. block offset (log2(block_size) bits)
	// 			2. set index (log2(number of sets) bits)
	// 			3. tag (remaining bits)
	// 				in this order:	| tag | set index | block offset |
	void read(unsigned long int address){

		int way_index;

		// Calculate the tag
		unsigned long int L1_tag = (address >> block_size) >> L1_sets;
		unsigned long int L2_tag = (address >> block_size) >> L2_sets;

		// Calculate the set index
		unsigned long int L1_set = (address >> block_size) % (L1_num_sets);
		unsigned long int L2_set = (address >> block_size) % (L2_num_sets);
		
		// **************************
		// 1. Check L1 cache
		// **************************
		L1_total++;
		way_index = find_in_cache(L1, L1_tag, L1_set, L1_assoc);
		if (way_index != -1) {
			L1_hits++;		// Add cycles for L1 hit
			return;
		}

		unsigned long int removed_address;
		bool was_valid = false;
		bool was_dirty = false;

		// **************************
		// 2. L1 miss, Check L2 cache
		// **************************
		L2_total++;
		way_index = find_in_cache(L2, L2_tag, L2_set, L2_assoc);
		if (way_index != -1) {
			L2_hits++;
			//need to add to L1 cache
			add_to_cache(L1, address, L1_tag, L1_set, L1_assoc, false, removed_address, was_valid, was_dirty);
			if(was_valid && was_dirty) {
				// If the removed block from L1 was valid and dirty, we need to write it back to L2 cache.
				update_L1_to_L2_cache(removed_address);
			}
			return;
		}
		// **************************
		// 3. L2 miss, Access memory
		// **************************
		//  Need to add to L2 cache
		mem_total++;
		bool was_in_L1 = false;
		add_to_cache(L2, address, L2_tag, L2_set, L2_assoc, false, removed_address, was_valid, was_dirty);
		if(was_valid){
			// While updating, may evict block from L2.
			// If the removed block from L2 was valid, need to update:
			// in real implementation, we would check if the removed block is in L1 cache and if so, evacuate it to memory, to save the inclusive property.
			// in this case, write back time is not counted, so we just need to evacuate it from L1 cache, and enter the *new* block to L1.
			was_in_L1 = evict_and_update_L1(removed_address, L1_tag, address, false);
		}
		if(!was_in_L1) {
			// If the *removed* block from L2 was not in L1 cache, we need to add the *new* block to L1 cache (it didn't enter).
			add_to_cache(L1, address, L1_tag, L1_set, L1_assoc, false, removed_address, was_valid, was_dirty);
			if(was_valid && was_dirty) {
				// If removed block from L1 that was valid and dirty, we need to write it back to L2 cache.
				update_L1_to_L2_cache(removed_address, was_dirty);
			}
		}
	}


	// ----------------------------------------
	// Function: write
	// ----------------------------------------
	void write(unsigned long int address){

		int way_index;

		// Calculate the tag
		unsigned long int L1_tag = (address >> block_size) >> L1_sets;
		unsigned long int L2_tag = (address >> block_size) >> L2_sets;

		// Calculate the set index
		unsigned long int L1_set = (address >> block_size) % (L1_num_sets);
		unsigned long int L2_set = (address >> block_size) % (L2_num_sets);

		// **************************
		// 1. Check L1 cache
		// **************************
		L1_total++;
		way_index = find_in_cache(L1, L1_tag, L1_set, L1_assoc);
		if (way_index != -1) {
			// L1 hit, update the block as dirty & add cycles to hit counter
			L1[way_index][L1_set].dirty_bit = true; 
			L1_hits++;		
			return;
		}

		unsigned long int removed_address;
		bool was_valid = false;
		bool was_dirty = false;

		// **************************
		// 2. L1 miss, Check L2 cache
		// **************************
		L2_total++;
		way_index = find_in_cache(L2, L2_tag, L2_set, L2_assoc);
		if (way_index != -1) {
			L2_hits++;
			if(write_allocate){
				//need to add to L1 cache
				add_to_cache(L1, address, L1_tag, L1_set, L1_assoc, true, removed_address, was_valid, was_dirty);
				if(was_valid && was_dirty) {
					// If the removed block from L1 was valid and dirty, we need to write it back to L2 cache.
					update_L1_to_L2_cache(removed_address);
				}
			} else {
				// If no write allocate, just mark the block as dirty in L2 cache
				L2[way_index][L2_set].dirty_bit = true;
				L2[way_index][L2_set].counter = 0; 		// Reset counter for LRU policy
			}
			return;
		}

		// **************************
		// 3. L2 miss, Access memory
		// **************************
		mem_total++;
		if(write_allocate){
			bool was_in_L1 = false;
			add_to_cache(L2, address, L2_tag, L2_set, L2_assoc, false, removed_address, was_valid, was_dirty);
			if(was_valid){			
				// While updating, may evict block from L2.
				// If the removed block from L2 was valid, need to update:
				// in real implementation, we would check if the removed block is in L1 cache and if so, evacuate it to memory, to save the inclusive property.
				// in this case, write back time is not counted, so we just need to evacuate it from L1 cache, and enter the *new* block to L1.
				was_in_L1 = evict_and_update_L1(removed_address, L1_tag, address, true);
			}
			if(!was_in_L1) {
				// If the *removed* block from L2 was not in L1 cache, we need to add the *new* block to L1 cache (it didn't enter).
				add_to_cache(L1, address, L1_tag, L1_set, L1_assoc, true, removed_address, was_valid, was_dirty);
				if(was_valid && was_dirty) {
					// If removed block from L1 that was valid and dirty, we need to write it back to L2 cache.
					update_L1_to_L2_cache(removed_address, was_dirty);
				}
			}
		}
		// if no write allocate, no need to update caches.
	}


	// ----------------------------------------
	// Function: get_avg
	// ----------------------------------------
	// Description: This function calculates the average access time based on the hits and misses in L1, L2, and memory.
	double get_avg() {

		return ((((double)L1_total) * L1_cycles) + (L2_total * L2_cycles) + ((mem_total) * mem_cyc)) / ((double)L1_total);

	}
};



int main(int argc, char **argv) {

	if (argc < 19) {
		cerr << "Not enough arguments" << endl;
		return 0;
	}

	// Get input arguments

	// File
	// Assuming it is the first argument
	char* fileString = argv[1];
	ifstream file(fileString); //input file stream
	string line;
	if (!file || !file.good()) {
		// File doesn't exist or some other error
		cerr << "File not found" << endl;
		return 0;
	}

	unsigned MemCyc = 0, BSize = 0, L1Size = 0, L2Size = 0, L1Assoc = 0,
			L2Assoc = 0, L1Cyc = 0, L2Cyc = 0, WrAlloc = 0;

	for (int i = 2; i < 19; i += 2) {
		string s(argv[i]);
		if (s == "--mem-cyc") {
			MemCyc = atoi(argv[i + 1]);
		} else if (s == "--bsize") {
			BSize = atoi(argv[i + 1]);
		} else if (s == "--l1-size") {
			L1Size = atoi(argv[i + 1]);
		} else if (s == "--l2-size") {
			L2Size = atoi(argv[i + 1]);
		} else if (s == "--l1-cyc") {
			L1Cyc = atoi(argv[i + 1]);
		} else if (s == "--l2-cyc") {
			L2Cyc = atoi(argv[i + 1]);
		} else if (s == "--l1-assoc") {
			L1Assoc = atoi(argv[i + 1]);
		} else if (s == "--l2-assoc") {
			L2Assoc = atoi(argv[i + 1]);
		} else if (s == "--wr-alloc") {
			WrAlloc = atoi(argv[i + 1]);
		} else {
			cerr << "Error in arguments" << endl;
			return 0;
		}
	}

	// building the cache object
	cache my_cache = cache(MemCyc, BSize, WrAlloc, L1Size, L1Assoc, L1Cyc, L2Size, L2Assoc, L2Cyc);

	while (getline(file, line)) {

		stringstream ss(line);
		string address;
		char operation = 0; // read (R) or write (W)
		if (!(ss >> operation >> address)) {
			// Operation appears in an Invalid format
			cout << "Command Format error" << endl;
			return 0;
		}

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);

		if (operation == 'r') {
			// Read operation
			my_cache.read(num);
		} else {
			// Write operation
			my_cache.write(num);
		}
	}

	double L1MissRate;
	double L2MissRate;
	double avgAccTime;

	avgAccTime = my_cache.get_avg();
	L1MissRate = (double)(my_cache.L1_total - my_cache.L1_hits) / my_cache.L1_total;
	L2MissRate = (double)(my_cache.L2_total - my_cache.L2_hits) / my_cache.L2_total;

	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
}
