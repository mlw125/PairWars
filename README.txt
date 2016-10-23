/* Original Readme

I have included the makefile that I used to run this program.
All that is need is for the program to run is to run make and then the program with an argument.
There is no error checking for there being no command argument so the program will likely crash or do other weird things.

The included log files have their seeds in the file name and will also have them inside the file itself.

*/

This program is called PairWars, as you saw. It involves a dealer and three players using multithreading with pthreads. It is written in C.

The game is simple. The dealer deals out a card to each player and then the first player goes. They get a card and check their hand fior a pair. If a pair is there then they win and the next round starts, beginning with the next player. If there is no pair then the next players goes and this repeats until there is a winner. Once three rounds are over then the game is over.

The game logs each move and what is happening and outputs a log of what happened, examples of which can be seen in the repository.

The program was designed to run omn linux. Run make and type ./PairWarsMain integer
