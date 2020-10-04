# sql_state_machine
example of making a database into a sort of state machine, in a TCP/IP client/server model. 

This code is just throw-away to start to experiment with ideas.

* sql_state_engine.c - main engine supporting reading / writing from the database, maintaining a log, and rebuilding the data store from snap shots
* sql_state_server.c - TCP/IP interface wrapper around the engine
* client_test.py - simple example client to issue commands to the database 

the data itself consists of only one table, with one column [for illustration purposes]

The idea of the transaction log is to have a series of SQL statements in there like this:

	0: UPDATE xxx
	1: UPDATE yyy
	2: UPDATE zzz
	[...]
	501: UPDATE xxx

Then we have a routine rollDatabaseBack() that can recreate the database to state "N"
