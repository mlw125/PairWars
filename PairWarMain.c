// Author: Matthew Williams
// I would like to add that this is my first program in C, so there may be things that are unusual.
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

// define the number of players as 3
#define NUM_PLAYERS 3
enum Role{PLAYER1, PLAYER2, PLAYER3};

// function prototypes
// any function that has a long as a parameter will be getting the player's id
// long was chosen as I was having issues with regular int variables.
void *deal(void *);
void *player(void *);
void init();
void shuffleAndDeal();
void destroyDeck();
void grabFromDeck(long);
bool checkForPair(long);
void discardCard(long);
void displayDeck();
void displayHand(long);

// struct for a card holding the card value and the next pointer
struct card {
  int x;
  struct card * next;
};

// global pointers to cards, used to hold the deck data and the hand of each player
struct card * deckHead;
struct card * deckTail;
struct card * hand[3];

// global variable to hold the rounds and turn number
long rounds;
long turn;

// boolean values to be used as conditions
bool isWinner;
bool playersDone[3];
bool dealing;

// the mutexes and condition variables that will be used.
pthread_mutex_t m_dealing;
pthread_cond_t c_dealing;

pthread_mutex_t m_playing;
pthread_cond_t c_playing;

// this is the dealer function
void *deal(void *seed)
{
    do
    {
        // lock the mutex and initialize everything needed for the round.
        pthread_mutex_lock(&m_dealing);
        // this will remove any connection to the previous deck, so as to make sure everything is correct
        destroyDeck();

        // this is simply for making the output look nicer.
        if((rounds+1) < 3)
        {
            // Display that the deck is being shuffled
            printf("Dealer shuffling.\n");
            FILE * fileIO = fopen("Log.txt", "a");
            fprintf(fileIO, "Dealer shuffling.\n");
            fclose(fileIO);
        } // end if

        // shuffle the deck and deal out one card to each player.
        shuffleAndDeal();
        // the dealer will now increment the round to the next one and tell the
        // players who goes first.
        rounds++;
        turn = rounds;
        // Tell the players that there is no winner and that the dealer is done.
        isWinner = false;
        dealing = false;

        // if the rounds are less than 3 (off by one) then display the round that will be run.
        if(rounds < 3)
        {
            printf("\nRound %ld\n", rounds+1);
            printf("---------------------------\n");

            FILE * fileIO = fopen("Log.txt", "a");
            fprintf(fileIO, "\nRound %ld\n", rounds+1);
            fprintf(fileIO, "---------------------------\n");
            fclose(fileIO);

            displayDeck();
        } // end if

        // let the players know that they can now play.
        pthread_cond_signal(&c_playing);
        pthread_mutex_unlock(&m_dealing);

        // if the rounds are less than 3 (off by 1) then wait for the round to finish.
        // Otherwise this will leave the while loop and exit the thread.
        if(rounds < 3)
        {
            pthread_mutex_lock(&m_dealing);
            pthread_cond_wait(&c_dealing, &m_dealing);
            pthread_mutex_unlock(&m_dealing);
        } // end if
    } while(rounds < 3); // end do while

    pthread_exit(NULL);
} // end void *deal

// this is the function for the players. the id that is passed will be 0-2, but for display purposes
// the id will be added to one.
void *player(void *playerId)
{
    // get the player id
    long id;
    id = (long)playerId;

    // while rounds are less than 3 (starting at 0) then play the game.
    while(rounds < 3)
    {
        // while there is no winner then the game will be played.
        while(isWinner == false)
        {
            // lock the mutex and wait for the signal to start playing. If it not the players turn then they
            // will signal another player and then wait again.
            pthread_mutex_lock(&m_dealing);
            while(turn != id)
            {
                pthread_cond_wait(&c_playing, &m_dealing);

                if(turn != id)
                    pthread_cond_signal(&c_playing); // end if
            } // end while

            // if there is no winner
            if(isWinner == false)
            {
                displayHand(id);
                // the player will first draw.
                printf("Player %ld is drawing.\n", id+1);
                grabFromDeck(id);

                // the player will then check if their hand contains a match.
                // if so then the hand will displayed along with the fact that this player has won.
                // the player will then notify the other players that they won.
                if(checkForPair(id) == true)
                {
                    displayHand(id);

                    FILE * fileIO = fopen("Log.txt", "a");
                    fprintf(fileIO ,"Player %ld has won.\n", id+1);
                    printf("Player %ld has won.\n", id+1);
                    fclose(fileIO);

                    playersDone[id] = true;
                    isWinner = true;
                    // this is for the loop at the end of the function
                    dealing = true;
                } // end if
                else
                {
                    // if the player is not a winner then they will discard one card randomly and display their hand
                    discardCard(id);
                    displayHand(id);
                } // end else

                // find the next player who will go.
                long temp = turn + 1;
                if(temp == 3)
                    turn = PLAYER1;
                else
                    turn += 1;

                // display the state of the deck before the next player
                displayDeck();

                // signal to the other players that they can now go.
                pthread_cond_signal(&c_playing);
                pthread_mutex_unlock(&m_dealing);
            } // end if
        } // end while

        // if there is a winner then the other players will fall into here.
        // this will make sure every other player is finished and when they are all done, the dealer can begin.
        if(playersDone[id] != true)
        {
            // this will get whatever player is now going and make them check the state of the end of the round.
            switch(id)
            {
                // if the player is Player 1
                case PLAYER1:
                    // check if the other players are ready and if so, then say you are ready (not totally needed)
                    // and let the dealer know that they can begin.
                    if(playersDone[PLAYER2] == true && playersDone[PLAYER3] == true )
                    {
                        playersDone[PLAYER1] = true;
                        pthread_cond_signal(&c_dealing);
                        pthread_mutex_unlock(&m_dealing);
                    } // end if
                    else
                    {
                        // state that you are ready
                        playersDone[PLAYER1] = true;

                        // get the next player, which is player 2 (off by one)
                        turn = PLAYER2;

                        // let the next player know they can go.
                        pthread_cond_broadcast(&c_playing);
                        pthread_mutex_unlock(&m_dealing);
                    }
                    break;
                // if this is Player 2, documentation will be the same as Player 1
                case PLAYER2:
                    if(playersDone[PLAYER1] == true && playersDone[PLAYER3] == true )
                    {
                        playersDone[PLAYER2] = true;
                        pthread_cond_signal(&c_dealing);
                        pthread_mutex_unlock(&m_dealing);
                    }
                    else
                    {
                        playersDone[PLAYER2] = true;

                        // get the next player, which is Player 3 (off by one)
                        turn = PLAYER3;

                        pthread_cond_broadcast(&c_playing);
                        pthread_mutex_unlock(&m_dealing);
                    }
                    break;
                    // if the player is Player 3, documentation will be the same as Player 1
                    case PLAYER3:
                        if(playersDone[PLAYER2] == true && playersDone[PLAYER1] == true )
                        {
                            playersDone[PLAYER3] = true;
                            pthread_cond_signal(&c_dealing);
                            pthread_mutex_unlock(&m_dealing);
                        }
                        else
                        {
                            playersDone[PLAYER3] = true;

                            // get the next player, which is Player 1
                            turn = PLAYER1;

                            pthread_cond_broadcast(&c_playing);
                            pthread_mutex_unlock(&m_dealing);
                        }
                        break;
                    default:
                        printf("Error\n");
                        break;
                } // end switch
        } // end if

        // not the best form I realize, but I was having so many issues with another lock so I went with this.
        // this will simply loop until the dealer is done.
        // then the players will start the round or they exit if no more rounds are to be played.
        while(dealing == true)
        {
        } // end while
    } // end while

 /*   // let any waiting players know that they can
    pthread_mutex_lock(&m_dealing);
    pthread_cond_broadcast(&c_playing);
    pthread_mutex_unlock(&m_dealing);*/

    pthread_exit(NULL);
} // end voif *player

int main(int argc, char *argv[])
{
    // create the thread variables and seed the random values
    // finally initialize everything that is needed.
    pthread_t players[NUM_PLAYERS];
    pthread_t dealer;
    srand(atoi(argv[1]));
    init();

    // display that the game is starting
    FILE * fileIO = fopen("Log.txt", "w");
    fprintf(fileIO, "Game is starting.\n");
    fprintf(fileIO, "Seed is: %s\n", argv[1]);
    fclose(fileIO);

    // create the threads
    int ret;
    ret = pthread_create(&dealer, NULL, deal, NULL);
    if (ret) {
        printf("ERROR; return code from pthread_create() is %d\n", ret);
        exit(-1);
    } // end if

    long t;
    for(t=0; t<NUM_PLAYERS; t++)
    {
        printf("In main: creating thread %ld\n", (t+1));
        ret = pthread_create(&players[t], NULL, player, (void *)t);
        if (ret) {
            printf("ERROR; return code from pthread_create() is %d\n", ret);
            exit(-1);
        } // end if
    } // end for

    // join the threads
    ret = pthread_join(dealer, NULL);
    if (ret) {
         printf("ERROR; return code from pthread_join() is %d\n", ret);
         exit(-1);
    } //end if
    printf("\nDealer Thread Joined.\n");
    for(t=0; t<NUM_PLAYERS; t++) {
        ret = pthread_join(players[t], NULL);
        if (ret) {
            printf("ERROR; return code from pthread_join() is %d\n", ret);
            exit(-1);
        } // end if
        printf("Player %ld thread joined.\n", t+1);
    } // end for

    fileIO = fopen("Log.txt", "a");
    fprintf(fileIO, "\nProgram is done.\n");
    fclose(fileIO);

    return 0;
} // end main

// this function will initialize everything needed for the game to start
void init()
{
    // initialize the mutexes and the condition variables
    pthread_mutex_init(&m_playing, NULL);
	pthread_cond_init(&c_playing, NULL);

    pthread_mutex_init (&m_dealing, NULL);
    pthread_cond_init (&c_dealing, NULL);

    // set the deck head and tail pointers to NULL
    deckHead = NULL;
    deckTail = NULL;

    // make sure the players have nothing in their hand
    hand[PLAYER1] = NULL;
    hand[PLAYER2] = NULL;
    hand[PLAYER3] = NULL;

    // set the boolean variables to what they need to be
    dealing = true;
    isWinner = false;

    // make sure none of the players are ready before they even begin
    playersDone[PLAYER1] = false;
    playersDone[PLAYER2] = false;
    playersDone[PLAYER3] = false;

    // set the rounds to before the first and turn to -1 just to make sure nobody goes.
    rounds = -1;
    turn = -1;
}

// this function will, as the name implies, shuffle the deck and then deal out the top three cards, one to each player
void shuffleAndDeal()
{
    // Deck of cards that haven't been shuffle yet.
    // 11 - J, 12- Q, 13 - K, 14 - A
    int deck[52] = {2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6,6,7,7,7,7,8,8,8,8,
                    9,9,9,9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,
                    14,14,14,14};
    // how many cards are left to shuffle
    int cardsLeft = 52;

    // while there are cards to shuffle
    while(cardsLeft != 0)
    {
        // get a random position in the deck array
        int randPos = rand() % 52;

        // if the card has not already been placed in to the deck linked list
        if(deck[randPos] != -1)
        {
            // allocate memory for a new card
            struct card * temp;
            temp = (struct card *)malloc(sizeof(struct card));
            temp->x = deck[randPos];

            // make sure this position will not be chosen again
            deck[randPos] = -1;

            // if the deck linked list is empty then place the card at the deck head.
            if(deckHead == NULL)
            {
                deckHead = temp;
                deckTail = deckHead;
                deckHead->next = NULL;
            } //end if
            // otherwise the card will go to the end of the deck
            else
            {
                deckTail->next = temp;
                deckTail = temp;
                deckTail->next = NULL;
            } // end else
            cardsLeft--; // decrement the number of cards to be shuffled
        } // end if
    } // end while

    // hand out the cards, one to each player
    hand[PLAYER1] = deckHead;
    deckHead = deckHead->next;
    hand[PLAYER1]->next = NULL;

    hand[PLAYER2] = deckHead;
    deckHead = deckHead->next;
    hand[PLAYER2]->next = NULL;

    hand[PLAYER3] = deckHead;
    deckHead = deckHead->next;
    hand[PLAYER3]->next = NULL;
} // end shuffleAndDeal()

// this function is for when a player draws from the deck
void grabFromDeck(long playerid)
{
    // grab the card from the deckHead and then move it to the player's hand.
    // Then move the deckHead to the next card in the deck.
    struct card * temp = deckHead;
    deckHead = deckHead->next;
    hand[playerid]->next = temp;
    temp->next = NULL;

    // display the card that the player drew
    FILE * fileIO = fopen("Log.txt", "a");
    fprintf(fileIO, "Player %ld draws: ", playerid+1);
    fprintf(fileIO, "%d\n", temp->x);
    fclose(fileIO);

    temp = NULL;
} // end grabFromDeck()

// this function is when the player needs to check if they have a pair.
bool checkForPair(long playerid)
{
    // if both cards have the same value then return true, else return false.
    if(hand[playerid]->x == hand[playerid]->next->x)
        return true; // end if
    else
        return false; // end else
} // end checkForPair()

// this function is used when a player discards a card from their hand.
void discardCard(long playerid)
{
    // get a random value between 1 and 2
    int disCard = rand() % 2 + 1;
    // if the random value is one then we will be removing the first card in the hand.
    // This will move the card to the end of the deck and move the head of the hand to
    // the next card.
    if(disCard == 1)
    {
        deckTail->next = hand[playerid];
        hand[playerid] = hand[playerid]->next;
        deckTail = deckTail->next;
        deckTail->next = NULL;
    } // end if
    else
    {
        // if the random value is 2 then we will remove the second card in the hand and move
        // that card to the end of the deck.
        struct card * temp = hand[playerid]->next;
        hand[playerid]->next = NULL;
        deckTail->next = temp;
        deckTail = temp;
        deckTail->next = NULL;
        temp = NULL;
    } // end else

    FILE * fileIO = fopen("Log.txt", "a");
    printf("Player %ld discards: ", playerid+1);
    printf("%d\n", deckTail->x);
    fprintf(fileIO, "Player %ld's discards: ", playerid+1);
    fprintf(fileIO, "%d\n", deckTail->x);
    fclose(fileIO);
} // end discardCard()

// this function will free up the memory that the game has used.
// it also functions a little like an initialize function for the dealer
void destroyDeck()
{
    // go through the deck and free the memory.
    struct card * temp = deckHead;

    // we need to make sure to free the memory in every players' hand
    int x;
    for(x = 0; x < NUM_PLAYERS; x++)
    {
        // if the hand as at least one card in it then we will place it on the deck
        // to be deleted further down. Otherwise do nothing.
        if(hand[x] != NULL)
        {
            // if the hand has two cards (most likely the winner
            if(hand[x]->next != NULL)
            {
                deckTail->next = hand[x];
                deckTail = hand[x]->next;
            } // end if
            // if the hand only has one card
            else
            {
                deckTail->next = hand[x];
                deckTail = deckTail->next;
            } // end else
            hand[x] = NULL;
        }
    } // end for

    while(deckHead != deckTail)
    {
      deckHead = deckHead->next;
      free(temp);
      temp = deckHead;
    } // end while

    free(deckHead);

    deckHead = NULL;
    deckTail = NULL;
    temp = NULL;

    // make sure the players are now also ready.
    playersDone[PLAYER1] = false;
    playersDone[PLAYER2] = false;
    playersDone[PLAYER3] = false;
} // end destroyDeck()

// this function simply display the deck to the console and to the log file
void displayDeck()
{
    struct card * temp = deckHead;
    FILE * fileIO = fopen("Log.txt", "a");

    fprintf(fileIO, "\nDeck: ");
    printf("\nDeck: ");
    while(temp != deckTail)
    {
        fprintf(fileIO, " %d,", temp->x);
        printf(" %d,", temp->x);
        temp = temp->next;
    }

    fprintf(fileIO, " %d\n\n", temp->x);
    printf(" %d\n\n", temp->x);
    temp = NULL;
    fclose(fileIO);
} // end displayDeck()

// this function simply displays the contents of a player's hand to console and file
// this is gathered by passing the player's id to this function.
void displayHand(long playerId)
{
    FILE * fileIO = fopen("Log.txt", "a");
    printf("Player %ld's Hand: ", playerId+1);
    fprintf(fileIO, "Player %ld's Hand: ", playerId+1);

    // if the player has only one card in their hand.
    if(hand[playerId]->next == NULL)
    {
        printf("%d\n", hand[playerId]->x);
        fprintf(fileIO, "%d\n", hand[playerId]->x);
    } // end if
    // if the player has two cards in their hand
    else
    {
        printf("%d", hand[playerId]->x);
        fprintf(fileIO, "%d", hand[playerId]->x);
        printf(", %d\n", hand[playerId]->next->x);
        fprintf(fileIO, ", %d\n", hand[playerId]->next->x);
    } // end else

    fclose(fileIO);
} // end displayHand()
