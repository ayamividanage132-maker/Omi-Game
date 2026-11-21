/* Beginner-friendly Omi game for four players.
   Team 1: Human (0) + Computer Partner (1)
   Team 2: Computer Opponent 1 (2) + Computer Opponent 2 (3)
   Compile: gcc -std=c11 -Wall -Wextra -pedantic omi_game.c -o omi_game
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PLAYERS 4
#define DECK_SIZE 32
#define HAND_SIZE 8

typedef enum { CLUBS, DIAMONDS, HEARTS, SPADES } Suit;
typedef enum { SEVEN, EIGHT, NINE, TEN, JACK, QUEEN, KING, ACE } Rank;
typedef struct { Suit suit; Rank rank; } Card;
typedef struct { char name[32]; Card hand[HAND_SIZE]; int count; } Player;

static const char *suit_names[] = {"Clubs", "Diamonds", "Hearts", "Spades"};
static const char *rank_names[] = {"7", "8", "9", "10", "Jack", "Queen", "King", "Ace"};
static const char *player_names[PLAYERS] = {"You", "Computer Partner", "Computer Opponent 1", "Computer Opponent 2"};

const char *suit_to_string(Suit s) { return (s >= CLUBS && s <= SPADES) ? suit_names[s] : "Unknown"; }
const char *rank_to_string(Rank r) { return (r >= SEVEN && r <= ACE) ? rank_names[r] : "?"; }
void print_card(Card c) { printf("%s of %s", rank_to_string(c.rank), suit_to_string(c.suit)); }

void initialize_deck(Card deck[DECK_SIZE]) {
    int i = 0;
    for (int s = CLUBS; s <= SPADES; s++)
        for (int r = SEVEN; r <= ACE; r++) {
            deck[i].suit = (Suit)s;
            deck[i].rank = (Rank)r;
            i++;
        }
}

void shuffle_deck(Card deck[DECK_SIZE]) {
    for (int i = DECK_SIZE - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Card temp = deck[i]; deck[i] = deck[j]; deck[j] = temp;
    }
}

void init_players(Player players[PLAYERS]) {
    for (int i = 0; i < PLAYERS; i++) {
        snprintf(players[i].name, sizeof(players[i].name), "%s", player_names[i]);
        players[i].count = 0;
    }
}

void deal_cards(Card deck[DECK_SIZE], int *next, Player players[PLAYERS], int amount) {
    for (int round = 0; round < amount; round++)
        for (int p = 0; p < PLAYERS; p++)
            if (*next < DECK_SIZE && players[p].count < HAND_SIZE)
                players[p].hand[players[p].count++] = deck[(*next)++];
}

void show_hand(const Player *p) {
    printf("\n%s's cards:\n", p->name);
    for (int i = 0; i < p->count; i++) {
        printf(" %d) ", i + 1); print_card(p->hand[i]); printf("\n");
    }
}

int has_suit(const Player *p, Suit suit) {
    for (int i = 0; i < p->count; i++) if (p->hand[i].suit == suit) return 1;
    return 0;
}

int card_is_legal(const Player *p, int index, int has_lead, Suit lead) {
    if (index < 0 || index >= p->count) return 0;
    if (!has_lead || p->hand[index].suit == lead) return 1;
    return !has_suit(p, lead); /* Must follow suit when possible. */
}

int read_number(const char *prompt, int min, int max) {
    char line[128], *end;
    long value;
    while (1) {
        printf("%s", prompt);
        if (!fgets(line, sizeof(line), stdin)) { printf("\nInput ended. Exiting.\n"); exit(0); }
        value = strtol(line, &end, 10);
        while (*end == ' ' || *end == '\t' || *end == '\n') end++;
        if (end != line && *end == '\0' && value >= min && value <= max) return (int)value;
        printf("Please enter a number from %d to %d.\n", min, max);
    }
}

int choose_human_card(const Player *p, int has_lead, Suit lead) {
    while (1) {
        int choice;
        show_hand(p);
        if (has_lead) printf("You must follow %s if you have it.\n", suit_to_string(lead));
        choice = read_number("Choose a card number: ", 1, p->count) - 1;
        if (card_is_legal(p, choice, has_lead, lead)) return choice;
        printf("That move is not legal. Try again.\n");
    }
}

/* Simple computer strategy: follow suit if possible; otherwise play first card. */
int choose_computer_card(const Player *p, int has_lead, Suit lead) {
    if (has_lead) {
        for (int i = 0; i < p->count; i++) if (p->hand[i].suit == lead) return i;
    }
    return 0;
}

Card remove_card(Player *p, int index) {
    Card chosen = p->hand[index];
    for (int i = index; i < p->count - 1; i++) p->hand[i] = p->hand[i + 1];
    p->count--;
    return chosen;
}

Suit choose_trump(void) {
    printf("\nChoose trump suit:\n1) Clubs  2) Diamonds  3) Hearts  4) Spades\n");
    return (Suit)(read_number("Enter 1-4: ", 1, 4) - 1);
}

/* Save partner's cards to the required supporting text file. */
void save_partner_hand(const Player *partner) {
    FILE *file = fopen("partner_hand.txt", "w");
    if (!file) { perror("Could not create partner_hand.txt"); return; }
    fprintf(file, "Computer partner's hand (for assignment support):\n");
    for (int i = 0; i < partner->count; i++)
        fprintf(file, "%s of %s\n", rank_to_string(partner->hand[i].rank), suit_to_string(partner->hand[i].suit));
    fclose(file);
}

/* played[] is in the order cards were played, beginning with leader. */
int determine_trick_winner(const Card played[PLAYERS], int leader, Suit trump) {
    Suit lead = played[0].suit;
    int best = 0;
    for (int i = 1; i < PLAYERS; i++) {
        int current_is_trump = (played[i].suit == trump);
        int best_is_trump = (played[best].suit == trump);
        if (current_is_trump && !best_is_trump) best = i;
        else if (current_is_trump && best_is_trump && played[i].rank > played[best].rank) best = i;
        else if (!current_is_trump && !best_is_trump && played[i].suit == lead && played[best].suit == lead && played[i].rank > played[best].rank) best = i;
        else if (!current_is_trump && !best_is_trump && played[i].suit == lead && played[best].suit != lead) best = i;
    }
    return (leader + best) % PLAYERS;
}

void log_line(FILE *log, const char *message) { if (log) fprintf(log, "%s\n", message); }

void play_game(void) {
    int scores[2] = {0, 0};
    int deal_number = 1;
    FILE *log = fopen("omi_game.log", "w");
    if (!log) perror("Could not create omi_game.log");

    printf("=== OMI CARD GAME ===\nYou and your partner play against two computers.\n");
    while (scores[0] < 10 && scores[1] < 10) {
        Card deck[DECK_SIZE];
        Player players[PLAYERS];
        int next = 0, leader = 0, tricks[2] = {0, 0};
        Suit trump;
        char message[160];

        printf("\n=== Deal %d | Team 1: %d - Team 2: %d ===\n", deal_number, scores[0], scores[1]);
        init_players(players); initialize_deck(deck); shuffle_deck(deck);
        deal_cards(deck, &next, players, 4); /* First four cards to each player. */
        show_hand(&players[0]);
        trump = choose_trump();
        printf("Trump is %s.\n", suit_to_string(trump));
        snprintf(message, sizeof(message), "Deal %d: trump is %s", deal_number, suit_to_string(trump)); log_line(log, message);
        deal_cards(deck, &next, players, 4); /* Remaining four cards to each player. */
        save_partner_hand(&players[1]);

        for (int trick = 0; trick < 8; trick++) {
            Card played[PLAYERS];
            int has_lead = 0;
            Suit lead = CLUBS;
            printf("\n--- Trick %d ---\n", trick + 1);
            for (int order = 0; order < PLAYERS; order++) {
                int who = (leader + order) % PLAYERS;
                int index;
                if (who == 0) index = choose_human_card(&players[who], has_lead, lead);
                else index = choose_computer_card(&players[who], has_lead, lead);
                played[order] = remove_card(&players[who], index);
                if (!has_lead) { lead = played[order].suit; has_lead = 1; }
                printf("%s plays ", players[who].name); print_card(played[order]); printf("\n");
                if (log) { fprintf(log, "%s played %s of %s\n", players[who].name, rank_to_string(played[order].rank), suit_to_string(played[order].suit)); fflush(log); }
            }
            int winner = determine_trick_winner(played, leader, trump);
            printf("%s wins the trick.\n", players[winner].name);
            tricks[(winner == 0 || winner == 1) ? 0 : 1]++;
            snprintf(message, sizeof(message), "%s won trick %d", players[winner].name, trick + 1); log_line(log, message);
            leader = winner;
        }

        printf("\nDeal tricks: Team 1 = %d, Team 2 = %d\n", tricks[0], tricks[1]);
        if (tricks[0] == 4 && tricks[1] == 4) { scores[0]++; scores[1]++; printf("4-4 draw: each team gets 1 point.\n"); }
        else if (tricks[0] > tricks[1]) { scores[0] += 2; printf("Team 1 gets 2 points.\n"); }
        else { scores[1] += 2; printf("Team 2 gets 2 points.\n"); }
        printf("Score: Team 1 = %d, Team 2 = %d\n", scores[0], scores[1]);
        snprintf(message, sizeof(message), "Deal %d result: tricks %d-%d; score %d-%d", deal_number, tricks[0], tricks[1], scores[0], scores[1]); log_line(log, message);
        deal_number++;
    }
    if (scores[0] >= 10) printf("\nYou and your partner win the match!\n");
    else printf("\nThe computer opponents win the match.\n");
    log_line(log, "Match finished.");
    if (log) fclose(log);
}

int main(void) {
    srand((unsigned int)time(NULL));
    play_game();
    return 0;
}

