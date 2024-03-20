// EduChain.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <zmq.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <cJSON.h>
#include <sqlite3.h>
#include <unordered_map>
#include <fstream>
#include <conio.h>
#include <algorithm> 

// Rotate right (circular right shift) operation
#define ROTR(value, bits) (((value) >> (bits)) | ((value) << (32 - (bits))))

std::string sha256(const std::string& input) {
    // Initial hash values (first 32 bits of the fractional parts of the square roots of the first 8 primes 2..19)
    uint32_t h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    // Constants (first 32 bits of the fractional parts of the cube roots of the first 64 primes 2..311)
    uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    // Pre-processing
    std::string paddedInput = input;
    // Append single '1' bit
    paddedInput += (char)0x80;

    // Append '0' bits to fill a 512-bit block minus 64 bits (length field)
    while ((paddedInput.size() % 64) != 56) {
        paddedInput += (char)0x00;
    }

    // Append length field
    uint64_t inputLength = input.size() * 8;
    for (int i = 0; i < 8; ++i) {
        paddedInput += (char)((inputLength >> ((7 - i) * 8)) & 0xFF);
    }

    // Process each 512-bit block
    for (size_t i = 0; i < paddedInput.size(); i += 64) {
        // Copy the block into 16 32-bit words
        uint32_t w[64];
        for (int j = 0; j < 16; ++j) {
            w[j] = (paddedInput[i + j * 4] << 24) |
                   (paddedInput[i + j * 4 + 1] << 16) |
                   (paddedInput[i + j * 4 + 2] << 8) |
                   (paddedInput[i + j * 4 + 3]);
        }

        // Extend the first 16 words into the remaining 48 words
        for (int j = 16; j < 64; ++j) {
            uint32_t s0 = ROTR(w[j - 15], 7) ^ ROTR(w[j - 15], 18) ^ (w[j - 15] >> 3);
            uint32_t s1 = ROTR(w[j - 2], 17) ^ ROTR(w[j - 2], 19) ^ (w[j - 2] >> 10);
            w[j] = w[j - 16] + s0 + w[j - 7] + s1;
        }

        // Initialize working variables to current hash values
        uint32_t a = h[0];
        uint32_t b = h[1];
        uint32_t c = h[2];
        uint32_t d = h[3];
        uint32_t e = h[4];
        uint32_t f = h[5];
        uint32_t g = h[6];
        uint32_t l = h[7];

        // Main loop
        for (int j = 0; j < 64; ++j) {
            uint32_t S1 = ROTR(e, 6) ^ ROTR(e, 11) ^ ROTR(e, 25);
            uint32_t ch = (e & f) ^ ((~e) & g);
            uint32_t temp1 = l + S1 + ch + k[j] + w[j];
            uint32_t S0 = ROTR(a, 2) ^ ROTR(a, 13) ^ ROTR(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = S0 + maj;

            l = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        // Update hash values
        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += l;
    }

    // Final hash value
    std::stringstream ss;
    for (int i = 0; i < 8; ++i) {
        ss << std::hex << std::setw(8) << std::setfill('0') << h[i];
    }

    return ss.str();
}

class Block {
public:
    int index;
    std::string previousHash;
    std::string timestamp;
    std::string data;
    std::string hash;

    Block(int idx, const std::string& prevHash, const std::string& data)
        : index(idx), previousHash(prevHash), data(data) {
        timestamp = getCurrentTimestamp();
        hash = calculateHash();
    }
    
    Block& operator=(const Block& other) {
        if (this != &other) {          
			index = other.index;
            previousHash = other.previousHash;
            timestamp = other.timestamp;
            data = other.data;
            hash = other.hash;
        }
        return *this;
    }
private:

    std::string getCurrentTimestamp() const {
        std::time_t now = std::time(nullptr);
        char buffer[100];
        std::tm timeinfo;
        if (localtime_s(&timeinfo, &now) == 0) {
            std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
        } else {
			std::cerr << "Failed to create timestamp. \n";
		}
        return buffer;
    }

    std::string calculateHash() const {
        return sha256(previousHash + timestamp + std::to_string(index) + data);
    }
};

class Blockchain {
public:
    Blockchain() {
        chain.emplace_back(0, "0", "Genesis Block");
    }

    void addBlock(const std::string& data) {
        const Block& previousBlock = getLatestBlock();
        chain.emplace_back(chain.size(), previousBlock.hash, data);
    }

    const Block& getLatestBlock() const {
        return chain.back();
    }

    const std::vector<Block>& getChain() const {
        return chain;
    }

    std::string toJson() const {
        cJSON* jsonBlockchain = cJSON_CreateArray();

        for (const auto& block : chain) {
            cJSON* jsonBlock = cJSON_CreateObject();
            cJSON_AddItemToObject(jsonBlock, "index", cJSON_CreateNumber(block.index));
            cJSON_AddItemToObject(jsonBlock, "previousHash", cJSON_CreateString(block.previousHash.c_str()));
            cJSON_AddItemToObject(jsonBlock, "timestamp", cJSON_CreateString(block.timestamp.c_str()));
            cJSON_AddItemToObject(jsonBlock, "data", cJSON_CreateString(block.data.c_str()));
            cJSON_AddItemToObject(jsonBlock, "hash", cJSON_CreateString(block.hash.c_str()));

            cJSON_AddItemToArray(jsonBlockchain, jsonBlock);
        }

        char* jsonString = cJSON_Print(jsonBlockchain);
        std::string result(jsonString);

        cJSON_Delete(jsonBlockchain);
        free(jsonString);

        return result;
    }

    static Blockchain fromJson(const std::string& jsonString) {
        Blockchain blockchain;
        cJSON* jsonBlockchain = cJSON_Parse(jsonString.c_str());

        if (jsonBlockchain == nullptr) {
            std::cerr << "Error parsing JSON string\n";
            return blockchain;
        }

        int arraySize = cJSON_GetArraySize(jsonBlockchain);
        for (int i = 0; i < arraySize; ++i) {
            cJSON* jsonBlock = cJSON_GetArrayItem(jsonBlockchain, i);
            if (jsonBlock == nullptr) {
                std::cerr << "Error parsing JSON string\n";
                continue;
            }

            int index = cJSON_GetObjectItem(jsonBlock, "index")->valueint;
            std::string previousHash = cJSON_GetObjectItem(jsonBlock, "previousHash")->valuestring;
            std::string timestamp = cJSON_GetObjectItem(jsonBlock, "timestamp")->valuestring;
            std::string data = cJSON_GetObjectItem(jsonBlock, "data")->valuestring;
            std::string hash = cJSON_GetObjectItem(jsonBlock, "hash")->valuestring;

            blockchain.chain.emplace_back(index, previousHash, data);
            blockchain.chain.back().timestamp = timestamp;
            blockchain.chain.back().hash = hash;
        }

        cJSON_Delete(jsonBlockchain);
        return blockchain;
    }

private:
    std::vector<Block> chain;
};

class SimplePeer {
public:
    SimplePeer() : context_(zmq_ctx_new()), socket_server_(nullptr), socket_client_(nullptr) {}

    ~SimplePeer() {
        if (socket_server_) {
            zmq_close(socket_server_);
        }
        if (socket_client_) {
            zmq_close(socket_client_);
        }
        zmq_ctx_destroy(context_);
    }

    void Start(Blockchain &bc) {
        std::string bindAddress, bindPort;
        std::cout << "Enter binding address: ";
        std::cin >> bindAddress;
        std::cout << "Enter binding port: ";
        std::cin >> bindPort;

        std::string bindEndpoint = "tcp://" + bindAddress + ":" + bindPort;
        socket_server_ = zmq_socket(context_, ZMQ_REP);

        int bindResult = zmq_bind(socket_server_, bindEndpoint.c_str());

        if (bindResult == -1) {
            char errorBuffer[256];
            strerror_s(errorBuffer, sizeof(errorBuffer), errno);
            std::cerr << "Error binding socket: " << errorBuffer << std::endl;
            return;
        }

        std::string targetAddress, targetPort;
        std::cout << "Enter target address: ";
        std::cin >> targetAddress;
        std::cout << "Enter target port: ";
        std::cin >> targetPort;

        std::string connectEndpoint = "tcp://" + targetAddress + ":" + targetPort;
		socket_client_ = zmq_socket(context_, ZMQ_REQ);

        int connectResult = zmq_connect(socket_client_, connectEndpoint.c_str());

        if (connectResult != -1)
        {
            std::cout << "Connected successfully\n";
        }
        else
        {
             char errorBuffer[256];
             strerror_s(errorBuffer, sizeof(errorBuffer), errno);
             std::cerr << "Error connecting: " << errorBuffer << std::endl;

        }
        
		std::thread communicationThread(&SimplePeer::CommunicationLoop, this, std::ref(bc));

        communicationThread.join();
    }

private:
    void CommunicationLoop(Blockchain &bc) {
       // while (true) {

            int c;
            std::cout << "1.Send local database to the peer network\n2.Receive database from the peer network\n";
            std::cin >> c;

            switch (c)
            {
            case 1:
            {
                std::string content = bc.toJson();
                int sendResult = zmq_send(socket_client_, content.c_str(), content.size(), 0);

                if (sendResult != -1) {
                    std::cout << "Sent message: " << content << std::endl;
                } else {
                    char errorBuffer[256];
                    strerror_s(errorBuffer, sizeof(errorBuffer), zmq_errno());
                    std::cerr << "Error sending message: " << errorBuffer << std::endl;
                }

            }
            break;
            case 2:
            {
                char buffer[100000];
                int recvResult = zmq_recv(socket_server_, buffer, sizeof(buffer), 0);

                if (recvResult != -1) {
                    std::string receivedMessage(buffer, recvResult);
                    std::cout << "Received message: " << receivedMessage << std::endl;
                    bc=bc.fromJson(receivedMessage);
                } else {
                    char errorBuffer[256];
                    strerror_s(errorBuffer, sizeof(errorBuffer), errno);
                    std::cerr << "Error receiveing message: " << errorBuffer << std::endl;
                }

            }
            break;
            default:break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
   // }
    void* context_;
    void* socket_server_;
    void* socket_client_;
};



std::string active_file = "student_grading_system.db";

const double elo_k_factor = 32.0;
const double elo_initial = 1000.0;

int load_or_save_db(sqlite3* pInMemory, const char* zFilename, int isSave) {
	int rc;                   /* Function return code */
	sqlite3* pFile;           /* Database connection opened on zFilename */
	sqlite3_backup* pBackup;  /* Backup object used to copy data */
	sqlite3* pTo;             /* Database to copy to (pFile or pInMemory) */
	sqlite3* pFrom;           /* Database to copy from (pFile or pInMemory) */

	/* Open the database file identified by zFilename. Exit early if this fails
	** for any reason. */
	rc = sqlite3_open(zFilename, &pFile);
	if (rc == SQLITE_OK) {

		/* If this is a 'load' operation (isSave==0), then data is copied
		** from the database file just opened to database pInMemory.
		** Otherwise, if this is a 'save' operation (isSave==1), then data
		** is copied from pInMemory to pFile.  Set the variables pFrom and
		** pTo accordingly. */
		pFrom = (isSave ? pInMemory : pFile);
		pTo = (isSave ? pFile : pInMemory);

		/* Set up the backup procedure to copy from the "main" database of
		** connection pFile to the main database of connection pInMemory.
		** If something goes wrong, pBackup will be set to NULL and an error
		** code and message left in connection pTo.
		**
		** If the backup object is successfully created, call backup_step()
		** to copy data from pFile to pInMemory. Then call backup_finish()
		** to release resources associated with the pBackup object.  If an
		** error occurred, then an error code and message will be left in
		** connection pTo. If no error occurred, then the error code belonging
		** to pTo is set to SQLITE_OK.
		*/
		pBackup = sqlite3_backup_init(pTo, "main", pFrom, "main");
		if (pBackup) {
			(void)sqlite3_backup_step(pBackup, -1);
			(void)sqlite3_backup_finish(pBackup);
		}
		rc = sqlite3_errcode(pTo);
	}

	/* Close the database connection opened on database file zFilename
	** and return the result of this function. */
	(void)sqlite3_close(pFile);
	return rc;
}

enum class definition_method {
	Auto,
	List,
	List_And_Store,
	Manual,
	Manual_And_Store,
	Calc
};

struct Table_Schema {
	const std::string table_name;
	int num_columns;
	std::vector<std::string> column_names;
	std::vector<std::string> column_types;
	std::vector<definition_method> column_definition_method;
};

#define TABLES_COUNT 4
const static Table_Schema tables[TABLES_COUNT] = {
{
	/* .table_name                = */    "Students",
	/* .num_columns               = */    3,
	/* .column_names              = */    {"StudentID", "FirstName", "LastName"},
	/* .cloumn_types              = */    {"INTEGER PRIMARY KEY AUTOINCREMENT", "TEXT NOT NULL", "TEXT NOT NULL"},
	/* .column_definition_method  = */    {definition_method::Auto, definition_method::Manual, definition_method::Manual}
},

{
	/* .table_names               = */    "Works",
	/* .num_columns               = */    5,
	/* .column_names              = */    {"WorkID", "Subject", "Type", "Description", "Difficulty"},
	/* .column_types              = */    {"INTEGER PRIMARY KEY AUTOINCREMENT", "INTEGER NO NULL", "TEXT NOT NULL", "TEXT NO NULL", "INTEGER"},
	/* .column_definition_method  = */    {definition_method::Auto, definition_method::Manual, definition_method::Manual, definition_method::Manual, definition_method::Manual}
},

{
	/* .table_names               = */    "Grades",
	/* .num_columns               = */    7,
	/* .column_names              = */    {"GradeID", "WorkID", "StudentID", "Accuracy", "ELOPoints" ,"PerformencePoints","Description", "FOREIGN", "FOREIGN"},
	/* .column_types              = */    {"INTEGER PRIMARY KEY AUTOINCREMENT", "INTEGER NO NULL", "INTEGER NO NULL", "DOUBLE", "DOUBLE", "DOUBLE", "TEXT NOT NULL", "KEY (WorkID) REFERENCES Works(WorkID)", "KEY (StudentID) REFERENCES Works(StudentID)"},
	/* .column_definition_method  = */    {definition_method::Auto, definition_method::List_And_Store, definition_method::List_And_Store, definition_method::Manual_And_Store, definition_method::Calc, definition_method::Calc, definition_method::Manual}
},

{ 
	/* .table_names               = */    "Points",
	/* .num_columns               = */    4,
	/* .column_names              = */    {"PointsID", "StudentID", "ELO", "Performence", "FOREIGN"},
	/* .column_types              = */    {"INTEGER PRIMARY KEY AUTOINCREMENT", "INTEGER NO NULL", "DOUBLE", "DOUBLE", "KEY (StudentID) REFERENCES Works(StudentID)"},
	/* .column_definition_method  = */    {definition_method::Auto, definition_method::Auto, definition_method::Auto}
}

};


const std::unordered_map<std::string, size_t> table_index = {
	{"Students", 0},
	{"Accomplishments", 1},
};


void initialize_database(sqlite3* db) {
	// Execute SQL commands to create tables 
	for (int i = 0; i < sizeof(tables) / sizeof(tables[0]); i++)
	{
		std::string createTableSQL;
		createTableSQL += "CREATE TABLE IF NOT EXISTS " + tables[i].table_name + " (";
		for (int j = 0; j < tables[i].num_columns; j++)
		{
			createTableSQL += tables[i].column_names[j] + tables[i].column_types[j];
			if (j < tables[i].num_columns - 1) {
				createTableSQL += ", ";
			}
		}

		createTableSQL +=");";

		// Execute SQL command
		char* errMsg = nullptr;
		int result = sqlite3_exec(db, createTableSQL.c_str(), nullptr, nullptr, &errMsg);

		if (result != SQLITE_OK) {
			std::cerr << "Error creating table: " << tables[i].table_name << " " << errMsg << std::endl;
			sqlite3_free(errMsg);
		}
		else {
			std::cout << "Table " << tables[i].table_name << " created successfully." << std::endl;
		}
	}
}

std::string commands[] = {
	"exit",
	"help",
	"stream_database_to_file",
	"load_database_from_file",
	"display_tables_and_contents",
	"add_instance",
	"connect_to_network"
};

int help_command_impl(sqlite3* db)
{
	std::cout << "--Available commands--:\n";
	std::cout << "_______________________\n";
	for (int i = 0; i < sizeof(commands) / sizeof(commands[0]); i++)
	{
		std::cout << commands[i] << "\n";
	}
	std::cout << "_______________________\n";
	return 0;
}

int exit_command_impl(sqlite3* db)
{
	std::cout << "Closing the database." << std::endl;
	sqlite3_close(db);
	std::cout << "Exiting the program..." << std::endl;
	exit(0);
	return 0;
}

int stream_database_to_file(sqlite3* pInMemory) {
	return load_or_save_db(pInMemory, active_file.c_str(), 1); // isSave=1 (save to file)
}

int load_database_from_file(sqlite3* pInMemory) {
	return load_or_save_db(pInMemory, active_file.c_str(), 0); // isSave=0 (load from file)
}
int display_table(sqlite3* db, const char* tableName) {
	char selectQuery[100];
	sqlite3_stmt* stmt2;

	sprintf_s(selectQuery, "SELECT * FROM %s;", tableName);
	if (sqlite3_prepare_v2(db, selectQuery, -1, &stmt2, nullptr) == SQLITE_OK) {
		while (sqlite3_step(stmt2) == SQLITE_ROW) {
			for (int i = 0; i < sqlite3_column_count(stmt2); i++) {
				if (i > 0) {
					std::cout << ", ";
				}
				const char* columnValue = reinterpret_cast<const char*>(sqlite3_column_text(stmt2, i));
				std::cout << sqlite3_column_name(stmt2, i) << ": " << columnValue;
			}
			std::cout << std::endl;
		}
	}
	sqlite3_finalize(stmt2);
	return 0;
}

int display_tables_and_contents(sqlite3* db) {
	const char* query = "SELECT name FROM sqlite_master WHERE type='table';";
	sqlite3_stmt* stmt;

	if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) == SQLITE_OK) {
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			const char* tableName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
			std::cout << "Table: " << tableName << std::endl;

			char selectQuery[100];
			sqlite3_stmt* stmt2;

			sprintf_s(selectQuery, "SELECT * FROM %s;", tableName);
			if (sqlite3_prepare_v2(db, selectQuery, -1, &stmt2, nullptr) == SQLITE_OK) {
				while (sqlite3_step(stmt2) == SQLITE_ROW) {
					for (int i = 0; i < sqlite3_column_count(stmt2); i++) {
						if (i > 0) {
							std::cout << ", ";
						}
						const char* columnValue = reinterpret_cast<const char*>(sqlite3_column_text(stmt2, i));
						std::cout << sqlite3_column_name(stmt2, i) << ": " << columnValue;
					}
					std::cout << std::endl;
				}
			}
			sqlite3_finalize(stmt2);
		}

		sqlite3_finalize(stmt);
	}
	return 0;
}

double get_from_table_and_id(sqlite3* db, const char* column_name, const char* table_name, const char* id_name, int ID)
{
	double res;
	sqlite3_stmt* stmt;
	char query[1024];
	sprintf_s(query, "SELECT %s FROM %s WHERE %s = %d", column_name, table_name, id_name, ID);

	int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		// Handle the error
	}
	rc = sqlite3_step(stmt);
	if (rc == SQLITE_ROW) {
		res = sqlite3_column_double(stmt, 0);
	}
	else {
		// Handle the case where the ID is not found or any other error
	}

	sqlite3_finalize(stmt);

	return res;
}

std::vector<double> get_all_from_table_and_id(sqlite3* db, const char* column_name, const char* table_name, const char* id_name, int ID)
{
	std::vector<double> res;
	sqlite3_stmt* stmt;
	char query[1024];
	sprintf_s(query, "SELECT %s FROM %s WHERE %s = %d", column_name, table_name, id_name, ID);

	int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		// Handle the error
	}
	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
		double tmp = sqlite3_column_double(stmt, 0);
		res.push_back(tmp);
	}
	if (rc != SQLITE_DONE) {
		// Handle the case where the ID is not found or any other error
	}

	sqlite3_finalize(stmt);

	return res;
}
void update_at_table_and_id(sqlite3* db, const char* column_name, const char* table_name, const char* id_name, int ID, double value)
{
	sqlite3_stmt* stmt;
	char query[1024];
	sprintf_s(query, "UPDATE %s SET %s = %f WHERE %s = %d", table_name, column_name, value, id_name, ID);

	int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
	if (rc != SQLITE_OK) {
		// Handle the error
	}
	rc = sqlite3_step(stmt);
	if (rc == SQLITE_ROW) {
	}
	else {
		// Handle the case where the ID is not found or any other error
	}

	sqlite3_finalize(stmt);
}

void insert_into_table(sqlite3* db, const std::string table_name, const int num_columns, std::vector<std::string>column_names, std::vector<std::string> values )
{
	sqlite3_stmt* stmt;
	std::string insertSQL ="INSERT INTO "+ table_name + " (";

	for (int j = 1; j < num_columns; j++)
	{
		insertSQL += column_names[j];
		if (j < num_columns - 1) {
			insertSQL += ", ";
		}
	}

	insertSQL += ")";
	insertSQL += " VALUES (";
	for (int j = 1; j < num_columns; j++)
	{
		insertSQL += "'" + values[j - 1] + "'";
		if (j < num_columns - 1) {
			insertSQL+= ", ";
		}
	}

	insertSQL += ")";

	if (sqlite3_prepare_v2(db, insertSQL.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {

		if (sqlite3_step(stmt) == SQLITE_DONE) {
			std::cout << "New instance added - " << table_name << std::endl;
			for (int i = 1; i < num_columns; i++)
			{
				std::cout << column_names[i] << ": " << values[i - 1] << std::endl;
			}

			if (table_name == "Students")
			{
				sqlite3_int64 lastRowID = sqlite3_last_insert_rowid(db);
				insertSQL = "INSERT INTO Points (StudentID, ELO, Performence) VALUES (" + std::to_string(lastRowID) + "," + std::to_string(elo_initial) + ","+ std::to_string(0.0)+")";
				if (sqlite3_prepare_v2(db, insertSQL.c_str(), -1, &stmt, nullptr) == SQLITE_OK) {
					if (sqlite3_step(stmt) == SQLITE_DONE) {
						std::cout << "New instance added - Points" << std::endl;
					}
					else {
						std::cerr << "Error adding instance to the database - Points" << std::endl;
					}
				}
				else {
					std::cerr << "Error preparing SQL statement" << std::endl;
				}
			}
		}
		else {
			std::cerr << "Error adding instance to the database - " << table_name << std::endl;
			for (int i = 1; i < num_columns; i++)
			{
				std::cerr << column_names[i] << ": " << values[i - 1] << std::endl;
			}
		}

		sqlite3_finalize(stmt);
	}
	else {
		std::cerr << "Error preparing SQL statement" << std::endl;
	}

}

int add_instance_command_impl(sqlite3* db) {

	std::cout << "Pick a table to add an instance to: \n";
	for (int i = 0; i < sizeof(tables) / sizeof(tables[0]); i++)
	{
		std::cout << i << ". : " << tables[i].table_name << std::endl;
	}
	int table_index;
	std::cin >> table_index;
	Table_Schema table = tables[table_index];
	// assert(sizeof(tables) / sizeof(tables[0]) > table_index, "Invalid table index");
	std::cout << "Enter instance details:" << std::endl;
	std::vector<std::string> values;
	std::unordered_map<std::string, std::string> stored_values;

	for (int i = 0; i < table.num_columns; i++)
	{
		std::string value;
		switch (table.column_definition_method[i])
		{
		case definition_method::Auto:
			continue;
		case definition_method::List:
			display_table(db, (table.column_names[i]== "WorkID") ? "Works" : "Students");
			std::cout << "Enter from the above: " << table.column_names[i] << " :\n";
			std::cin >> value;
			values.push_back(value);
			break;
		case definition_method::List_And_Store:
			display_table(db, (table.column_names[i]== "WorkID") ? "Works" : "Students");
			std::cout << "Enter from the above " << table.column_names[i] << " :\n";
			std::cin >> value;
			stored_values[table.column_names[i]] = value;
			values.push_back(value);
			break;
		case definition_method::Manual:
			std::cout << table.column_names[i] << ": ";
			std::cin >> value;
			values.push_back(value);
			break;
		case definition_method::Manual_And_Store:
			std::cout << table.column_names[i] << ": ";
			std::cin >> value;
			stored_values[table.column_names[i]] = value;
			values.push_back(value);
			break;
		case definition_method::Calc:
		{
			auto it = stored_values.find("WorkID");
			if (it == stored_values.end())
			{
				std::cerr << "WorkID not found in stored values" << std::endl;
				exit(1);
			}
			int workID = std::stoi(it->second);
			it = stored_values.find("Accuracy");
			if (it == stored_values.end())
			{
				std::cerr << "Accuracy not found in stored values" << std::endl;
				exit(1);
			}
			double accuracy = std::stod(it->second);
			double difficulty = get_from_table_and_id(db, "Difficulty", "Works", "WorkID", workID);

			it = stored_values.find("WorkID");
			if (it == stored_values.end())
			{
				std::cerr << "WorkID not found in stored values" << std::endl;
				exit(1);
			}
			int StudentID = std::stoi(it->second);

			if (table.column_names[i] == "ELOPoints")
			{
				double current_elo = get_from_table_and_id(db, "ELO", "Points", "StudentID", StudentID);
				const double c = 400.0;
				double expected_accuracy = current_elo / difficulty; //c * std::log((std::expf(difficulty/c)+std::expf(current_elo/c)) / (1+std::expf(difficulty/c)));
				double change = elo_k_factor * (accuracy - expected_accuracy); // Calculate the ELOPoints based on the difficulty, current_elo and accuracy
				value = std::to_string(change);
				update_at_table_and_id(db, "ELO", "Points", "StudentID", StudentID, current_elo + change);

			}
			else if (table.column_names[i] == "PerformencePoints")
			{
				double performance = difficulty * accuracy;  // Calculate the PerformencePoints based on the difficulty and accuracy
				value = std::to_string(performance);

				std::vector<double> points = get_all_from_table_and_id(db, "PerformencePoints", "Grades", "StudentID", StudentID);
				points.push_back(performance);
				double total = 0;

				std::sort(points.begin(), points.end(), std::greater<double>());
				for (int i = 0; i < points.size(); i++)
				{
					double weight = std::pow(0.95, i);
					total += points[i] * weight;
				}

				update_at_table_and_id(db, "Performence", "Points", "StudentID", StudentID, total);
			}
			else
			{
				// Unreachable
				exit(1);
			}
			std::cout << "Calculated " << table.column_names[i] << " : " << value << " \n";
			values.push_back(value);

		}
		break;
		default:
			// Unreachable
			exit(1);
			break;
		}

	}
	
	insert_into_table(db, table.table_name, table.num_columns, table.column_names, values);
	return 0;
}

Blockchain database_to_blockchain(sqlite3* db) {
	Blockchain bc;
	const char* query = "SELECT name FROM sqlite_master WHERE type='table';";
	sqlite3_stmt* stmt;

	if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) == SQLITE_OK) {
		while (sqlite3_step(stmt) == SQLITE_ROW) {
			const char* tableName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
			std::string table= std::string("Table: ")+tableName;

			char selectQuery[100];
			sqlite3_stmt* stmt2;

			sprintf_s(selectQuery, "SELECT * FROM %s;", tableName);
			if (sqlite3_prepare_v2(db, selectQuery, -1, &stmt2, nullptr) == SQLITE_OK) {
				while (sqlite3_step(stmt2) == SQLITE_ROW) {
					std::string data ="";
					for (int i = 0; i < sqlite3_column_count(stmt2); i++) {
						data += ", ";
						const char* columnValue = reinterpret_cast<const char*>(sqlite3_column_text(stmt2, i));
						data += sqlite3_column_name(stmt2, i) + std::string(": ") + columnValue;
					}
					data += "\n";
					bc.addBlock(table + data);
				}
			}
			sqlite3_finalize(stmt2);
		}

		sqlite3_finalize(stmt);
	}
	return bc;
}

std::string chop_by_delimiter(std::string &input, char delimiter) {
    // Find the position of the delimiter in the input string
    size_t delimiterPos = input.find(delimiter);
    
    // If the delimiter is not found, return the entire input string and clear the input string
    if (delimiterPos == std::string::npos) {
        std::string result = input;
        input.clear();
        return result;
    }
    
    // Extract the substring before the delimiter
    std::string result = input.substr(0, delimiterPos);
    
    // Erase the part of the input string before the delimiter
    input.erase(0, delimiterPos + 1);
    
    // Remove white space characters from the result
    result.erase(std::remove_if(result.begin(), result.end(), [](unsigned char c) { return std::isspace(c); }), result.end());

    // Return the extracted substring
    return result;
}

void blockchain_to_database(sqlite3* db, Blockchain bc)
{
	for (int i=1; i<bc.getChain().size();i++)
	{
		auto block = bc.getChain()[i];
		std::string data = block.data;
		std::vector<std::string> column_names;
		std::vector<std::string> values;

		// Discard first name
		(void) chop_by_delimiter(data, ':');
		// First value is always table name
		const std::string table_name = chop_by_delimiter(data,',');

		while (data.size() > 0)
		{
			column_names.push_back(chop_by_delimiter(data, ':'));
			values.push_back(chop_by_delimiter(data, ','));

		}
		
		if (table_name != "sqlite_sequence")
		{
			insert_into_table(db, table_name, column_names.size(), column_names, values);
		}
	}
		
}

int connect_to_network_impl(sqlite3* db)
{    
	Blockchain bc = database_to_blockchain(db);
	SimplePeer peer;
	peer.Start(bc);
		
	blockchain_to_database(db,bc);
	return 0;
}

const std::unordered_map<std::string, int (*)(sqlite3*)> command_functions = {
	{"exit", &exit_command_impl},
	{"help", &help_command_impl},
	{"stream_database_to_file", &stream_database_to_file},
	{"load_database_from_file", &load_database_from_file},
	{"display_tables_and_contents", &display_tables_and_contents},
	{"add_instance", &add_instance_command_impl},
	{"connect_to_network_impl", &connect_to_network_impl}
};

std::string command_completion(const std::string& partialCommand) {
	std::string matchedCommand;

	for (const auto& entry : command_functions) {
		const std::string& command = entry.first;

		// Check if the command starts with the partial input
		if (command.compare(0, partialCommand.size(), partialCommand) == 0) {
			// If it does, set it as the matched command
			matchedCommand = command;
		}
	}

	return matchedCommand;
}

int main() {
    cJSON_Hooks hooks = { 0 }; 
	cJSON_InitHooks(&hooks);

	sqlite3* db;
	int dbResult = sqlite3_open("student_grading_system.db", &db);

	if (dbResult != SQLITE_OK) {
		std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
		return 1;
	}


	// Initialize the database
	initialize_database(db);
	std::vector<std::string> commands_history;
	int history_index = 0;

	// Main loop to accept commands
	while (true) {
		std::cout << "Enter a command: \n";
		std::string command;
		char ch;

		// Read characters until Enter is pressed
		while ((ch = _getch()) != '\r') {

			if (ch == 'à') {
				ch = _getch();
				if (ch == 72) {
					// Up arrow key
					if (!commands_history.empty()) {
						history_index = std::min(history_index + 1, static_cast<int>(commands_history.size()));
						// Erase the current command
						for (size_t i = 0; i < command.size(); ++i) {
							std::cout << "\b \b";
						}

						command = commands_history[commands_history.size() - history_index];
						std::cout << command;
					}
				}
				else if (ch == 80) {
					// Down arrow key
					if (!commands_history.empty()) {
						history_index = std::max(history_index - 1, 1);
						// Erase the current command
						for (size_t i = 0; i < command.size(); ++i) {
							std::cout << "\b \b";
						}

						command = commands_history[commands_history.size() - history_index];
						std::cout << command;
					}
				}
			}
			// Handle backspace key
			else
			{
				history_index = 0;
				if (ch == '\b') {
					if (!command.empty()) {
						std::cout << "\b \b";
						command.pop_back();
					}
				}
				// Handle tab key for command completion
				else if (ch == '\t') {
					std::string matchedCommand = command_completion(command);

					if (!matchedCommand.empty()) {
						// Erase the current command
						for (size_t i = 0; i < command.size(); ++i) {
							std::cout << "\b \b";
						}
						std::cout << matchedCommand;

						// Update the partialCommand with the matched command
						command = matchedCommand;
					}
				}
				else {
					std::cout << ch;
					command += ch;
				}
			}
		}
		std::cout << std::endl;
		history_index = 0;

		auto it = command_functions.find(command);

		if (it == command_functions.end())
		{
			std::cout << "Invalid command: " << command << std::endl;
			help_command_impl(db);
		}
		else
		{
			// Execute command implentation function
			it->second(db);
			commands_history.push_back(command);
		}

	}
	// Close the database connection when done
	std::cout << "Closing the database." << std::endl;
	sqlite3_close(db);

	return 0;
}
