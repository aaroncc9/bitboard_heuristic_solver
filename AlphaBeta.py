import sys

class Jumpy3:
    def __init__(self):
        # position counter
        self.positions_evaluated = 0
    
    def read_board(self, filename):
        with open(filename, 'r') as f:
            return f.read().strip()
    
    def write_board(self, filename, board):
        with open(filename, 'w') as f:
            f.write(board)
    
    def is_white_win(self, board):
        return 'W' not in board
    
    def is_black_win(self, board):
        return 'B' not in board
        
    def estimate_position(self, board):
        # caculate MINIMAX estimate
        self.positions_evaluated += 1
        
        if self.is_white_win(board):
            return 100
        elif self.is_black_win(board):
            return -100
        else:
            white_king_index = board.find('W')
            black_king_index = board.find('B')
            return white_king_index + black_king_index - 15

    def generate_white_moves(self, board):
        moves = []
        board_list = list(board)
        
        for i in range(len(board)):
            if board[i] in ['w', 'W']:
                # Create a copy of the board to simulate move
                new_board = board_list.copy()
                
                # Move out of board
                if i == 15:
                    new_board[i] = 'x'
                    moves.append(''.join(new_board))
                    continue
                
                # Move one forward
                if i+1 < len(board) and board[i+1] == 'x':
                    new_board[i+1] = board[i]
                    new_board[i] = 'x'
                    moves.append(''.join(new_board))
                    continue
                
                # Jump
                # Find the next empty square to the right
                j = i + 1
                while j < len(board) and board[j] != 'x':
                    j += 1
                
                # Jump out of board
                if j >= len(board):
                    new_board[i] = 'x'
                    moves.append(''.join(new_board))
                    continue
                
                # Regular jump
                new_board[j] = board[i]
                new_board[i] = 'x'
                
                # Check if jump is over one black piece
                if j - i == 2 and board[i+1] in ['b', 'B']:
                    # Find the rightmost empty square
                    k = len(board) - 1
                    while k >= 0 and board[k] != 'x':
                        k -= 1
                    
                    if k >= 0:  # If there is an empty square
                        new_board[k] = board[i+1]
                        new_board[i+1] = 'x'
                
                moves.append(''.join(new_board))
        
        return moves

    def flip_board(self, board):
        # flip board to swap turns
        flipped = []
        for char in reversed(board):
            if char == 'W':
                flipped.append('B')
            elif char == 'B':
                flipped.append('W')
            elif char == 'w':
                flipped.append('b')
            elif char == 'b':
                flipped.append('w')
            else:
                flipped.append('x')
        return ''.join(flipped)

    def generate_black_moves(self, board):
        # generate black moves using generate_white_moves function
        flipped_board = self.flip_board(board)
        white_moves = self.generate_white_moves(flipped_board)
        return [self.flip_board(move) for move in white_moves]

    def alpha_beta(self, board, depth, alpha, beta, is_maximizing):
        # implement alpha beta algorithm. is_maximizing keeps track of turn. white turn if true, black turn if false.

        # ending cases
        if depth == 0 or self.is_white_win(board) or self.is_black_win(board):
            return self.estimate_position(board)
        
        if is_maximizing:
            # White's turn (maximizing player)
            best_value = float('-inf')
            for move in self.generate_white_moves(board):
                value = self.alpha_beta(move, depth - 1, alpha, beta, False)
                best_value = max(best_value, value)
                alpha = max(alpha, best_value)
                if beta <= alpha:
                    # Beta cutoff
                    break
            return best_value
        else:
            # Black's turn (minimizing player)
            best_value = float('inf')
            for move in self.generate_black_moves(board):
                value = self.alpha_beta(move, depth - 1, alpha, beta, True)
                best_value = min(best_value, value)
                beta = min(beta, best_value)
                if beta <= alpha:
                    # Alpha cutoff
                    break
            return best_value

    def find_best_move(self, board, depth):
        # caculate best move using alpha beta
        best_value = float('-inf')
        best_move = None
        alpha = float('-inf')
        beta = float('inf')
        
        # Generate all possible moves for White
        possible_moves = self.generate_white_moves(board)
        
        # Evaluate each move using Alpha-Beta pruning
        for move in possible_moves:
            # For each move, evaluate the position after Black's best response
            value = self.alpha_beta(move, depth - 1, alpha, beta, False)
            
            # Select the move with the highest value
            if value > best_value:
                best_value = value
                best_move = move
            
            # Update alpha
            alpha = max(alpha, best_value)
        
        return best_move, best_value

if __name__ == "__main__":
    # Check command line arguments
    if len(sys.argv) != 4:
        print("Usage: python AlphaBeta.py <input_file> <output_file> <depth>")
        sys.exit(1)
    
    input_file = sys.argv[1]
    output_file = sys.argv[2]
    depth = int(sys.argv[3])
    
    # Initialize game and read board
    game = Jumpy3()
    initial_board = game.read_board(input_file)
    
    # Find best move for White
    best_move, best_value = game.find_best_move(initial_board, depth)
    
    # Write output to file
    game.write_board(output_file, best_move)
    
    # Print results
    print(f"Output board position: {best_move}")
    print(f"Positions evaluated by static estimation: {game.positions_evaluated}.")
    print(f"MINIMAX estimate: {best_value}.")