#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <limits>
#include <chrono>
#include <bitset>

using namespace std;

// Jumpy3 with separate bitboards for each piece type
class Jumpy3 {
private:
    // Counters
    int positions_evaluated = 0;
    bool time_limit_reached = false;
    chrono::time_point<chrono::steady_clock> start_time;
    int time_limit_seconds = 15;
    int max_depth_reached = 0;

public:
    // Board representation using separate bitboards
    struct Board {
        uint16_t white_pawns;  // 16 bits, one for each square
        uint16_t white_king;   // 16 bits, one for each square
        uint16_t black_pawns;  // 16 bits, one for each square
        uint16_t black_king;   // 16 bits, one for each square
    };

    // Constructor with time limit
    Jumpy3(int time_limit = 15) : time_limit_seconds(time_limit) {}

    // Platform-independent find lowest set bit (replacement for __builtin_ctz)
    int findLowestSetBit(uint16_t bitboard) {
        if (bitboard == 0) return -1;
        
        // Find the position of the lowest set bit
        int pos = 0;
        while ((bitboard & (1 << pos)) == 0) {
            pos++;
            if (pos >= 16) return -1; // Safety check
        }
        return pos;
    }

    // Convert string board representation to bitboards
    Board stringToBoard(const string& board_str) {
        Board board = {0, 0, 0, 0};
        
        for (size_t i = 0; i < board_str.size(); i++) {
            uint16_t bit = static_cast<uint16_t>(1 << i); // Explicit cast
            
            switch (board_str[i]) {
                case 'w': board.white_pawns |= bit; break;
                case 'W': board.white_king |= bit; break;
                case 'b': board.black_pawns |= bit; break;
                case 'B': board.black_king |= bit; break;
                // 'x' doesn't set any bits
            }
        }
        
        return board;
    }

    // Convert bitboards to string representation
    string boardToString(const Board& board) {
        string result(16, 'x'); // Initialize with all empty squares
        
        for (int i = 0; i < 16; i++) {
            uint16_t bit = static_cast<uint16_t>(1 << i); // Explicit cast
            
            if ((board.white_pawns & bit) != 0) result[i] = 'w';
            else if ((board.white_king & bit) != 0) result[i] = 'W';
            else if ((board.black_pawns & bit) != 0) result[i] = 'b';
            else if ((board.black_king & bit) != 0) result[i] = 'B';
            // else it remains 'x'
        }
        
        return result;
    }

    // Read board from file
    Board readBoard(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Could not open file " << filename << endl;
            exit(1);
        }
        string board_str;
        file >> board_str;
        file.close();
        return stringToBoard(board_str);
    }

    // Write board to file
    void writeBoard(const string& filename, const Board& board) {
        ofstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Could not open file " << filename << endl;
            exit(1);
        }
        file << boardToString(board);
        file.close();
    }

    // Check if square is empty
    bool isEmpty(const Board& board, int pos) {
        uint16_t bit = static_cast<uint16_t>(1 << pos); // Explicit cast
        return ((board.white_pawns & bit) == 0 && (board.white_king & bit) == 0 && 
                (board.black_pawns & bit) == 0 && (board.black_king & bit) == 0);
    }

    // Check if White has won (W is not on the board)
    bool isWhiteWin(const Board& board) {
        return board.white_king == 0;
    }

    // Check if Black has won (B is not on the board)
    bool isBlackWin(const Board& board) {
        return board.black_king == 0;
    }

    // Check if time limit has been reached
    bool checkTimeLimit() {
        auto current_time = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::seconds>(current_time - start_time).count();
        if (elapsed >= time_limit_seconds) {
            time_limit_reached = true;
            return true;
        }
        return false;
    }

    // Find the position of king
    int findKing(uint16_t king_bitboard) {
        return findLowestSetBit(king_bitboard);
    }

    // Static evaluation function as defined in the handout
    int estimatePosition(const Board& board) {
        positions_evaluated++;

        if (isWhiteWin(board)) {
            return 100;
        } else if (isBlackWin(board)) {
            return -100;
        } else {
            int white_king_index = findKing(board.white_king);
            int black_king_index = findKing(board.black_king);
            return white_king_index + black_king_index - 15;
        }
    }

    // Generate all possible moves for White
    vector<Board> generateWhiteMoves(const Board& board) {
        vector<Board> moves;
        
        // Process white king
        if (board.white_king) {
            int kingPos = findKing(board.white_king);
            processWhitePieceMoves(board, kingPos, true, moves);
        }
        
        // Process white pawns
        uint16_t pawns = board.white_pawns;
        while (pawns) {
            int pawnPos = findLowestSetBit(pawns);
            processWhitePieceMoves(board, pawnPos, false, moves);
            pawns &= static_cast<uint16_t>(pawns - 1); // Clear the lowest set bit
        }
        
        return moves;
    }

    // Process moves for a white piece (king or pawn)
    void processWhitePieceMoves(const Board& board, int pos, bool isKing, vector<Board>& moves) {
        // Create a copy of the board for this move
        Board new_board = board;
        
        // Remove piece from original position
        uint16_t pos_bit = static_cast<uint16_t>(1 << pos);
        if (isKing) {
            new_board.white_king &= static_cast<uint16_t>(~pos_bit);
        } else {
            new_board.white_pawns &= static_cast<uint16_t>(~pos_bit);
        }
        
        // Move out of board
        if (pos == 15) {
            moves.push_back(new_board);
            return;
        }
        
        // Move one forward if the next square is empty
        if (isEmpty(board, pos + 1)) {
            Board forward_board = new_board;
            uint16_t new_pos_bit = static_cast<uint16_t>(1 << (pos + 1));
            if (isKing) {
                forward_board.white_king |= new_pos_bit;
            } else {
                forward_board.white_pawns |= new_pos_bit;
            }
            moves.push_back(forward_board);
            return;
        }
        
        // Jump - find the next empty square to the right
        int emptyPos = pos + 1;
        while (emptyPos < 16 && !isEmpty(board, emptyPos)) {
            emptyPos++;
        }
        
        // Jump out of board
        if (emptyPos >= 16) {
            moves.push_back(new_board);
            return;
        }
        
        // Regular jump
        Board jump_board = new_board;
        uint16_t empty_pos_bit = static_cast<uint16_t>(1 << emptyPos);
        if (isKing) {
            jump_board.white_king |= empty_pos_bit;
        } else {
            jump_board.white_pawns |= empty_pos_bit;
        }
        
        // Check if jump is over one black piece
        if (emptyPos - pos == 2) {
            uint16_t middle_pos_bit = static_cast<uint16_t>(1 << (pos + 1));
            bool is_black_pawn = (board.black_pawns & middle_pos_bit) != 0;
            bool is_black_king = (board.black_king & middle_pos_bit) != 0;
            
            if (is_black_pawn || is_black_king) {
                // Find the rightmost empty square
                int rightmost_empty = 15;
                while (rightmost_empty >= 0 && !isEmpty(jump_board, rightmost_empty)) {
                    rightmost_empty--;
                }
                
                if (rightmost_empty >= 0) {
                    // Move the jumped piece to the rightmost empty square
                    uint16_t rightmost_bit = static_cast<uint16_t>(1 << rightmost_empty);
                    if (is_black_pawn) {
                        jump_board.black_pawns &= static_cast<uint16_t>(~middle_pos_bit);
                        jump_board.black_pawns |= rightmost_bit;
                    } else { // It's a black king
                        jump_board.black_king &= static_cast<uint16_t>(~middle_pos_bit);
                        jump_board.black_king |= rightmost_bit;
                    }
                }
            }
        }
        
        moves.push_back(jump_board);
    }

    // Flip the board, swapping White and Black pieces and reversing the order
    Board flipBoard(const Board& board) {
        Board flipped = {0, 0, 0, 0};
        
        for (int i = 0; i < 16; i++) {
            uint16_t bit = static_cast<uint16_t>(1 << i);
            uint16_t flipped_bit = static_cast<uint16_t>(1 << (15 - i));
            
            if ((board.white_pawns & bit) != 0) flipped.black_pawns |= flipped_bit;
            if ((board.white_king & bit) != 0) flipped.black_king |= flipped_bit;
            if ((board.black_pawns & bit) != 0) flipped.white_pawns |= flipped_bit;
            if ((board.black_king & bit) != 0) flipped.white_king |= flipped_bit;
        }
        
        return flipped;
    }

    // Generate all possible moves for Black by flipping the board
    vector<Board> generateBlackMoves(const Board& board) {
        Board flipped_board = flipBoard(board);
        vector<Board> white_moves = generateWhiteMoves(flipped_board);
        
        vector<Board> black_moves;
        for (const auto& move : white_moves) {
            black_moves.push_back(flipBoard(move));
        }
        
        return black_moves;
    }

    // MINIMAX algorithm implementation with time limit
    int minimax(const Board& board, int depth, bool is_maximizing, int current_depth) {
        // Check if time limit has been reached
        if (checkTimeLimit()) {
            return estimatePosition(board);
        }
        
        // Track maximum depth
        max_depth_reached = max(max_depth_reached, current_depth);
        
        // Base cases: terminal nodes or maximum depth reached
        if (depth == 0 || isWhiteWin(board) || isBlackWin(board)) {
            return estimatePosition(board);
        }
        
        if (is_maximizing) {
            // White's turn (maximizing player)
            int best_value = numeric_limits<int>::min();
            vector<Board> moves = generateWhiteMoves(board);
            
            for (const auto& move : moves) {
                if (time_limit_reached) break;
                int value = minimax(move, depth - 1, false, current_depth + 1);
                best_value = max(best_value, value);
            }
            
            return best_value;
        } else {
            // Black's turn (minimizing player)
            int best_value = numeric_limits<int>::max();
            vector<Board> moves = generateBlackMoves(board);
            
            for (const auto& move : moves) {
                if (time_limit_reached) break;
                int value = minimax(move, depth - 1, true, current_depth + 1);
                best_value = min(best_value, value);
            }
            
            return best_value;
        }
    }

    // Find the best move for White using iterative deepening within time limit
    pair<Board, int> findBestMoveWithTimeLimit(const Board& board) {
        // Reset state
        time_limit_reached = false;
        positions_evaluated = 0;
        max_depth_reached = 0;
        start_time = chrono::steady_clock::now();
        
        Board best_move = board;  // Default to the current board
        int best_value = numeric_limits<int>::min();
        
        // Iterative deepening - start from depth 1 and increase until time runs out
        for (int depth = 1; !time_limit_reached; depth++) {
            Board current_best_move = board;
            int current_best_value = numeric_limits<int>::min();
            
            // Generate all possible moves for White
            vector<Board> possible_moves = generateWhiteMoves(board);
            
            bool depth_completed = true;
            // Evaluate each move using MINIMAX
            for (const auto& move : possible_moves) {
                if (checkTimeLimit()) {
                    depth_completed = false;
                    break;
                }
                
                // For each move, evaluate the position after Black's best response
                int value = minimax(move, depth - 1, false, 1);
                
                // Select the move with the highest value
                if (value > current_best_value) {
                    current_best_value = value;
                    current_best_move = move;
                }
            }
            
            // Only update the best move if we completed the search at this depth
            if (depth_completed) {
                best_move = current_best_move;
                best_value = current_best_value;
                cout << "Completed search at depth " << depth << endl;
            } else {
                cout << "Time limit reached during depth " << depth << " search" << endl;
                break;
            }
        }
        
        return make_pair(best_move, best_value);
    }

    // Get number of positions evaluated
    int getPositionsEvaluated() const {
        return positions_evaluated;
    }
    
    // Get maximum depth reached
    int getMaxDepthReached() const {
        return max_depth_reached;
    }
};

int main(int argc, char* argv[]) {
    // Check command-line arguments
    if (argc < 3 || argc > 4) {
        cerr << "Usage: " << argv[0] << " <input_file> <output_file> [time_limit_seconds]" << endl;
        return 1;
    }
    
    string input_file = argv[1];
    string output_file = argv[2];
    int time_limit = (argc == 4) ? stoi(argv[3]) : 15;  // Default to 15 seconds
    
    // Initialize game and read board
    Jumpy3 game(time_limit);
    Jumpy3::Board initial_board = game.readBoard(input_file);
    
    // Find best move for White within time limit
    auto start = chrono::steady_clock::now();
    pair<Jumpy3::Board, int> result = game.findBestMoveWithTimeLimit(initial_board);
    Jumpy3::Board best_move = result.first;
    int best_value = result.second;
    auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    
    // Write output to file
    game.writeBoard(output_file, best_move);
    
    // Print results
    cout << "Output board position: " << game.boardToString(best_move) << endl;
    cout << "Positions evaluated by static estimation: " << game.getPositionsEvaluated() << "." << endl;
    cout << "MINIMAX estimate: " << best_value << "." << endl;
    cout << "Maximum depth reached: " << game.getMaxDepthReached() << endl;
    cout << "Total time taken: " << duration / 1000.0 << " seconds" << endl;
    
    return 0;
}