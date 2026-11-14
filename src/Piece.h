#pragma once
#include "enums.h"
#include <stdint.h>

enum class Team
{
	NONE,
	WHITE,
	BLACK,
};

enum class PieceType
{
	PAWN,
	KNIGHT,
	BISHOP,
	ROOK,
	QUEEN,
	KING,
	EMPTY,
};

enum class TeamedPieceType : uint8_t
{
	EMPTY,
	WHITE_PAWN,
	WHITE_KNIGHT,
	WHITE_BISHOP,
	WHITE_ROOK,
	WHITE_QUEEN,
	WHITE_KING,
	BLACK_PAWN,
	BLACK_KNIGHT,
	BLACK_BISHOP,
	BLACK_ROOK,
	BLACK_QUEEN,
	BLACK_KING,
};

class Piece
{
public:
	Piece() = default;
	Piece(TeamedPieceType teamedType) { teamedType = teamedType; }
	Piece(PieceType type, Team team) 
	{
		if (type == PieceType::EMPTY || team == Team::NONE) teamedType = TeamedPieceType::EMPTY;
		else 
		{
			bool white = team == Team::WHITE;
			using enum TeamedPieceType;
			switch (type)
			{
			case PieceType::PAWN: teamedType = white ? WHITE_PAWN : BLACK_PAWN; break;
			case PieceType::KNIGHT: teamedType = white ? WHITE_KNIGHT : BLACK_KNIGHT; break;
			case PieceType::BISHOP: teamedType = white ? WHITE_BISHOP : BLACK_BISHOP; break;
			case PieceType::ROOK: teamedType = white ? WHITE_ROOK : BLACK_ROOK; break;
			case PieceType::QUEEN: teamedType = white ? WHITE_QUEEN : BLACK_QUEEN; break;
			case PieceType::KING: teamedType = white ? WHITE_KING : BLACK_KING; break;
			default: break;
			}
		}
	}
	Team getTeam() const
	{
		switch (teamedType)
		{
		case TeamedPieceType::EMPTY: 
			return Team::NONE;
		case TeamedPieceType::WHITE_PAWN:
		case TeamedPieceType::WHITE_KNIGHT:
		case TeamedPieceType::WHITE_BISHOP:
		case TeamedPieceType::WHITE_ROOK:
		case TeamedPieceType::WHITE_QUEEN:
		case TeamedPieceType::WHITE_KING:
			return Team::WHITE;
		default: 
			return Team::BLACK;
		}
	}
	PieceType getType() const
	{
		switch (teamedType)
		{
		case TeamedPieceType::EMPTY:
			return PieceType::EMPTY;
		case TeamedPieceType::WHITE_PAWN:
		case TeamedPieceType::BLACK_PAWN:
			return PieceType::PAWN;
		case TeamedPieceType::WHITE_KNIGHT:
		case TeamedPieceType::BLACK_KNIGHT:
			return PieceType::KNIGHT;
		case TeamedPieceType::WHITE_BISHOP:
		case TeamedPieceType::BLACK_BISHOP:
			return PieceType::BISHOP;
		case TeamedPieceType::WHITE_ROOK:
		case TeamedPieceType::BLACK_ROOK:
			return PieceType::ROOK;
		case TeamedPieceType::WHITE_QUEEN:
		case TeamedPieceType::BLACK_QUEEN:
			return PieceType::QUEEN;
		//case TeamedPieceType::WHITE_KING:
		//case TeamedPieceType::BLACK_KING:
		default:
			return PieceType::KING;
		}
	}
	//void setType()

	Team __declspec(property(get = getTeam)) team;
	PieceType __declspec(property(get = getType)) type;
	TeamedPieceType teamedType = TeamedPieceType::EMPTY;
private:
	
};