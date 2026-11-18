#ifndef CARD_H
#define CARD_H

// Card suits
enum Suit
{
	CLUBS,
	DIAMONDS,
	HEARTS,
	SPADES
};

// Card ranks
enum Rank
{
	SEVEN,
	EIGHT,
	NINE,
	TEN,
	JACK,
	QUEEN,
	KING,
	ACE
};

// Card structure
struct Card
{
	enum Rank rank;
	enum Suit suit;
};

// Function declaration
void displayCard(struct Card card);

#endif
