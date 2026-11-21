#include "enums.h"


std::string StringifyEnum(MoveCheckStatus status)
{
	using enum MoveCheckStatus;
	switch (status)
	{
	case LEGAL: return "Legal move.";
	case LEGALITY_CHECK_SKIPPED: return "Legality check skipped.";
	case ILLEGAL_REASON_NOT_SPECIFIED: return "Illegal with reason unspecified.";

	case SRC_OUT_OF_BOUNDS: return "The source square is outside the chessboard.";
	case DST_OUT_OF_BOUNDS: return "The destination square is outside the chessboard.";
	case STATIONARY_MOTION: return "The source and destination squares are the same.";
	case TURN_TEAM_MISMATCH: return "Attempted to move a piece belonging to the other side.";
	case MOVING_EMPTY_SQUARE: return "Attempted to move empty square.";
	case MOVING_OBSTRUCTED_BY_ALLY: return "Destination square is occupied by an allied piece.";
	case TRYING_TO_CAPTURE_KING: return "A king cannot be captured.";

	case CASTLING_CHECK_ON_NONCASTLING_MOVE: return "Castling validation was requested for a non-castling move.";
	case CASTLING_RIGHTS_ABSENT: return "The corresponding team has no castling right for this castle.";
	case CASTLING_NOT_KING: return "The expected king square does not contain a king.";
	case CASTLING_NOT_ROOK: return "The expected rook square does not contain a rook.";
	case CASTLING_KING_MOVED: return "The king has already moved, so castling is not allowed.";
	case CASTLING_ROOK_MOVED: return "The rook has already moved, so castling is not allowed.";
	case CASTLING_KING_ROOK_TEAM_MISMATCH: return "The castling king and rook belong to different sides.";
	case CASTLING_TEAM_MISMATCHED_KING_TEAM: return "The king does not belong to the side attempting to castle.";
	case CASTLING_ATTEMPTED_ON_OPPONENTS_TURN: return "Castling attempted on opponent's turn.";
	case CASTLING_PATH_OBSTRUCTED: return "Castling path is obstructed by another piece.";
	case CASTLING_PATH_PASSES_THROUGH_CHECK: return "Castling path passes out of, through, or into check.";

	case MOVING_BAD_KNIGHT_JUMP: return "A knight must move in an L-shape: two squares in one direction and one perpendicular.";
	case PROMOTION_OF_NON_PAWN_PIECE: return "Attempted to promote non-pawn piece.";
	case PAWN_PROMOTION_ON_NONLAST_RANK: return "A pawn can only promote when reaching the last rank.";
	case PAWN_PROMOTION_ELIDED: return "A pawn reaching the last rank must promote.";
	case PAWN_BACKWARDS_MOVE: return "A pawn must move forward toward the opponent's side.";
	case MOVED_PAWN_STEP_TOO_FAR: return "A pawn that has already moved can only advance one square.";
	case NOT_MOVED_PAWN_STEP_TOO_FAR: return "A pawn on its starting square can advance at most two squares.";
	case PAWN_MORE_THAN_ONE_FILE: return "A pawn cannot move more than one file sideways.";
	case PAWN_DIAGONAL_MOVE_ON_FIRST_2MOVE: return "An pawn cannot move to a side when moving forward by 2 steps on it's first move.";
	case PAWN_WRONG_ENPASSANT: return "Invalid en passant capture. An en passant capture must be a one-square diagonal move.";
	case PAWN_WRONG_ENPASSANT_2: return "The en passant target square must be reached with a one-square diagonal pawn move.";
	case PAWN_FILE_MOVE_ON_NON_CAPTURE: return "A pawn cannot move diagonally to an empty square unless capturing en passant.";
	case PAWN_NO_FILE_MOVE_ON_CAPTURE: return "A pawn must capture diagonally.";
	case PAWN_GENERAL_MOVEMENT_ILLEGAL: return "Pawn movement fits none of these rules prior to pathing check: one step forward, two steps forward for unmoved, diagonal capture, diagonal en passant capture.";

	case KING_TOO_FAR: return "A king can only move one square in any direction, except when castling.";
	case KING_RUNS_INTO_ANOTHER_KING: return "A king can't move into squares attacked by another king.";
	case MOVING_SLIDING_PATHS_EXIST_BUT_NONE_MATCH: return "Sliding piece path exists but none match (this usually means that chosen square is on the right path, but obstructed).";
	case MOVING_TEAMS_KING_IN_CHECK: return "Move puts or leaves moving team's king in check.";

	default: return "Unknown move-check status: " + std::to_string(int(status));
	}
}

std::string StringifyEnum(GameStatus status)
{
	using enum GameStatus;
	switch (status)
	{
	case GameStatus::ONGOING: return "Game in-progress.";
	case GameStatus::WHITE_WON_BY_TIME: return "White wins: black ran out of time.";
	case GameStatus::BLACK_WON_BY_TIME: return "Black wins: white ran out of time.";
	case GameStatus::WHITE_WON_BY_CHECKMATE: return "White won by checkmate.";
	case GameStatus::BLACK_WON_BY_CHECKMATE: return "Black won by checkmate.";
	case GameStatus::WHITE_WON_ENEMY_RESIGNED: return "White wins: black resigned.";
	case GameStatus::BLACK_WON_ENEMY_RESIGNED: return "Black wins: white resigned.";
	case GameStatus::DRAW_ACCEPTED: return "Sides agreed to a draw.";
	case GameStatus::DRAW_STALEMATE: return "Stalemate.";
	case GameStatus::DRAW_THREEFOLD_REPETITION: return "Draw by threefold repetition.";
	case GameStatus::DRAW_FIFTY_MOVE_RULE: return "Draw by 50-move rule.";
	default: return "Unknown GameStatus: " + std::to_string(int(status));
	}
}

std::string StringifyEnum(PieceType type)
{
	using enum PieceType;
	switch (type)
	{
	case PieceType::PAWN: return "Pawn";
	case PieceType::KNIGHT: return "Knight";
	case PieceType::BISHOP: return "Bishop";
	case PieceType::ROOK: return "Rook";
	case PieceType::QUEEN: return "Queen";
	case PieceType::KING: return "King";
	case PieceType::EMPTY: return "Empty piece type";
	default: return "Unknown PieceType value: " + std::to_string(int(type));
	}
}