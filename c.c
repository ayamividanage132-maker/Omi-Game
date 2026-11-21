#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#define DECK_SIZE 32
#define NUM_PLAYERS 4
#define HAND_SIZE 8

/* ================= CARD DEFINITIONS ================= */
typedef enum {
    CLUBS,
    DIAMONDS,
    HEARTS,
    SPADES
} Suit;

typedef enum {
    SEVEN,
    EIGHT,
    NINE,
    TEN,
    JACK,
    QUEEN,
    KING,
    ACE
} Rank;

typedef struct {
    Suit suit;
    Rank rank;
} Card;

const char* suit_to_string(Suit s) {
    switch (s) {
        case CLUBS:    return "Clubs";
        case DIAMONDS: return "Diamonds";
        case HEARTS:   return "Hearts";
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

/* ================= DECK MANAGEMENT ================= */
void init_deck(Card *deck) {
    int idx = 0;
    for (int s = CLUBS; s <= SPADES; s++) {
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

/* ================= PLAYER DISPLAY & VALIDATION ================= */
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

/* ================= TRUMP SELECTION ================= */
Suit select_trump(int selector_id, Card *first_four_cards) {
    if (selector_id == 0) {
        display_hand(first_four_cards, 4);
        int choice = 0;
        while (choice < 1 || choice > 4) {
            printf("Select Trump Suit (1: Clubs, 2: Diamonds, 3: Hearts, 4: Spades): ");
            if (scanf("%d", &choice) != 1) {
                while (getchar() != '\n');
            }
        }
        return (Suit)(choice - 1);
    } else {
        int suit_scores[4] = {0, 0, 0, 0};
        for (int i = 0; i < 4; i++) {
            suit_scores[first_four_cards[i].suit] += (first_four_cards[i].rank + 1);
        }
        int best_suit = 0;
        for (int s = 1; s < 4; s++) {
            if (suit_scores[s] > suit_scores[best_suit]) best_suit = s;
        }
        return (Suit)best_suit;
    }
}

/* ================= TRICK WINNER LOGIC ================= */
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
    Suit led_suit = trick[0].suit;
    int best_offset = 0;

    for (int i = 1; i < NUM_PLAYERS; i++) {
        if (card_beats(trick[i], trick[best_offset], led_suit, trump)) {
            best_offset = i;
        }
    }
    return (lead_player + best_offset) % NUM_PLAYERS;
}

/* ================= FILE-BASED PARTNER & COMPUTER AI ================= */
void save_partner_hand_to_file(const char *filename, Card *hand, int count) {
    FILE *fp = fopen(filename, "w");
    if (!fp) return;
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d %d\n", (int)hand[i].suit, (int)hand[i].rank);
    }
    fclose(fp);
}

int load_partner_hand_from_file(const char *filename, Card *hand) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;
    int count = 0;
    int s, r;
    while (count < HAND_SIZE && fscanf(fp, "%d %d", &s, &r) == 2) {
        hand[count].suit = (Suit)s;
        hand[count].rank = (Rank)r;
        count++;
    }
    fclose(fp);
    return count;
}

int find_current_trick_winner_offset(Card *trick, int cards_played, Suit led_suit, Suit trump) {
    int best = 0;
    for (int i = 1; i < cards_played; i++) {
        if (card_beats(trick[i], trick[best], led_suit, trump)) {
            best = i;
        }
    }
    return best;
}

Card get_computer_partner_move(const char *filename, Card *hand, int *count, Card *trick, int cards_played, Suit led_suit, Suit trump) {
    Card file_hand[HAND_SIZE];
    int file_count = load_partner_hand_from_file(filename, file_hand);
    if (file_count > 0) {
        *count = file_count;
        for (int i = 0; i < file_count; i++) {
            hand[i] = file_hand[i];
        }
    }

    int chosen_idx = 0;
    if (cards_played == 0) {
        for (int i = 1; i < *count; i++) {
            if (hand[i].rank > hand[chosen_idx].rank) chosen_idx = i;
        }
    } else {
        int win_offset = find_current_trick_winner_offset(trick, cards_played, led_suit, trump);
        bool teammate_winning = (cards_played >= 2 && win_offset == cards_played - 2);

        int best_idx = -1;
        for (int i = 0; i < *count; i++) {
            if (!is_valid_move(hand, *count, i, led_suit, false)) continue;
            if (best_idx == -1) {
                best_idx = i;
                continue;
            }
            if (teammate_winning) {
                if (hand[i].rank < hand[best_idx].rank) best_idx = i;
            } else {
                bool i_beats = card_beats(hand[i], trick[win_offset], led_suit, trump);
                bool best_beats = card_beats(hand[best_idx], trick[win_offset], led_suit, trump);
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

    save_partner_hand_to_file(filename, hand, *count);
    return selected;
}

Card get_computer_opponent_move(Card *hand, int *count, Card *trick, int cards_played, Suit led_suit, Suit trump) {
    int chosen_idx = 0;
    if (cards_played == 0) {
        for (int i = 1; i < *count; i++) {
            if (hand[i].rank > hand[chosen_idx].rank) chosen_idx = i;
        }
    } else {
        int win_offset = find_current_trick_winner_offset(trick, cards_played, led_suit, trump);
        bool teammate_winning = (cards_played >= 2 && win_offset == cards_played - 2);

        int best_idx = -1;
        for (int i = 0; i < *count; i++) {
            if (!is_valid_move(hand, *count, i, led_suit, false)) continue;
            if (best_idx == -1) {
                best_idx = i;
                continue;
            }
            if (teammate_winning) {
                if (hand[i].rank < hand[best_idx].rank) best_idx = i;
            } else {
                bool i_beats = card_beats(hand[i], trick[win_offset], led_suit, trump);
                bool best_beats = card_beats(hand[best_idx], trick[win_offset], led_suit, trump);
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

Card get_computer_move(int player_id, Card *hand, int *count, Card *trick, int cards_played, Suit led_suit, Suit trump) {
    if (player_id == 2) {
        return get_computer_partner_move("partner_hand.txt", hand, count, trick, cards_played, led_suit, trump);
    }
    return get_computer_opponent_move(hand, count, trick, cards_played, led_suit, trump);
}

/* ================= SCORING SYSTEM ================= */
typedef struct {
    int team_points[2];
    bool next_deal_special;
} GameScore;

void init_score(GameScore *gs) {
    gs->team_points[0] = 0;
    gs->team_points[1] = 0;
gs->next_deal_special = false;
}
int update_round_score(GameScore *gs, int *tricks_won, int trump_team) {
if (tricks_won[0] == 4 && tricks_won[1] == 4) {
printf("Deal ended in a 4-4 draw! Next deal is a special 2-point deal.\n");
gs->next_deal_special = true;
return -1;
}
int winner = (tricks_won[0] > tricks_won[1]) ? 0 : 1;
int points = 1;
if (gs->next_deal_special) {
points = 2;
gs->next_deal_special = false;
printf("Special Deal Won! Team %d earns 2 points!\n", winner + 1);
} else if (winner != trump_team) {
points = 2;
printf("Team %d defeated the trump team and earns 2 points!\n", winner + 1);
} else {
points = 1;
printf("Team %d defended their trump and earns 1 point!\n", winner + 1);
}
gs->team_points[winner] += points;
return winner;
}
void print_scoreboard(const GameScore *gs) {
printf("\n=== SCOREBOARD (First to 10 Points) ===\n");
printf("Team 1 (Player 1 & Partner P3): %d points\n", gs->team_points[0]);
printf("Team 2 (Player 2 & Player 4):   %d points\n", gs->team_points[1]);
printf("=======================================\n");
}
/* ================= LOGGING ================= */
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
/* ================= MAIN GAME LOOP ================= /
const char player_name(int id) {
switch (id) {
case 0: return "Player 1 (You)";
case 1: return "Player 2 (Computer Opponent 1)";
case 2: return "Player 3 (Computer Partner)";
case 3: return "Player 4 (Computer Opponent 2)";
default: return "Unknown";
}
}
void start_omi_game(void) {
GameScore gs;
init_score(&gs);
init_log("omi_game.log");
int trump_selector = 0;
int deal = 1;
char log_buf[256];
printf("========================================\n");
printf("         WELCOME TO OMI (C Edition)     \n");
printf("========================================\n");
while (gs.team_points[0] < 10 && gs.team_points[1] < 10) {
printf("\n================ DEAL %d ================\n", deal);
Card deck[DECK_SIZE];
Card hands[NUM_PLAYERS][HAND_SIZE];
int hand_counts[NUM_PLAYERS] = {8, 8, 8, 8};
init_deck(deck);
shuffle_deck(deck);
/* 1. Distribute first 4 cards to each player */
deal_cards(deck, 0, hands, 0, 4);
/* 2. Select trump suit */
printf("%s is selecting the trump suit...\n", player_name(trump_selector));
Suit trump = select_trump(trump_selector, hands[trump_selector]);
printf("Trump Suit for Deal %d: %s\n", deal, suit_to_string(trump));
snprintf(log_buf, sizeof(log_buf), "Deal %d - Trump selected by %s: %s",
deal, player_name(trump_selector), suit_to_string(trump));
write_log(log_buf);
/* 3. Distribute remaining 4 cards to each player */
deal_cards(deck, 16, hands, 4, 4);
/* 4. Save Computer Partner's (Player 3) 8 cards to file */
save_partner_hand_to_file("partner_hand.txt", hands[2], 8);
int tricks_won[2] = {0, 0};
int lead_player = trump_selector;
/* 5. Play 8 tricks */
for (int t = 1; t <= 8; t++) {
printf("\n--- Trick %d (Trump: %s) ---\n", t, suit_to_string(trump));
Card trick[NUM_PLAYERS];
for (int turn = 0; turn < NUM_PLAYERS; turn++) {
int current_player = (lead_player + turn) % NUM_PLAYERS;
Suit led_suit = (turn == 0) ? CLUBS : trick[0].suit;
if (current_player == 0) {
trick[turn] = get_human_move(hands[0], &hand_counts[0], led_suit, turn == 0);
} else {
trick[turn] = get_computer_move(current_player, hands[current_player],
&hand_counts[current_player], trick, turn, led_suit, trump);
}
printf("%s played ", player_name(current_player));
print_card(trick[turn]);
printf("\n");
snprintf(log_buf, sizeof(log_buf), "Deal %d Trick %d - %s played %s of %s",
deal, t, player_name(current_player),
rank_to_string(trick[turn].rank), suit_to_string(trick[turn].suit));
write_log(log_buf);
}
int winner = determine_trick_winner(trick, lead_player, trump);
tricks_won[winner % 2]++;
lead_player = winner;
printf("--> %s wins Trick %d! (Team 1: %d | Team 2: %d)\n",
player_name(winner), t, tricks_won[0], tricks_won[1]);
snprintf(log_buf, sizeof(log_buf), "Deal %d Trick %d Winner: %s", deal, t, player_name(winner));
write_log(log_buf);
}
int deal_winner = update_round_score(&gs, tricks_won, trump_selector % 2);
print_scoreboard(&gs);
snprintf(log_buf, sizeof(log_buf),
"Deal %d Outcome - Winner: %s | Score -> Team 1: %d, Team 2: %d",
deal, (deal_winner == -1) ? "Draw" : (deal_winner == 0 ? "Team 1" : "Team 2"),
gs.team_points[0], gs.team_points[1]);
write_log(log_buf);
trump_selector = (trump_selector + 1) % NUM_PLAYERS;
deal++;
}
int match_winner = (gs.team_points[0] >= 10) ? 1 : 2;
printf("\n========================================\n");
printf("               GAME OVER!               \n");
printf("      TEAM %d WINS THE OMI MATCH!        \n", match_winner);
printf("========================================\n");
snprintf(log_buf, sizeof(log_buf), "FINAL MATCH RESULT: Team %d Wins (%d - %d)",
match_winner, gs.team_points[0], gs.team_points[1]);
write_log(log_buf);
close_log();
}
int main(void) {
srand((unsigned int)time(NULL));
start_omi_game();
return 0;
}
