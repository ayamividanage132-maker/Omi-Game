#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define DECK_SIZE 32
#define NUM_PLAYERS 4
#define HAND_SIZE 8

/* ================= 1. CARD DEFINITIONS ================= */
typedef enum { HEARTS, DIAMONDS, CLUBS, SPADES } Suit;
typedef enum { SEVEN = 7, EIGHT, NINE, TEN, JACK, QUEEN, KING, ACE } Rank;

typedef struct {
    Suit suit;
    Rank rank;
} Card;

const char* suit_to_string(Suit s) {
    switch (s) {
        case HEARTS:   return "Hearts";
        case DIAMONDS: return "Diamonds";
        case CLUBS:    return "Clubs";
        case SPADES:   return "Spades";
        default:       return "Unknown";
    }
}

const char* rank_to_string(Rank r) {
    switch (r) {
        case SEVEN: return "7";
        case EIGHT: return "8";
        case NINE:  return "9";
        case TEN:   return "10";
        case JACK:  return "J";
        case QUEEN: return "Q";
        case KING:  return "K";
        case ACE:   return "A";
        default:    return "?";
    }
}

void print_card(Card c) {
    printf("[%s of %s]", rank_to_string(c.rank), suit_to_string(c.suit));
}

/* ================= 2. DECK MANAGEMENT ================= */
void init_deck(Card *deck) {
    int idx = 0;
    for (int s = HEARTS; s <= SPADES; s++) {
        for (int r = SEVEN; r <= ACE; r++) {
            deck[idx].suit = (Suit)s;
            deck[idx].rank = (Rank)r;
            idx++;
        }
    }
}

void shuffle_deck(Card *deck) {
    for (int i = DECK_SIZE - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Card temp = deck[i];
        deck[i] = deck[j];
        deck[j] = temp;
    }
}

void deal_cards(Card *deck, int start_idx, Card hands[NUM_PLAYERS][HAND_SIZE], int hand_offset, int count) {
    int idx = start_idx;
    for (int p = 0; p < NUM_PLAYERS; p++) {
        for (int c = 0; c < count; c++) {
            hands[p][hand_offset + c] = deck[idx++];
        }
    }
}

/* ================= 3. PLAYER INPUT & VALIDATION ================= */
void display_hand(Card *hand, int count) {
    printf("\nYour Hand:\n");
    for (int i = 0; i < count; i++) {
        printf("  %d: ", i + 1);
        print_card(hand[i]);
        printf("\n");
    }
}

bool has_suit(Card *hand, int count, Suit suit) {
    for (int i = 0; i < count; i++) {
        if (hand[i].suit == suit) return true;
    }
    return false;
}

bool is_valid_move(Card *hand, int count, int choice, Suit led_suit, bool is_first_card) {
    if (choice < 0 || choice >= count) return false;
    if (is_first_card) return true;
    if (has_suit(hand, count, led_suit) && hand[choice].suit != led_suit) {
        return false;
    }
    return true;
}

Card get_human_move(Card *hand, int *count, Suit led_suit, bool is_first_card) {
    int choice = -1;
    display_hand(hand, *count);

    while (true) {
        printf("Choose a card (1-%d): ", *count);
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("Invalid input. Please enter a number.\n");
            continue;
        }
        choice--;
        if (is_valid_move(hand, *count, choice, led_suit, is_first_card)) {
            break;
        }
        printf("Invalid move! You must follow the led suit (%s) if you have it.\n", suit_to_string(led_suit));
    }

    Card selected = hand[choice];
    for (int i = choice; i < *count - 1; i++) {
        hand[i] = hand[i + 1];
    }
    (*count)--;
    return selected;
}

/* ================= 4. TRUMP SELECTION ================= */
Suit select_trump(int selector_id, Card *first_four_cards) {
    if (selector_id == 0) {
        display_hand(first_four_cards, 4);
        int choice = 0;
        while (choice < 1 || choice > 4) {
            printf("Select Trump Suit (1: Hearts, 2: Diamonds, 3: Clubs, 4: Spades): ");
            if (scanf("%d", &choice) != 1) {
                while (getchar() != '\n');
            }
        }
        return (Suit)(choice - 1);
    } else {
        int suit_scores[4] = {0, 0, 0, 0};
        for (int i = 0; i < 4; i++) {
            suit_scores[first_four_cards[i].suit] += first_four_cards[i].rank;
        }
        int best_suit = 0;
        for (int s = 1; s < 4; s++) {
            if (suit_scores[s] > suit_scores[best_suit]) best_suit = s;
        }
        return (Suit)best_suit;
    }
}

/* ================= 5. TRICK WINNER LOGIC ================= */
bool card_beats(Card challenger, Card current_best, Suit led_suit, Suit trump) {
    (void)led_suit;
    if (challenger.suit == trump && current_best.suit != trump) return true;
    if (challenger.suit != trump && current_best.suit == trump) return false;
    if (challenger.suit == current_best.suit) {
        return challenger.rank > current_best.rank;
    }
    return false;
}

int determine_trick_winner(Card *trick, int lead_player, Suit trump) {
    Suit led_suit = (*trick).suit;
    int best_offset = 0;

    for (int i = 1; i < NUM_PLAYERS; i++) {
        if (card_beats(trick[i], trick[best_offset], led_suit, trump)) {
            best_offset = i;
        }
    }
    return (lead_player + best_offset) % NUM_PLAYERS;
}

/* ================= 6. COMPUTER AI STRATEGY ================= */
Card get_computer_move(int player_id, Card *hand, int *count, Card *trick, int cards_played, Suit led_suit, Suit trump) {
    int chosen_idx = 0;

    if (cards_played == 0) {
        for (int i = 1; i < *count; i++) {
            if (hand[i].rank > hand[chosen_idx].rank) chosen_idx = i;
        }
    } else {
        int current_winner = 0;
        for (int i = 1; i < cards_played; i++) {
            if (card_beats(trick[i], trick[current_winner], led_suit, trump)) {
                current_winner = i;
            }
        }
        int winning_player = (player_id - cards_played + current_winner + NUM_PLAYERS) % NUM_PLAYERS;
        bool partner_winning = (winning_player == (player_id + 2) % NUM_PLAYERS);

        int best_idx = -1;
        for (int i = 0; i < *count; i++) {
            if (!is_valid_move(hand, *count, i, led_suit, false)) continue;

            if (best_idx == -1) {
                best_idx = i;
                continue;
            }
            if (partner_winning) {
                if (hand[i].rank < hand[best_idx].rank) best_idx = i;
            } else {
                bool i_beats = card_beats(hand[i], trick[current_winner], led_suit, trump);
                bool best_beats = card_beats(hand[best_idx], trick[current_winner], led_suit, trump);
                if (i_beats && !best_beats) best_idx = i;
                else if (i_beats == best_beats && hand[i].rank < hand[best_idx].rank) best_idx = i;
            }
        }
        chosen_idx = best_idx;
    }

    Card selected = hand[chosen_idx];
    for (int i = chosen_idx; i < *count - 1; i++) {
        hand[i] = hand[i + 1];
    }
    (*count)--;
    return selected;
}

/* ================= 7. SCORING SYSTEM ================= */
typedef struct {
    int team_tokens[2];
    int bonus_pool;
} GameScore;

void init_score(GameScore *gs) {
    *(gs->team_tokens) = 0;
    *(gs->team_tokens + 1) = 0;
    gs->bonus_pool = 0;
}

void update_round_score(GameScore *gs, int *tricks_won, int trump_team) {
    int t1_tricks = *tricks_won;
    int t2_tricks = *(tricks_won + 1);

    if (t1_tricks == 4 && t2_tricks == 4) {
        printf("Round tied 4-4! +1 bonus token added to the next round.\n");
        gs->bonus_pool += 1;
        return;
    }

    int winner = (t1_tricks > t2_tricks) ? 0 : 1;
    int points = 1 + gs->bonus_pool;
    gs->bonus_pool = 0;

    if (tricks_won[winner] == 8) {
        points += 1;
        printf("KAPOTHI! Team %d won all 8 tricks!\n", winner + 1);
    } else if (winner != trump_team) {
        points += 1;
        printf("Team %d broke the trump team!\n", winner + 1);
    }

    gs->team_tokens[winner] += points;
    printf("Team %d earns %d token(s) this round!\n", winner + 1, points);
}

void print_scoreboard(const GameScore *gs) {
    printf("\n=== SCOREBOARD (First to 10 Tokens) ===\n");
    printf("Team 1 (You & Partner P3): %d tokens\n", *(gs->team_tokens));
    printf("Team 2 (Opponent P2 & P4): %d tokens\n", *(gs->team_tokens + 1));
    printf("=======================================\n");
}

/* ================= 8. FILE LOGGING ================= */
static FILE *log_file = NULL;

void init_log(const char *filename) {
    log_file = fopen(filename, "w");
    if (log_file) {
        fprintf(log_file, "Omi Game Session Started\n");
        fflush(log_file);
    }
}

void write_log(const char *message) {
    if (!log_file) return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(log_file, "[%02d:%02d:%02d] %s\n", t->tm_hour, t->tm_min, t->tm_sec, message);
    fflush(log_file);
}

void close_log(void) {
    if (log_file) {
        write_log("Omi Game Session Ended");
        fclose(log_file);
        log_file = NULL;
    }
}

/* ================= 9. MAIN GAME ENGINE ================= */
const char* player_name(int id) {
    switch (id) {
        case 0: return "You (P1)";
        case 1: return "Opponent (P2)";
        case 2: return "Partner (P3)";
        case 3: return "Opponent (P4)";
        default: return "Unknown";
    }
}

void start_omi_game(void) {
    GameScore gs;
    init_score(&gs);
    init_log("omi_game.log");

    int dealer = 0;
    int round = 1;

    while (*(gs.team_tokens) < 10 && *(gs.team_tokens + 1) < 10) {
        printf("\n================ ROUND %d ================\n", round);
        Card deck[DECK_SIZE];
        Card hands[NUM_PLAYERS][HAND_SIZE];
        int hand_counts[NUM_PLAYERS] = {8, 8, 8, 8};

        init_deck(deck);
        shuffle_deck(deck);

        /* Step 1: Distribute first 4 cards to each player */
        deal_cards(deck, 0, hands, 0, 4);

        /* Step 2: Trump selector chooses trump based on first 4 cards */
        int trump_selector = (dealer + 1) % NUM_PLAYERS;
        printf("%s is selecting the trump suit...\n", player_name(trump_selector));

