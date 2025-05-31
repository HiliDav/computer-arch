/* 046267 Computer Architecture - Winter 20/21 - HW #2 */

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

class set{
	public:
		unsigned long int address;
		int tag;
		int block;
		char dirty_bit;
		int counter;
		char valid;

		set(){
			counter = 0;
			dirty_bit = 0;
			valid = 0;
		}
};

class cache{
	public:
		unsigned memory_cycles;
		unsigned block_size;
		unsigned write_allocate;
		unsigned L1_size;
		unsigned L1_ass;
		unsigned L1_cycles;
		unsigned L2_size;
		unsigned L2_ass;
		unsigned L2_cycles;
		unsigned L1_sets;
		unsigned L2_sets;
		unsigned L1_num_sets;
		unsigned L2_num_sets;

		unsigned l1_hits;
		unsigned l1_total;
		unsigned l2_hits;
		unsigned l2_total;
		unsigned mem_total;

		vector< vector < set > > L1;
		vector< vector < set > > L2;

		cache(		
			unsigned mem_cycles, unsigned b_size, unsigned w_allocate, unsigned L1_s, unsigned L1_asso, unsigned L1_cyc,
			unsigned L2_s, unsigned L2_asso, unsigned L2_cyc)
			{
				memory_cycles = mem_cycles;
				block_size = b_size;
				write_allocate = w_allocate;
				L1_size = L1_s;
				L1_ass = L1_asso;
				L1_cycles = L1_cyc;
				L2_size = L2_s;
				L2_ass = L2_asso;
				L2_cycles = L2_cyc;

				l1_hits = 0;
				l1_total = 0;
				l2_hits = 0;
				l2_total = 0;
				mem_total = 0;

				L1 = vector< vector < set > > ((int)(pow(2,L1_ass)));
				L1_sets = L1_size - block_size - L1_ass;
				L1_num_sets =  ((int)(pow(2,L1_sets)));
				for (int i=0; i<((int)(pow(2,L1_ass))); i++){
					L1[i] = vector < set >(L1_num_sets);
				}
				L2 = vector< vector < set > > ((int)(pow(2,L2_ass)));
				L2_sets = L2_size - block_size - L2_ass;
				// cout << "L2_sets log: " << L2_sets << "\n";
				L2_num_sets =  ((int)(pow(2,L2_sets)));
				// cout << "L2_sets dec: " << L2_num_sets << "\n";
				for (int i=0; i<((int)(pow(2,L2_ass))); i++){
					L2[i] = vector < set >(L2_num_sets);
				}
		};

		double get_avg(){
			return ((((double)l1_total) * L1_cycles) + (l2_total * L2_cycles) + ((mem_total) * memory_cycles)) / ((double)l1_total);
		};
		
		int check_cache(unsigned long int tag, unsigned long int set_index, vector< vector < set > > &chache_level, unsigned ways_log){
			int way_index = -1;
			for (int way=0; way < ((int) pow(2,ways_log)); way++){
				if ((chache_level[way][set_index].tag == tag) && (chache_level[way][set_index].valid == 1)){
					chache_level[way][set_index].counter = 0;
					way_index = way;
				}
				else{
					chache_level[way][set_index].counter++;
				}
			}
			// cout << "check cache: " << way_index << "\n";
			return way_index;	
		}

		void add_to_cache_LRU(unsigned long int new_address, unsigned long int tag, unsigned long int set_index, vector< vector < set > > &chache_level, char is_dirty, unsigned long int &removed_address, char &was_valid, char &was_dirty, unsigned ways_log){
			int oldest_way = 0;
			int oldest_counter = 0;
			for (int way=0; way < ((int) pow(2,ways_log)); way++){
				if (chache_level[way][set_index].counter > oldest_counter){
					oldest_counter = chache_level[way][set_index].counter;
					oldest_way = way;
				}
			}
			removed_address = chache_level[oldest_way][set_index].address;
			was_valid = chache_level[oldest_way][set_index].valid;
			was_dirty =  chache_level[oldest_way][set_index].dirty_bit;

			// cout << "LRU removed way: " << oldest_way << "\n";
			
			chache_level[oldest_way][set_index].counter = 0;
			chache_level[oldest_way][set_index].tag = tag;
			chache_level[oldest_way][set_index].valid = 1;
			chache_level[oldest_way][set_index].dirty_bit = is_dirty;
			chache_level[oldest_way][set_index].address = new_address;
		}

		void update_lower_cache(unsigned long int removed_address){
			unsigned long int r_tag = (removed_address >> block_size) >> L2_sets;
			unsigned long int r_set = (removed_address >> block_size) % (L2_num_sets);
			for (int way=0; way < ((int) pow(2,L2_ass)); way++){
				if (L2[way][r_set].tag == r_tag && L2[way][r_set].valid ==1){
					L2[way][r_set].counter = 0;
					// chache_level[set_index][set_index].tag = tag;
					L2[way][r_set].valid = 1;
					L2[way][r_set].dirty_bit = 1;
				}
				else{
					L2[way][r_set].counter++;
				}
			}
		}

		bool update_higher_cache(unsigned long int removed_address, unsigned long int new_tag, unsigned long int new_address, int is_dirty){
			unsigned long int r_tag = (removed_address >> block_size) >> L1_sets;
			unsigned long int r_set = (removed_address >> block_size) % (L1_num_sets);
			bool success = false;
			for (int way=0; way < ((int) pow(2,L1_ass)); way++){
				if (L1[way][r_set].tag == r_tag && L1[way][r_set].valid == 1){
					L1[way][r_set].counter = 0;
					L1[way][r_set].tag = new_tag;
					L1[way][r_set].valid = 1;
					L1[way][r_set].dirty_bit = is_dirty;
					L1[way][r_set].address = new_address;
					success = true;
				}
			}
			return success;
		}

		void read(unsigned long int dec_address){

			unsigned long int L1_tag = (dec_address >> block_size) >> L1_sets;
			unsigned long int L2_tag = (dec_address >> block_size) >> L2_sets;

			unsigned long int L1_set = (dec_address >> block_size) % (L1_num_sets);
			unsigned long int L2_set = (dec_address >> block_size) % (L2_num_sets);

			int way_index;
			// check L1
			// printf("totall1: %d\n", l1_total);
			l1_total++;
			way_index = check_cache(L1_tag, L1_set, L1, L1_ass);
			if (way_index != -1){
				l1_hits++;
				// add cycles
				return;
			}

			unsigned long int removed_address;
			char was_valid = 0;
			char was_dirty = 0;

			// check L2
			l2_total++;
			way_index = check_cache(L2_tag, L2_set, L2, L2_ass);
			if (way_index != -1){
				add_to_cache_LRU(dec_address, L1_tag, L1_set, L1, 0, removed_address, was_valid, was_dirty, L1_ass);
				if (was_dirty && was_valid){
					update_lower_cache(removed_address);
				}
				l2_hits++;
				// add cycles
				return;
			}
			mem_total++;
			bool success = false;
			add_to_cache_LRU(dec_address, L2_tag, L2_set, L2, 0, removed_address, was_valid, was_dirty, L2_ass);
			if (was_valid){
				success = update_higher_cache(removed_address, L1_tag, dec_address, 0);
			}
			if (!success){
				add_to_cache_LRU(dec_address, L1_tag, L1_set, L1, 0, removed_address, was_valid, was_dirty, L1_ass);
				if (was_valid && was_dirty){
					unsigned long int r_tag = (removed_address >> block_size) >> L2_sets;
					unsigned long int r_set = (removed_address >> block_size) % (L2_num_sets);

					for (int way=0; way < ((int) pow(2,L2_ass)); way++){
						if (L2[way][r_set].tag == r_tag && L2[way][r_set].valid == 1){
							L2[way][r_set].counter = 0;
							L2[way][r_set].dirty_bit = was_dirty;
						}
						else{
							L2[way][r_set].counter++;
						}
					}
				}
			}
			//add_mem_cycles
		}

		void write(unsigned long int dec_address){

			unsigned long int L1_tag = (dec_address >> block_size) >> L1_sets;
			unsigned long int L2_tag = (dec_address >> block_size) >> L2_sets;

			unsigned long int L1_set = (dec_address >> block_size) % (L1_num_sets);
			unsigned long int L2_set = (dec_address >> block_size) % (L2_num_sets);
			
			int way_index;
			// check L1
			l1_total++;
			way_index = check_cache(L1_tag, L1_set, L1, L1_ass);
			if (way_index != -1){
				L1[way_index][L1_set].dirty_bit = 1;
				l1_hits++;
				// add cycles
				return;
			}
			// cout << "not L1" << "\n";
			unsigned long int removed_address;
			char was_valid = 0;
			char was_dirty = 0;

			// check L2
			l2_total++;
			way_index = check_cache(L2_tag, L2_set, L2, L2_ass);
			if (way_index != -1){
				l2_hits++;
				if (write_allocate){
					add_to_cache_LRU(dec_address, L1_tag, L1_set, L1, 1, removed_address, was_valid, was_dirty, L1_ass);
					if (was_valid && was_dirty){ // was_dirty here?
						unsigned long int r_tag = (removed_address >> block_size) >> L2_sets;
						unsigned long int r_set = (removed_address >> block_size) % (L2_num_sets);

						for (int way=0; way < ((int) pow(2,L2_ass)); way++){
							if (L2[way][r_set].tag == r_tag && L2[way][r_set].valid == 1){
								L2[way][r_set].counter = 0;
								L2[way][r_set].dirty_bit = was_dirty;
							}
							else{
								L2[way][r_set].counter++;
							}
						}
					}
				}
				else{
					L2[way_index][L2_set].dirty_bit = 1;
					L2[way_index][L2_set].counter = 0;
				}
				// add cycles
				return;
			}
			mem_total++;
			// cout << "not L2" << "\n";
			if (write_allocate){
				bool success = false;
				add_to_cache_LRU(dec_address, L2_tag, L2_set, L2, 0, removed_address, was_valid, was_dirty, L2_ass);
				if (was_valid){
					success = update_higher_cache(removed_address, L1_tag, dec_address, 1);
				}

				if (!success){
					add_to_cache_LRU(dec_address, L1_tag, L1_set, L1, 1, removed_address, was_valid, was_dirty, L1_ass);
					if (was_valid && was_dirty){ // was_dirty here?
						unsigned long int r_tag = (removed_address >> block_size) >> L2_sets;
						unsigned long int r_set = (removed_address >> block_size) % (L2_num_sets);

						for (int way=0; way < ((int) pow(2,L2_ass)); way++){
							if (L2[way][r_set].tag == r_tag && L2[way][r_set].valid == 1){
								L2[way][r_set].counter = 0;
								L2[way][r_set].dirty_bit = was_dirty;
							}
							else{
								L2[way][r_set].counter++;
							}
						}
					}
				}
			}
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

		// DEBUG - remove this line
		// cout << "operation: " << operation;

		string cutAddress = address.substr(2); // Removing the "0x" part of the address

		// DEBUG - remove this line
		// cout << ", address (hex)" << cutAddress;

		unsigned long int num = 0;
		num = strtoul(cutAddress.c_str(), NULL, 16);
		// DEBUG - remove this line
		// cout << " (dec) " << num << endl;
		
		if (operation == 'r'){
			// printf("aaaa\n");
			my_cache.read(num);
		}
		else{
			// printf("bbbbbb\n");
			my_cache.write(num);
		}

	}

	double L1MissRate;
	double L2MissRate;
	double avgAccTime;

	avgAccTime = my_cache.get_avg();
	L1MissRate = ((double)(my_cache.l1_total - my_cache.l1_hits)) / my_cache.l1_total;
	L2MissRate = ((double)(my_cache.l2_total - my_cache.l2_hits)) / my_cache.l2_total;

	// printf("l1miss=%d ", my_cache.l1_total - my_cache.l1_hits);
	// printf("l1tot=%d ", my_cache.l1_total);
	// printf("l2tot=%d ", my_cache.l2_total);
	// printf("l2hits=%d\n", my_cache.l2_hits);
	// printf("l1miss=%d ", my_cache.l1_total - my_cache.l1_hits);
    // printf("L1tot=%d ", my_cache.l1_total);
    // printf("l2miss=%d ", my_cache.l2_total - my_cache.l2_hits);
    // printf("L2tot=%d\n", my_cache.l2_total);


	printf("L1miss=%.03f ", L1MissRate);
	printf("L2miss=%.03f ", L2MissRate);
	printf("AccTimeAvg=%.03f\n", avgAccTime);

	return 0;
};
