#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <vector>

using namespace std;

//each cache line has a size of 32 bytes 
void directMapped(char* in, ofstream& out, int size){
	string instr;
	unsigned long long addr;
	int totalBlocks = size / 32; //size of cache / bytes per block

	array<long long, 1024> cache;
	cache.fill(-1);
	
	ifstream input(in);
	int hit = 0;
	int access = 0;
	while (input >> instr >> hex >> addr ){
		access++;
		int blockAddr = addr / 32; //blockSize = 32
		int blockNum = blockAddr % totalBlocks;
		if (cache[blockNum] == -1){ //cold start miss
			cache[blockNum] = blockAddr/totalBlocks;
		}
		else if (cache[blockNum] == blockAddr/totalBlocks){
			hit++;
		}
		else{
			cache[blockNum] = blockAddr/totalBlocks; //miss
		}
	}
	if (size != 32768){
		out << hit << "," << access << "; ";
	}
	else{
		out << hit << "," << access << ";";
	}
}

void directMapped(char* in, ofstream& out){
	//PERFECTO
	directMapped(in, out, 1024);
	directMapped(in, out, 4096);
	directMapped(in, out, 16384);
	directMapped(in, out, 32768);
	out << endl;
}

void setAssociative(char * in, ofstream& out, int ways){
	//each cache line size is 32 bytes
	//16KB cache
	//least recently used replacement policy is implemented
	string instr;
	unsigned long long addr;
	int sets = 512 / ways; //(size of cache / bytes per block) / # ways

	array<long long, 512> cache;
	cache.fill(-1); //-1 is invalid
	array<int, 512> recency;
	recency.fill(0);
	
	ifstream input(in);
	int hit = 0;
	int access = 0;
	while (input >> instr >> hex >> addr ){
		access++;
		int blockAddr = addr / 32; //32 = blockSize
		int setNum = blockAddr % sets;
		int index = setNum * ways; //cache index
		bool inCache = false; //addr in cache
		int LRU = index;
		for (int i = 0; i < ways && !inCache; i++){
			if (cache[index] == -1){ //cold start miss
				cache[index] = blockAddr/sets;
				recency[index] = access;
				inCache = true;
			}
			else if (cache[index] == blockAddr/sets){ //cache hit
				hit++;
				recency[index] = access;
				inCache = true;
			}
			if (recency[index] < recency[LRU]){
				LRU = index;
			}
			index++;
		}
		if (!inCache){ //conflict miss
			cache[LRU] = blockAddr/sets;
			recency[LRU] = access;
		}
	}
	if (ways != 16){
		out << hit << "," << access << "; ";
	}
	else{
		out << hit << "," << access << ";";
	}
}

void setAssociative(char * in, ofstream& out){
	//PERFECTO
	setAssociative(in, out, 2);
	setAssociative(in, out, 4);
	setAssociative(in, out, 8);
	setAssociative(in, out, 16);
	out << endl;
}

void fullyAssociativeLRU(char * in, ofstream& out){
	//PERFECTO
	string instr;
	unsigned long long addr;
	array<long long, 512> cache; //16KB / 32B per block = 512 blocks
	cache.fill(-1); //-1 is invalid
	array<int, 512> recency;
	recency.fill(0);
	
	ifstream input(in);
	int hit = 0;
	int access = 0;
	while (input >> instr >> hex >> addr ){
		access++;
		int tag = addr >> 5; //5 bits for byte offset
		bool inCache = false; //addr in cache
		int LRU = 0;
		for (unsigned int i = 0; i < cache.size() && !inCache; i++){
			if (cache[i] == -1){ //cold start miss
				cache[i] = tag;
				recency[i] = access;
				inCache = true;
			}
			else if (cache[i] == tag){ //cache hit
				hit++;
				recency[i] = access;
				inCache = true;
			}
			if (recency[i] < recency[LRU]){
				LRU = i;
			}
		}
		if (!inCache){ //conflict miss
			cache[LRU] = tag;
			recency[LRU] = access;
		}
	}
	out << hit << "," << access << ";" << endl;;	
}

void fullyAssociativePLRU(char *in, ofstream& out){
	//PERFECTO
	string instr;
	unsigned long long addr;

	array<long long, 512> cache; //16KB / 32B per block = 512 blocks
	cache.fill(-1); //-1 is invalid
	vector<long long> bottom (8);
	fill(bottom.begin(), bottom.end(), 0);
	int top = 0;
	
	ifstream input(in);
	int hit = 0;
	int access = 0;
	while (input >> instr >> hex >> addr ){
		access++;
		int tag = addr >> 5; //5 bits for byte offset
		bool inCache = false; //addr in cache
		for (unsigned int i = 0; i < cache.size() && !inCache; i++){
			if (cache[i] == tag){ //cache hit
				hit++;
				inCache = true;
				int t = i>>6; //bit in top
				top &= ~(1<<(3-t/2)); //clear
				top |= (t%2)<<(3-t/2);
				top &= ~(1<<(5-t/4)); //clear
				top |= ((t/2)%2)<<(5-t/4);
				top &= ~(1<<6); //clear
				top |= ((t/4)%2)<<6;

				long long bits = bottom[t];
				int b = i % 64;
				int groups = 2;
				int offset = 31;
				while (groups <= 64){
					bits &= ~(1UL << (offset - b/groups)); //clear
					long long bit = (b / (groups>>1)) % 2;
					bits |= bit << (offset - b/groups);
					groups <<= 1;
					offset += 64/groups;
				}
				bottom[t] = bits;
			}
		}
		if (!inCache){ //miss
			int t = 0;
			int i = 0;
			int offsetT = 4;
			while (i < 7){
				int bit = (top & (1 << (6-i))) >> (6-i);
				top ^= (1<<(6-i));
				t += offsetT * (1-bit); //if bit = 0 then bigger, else then smaller
				offsetT >>= 1;
				i = i*2 + (2-bit);
			}
			long long bits = bottom[t];
			int b = 0;
			int j = 0;
			int offsetB = 32;
			while (j < 63){
				long long bit = (bits & (1UL << (62-j))) >> (62-j);
				bits ^= (1UL<<(62-j));
				b += offsetB * (1-bit); //if bit = 0 then bigger, else then smaller
				offsetB >>= 1;
				j = j*2 + (2-bit);
			}
			bottom[t] = bits;
			int index = t*64 + b;
			cache[index] = tag;
		}
	}
	out << hit << "," << access << ";" << endl;;	
}

void setAssocNoAllocOnWriteMiss(char* in, ofstream& out, int ways){
	//each cache line size is 32 bytes
	//16KB cache
	//least recently used replacement policy is implemented
	string instr;
	unsigned long long addr;
	int sets = 512 / ways; //(size of cache / bytes per block) / # ways

	array<long long, 512> cache;
	cache.fill(-1); //-1 is invalid
	array<int, 512> recency;
	recency.fill(0);
	
	ifstream input(in);
	int hit = 0;
	int access = 0;
	while (input >> instr >> hex >> addr ){
		access++;
		int blockAddr = addr / 32; //32 = blockSize
		int setNum = blockAddr % sets;
		int index = setNum * ways; //cache index
		bool inCache = false; //addr in cache
		int LRU = index;
		for (int i = 0; i < ways && !inCache; i++){
			if (instr != "S" && cache[index] == -1){ //cold start miss
				cache[index] = blockAddr/sets;
				recency[index] = access;
				inCache = true;
			}
			else if (cache[index] == blockAddr/sets){ //cache hit
				hit++;
				recency[index] = access;
				inCache = true;
			}
			if (recency[index] < recency[LRU]){
				LRU = index;
			}
			index++;
		}
		if (!inCache){ //conflict miss
			if (instr != "S"){
				cache[LRU] = blockAddr/sets;
				recency[LRU] = access;
			}
		}
	}
	if (ways != 16){
		out << hit << "," << access << "; ";
	}
	else{
		out << hit << "," << access << ";";
	}

}

void setAssocNoAllocOnWriteMiss(char * in, ofstream& out){
	//PERFECTO
	setAssocNoAllocOnWriteMiss(in, out, 2);
	setAssocNoAllocOnWriteMiss(in, out, 4);
	setAssocNoAllocOnWriteMiss(in, out, 8);
	setAssocNoAllocOnWriteMiss(in, out, 16);
	out << endl;
}

void setAssocPreFetch(char * in, ofstream& out, int ways){
	//PERFECTO
	string instr;
	unsigned long long addr;
	int sets = 512 / ways; //(size of cache / bytes per block) / # ways

	array<long long, 512> cache;
	cache.fill(-1); //-1 is invalid
	array<int, 512> recency;
	recency.fill(0);
	
	ifstream input(in);
	int hit = 0;
	int access = 0;
	int lines = 0;
	while (input >> instr >> hex >> addr ){
		lines++;
		access++;
		int blockAddr = addr / 32; //32 = blockSize
		int setNum = blockAddr % sets;
		int index = setNum * ways; //cache index
		bool inCache = false; //addr in cache
		int LRU = index;
		for (int i = 0; i < ways && !inCache; i++){
			if (cache[index] == -1){ //cold start miss
				cache[index] = blockAddr/sets;
				recency[index] = access++;
				inCache = true;
			}
			else if (cache[index] == blockAddr/sets){ //cache hit
				hit++;
				recency[index] = access++;
				inCache = true;
			}
			if (recency[index] < recency[LRU]){
				LRU = index;
			}
			index++;
		}
		if (!inCache){ //conflict miss
			cache[LRU] = blockAddr/sets;
			recency[LRU] = access++;
		}
		int nextBlockAddr = blockAddr + 1;
		int nextSetNum = nextBlockAddr % sets;
		int nextIndex = nextSetNum * ways;
		inCache = false;
		LRU = nextIndex;
		for (int i = 0; i < ways && !inCache; i++){
			if (cache[nextIndex] == -1){
				cache[nextIndex] = nextBlockAddr/sets;
				recency[nextIndex] = access;
				inCache = true;
			}
			else if (cache[nextIndex] == nextBlockAddr/sets){
				recency[nextIndex] = access;
				inCache = true;
			}
			if (recency[nextIndex] < recency[LRU]){
				LRU = nextIndex;
			}
			nextIndex++;
		}
		if(!inCache){
			cache[LRU] = nextBlockAddr/sets;
			recency[LRU] = access;
		}
	}
	if (ways != 16){
		out << hit << "," << lines << "; ";
	}
	else{
		out << hit << "," << lines << ";";
	}
}

void setAssocPreFetch(char * in, ofstream& out){
	setAssocPreFetch(in, out, 2);
	setAssocPreFetch(in, out, 4);
	setAssocPreFetch(in, out, 8);
	setAssocPreFetch(in, out, 16);
	out << endl;
}

void setAssocPreFetchOnMiss(char * in, ofstream& out, int ways){
	//PERFECTO
	string instr;
	unsigned long long addr;
	int sets = 512 / ways; //(size of cache / bytes per block) / # ways

	array<long long, 512> cache;
	cache.fill(-1); //-1 is invalid
	array<int, 512> recency;
	recency.fill(0);
	
	ifstream input(in);
	int hit = 0;
	int access = 0;
	int lines = 0;
	while (input >> instr >> hex >> addr ){
		lines++;
		access++;
		int blockAddr = addr / 32; //32 = blockSize
		int setNum = blockAddr % sets;
		int index = setNum * ways; //cache index
		bool inCache = false; //addr in cache
		bool miss = false;
		int LRU = index;
		for (int i = 0; i < ways && !inCache; i++){
			if (cache[index] == -1){ //cold start miss
				cache[index] = blockAddr/sets;
				recency[index] = access++;
				inCache = true;
				miss = true;
			}
			else if (cache[index] == blockAddr/sets){ //cache hit
				hit++;
				recency[index] = access++;
				inCache = true;
			}
			if (recency[index] < recency[LRU]){
				LRU = index;
			}
			index++;
		}
		if (!inCache){ //conflict miss
			cache[LRU] = blockAddr/sets;
			recency[LRU] = access++;
			miss = true;
		}
		if (miss){
			int nextBlockAddr = blockAddr + 1;
			int nextSetNum = nextBlockAddr % sets;
			int nextIndex = nextSetNum * ways;
			inCache = false;
			LRU = nextIndex;
			for (int i = 0; i < ways && !inCache; i++){
				if (cache[nextIndex] == -1){
					cache[nextIndex] = nextBlockAddr/sets;
					recency[nextIndex] = access;
					inCache = true;
				}
				else if (cache[nextIndex] == nextBlockAddr/sets){
					recency[nextIndex] = access;
					inCache = true;
				}
				if (recency[nextIndex] < recency[LRU]){
					LRU = nextIndex;
				}
				nextIndex++;
			}
			if(!inCache){
				cache[LRU] = nextBlockAddr/sets;
				recency[LRU] = access;
			}
		}
	}
	if (ways != 16){
		out << hit << "," << lines << "; ";
	}
	else{
		out << hit << "," << lines << ";";
	}	
}

void setAssocPreFetchOnMiss(char * in, ofstream& out){
	setAssocPreFetchOnMiss(in, out, 2);
	setAssocPreFetchOnMiss(in, out, 4);
	setAssocPreFetchOnMiss(in, out, 8);
	setAssocPreFetchOnMiss(in, out, 16);
	out << endl;
}

int main(int argc, char* argv[]){
	if (argc < 3) return -1;
	ofstream output(argv[2], ios::out | ios::trunc);
	char * input = argv[1];

	cout << "direct mapped..." << endl;
	directMapped(input, output);
	cout << "set associative..." << endl;
	setAssociative(input, output);
	cout << "fully associative LRU ..." << endl;
	fullyAssociativeLRU(input, output);
	cout << "fully associative pLRU..." << endl;
	fullyAssociativePLRU(input, output);
	cout << "set associative no alloc on write miss..." << endl;
	setAssocNoAllocOnWriteMiss(input, output);
	cout << "set associative prefetch..." << endl;
	setAssocPreFetch(input, output);
	cout << "set associative prefetch on miss..." << endl;
	setAssocPreFetchOnMiss(input, output);
	
	output.close();
	return 0;
}
