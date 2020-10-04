# sql_state_machine
Example of making a database into a sort of state machine. (This code is just throw-away to start to experiment with ideas.)

the data itself consists of only one table, with one column [for illustration purposes]

The idea of the transaction log is to have a series of SQL statements in there like this:

	0: UPDATE xxx
	1: UPDATE yyy
	2: UPDATE zzz
	[...]
	501: UPDATE xxx

Then we have a routine rollDatabaseBack() that can recreate the database to state "N"


