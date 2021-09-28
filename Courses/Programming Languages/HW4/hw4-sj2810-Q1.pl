male(brian).
male(kevin).
male(zhane).
male(fred).
male(jake).
male(bob).
male(stephen).
male(tom).
male(paul).

parent(tom,stephen).
parent(stephen,jennifer).
parent(melissa,brian).
parent(mary,sarah).
parent(bob,jane).
parent(paul,kevin).
parent(tom,mary).
parent(jake,bob).
parent(zhane,melissa).
parent(stephen,paul).
parent(emily,bob).
parent(zhane,mary).

grandfather(X,Y) :- male(X), parent(X,Z),parent(Z,Y).

sister(mary,stephen).
sister(melissa,stephen).
sister(mary,melissa).
sister(melissa,mary).

brother(stephen,mary).
brother(stephen,melissa).

aunt(X,Y) :- sister(X,Z), parent(Z,Y).
uncle(X,Y) :- brother(X,Z), parent(Z,Y).

/* Test cases 
aunt(mary,jennifer).
aunt(mary,melissa).
aunt(mary,brian).

uncle(stephen,sarah).
uncle(stephen,brian).
*/