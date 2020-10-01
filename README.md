# sql_state_machine
example of making a database into a sort of state machine, in a TCP/IP client/server model. 

This code is just throw-away to start to expiriment with ideas.

sql_state_engine.c - main engine supporting reading / writing from the database, maintaining a log, taking snapshots, and rebuilding the data store from snap shots
sql_state_server.c - TCP/IP interface wrapper around the engine
client_test.py - simple example client to issue commands to the database 

the data itself consists of only one table, with one column

NOTE: I didn't really test any of this code yet. 

This initial comit is just an example of how to structure something this (i.e. into client/server), and a way to expiriment with different ideas.

The idea of the transaction log is to have a series of SQL statements in ther like this:

0: UPDATE xxx
1: UPDATE yyy
2: UPDATE zzz
[...]
500: SNAPSHOT_1
501: UPDATE xxx

* the lines that say SNAPSHOT are written to the log whenever we issue a SNAPSHOT command

* (Because of how SQLite is implemented, a database snapshot just consists of a filecopy)

* We have a routine rollDatabaseBack() that can roll back the database to any numbered line above. Currently it does that just by executing each line in order. Next I will make the engine start from the closest available snap_shot to N, and then rebuild from there.

