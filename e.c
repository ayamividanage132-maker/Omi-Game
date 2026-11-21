/* Beginner-friendly Omi game for four players.
   Team 1: You + Computer Partner
   Team 2: Computer Opponent 1 + Computer Opponent 2
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PLAYERS 4
#define DECK_SIZE 32
#define HAND_SIZE 8

typedef enum {
    CLUBS, DIAMONDS, HEARTS, SPADES
} Suit;

typedef enum {
    SEVEN, EIGHT, NINE, TEN,
    JACK, QUEEN, KING, ACE
} Rank;

typedef struct {
    Suit suit;
    Rank rank;
} Card;

typedef struct {
    char name[32];
    Card hand[HAND_SIZE];
    int count;
} Player;

static const char *suit_names[] = {
    "Clubs", "Diamonds", "Hearts", "Spades"
};

static const char *rank_names[] = {
    "7", "8", "9", "10",
    "Jack", "Queen", "King", "Ace"
};

static const char *player_names[PLAYERS] = {
    "You",
    "Computer Partner",
    "Computer Opponent 1",
    "Computer Opponent 2"
};

/* Convert a suit into readable text. */
const char *suit_to_string(Suit s) {
    if (s >= CLUBS && s <= SPADES)
        return suit_names[s];

    return "Unknown";
}

/* Convert a rank into readable text. */
const char *rank_to_string(Rank r) {
    if (r >= SEVEN && r <= ACE)
        return rank_names[r];

    return "?";
}

/* Display one card. */
void print_card(Card c) {
    printf("%s of %s",
           rank_to_string(c.rank),
           suit_to_string(c.suit));
}

/* Create the 32-card deck. */
void initialize_deck(Card deck[DECK_SIZE]) {
    int i = 0;

    for (int s = CLUBS; s <= SPADES; s++) {
        for (int r = SEVEN; r <= ACE; r++) {
            deck[i].suit = (Suit)s;
            deck[i].rank = (Rank)r;
            i++;
        }
    }
}

/* Shuffle the deck randomly. */
void shuffle_deck(Card deck[DECK_SIZE]) {
    for (int i = DECK_SIZE - 1; i > 0; i--) {
        int j = rand() % (i + 1);

        Card temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

/* Initialize the four players. */
void init_players(Player players[PLAYERS]) {
    for (int i = 0; i < PLAYERS; i++) {
        snprintf(players[i].name,
                 sizeof(players[i].name),
                 "%s",
                 player_names[i]);

        players[i].count = 0;
    }
}

/* Deal cards to all four players. */
void deal_cards(Card deck[DECK_SIZE],
                int *next,
                Player players[PLAYERS],
                int amount) {

    for (int round = 0; round < amount; round++) {
        for (int p = 0; p < PLAYERS; p++) {

            if (*next < DECK_SIZE &&
                players[p].count < HAND_SIZE) {

                players[p].hand[players[p].count++] =
                    deck[(*next)++];
            }
        }
    }
}

/* Display a player's cards. */
void show_hand(const Player *p) {
    printf("\n%s's cards:\n", p->name);

    for (int i = 0; i < p->count; i++) {
        printf(" %d) ", i + 1);
        print_card(p->hand[i]);
        printf("\n");
    }
}

/* Check whether a player has a particular suit. */
int has_suit(const Player *p, Suit suit) {
    for (int i = 0; i < p->count; i++) {
        if (p->hand[i].suit == suit)
            return 1;
    }

    return 0;
}

/* Check whether a selected card is legal. */
int card_is_legal(const Player *p,
                  int index,
                  int has_lead,
                  Suit lead) {

    if (index < 0 || index >= p->count)
        return 0;

    if (!has_lead || p->hand[index].suit == lead)
        return 1;

    return !has_suit(p, lead);
}

/* Read and validate a number from the user. */
int read_number(const char *prompt, int min, int max) {
    char line[128];
    char *end;
    long value;

    while (1) {
        printf("%s", prompt);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\nInput ended. Exiting.\n");
            exit(0);
        }

        value = strtol(line, &end, 10);

        while (*end == ' ' || *end == '\t' || *end == '\n')
            end++;

        if (end != line &&
            *end == '\0' &&
            value >= min &&
            value <= max) {

            return (int)value;
        }

        printf("Please enter a number from %d to %d.\n",
               min, max);
    }
}

/* Ask the human player to select a card. */
int choose_human_card(const Player *p,
                      int has_lead,
                      Suit lead) {

    while (1) {
        int choice;

        show_hand(p);

        if (has_lead) {
            printf("You must follow %s if you have it.\n",
                   suit_to_string(lead));
        }

        choice = read_number(
            "Choose a card number: ",
            1,
            p->count
        ) - 1;

        if (card_is_legal(p, choice, has_lead, lead))
            return choice;

        printf("That move is not legal. Try again.\n");
    }
}

/* Simple computer strategy.
   Follow the lead suit if possible.
   Otherwise, play the first card.
*/
int choose_computer_card(const Player *p,
                         int has_lead,
                         Suit lead) {

    if (has_lead) {
        for (int i = 0; i < p->count; i++) {
            if (p->hand[i].suit == lead)
                return i;
        }
    }

    return 0;
}

/* Remove a card from a player's hand. */
Card remove_card(Player *p, int index) {
    Card chosen = p->hand[index];

    for (int i = index; i < p->count - 1; i++) {
        p->hand[i] = p->hand[i + 1];
    }

    p->count--;

    return chosen;
}

/* Ask the human player to choose the trump suit. */
Suit choose_trump(void) {
    printf("\nChoose trump suit:\n");
    printf("1) Clubs\n");
    printf("2) Diamonds\n");
    printf("3) Hearts\n");
    printf("4) Spades\n");

    return (Suit)(
        read_number("Enter 1-4: ", 1, 4) - 1
    );
}

/* Save the computer partner's cards to a text file. */
void save_partner_hand(const Player *partner) {
    FILE *file = fopen("partner_hand.txt", "w");

    if (!file) {
        perror("Could not create partner_hand.txt");
        return;
    }

    fprintf(file, "Computer partner's hand:\n");

    for (int i = 0; i < partner->count; i++) {
        fprintf(file, "%s of %s\n",
                rank_to_string(partner->hand[i].rank),
                suit_to_string(partner->hand[i].suit));
    }

    fclose(file);
}

/* Determine the winner of a trick.
   A trump card beats a non-trump card.
   Otherwise, the highest card of the lead suit wins.
*/
int determine_trick_winner(const Card played[PLAYERS],
                           int leader,
                           Suit trump) {

    Suit lead = played[0].suit;
    int best = 0;

    for (int i = 1; i < PLAYERS; i++) {

        int current_is_trump =
            (played[i].suit == trump);

        int best_is_trump =
            (played[best].suit == trump);

        /* A trump beats a non-trump. */
        if (current_is_trump && !best_is_trump) {
            best = i;
        }

        /* Higher trump beats lower trump. */
        else if (current_is_trump &&
                 best_is_trump &&
                 played[i].rank > played[best].rank) {

            best = i;
        }

        /* Higher card of the lead suit wins. */
        else if (!current_is_trump &&
                 !best_is_trump &&
                 played[i].suit == lead &&
                 played[best].suit == lead &&
                 played[i].rank > played[best].rank) {

            best = i;
        }

        /* Lead suit beats a different non-trump suit. */
        else if (!current_is_trump &&
                 !best_is_trump &&
                 played[i].suit == lead &&
                 played[best].suit != lead) {

            best = i;
        }
    }

    return (leader + best) % PLAYERS;
}

/* Write a message to the log file. */
void log_line(FILE *log, const char *message) {
    if (log)
        fprintf(log, "%s\n", message);
}

/* Main game function. */
void play_game(void) {
    int scores[2] = {0, 0};
    int deal_number = 1;

    FILE *log = fopen("omi_game.log", "w");

    if (!log)
        perror("Could not create omi_game.log");

    /* GAME STARTED message */
    printf("\n========================================\n");
    printf("              GAME STARTED\n");
    printf("========================================\n");

    printf("You and your partner play against two computers.\n");

    while (scores[0] < 10 && scores[1] < 10) {

        Card deck[DECK_SIZE];
        Player players[PLAYERS];

        int next = 0;
        int leader = 0;
        int tricks[2] = {0, 0};

        Suit trump;
        char message[160];

        printf("\n=== Deal %d | Team 1: %d - Team 2: %d ===\n",
               deal_number,
               scores[0],
               scores[1]);

        init_players(players);

        initialize_deck(deck);
        shuffle_deck(deck);

        /* Deal the first four cards to each player. */
        deal_cards(deck, &next, players, 4);

        show_hand(&players[0]);

        trump = choose_trump();

        printf("Trump is %s.\n", suit_to_string(trump));

        snprintf(message, sizeof(message),
                 "Deal %d: trump is %s",
                 deal_number,
                 suit_to_string(trump));

        log_line(log, message);

        /* Deal the remaining four cards to each player. */
        deal_cards(deck, &next, players, 4);

        save_partner_hand(&players[1]);

        /* Play eight tricks in each deal. */
        for (int trick = 0; trick < 8; trick++) {

            Card played[PLAYERS];

            int has_lead = 0;
            Suit lead = CLUBS;

            printf("\n--- Trick %d ---\n", trick + 1);

            /* Each player plays one card. */
            for (int order = 0; order < PLAYERS; order++) {

                int who = (leader + order) % PLAYERS;
                int index;

                if (who == 0) {
                    index = choose_human_card(
                        &players[who],
                        has_lead,
                        lead
                    );
                } else {
                    index = choose_computer_card(
                        &players[who],
                        has_lead,
                        lead
                    );
                }

                played[order] = remove_card(
                    &players[who],
                    index
                );

                /* First card determines the lead suit. */
                if (!has_lead) {
                    lead = played[order].suit;
                    has_lead = 1;
                }

                printf("%s plays ",
                       players[who].name);

                print_card(played[order]);
                printf("\n");

                if (log) {
                    fprintf(log, "%s played %s of %s\n",
                            players[who].name,
                            rank_to_string(played[order].rank),
                            suit_to_string(played[order].suit));

                    fflush(log);
                }
            }

            /* Find the winner of this trick. */
            int winner = determine_trick_winner(
                played,
                leader,
                trump
            );

            printf("%s wins the trick.\n",
                   players[winner].name);

            /* Players 0 and 1 are Team 1. */
            if (winner == 0 || winner == 1)
                tricks[0]++;
            else
                tricks[1]++;

            snprintf(message, sizeof(message),
                     "%s won trick %d",
                     players[winner].name,
                     trick + 1);

            log_line(log, message);

            /* Trick winner leads the next trick. */
            leader = winner;
        }

        /* Calculate the points for the completed deal. */
        printf("\nDeal tricks: Team 1 = %d, Team 2 = %d\n",
               tricks[0],
               tricks[1]);

        if (tricks[0] == 4 && tricks[1] == 4) {
            scores[0]++;
            scores[1]++;

            printf("4-4 draw: each team gets 1 point.\n");
        }

        else if (tricks[0] > tricks[1]) {
            scores[0] += 2;

            printf("Team 1 gets 2 points.\n");
        }

        else {
            scores[1] += 2;

            printf("Team 2 gets 2 points.\n");
        }

        printf("Score: Team 1 = %d, Team 2 = %d\n",
               scores[0],
               scores[1]);

        /* DEAL OVER message */
        printf("\n--------------- DEAL OVER ---------------\n");

        /* Show this message only after the first deal. */
        if (deal_number == 1) {
            printf("             FIRST DEAL OVER\n");
        }

        snprintf(message, sizeof(message),
                 "Deal %d result: tricks %d-%d; score %d-%d",
                 deal_number,
                 tricks[0],
                 tricks[1],
                 scores[0],
                 scores[1]);

        log_line(log, message);

        deal_number++;
    }

    /* Display the winning team. */
    if (scores[0] >= 10) {
        printf("\nYou and your partner win the match!\n");
    } else {
        printf("\nThe computer opponents win the match.\n");
    }

    /* GAME END message */
    printf("\n========================================\n");
    printf("                GAME END\n");
    printf("========================================\n");

    log_line(log, "Match finished.");

    if (log)
        fclose(log);
}

/* Program execution starts here. */
int main(void) {
    srand((unsigned int)time(NULL));

    play_game();

    return 0;
}
