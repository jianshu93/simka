/*
 * Simka2Database.h
 *
 *  Created on: 10 nov. 2016
 *      Author: gbenoit
 */

#ifndef GATB_SIMKA_SRC_SIMKA2_SIMKA2DATABASE_H_
#define GATB_SIMKA_SRC_SIMKA2_SIMKA2DATABASE_H_

class Simka2Database {

public:

	string _dir;
	vector<string> _entries;
	map<string, string> _entriesInfos;
	set<string> _uniqKmerSpectrumDirs;
	u_int64_t _nbProcessedDataset;
	u_int64_t _maxDatasets;

	Simka2Database(){
	}

	Simka2Database(const string& dir, u_int64_t maxDatasets){
		_dir = dir;
		_maxDatasets = maxDatasets;
		load();
	}

	~Simka2Database(){

	}

	void load(){
		loadDistanceMatrixInfo();
		loadDatabase();
	}

	void loadDatabase(){
		u_int64_t nbDatasets = 0;
		string databaseFilename = _dir + "/simka_database.csv";

		ifstream databaseFile(databaseFilename.c_str());

		string line;
		string linePart;
		vector<string> fields;

		getline(databaseFile, line); //skip header

		while(getline(databaseFile, line)){

			if(nbDatasets >= _maxDatasets) break;

			line.erase(std::remove(line.begin(),line.end(),' '),line.end());
			if(line == "") continue;

			fields.clear();

			stringstream lineStream(line);
			while(getline(lineStream, linePart, ';')){
				fields.push_back(linePart);
			}

			string id = fields[0];
			string kmerSpectrumDir = fields[1];

			_entries.push_back(id);
			_entriesInfos[id] = kmerSpectrumDir;
			_uniqKmerSpectrumDirs.insert(kmerSpectrumDir);

			nbDatasets += 1;
		}

		databaseFile.close();

	}
	void loadDistanceMatrixInfo(){
		string proccessedIdsFilename = _dir + "/distance/matrix_binary/matrix_infos.bin";
		if(System::file().doesExist(proccessedIdsFilename)){

			ifstream proccessedIdsFile(proccessedIdsFilename.c_str(), std::ios::binary);
			proccessedIdsFile.read((char*)(&_nbProcessedDataset), sizeof(_nbProcessedDataset));
			proccessedIdsFile.close();
		}
		else{
			_nbProcessedDataset = 0;
		}

	}
};

#endif /* GATB_SIMKA_SRC_SIMKA2_SIMKA2DATABASE_H_ */