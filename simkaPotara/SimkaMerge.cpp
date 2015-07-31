//! [snippet1]
// We include what we need for the test



#include <gatb/gatb_core.hpp>
#include <SimkaAlgorithm.hpp>
#include <SimkaDistance.hpp>

// We use the required packages
using namespace std;


/********************************************************************************/
class SimkaMerge : public Tool
{
public:

	SimkaMerge () : Tool ("SimkaMerge")
    {
		//Original input filename given to simka. Used to recreate dataset id list
        getParser()->push_back (new OptionOneParam (STR_KMER_SIZE,   "kmer size", true));
        getParser()->push_back (new OptionOneParam (STR_URI_INPUT,   "input filename", true));
        getParser()->push_back (new OptionOneParam ("-out-tmp-simka",   "tmp output", true));
        getParser()->push_back (new OptionOneParam ("-partition-id",   "bank name", true));
        getParser()->push_back (new OptionOneParam ("-nb-cores",   "bank name", true));
        getParser()->push_back (new OptionOneParam ("-max-memory",   "bank name", true));

    }

    void execute ()
    {


    	size_t kmerSize =  getInput()->getInt(STR_KMER_SIZE);
    	size_t partitionId =  getInput()->getInt("-partition-id");
    	string inputFilename =  getInput()->getStr(STR_URI_INPUT);
    	string outputDir =  getInput()->getStr("-out-tmp-simka");

    	Parameter params(*this, inputFilename, outputDir, partitionId);

        Integer::apply<Functor,Parameter> (kmerSize, params);

    }


    struct Parameter
    {
        Parameter (SimkaMerge& tool, string inputFilename, string outputDir, size_t partitionId) : tool(tool), inputFilename(inputFilename), outputDir(outputDir), partitionId(partitionId) {}
        SimkaMerge& tool;
        string inputFilename;
        string outputDir;
        size_t partitionId;
    };


    template<size_t span=KMER_DEFAULT_SPAN>
    class StorageIt
    {

    public:


        typedef typename Kmer<span>::Type                                       Type;
        typedef typename Kmer<span>::Count                                      Count;
        //typedef typename Kmer<span>::ModelCanonical                             ModelCanonical;
        //typedef typename ModelCanonical::Kmer                                   KmerType;

        StorageIt(const string& h5filename, size_t bankId, size_t partitionId){

        	//cout << h5filename << endl;
        	_bankId = bankId;
        	_partitionId = partitionId;

    		//string h5filename2 = "/WORKS/gbenoit/env/AHX_CFNIOSF_4_1_C0YDFACXX.h5";
    		Storage* storage1 = StorageFactory(STORAGE_HDF5).load (h5filename);
    		//storages.push_back(storage1);
    		//LOCAL (storage1);
    		//Storage* storage2 = StorageFactory(STORAGE_HDF5).load (h5filename2);
    		//LOCAL (storage2);



    		Group& dskGroup1 = storage1->root().getGroup("dsk");
    		//Group& dskGroup2 = storage2->root().getGroup("dsk");
    		string nbPartitionsStrg = dskGroup1.getGroup("solid").getProperty ("nb_partitions");
    		size_t nbPartitions = atol (nbPartitionsStrg.c_str());


    		Partition<Count>& partition1 = dskGroup1.getPartition<Count>("solid");

    	    Iterator<Count>* it2 = partition1.iterator();
    		//Partition<Count>& partition2 = dskGroup2.getPartition<Count>("solid");
    		//partitions.push_back(&partition1);

    		Collection<Count>& kmers1 = partition1[_partitionId];
    		//collections.push_back(&kmers1);

    		_it = kmers1.iterator();

    		//it2->first();
    		//while(!it2->isDone()){
    		//	cout << it2->item().value.toString(31) << endl;
    		//	it2->next();
    		//}
        }

        ~StorageIt(){

        }

        void setPartitionId(size_t partitionId){
        	_partitionId = partitionId;
        }

    	bool next(){
    		_it->next();

    		//cout << "is done?" <<  _it->isDone() << endl;
    		return !_it->isDone();
    	}

    	Type& value(){
    		return _it->item().value;
    	}

    	u_int16_t abundance(){
    		return _it->item().abundance;
    	}

    	u_int16_t getBankId(){
    		return _bankId;
    	}

    	u_int16_t _bankId;
    	u_int16_t _partitionId;
        Iterator<Count>* _it;
    };


    class SimkaCounterBuilderMerge
    {
    public:

        /** Constructor.
         * \param[in] nbBanks : number of banks parsed during kmer counting.
         */
    	SimkaCounterBuilderMerge (size_t nbBanks=1)  :  _abundancePerBank(nbBanks)  {}

        /** Get the number of banks.
         * \return the number of banks. */
        size_t size() const  { return _abundancePerBank.size(); }

        /** Initialization of the counting for the current kmer. This method should be called
         * when a kmer is seen for the first time.
         * \param[in] idxBank : bank index where the new current kmer has been found. */
        void init (size_t idxBank, u_int16_t abundance)
        {
            for (size_t k=0; k<_abundancePerBank.size(); k++)  { _abundancePerBank[k]=0; }
            _abundancePerBank [idxBank]= abundance;
        }

        /** Increase the abundance of the current kmer for the provided bank index.
         * \param[in] idxBank : index of the bank */
        void increase (size_t idxBank, u_int16_t abundance)  {  _abundancePerBank [idxBank] += abundance;  }

        /** Set the abundance of the current kmer for the provided bank index.
         * \param[in] idxBank : index of the bank */
        void set (CountNumber val, size_t idxBank=0)  {  _abundancePerBank [idxBank] = val;  }

        /** Get the abundance of the current kmer for the provided bank index.
         * \param[in] idxBank : index of the bank
         * \return the abundance of the current kmer for the given bank. */
        CountNumber operator[] (size_t idxBank) const  { return _abundancePerBank[idxBank]; }

        /** */
        const CountVector& get () const { return _abundancePerBank; }

        void print(const string& kmer){
    		cout << kmer << ": ";
        	for(size_t i=0; i<size(); i++){
        		cout << _abundancePerBank[i] << " ";
        	}
        	cout << endl;
        }

    private:
        CountVector _abundancePerBank;
    };



    template<size_t span>
    struct Functor  {

        typedef typename Kmer<span>::Type                                       Type;
        typedef typename Kmer<span>::Count                                      Count;

        typedef std::pair<u_int16_t, Type> kxp; //id pointer in vec_pointer , value
        struct kxpcomp { bool operator() (kxp l,kxp r) { return ((r.second) < (l.second)); } } ;

        Functor(){
        	_abundanceThreshold.first = 0;
        	_abundanceThreshold.second = 0;
        }

    	void operator ()  (Parameter& p)
		{

    		removeStorage(p);

            _partitiontId = p.partitionId;

			createDatasetIdList(p);

			//vector<string> filenames;
			//for(size_t i=0; i<_datasetIds.size(); i++){

			//}

			_nbBanks = _datasetIds.size();
			createProcessor();

			vector<StorageIt<span>*> its;
			//vector<Partition<Count>*> partitions;
			//vector<Collection<Count>*> collections;
			//vector<Iterator<Count>*> its;
			std::priority_queue< kxp, vector<kxp>,kxpcomp > pq;
			//vector<Storage*> storages;

			for(size_t i=0; i<_nbBanks; i++){

				string solidH5Filename = p.outputDir + "/solid/" +  _datasetIds[i] + ".h5";
				StorageIt<span>* it = new StorageIt<span>(solidH5Filename, i, p.partitionId);
				its.push_back(it);

			}


			for(size_t i=0; i<_nbBanks; i++){

				StorageIt<span>* it = its[i];

				it->_it->first();

				//while(!it->_it->isDone()){

				//	it->_it->next();
				//	cout << it->_it->item().value.toString(_kmerSize) << " " << it->_it->item().abundance << endl;
				//}

			}


			//for(size_t i=0; i<nbPartitions; i++){


			//for(size_t j=0; i<nbBanks; i++){

				//fill the  priority queue with the first elems
				for (size_t i=0; i<_nbBanks; i++)
				{

					StorageIt<span>* it = its[i];
					//it->_it->first();

					//Count& count = it->_it->item();

					if(!it->_it->isDone()){
						Count& count = it->_it->item();

						pq.push(kxp(i, count.value));
						//it->next();
					}
					//if(it->next())  {  pq.push(kxp(i, it->value()));  }
				}

				u_int16_t best_p;
				Type previous_kmer;
				SimkaCounterBuilderMerge solidCounter(_nbBanks);


				if (pq.size() != 0) // everything empty, no kmer at all
				{
					best_p = pq.top().first ; pq.pop();

					previous_kmer = its[best_p]->value();

					solidCounter.init (its[best_p]->getBankId(), its[best_p]->abundance());

					//merge-scan all 'virtual' arrays and output counts
					while (1)
					{
						//go forward in this array or in new array of reaches end of this one
						if (! its[best_p]->next())
						{
							//reaches end of one array
							if(pq.size() == 0) break; //everything done

							//otherwise get new best
							best_p = pq.top().first ; pq.pop();
						}

						if (its[best_p]->value() != previous_kmer )
						{
							//if diff, changes to new array, get new min pointer
							pq.push(kxp(its[best_p]->getBankId(), its[best_p]->value())); //push new val of this pointer in pq, will be counted later

							best_p = pq.top().first ; pq.pop();

							//if new best is diff, this is the end of this kmer
							if(its[best_p]->value()!=previous_kmer )
							{
								insert(previous_kmer, solidCounter);
								//solidCounter.print(previous_kmer.toString(31));

								solidCounter.init (its[best_p]->getBankId(), its[best_p]->abundance());
								previous_kmer = its[best_p]->value();
							}
							else
							{
								solidCounter.increase (its[best_p]->getBankId(), its[best_p]->abundance());
							}
						}
						else
						{
							solidCounter.increase (its[best_p]->getBankId(), its[best_p]->abundance());
						}
					}

					insert(previous_kmer, solidCounter);
					//solidCounter.print(previous_kmer.toString(31));
					//last elem
					//this->insert (previous_kmer, solidCounter);
				}
			//}

			saveStats(p);
			writeFinishSignal(p);

		}

    	void createDatasetIdList(Parameter& p){

    		string datasetIdFilename = p.outputDir + "/" + "datasetIds";
    		IFile* inputFile = System::file().newFile(datasetIdFilename, "rb");
    		//IFile* bankFile = System::file().newFile(_banksInputFilename, "wb");

    		inputFile->seeko(0, SEEK_END);
    		u_int64_t size = inputFile->tell();
    		inputFile->seeko(0, SEEK_SET);
    		char buffer2[size];
    		inputFile->fread(buffer2, size, size);
    		string fileContents(buffer2, size);

    		string line;
    		string linePart;
    		vector<string> linePartList;
    		stringstream fileContentsStream(fileContents);

    		//string bankFileContents = "";

    		//u_int64_t lineIndex = 0;

    		while(getline(fileContentsStream, line)){

    			if(line == "") continue;

    			_datasetIds.push_back(line);
    		}

    		//bankFileContents.erase(bankFileContents.size()-1);
    		//bankFileContents.pop_back(); // "remove last /n

    		//bankFile->fwrite(bankFileContents.c_str(), bankFileContents.size(), 1);

    		delete inputFile;
        }

    	void createProcessor(){

        	_stats = new SimkaStatistics(_nbBanks);
        	_processor = new SimkaCountProcessor<span> (*_stats, _nbBanks, _abundanceThreshold, SUM, false, 0);
        	_processor->use();

        	_processors.push_back(_processor->clone());
    	}

    	void insert(const Type& kmer, const SimkaCounterBuilderMerge& counter){
    		//cout <<_partitiontId << " "<< kmer.toString(31) << endl;
    		_processors[0]->process (_partitiontId, kmer, counter.get(), 0);
    	}

    	void removeStorage(Parameter& p){
    	    Storage* storage = 0;
            storage = StorageFactory(STORAGE_HDF5).create (p.outputDir + "/stats/part_" + SimkaAlgorithm<>::toString(p.partitionId) + ".stats", true, true);
            LOCAL (storage);
    	}

    	void saveStats(Parameter& p){
			_processor->finishClones(_processors);
			_processors[0]->forget();
			_processor->forget();

    	    Storage* storage = 0;
            storage = StorageFactory(STORAGE_HDF5).create (p.outputDir + "/stats/part_" + SimkaAlgorithm<>::toString(p.partitionId) + ".stats", true, false);
            LOCAL (storage);

    		_stats->save(storage->getGroup(""));

    	}

		void writeFinishSignal(Parameter& p){
			string finishFilename = p.outputDir + "/merge_synchro/" +  SimkaAlgorithm<>::toString(p.partitionId) + ".ok";
			IFile* file = System::file().newFile(finishFilename, "w");
			delete file;
		}

		size_t _nbBanks;
		pair<size_t, size_t> _abundanceThreshold;
        vector<string> _datasetIds;
        size_t _partitiontId;
        SimkaStatistics* _stats;
    	SimkaCountProcessor<span>* _processor;
    	vector<ICountProcessor<span>*> _processors;
    };


};



int main (int argc, char* argv[])
{
    try
    {
    	SimkaMerge().run (argc, argv);
    }
    catch (Exception& e)
    {
        std::cout << "EXCEPTION: " << e.getMessage() << std::endl;
        return EXIT_FAILURE;
    }
}


//! [snippet1]