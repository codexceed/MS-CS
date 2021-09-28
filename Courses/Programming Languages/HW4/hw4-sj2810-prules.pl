remove_item(I,L,O) :- remove_item(I,L,[],O).
remove_item(I,[I|L],Aux,O) :- remove_item(I,L,Aux,O), !.
remove_item(I,[J|L],Aux,O) :- append(Aux,[J],X), remove_item(I,L,X,O).
remove_item(_,[],Aux,Aux).

remove_items([I],L,O) :- remove_item(I,L,O).
remove_items([Item|I],L,O) :- remove_item(Item,L,A), remove_items(I,A,O).

intersection2(L1,L2,F) :- intersection2(L1,L2,[],F).
intersection2([L|L1],L2,Aux,F) :- member(L,L2), append(Aux,[L],O), remove_item(L,L1,R1), remove_item(L,L2,R2), intersection2(R1,R2,O,F), !.
intersection2([L|L1],L2,Aux,F) :- \+member(L,L2), remove_item(L,L1,R1), intersection2(R1,L2,Aux,F), !.
intersection2([],_,Aux,Aux).

is_set([L|Ls]) :- \+member(L,Ls), is_set(Ls).
is_set([]).

disjunct_union(L1,L2,U) :- intersection2(L1,L2,I), append(L1,L2,C), remove_items(I,C,U).

remove_dups(L1,L2) :- remove_dups(L1,[],L2).
remove_dups([L|L1],Aux,L2) :- \+member(L,L1), append(Aux,[L],O), remove_dups(L1,O,L2), !.
remove_dups([L|L1],Aux,L2) :- member(L,L1), remove_dups(L1,Aux,L2),!.
remove_dups([],Aux,Aux).

union(L1,L2,U) :- append(L1,L2,C), remove_dups(C,U).