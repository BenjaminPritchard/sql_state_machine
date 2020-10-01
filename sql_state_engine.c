//-----------------------------------------------------------------------------------------------
//
// SQL_State_Engine.c
//
// simple test app for treating a SQLite database as a state machine that can be 
// recreated (i.e rolled back) using snapshots &/or a read-only transaction log
//
// Benajmin Pritchard
// 1-October-2020
//
// In the interest of simplicity, this code is setup to be compiled in an executable.
// 
// You can use it two ways:
//		1) from the command line, to interact with the database
//		2) via TCP
// 
// (for details over how to interact with this via TCP, see sql_state_server.c)
//
// documentation on using SQLite from C:
// https://sqlite.org/cintro.html	
//
// Version History:
//		see version_history.txt
//
//-----------------------------------------------------------------------------------------------


#include <stdio.h>
#include <sqlite3.h> 
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define STRING_MAX	80
#define TRUE 1
#define FALSE 0 

const char *VersionString 	= "1.0";	
const char *TransactionFile = "transaction.log";	

// globals
sqlite3 *db;
char 	*zErrMsg = 0;
int 	rc;
char 	*sql;
int 	directorID = -1;
int 	finished = FALSE;

// callback from SQLite
// displays the contents of our database
static int callback(void *data, int argc, char **argv, char **azColName){
	int i;
	fprintf(stderr, "%s\n", (const char*)data);

	for(i = 0; i<argc; i++){
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}

	printf("\n");
	return 0;
}

// makes one atomic write to our transaction log
int writeToTransactionLog(char *string_to_write)
{
	
	FILE* file; 
	char line[256];
	int counter = 0;

	file = fopen(TransactionFile, "a");

	if (!file) {
		fprintf(stderr, "cannot open file: %s\n", TransactionFile);
		return 0;
	}
	
	fprintf (file, string_to_write);
	fclose(file);
	
	return TRUE;
}


// returns TRUE if we could update...
bool update_database() 
{
	char sql[256];

	// for this example, we only have one table, with one value
	sprintf(sql, "UPDATE data set value = value + 1\n");
	writeToTransactionLog(sql);

	rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);

	if( rc != SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		directorID = -1;
	} else {
		directorID = sqlite3_last_insert_rowid(db);		
		printf("database updated successfully\n");
	}

	return (rc == SQLITE_OK);

}

// read each line from the transaction log, and executes it against the database...
void dumpTransactionLog(const char* input_file)
{
	FILE* file; 
	char line[256];
	int counter = 0;

	file = fopen(input_file, "r");

	if (!file) {
		fprintf(stderr, "cannot open file: %s\n", input_file);
		return;
	}

	printf("parsing transaction log: %s\n", input_file);
	
	while (fgets(line, sizeof(line), file)) {
		printf(line);
	}
	
	fclose(file);
}

// read each line from the transaction log, and executes it against the database...
void buildDatabaseFromTransactionLog(const char* input_file)
{
	FILE* file; 
	char line[256];
	int counter = 0;

	file = fopen(input_file, "r");

	if (!file) {
		fprintf(stderr, "cannot open file: %s\n", input_file);
		return;
	}

	printf("parsing transaction log: %s\n", input_file);
	
	while (fgets(line, sizeof(line), file)) {

		// strip off trailing new line
		if (line[strlen(line)-1] == '\n') {
			line[strlen(line) - 1] = 0;
		}
		
		update_database(line);
	}
	
	fclose(file);
}

void PrintHelp()
{
	printf("commands:\n"
	" 0 [enter] exit \n"
	" 1 [enter] show state of database \n"
	" 2 [enter] show readonly transaction log \n" 
	" 3 [enter] update the database \n"
	" 4 [enter] roll database back to state n \n");

}

// right now, we ignore snapshots [which aren't implemented yet]
// and just rebuild soley from the begining of the transaction
// log
void rollDatabaseBack(const int prev_state_number)
{
	// TODO: figure out which snapshot to start from
	// figure out where in the transaction log the snapshot is
	// and only start from that point...
	
	// for now, just execute each line in the transaction log,
	// which will rebuild the database's state...
	
	buildDatabaseFromTransactionLog(TransactionFile);
}

void updateDatabase()
{
	update_database();
}

void showDatabase()
{

	int rowcount;
	sqlite3_stmt *stmt;
	
	char *sql = "SELECT * FROM data";
	
	// will call callback function once for each row that is returned...

	rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}

	// we need another version that will step through the results
	
}


void HandleKeyboard()
{
	int len;
	char line[STRING_MAX];

	fgets(line, STRING_MAX, stdin);
	len = strlen(line);
	if (len > 0) line[len - 1] = 0;
	
	if (strcmp(line, "?") == 0) {
		PrintHelp();
	}

	if (strcmp(line, "0") == 0) {
		finished = TRUE;
	}

	if (strcmp(line, "1") == 0) {
		showDatabase();
	}

	if (strcmp(line, "2") == 0) {
		dumpTransactionLog(TransactionFile);
	}
	
	if (strcmp(line, "3") == 0) {
		update_database();
		// TODO: check return value...
	}

	if (strcmp(line, "4") == 0) {
		printf("Enter State Number: ");
		int n;
		if (scanf("%d", &n) == 1) {
			rollDatabaseBack(n);
		}	
	}
}

void InitEngine()
{
	printf("sql_state_engine, version %s\n", VersionString);
	
	rc = sqlite3_open("data.sqlite", &db);

	if( rc ) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		exit(0);
	} 
	
}

void Shutdown() 
{
	sqlite3_close(db);
}

int main(int argc, char* argvp[])
{
	InitEngine();
	
	PrintHelp();
	while (!finished) {
		HandleKeyboard();
	} 
	
	Shutdown();
	return 0;
}
