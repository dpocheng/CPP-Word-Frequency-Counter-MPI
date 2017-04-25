// Copyright 2017 Pok On Cheng
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/************************
 * Pok On Cheng         *
 * pocheng (74157306)   *
 * CompSci 131          *
 * Lab2-W17 Part B      *
 ************************/

#include "mpi.h"
#include <algorithm>
#include <functional>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>
const static int ARRAY_SIZE = 130000;
using Lines = char[ARRAY_SIZE][16];

// To remove punctuations
struct letter_only: std::ctype<char> 
{
    letter_only(): std::ctype<char>(get_table()) {}

    static std::ctype_base::mask const* get_table()
    {
        static std::vector<std::ctype_base::mask> 
            rc(std::ctype<char>::table_size,std::ctype_base::space);

        std::fill(&rc['A'], &rc['z'+1], std::ctype_base::alpha);
        return &rc[0];
    }
};

void DoOutput(std::string word, int result)
{
    std::cout << "Word Frequency: " << word << " -> " << result << std::endl;
}

/***************** Add your functions here *********************/

// To compute word frequency
int Compute_word_frequency(std::vector<std::string> my_string_vector, std::string search_word) {
	int my_count = 0;
	for (int word = 0; word < my_string_vector.size(); word++) {
		if (my_string_vector[word] == search_word) {
			my_count += 1;
		}
	}
	return my_count;
}

int main(int argc, char* argv[])
{
    int processId;
    int num_processes;
    int *to_return = NULL;
    double start_time, end_time;
 
    // Setup MPI
    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &processId);
    MPI_Comm_size( MPI_COMM_WORLD, &num_processes);
 
    // Three arguments: <input file> <search word> <part B1 or part B2 to execute>
    if(argc != 4)
    {
        if(processId == 0)
        {
            std::cout << "ERROR: Incorrect number of arguments. Format is: <filename> <word> <b1/b2>" << std::endl;
        }
        MPI_Finalize();
        return 0;
    }
	std::string word = argv[2];
 
    Lines lines;
	// Read the input file and put words into char array(lines)
    if (processId == 0) {
        std::ifstream file;
		file.imbue(std::locale(std::locale(), new letter_only()));
		file.open(argv[1]);
		std::string workString;
		int i = 0;
		while(file >> workString){
			memset(lines[i], '\0', 16);
			memcpy(lines[i++], workString.c_str(), workString.length());
		}
    }
	
	/***************** Add code as per your requirement below *****************/ 

	start_time=MPI_Wtime();
	
	// Initialize sendcounts, displs, and recvcount
	int count = ARRAY_SIZE/num_processes;
	int leftovers = ARRAY_SIZE%num_processes;
	int recvcount = (processId == 0) ? (count+leftovers)*16 : count*16;
	int pos = 0;
	int sendcounts[num_processes];
	int displs[num_processes];
	for (int i = 0; i < num_processes; i++) {
		if (i == 0) {
			sendcounts[i] = (count+leftovers)*16;
			displs[i] = pos;
			pos += sendcounts[i];
		}
		else {
			sendcounts[i] = count*16;
			displs[i] = pos;
			pos += sendcounts[i];
		}
	}
	
	// Scatter lines from Root to other nodes
	Lines my_lines;
	MPI_Scatterv(&lines, sendcounts, displs, MPI_CHAR, &my_lines, recvcount, MPI_CHAR, 0, MPI_COMM_WORLD);
	
	// Create strings from my_lines
	std::vector<std::string> my_string_vector;
	for (int line = 0; line < (recvcount/16); line++) {
		my_string_vector.push_back(my_lines[line]);
	}
	
	// Computer word frequency
	int my_count;
	my_count = Compute_word_frequency(my_string_vector, word);
	
	int finalResult = 0;
	
	if( argv[3] == "b1" )
	{
		// Reduction for Part B1
		MPI_Reduce(&my_count, &finalResult, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
	} else {
		// Point-To-Point communication for Part B2
		if (processId == 0) {
			MPI_Send(&my_count, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
			//std::cout << "Processor " << processId << std::endl;
			int total_count = 0;
			MPI_Recv(&total_count, 1, MPI_INT, num_processes-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			finalResult = total_count;
		}
		else if (processId == num_processes-1) {
			int total_count = 0;
			MPI_Recv(&total_count, 1, MPI_INT, processId-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//std::cout << "Processor " << processId << std::endl;
			total_count += my_count;
			MPI_Send(&total_count, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
		}
		else {
			int total_count = 0;
			MPI_Recv(&total_count, 1, MPI_INT, processId-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//std::cout << "Processor " << processId << std::endl;
			total_count += my_count;
			MPI_Send(&total_count, 1, MPI_INT, processId+1, 0, MPI_COMM_WORLD);
		}
	}
	
    if(processId == 0)
    {
        // Output the search word's frequency here
		DoOutput(word, finalResult);
		end_time=MPI_Wtime();
        std::cout << "Time: " << ((double)end_time-start_time) << std::endl;
    }
 
    MPI_Finalize();
 
    return 0;
}
