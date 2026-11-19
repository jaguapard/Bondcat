#pragma once
#include "Move.h"
#include <string>
#include "Square.h"
#include <optional>
#include <stdint.h>
#include <unordered_map>

struct CastlingRights
{
	bool whiteKingside = false, whiteQueenside = false, blackKingside = false, blackQueenside = false;

	//Returs whether or not a castling right corresponding this moveType is present. Returns false if moveType is not a castling move.
	bool corresponding(MoveType moveType) const
	{
		switch (moveType)
		{
		case MoveType::WHITE_KINGSIDE_CASTLE: return whiteKingside;
		case MoveType::BLACK_KINGSIDE_CASTLE: return blackKingside;
		case MoveType::WHITE_QUEENSIDE_CASTLE: return whiteQueenside;
		case MoveType::BLACK_QUEENSIDE_CASTLE: return blackQueenside;
		default: return false;
		}
	}

	bool allPresent() const
	{
		return whiteKingside && whiteQueenside && blackKingside && blackQueenside;
	}

	bool nonePresent() const
	{
		return !whiteKingside && !whiteQueenside && !blackKingside && !blackQueenside;
	}

	bool operator==(const CastlingRights& rhs) const
	{
		return whiteKingside == rhs.whiteKingside && blackKingside == rhs.blackKingside && whiteQueenside == rhs.whiteQueenside && blackQueenside == rhs.blackQueenside;
	}
};

struct FEN_Codec
{
	uint32_t halfmoves, fullmoves;
	CastlingRights castlingRights;
	bool whitesTurn;
	std::optional<Square> enpassant; //current turn's en passant square
	std::array<Piece, 64> board = { TeamedPieceType::EMPTY }; //board as array of pieces. default-initialized to fully empty board

	FEN_Codec() = default;
	FEN_Codec(const std::string& fen);	//extract data from FEN string
	std::string toFEN() const; //serializes data from this object to FEN string

	//static const std::unordered_map<char, TeamedPieceType> fenCharTo
};