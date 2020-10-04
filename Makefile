all: sql_state_engine 
sql_state_engine: sql_state_engine.c
	gcc sql_state_engine.c -g -l sqlite3 -o sql_state_engine
