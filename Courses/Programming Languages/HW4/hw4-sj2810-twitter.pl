/* Public Users */
user($jack,public).
user($john,public).
user($joseph,public).
user($sarthak,public).
user($atharva,public).
user($simran,public).
user($carl,public).
user($sidhant,public).
user($zeus,public).

/* Protected Users */
user($prerna,protected).
user($candace,protected).
user($phineas,protected).
user($lothar,protected).
user($logan,protected).
user($eren,protected).
user($mikasa,protected).
user($akasha,protected).
user($lanaya,protected).

/* Followers */
follows($john,$zeus).
follows($logan,$zeus).
follows($akasha,$zeus).
follows($simran,$zeus).

follows($sarthak,$prerna).
follows($jack,$prerna).
follows($sarthak,$prerna).
follows($atharva,$prerna).
follows($simran,$prerna).
follows($lothar,$prerna).
follows($lanaya,$prerna).
follows($logan,$prerna).
follows($carl,$prerna).

follows($eren,$akasha).
follows($john,$akasha).
follows($sarthak,$akasha).

follows($prerna,$sarthak).
follows($joseph,$sarthak).
follows($sidhant,$sarthak).

follows($mikasa,$sidhant).

follows($carl,$logan).
follows($phineas,$logan).

follows($candace,$carl).
follows($phineas,$carl).

/* Tweets */
tweet($zeus,1,[face,up,your,mortality]).
tweet($zeus,23,[divine,intervention]).
tweet($zeus,3,[make,your,offerings,zeus,is,here]).
tweet($zeus,2,[by,my,whiskers]).

tweet($prerna,33,[padinga]).
tweet($prerna,12,[i,love,it,when,you,call,me,senorita]).
tweet($prerna,10,[$sarthak,lakes,are,man,made]).
tweet($prerna,9,[i,was,very,magnanimous,but,now,im,timid]).
tweet($prerna,8,[such,is,life]).
tweet($prerna,7,[eat,sleep,pizza,dota,repeat]).
tweet($prerna,44,[kuchh,log]).
tweet($prerna,52,[$sarthak,what,do,you,want]).
tweet($prerna,13,[do,you,love,me]).
tweet($prerna,11,[padinga]).
tweet($prerna,83,[i,missed,my,stun,again]).
tweet($prerna,21,[kya,hai]).

tweet($atharva,37,[froggo]).

tweet($akasha,4,[there,will,be,pain]).
tweet($akasha,5,[pain,builds,character]).
tweet($akasha,42,[long,live,your,queen]).
tweet($akasha,92,[i,know,im,a,pain,but,you,love,me,anyway]).
tweet($akasha,104,[my,voice,returns]).

tweet($sarthak,100,[$prerna,why,are,you,like,this]).
tweet($sarthak,101,[have,you,tried,not,being,like,this]).
tweet($sarthak,17,[ju,wan,dots]).

tweet($sidhant,66,[im,so,good]).
tweet($sidhant,65,[blep]).

tweet($logan,1000,[fus,roh,da]).

/* Retweets */
retweet($akasha,23).

retweet($prerna,17).
retweet($prerna,42).
retweet($prerna,65).
retweet($prerna,2).

retweet($sarthak,23).
retweet($sarthak,7).

retweet($sidhant,17).
retweet($sidhant,23).

retweet($mikasa,23).

retweet($carl,2).
retweet($carl,1000).

/* Structures */
feedhelper(U,F,M,I) :- feedaggregate(U,F,M,I).
feedaggregate(U,F,M,I) :- follows(U,F), tweet(F,I,M).
feedaggregate(U,F,M,I) :- follows(U,F), retweet(F,I), tweet(J,I,M), user(J,protected), follows(U,J).
feedaggregate(U,F,M,I) :- follows(U,F), retweet(F,I), tweet(J,I,M), user(J,public).

feed(U,M) :- uniquefeed(U,O),remove_ident(O,M).

uniquefeed(U,R) :- setof([I,F|M],feedhelper(U,F,M,I),R).

remove_ident([],[]).
remove_ident([[_|Y]|T1],[H2|T2]) :- Y=H2,remove_ident(T1,T2).

search(K,U,M) :- tweet(U,_,M), member(K,M).


isviral(S,I,R) :- isviral_util(S,I,R,[]).
isviral_util(S,I,R,_) :- tweet(S,I,_), follows(T,S), retweet(T,I), follows(R,T).
isviral_util(S,I,R,Q) :- tweet(S,I,_), follows(T,S), retweet(T,I), isviral_util2(T,I,R,[S|Q]).
isviral_util2(S,I,R,Q) :- \+ member(S,Q), retweet(S,I), follows(T,S), retweet(T,I), follows(R,T).
isviral_util2(S,I,R,Q) :- \+ member(S,Q), retweet(S,I), follows(T,S), retweet(T,I), isviral_util2(T,I,R,[S|Q]).

isviral(S,I,R,M) :- isviral_util3(S,I,R,[],M,2).
isviral_util3(S,I,R,_,M,M) :- tweet(S,I,_), follows(T,S), retweet(T,I), follows(R,T).
isviral_util3(S,I,R,Q,M,Count) :- tweet(S,I,_), follows(T,S), retweet(T,I), B is Count + 1, isviral_util4(T,I,R,[S|Q],M,B).
isviral_util4(S,I,R,Q,M,M) :- \+ member(S,Q), retweet(S,I), follows(T,S), retweet(T,I), follows(R,T).
isviral_util4(S,I,R,Q,M,Count) :- \+ member(S,Q), retweet(S,I), follows(T,S), retweet(T,I), B is Count + 1, isviral_util4(T,I,R,[S|Q],M,B).

followers(U,F) :- follows(F,U).
tweets(U,T) :- tweet(U,_,T).
retweeters(I,U) :- retweet(U,I).
