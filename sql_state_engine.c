//-----------------------------------------------------------------------------------------------
//
// SQL_State_Engine.c
//
// simple test app for treating a SQLite database as a state machine that can be 
// recreated (i.e rolled back) using an append-only transaction log
//
// Benajmin Pritchard
// 1-October-2020
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

const char *VersionString 	= "1.1";	
const char *TransactionFile = "transaction.log";
const char *DatabaseName	= "data.sqlite";	

// globals
sqlite3 *db;
char 	*zErrMsg = 0;
int 	rc;
bool	finished = FALSE;

// callback from SQLite
// called once per row
static int callback(void *data, int argc, char **argv, char **azColName){
	
	// just print out the SQL statement that resulted
	fprintf(stderr, "%s\n", (const char*)data);

	// in this example program, we only get one column back
	// but theoretically we could get back multiple columns, so just print
	// them all out
	for(int i = 0; i<argc; i++){
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}

	printf("\n");
	return 0;
}

// returns total number of entries in the transaction log
int lines_in_transaction_log()
{
	FILE* file; 
	char line[256];
	int counter = 0;

	file = fopen(TransactionFile, "r");

	if (!file) {
		fprintf(stderr, "cannot open file: %s\n", TransactionFile);
		return 0;
	}

	while (fgets(line, sizeof(line), file)) {
		counter++;
	}
	
	fclose(file);
	
	return counter;
}


// makes one atomic write to our transaction log
bool writeToTransactionLog(char *string_to_write)
{
	
	FILE* file; 
	char line[256];

	file = fopen(TransactionFile, "a");

	if (!file) {
		fprintf(stderr, "cannot open file: %s\n", TransactionFile);
		return 0;
	}
	
	sprintf(line, "%i,%s", lines_in_transaction_log()+1, string_to_write);
	
	fprintf (file, line);
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
	} else {
		printf("database updated successfully\n");
	}

	return (rc == SQLITE_OK);
}

// returns TRUE if we could update...
bool execute_SQL(const char *sql) 
{
	rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);

	if( rc != SQLITE_OK ){
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	} else {
		printf("database updated successfully\n");
	}

	return (rc == SQLITE_OK);
}

// just print out the transaction log
void dumpTransactionLog()
{
	FILE* file; 
	char line[256];
	int counter = 0;

	file = fopen(TransactionFile, "r");

	if (!file) {
		fprintf(stderr, "cannot open file: %s\n", TransactionFile);
		return;
	}

	while (fgets(line, sizeof(line), file)) {
		printf(line);
	}
	
	fclose(file);
}


// read each line from the transaction log (up to state_number), 
// and executes it against the database.
void buildDatabaseFromTransactionLog(const char* input_file, const int state_number)
{
	FILE* file; 
	char line[256];
	char *line_ptr;
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
		
		// strip off the initial line number
		line_ptr = line;
		while (*line_ptr != ',')
			line_ptr++;
		
		// now skip the comma...
		line_ptr++;
		
		if (counter++ < state_number)
			execute_SQL(line_ptr);
		else 
			break;	// we are done; only go up to state "n" [i.e. line #n]
	}
	
	fclose(file);
}

void PrintHelp()
{
	printf("commands:\n"
	" 0 [enter] exit \n"
	" 1 [enter] show state of database \n"
	" 2 [enter] print readonly transaction log \n" 
	" 3 [enter] update the database \n"
	" 4 [enter] roll database back to state n \n");
	
	fflush(stdout);
}

void ApplyInitialSchema()
{
	execute_SQL("CREATE TABLE IF NOT EXISTS data (value int);");
	execute_SQL("INSERT into data (value) values (0);");
}

void rollDatabaseBack(const int prev_state_number)
{
	
	// close our existing connection to the database
	sqlite3_close(db);
	
	// delete the database all together
	if(remove(DatabaseName) != 0) {
	  printf("Error: unable to delete database file");
	  exit(0);
	}
	
	// create a new database...
	printf("Rebuilding database...\n");
	
	rc = sqlite3_open(DatabaseName, &db);

	if( rc ) {
		fprintf(stderr, "Can't create database file %s. Error: %s\n", DatabaseName, sqlite3_errmsg(db));
		exit(0);
	} 
	
	// database is blank right now, so create our table
	ApplyInitialSchema();
	
	// and rebuild it from the log file, back to state "N"
	buildDatabaseFromTransactionLog(TransactionFile, prev_state_number);

	printf("database successfully rebuilt\n");
	
}

void showDatabase()
{
	int rowcount;
	sqlite3_stmt *stmt;
	
	const char *sql = "SELECT * FROM data";
	
	// will call callback function once for each row that is returned...

	rc = sqlite3_exec(db, sql, callback, (void*)sql, &zErrMsg);
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
	
	if (len == 0 && strcmp(line, "?") == 0) {
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
	}

	if (strcmp(line, "4") == 0) {
		printf("Enter State Number: ");
		int n;
		if (scanf("%d", &n) == 1) {
			rollDatabaseBack(n);
		}	
	}
}

bool FileExists(const char * filename){
    FILE *file;
    if (file = fopen(filename, "r")){
        fclose(file);
        return TRUE;
    }
    return FALSE;
}

void InitEngine()
{
	bool should_initialize = !FileExists(DatabaseName);
	
	printf("sql_state_engine, version %s\n", VersionString);
	
	rc = sqlite3_open(DatabaseName, &db);

	if( rc ) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		exit(0);
	} 
	
	if (should_initialize) 
	{
		printf("Creating database for initial use...\n");
		ApplyInitialSchema();
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
