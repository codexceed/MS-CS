female(mary).
male(john).
female(susan).
male(joseph).
father(john,mary).
father(joseph,john).
mother(susan,mary).
parent(X,Y) :- father(X,Y).
parent(X,Y) :- mother(X,Y).
daughter(X,Y) :- parent(Y,X), female(X).
grandfather(X,Y) :- male(X), parent(X,Z), parent(Z,Y).