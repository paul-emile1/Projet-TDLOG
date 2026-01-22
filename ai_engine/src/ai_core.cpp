#include <iostream>
#include <algorithm>
#include <limits>
#include <cstring> 
#include <vector>
#include <fstream> 

using namespace std;

//  CONSTANTES 
const int ROWS = 6;
const int COLS = 7;
const int WIN_SCORE = 100000000; 

const int AI_PIECE = 1; 
const int PLAYER_PIECE = -1;

//  UTILITAIRES 
int idx(int r, int c) { return r * COLS + c; }
bool is_valid_location(int* board, int col) { return board[idx(0,col)] == 0; }

int get_next_open_row(int* board, int col) {
    int i = ROWS - 1;
    while(i>=0 && board[idx(i,col)] != 0) i -= 1;
    return i;
}

//  LOGIQUE VICTOIRE 
bool check_win(int* board, int player) {
    // Horizontale
    for(int r=0; r<ROWS; r++)
        for(int c=0; c<COLS-3; c++)
            if(board[idx(r,c)]==player && board[idx(r,c+1)]==player && board[idx(r,c+2)]==player && board[idx(r,c+3)]==player) return true;
    // Verticale
    for(int c=0; c<COLS; c++)
        for(int r=0; r<ROWS-3; r++)
            if(board[idx(r,c)]==player && board[idx(r+1,c)]==player && board[idx(r+2,c)]==player && board[idx(r+3,c)]==player) return true;
    // Diagonales
    for(int c=0; c<COLS-3; c++) {
        for(int r=0; r<ROWS-3; r++)
            if(board[idx(r,c)]==player && board[idx(r+1,c+1)]==player && board[idx(r+2,c+2)]==player && board[idx(r+3,c+3)]==player) return true;
        for(int r=3; r<ROWS; r++)
            if(board[idx(r,c)]==player && board[idx(r-1,c+1)]==player && board[idx(r-2,c+2)]==player && board[idx(r-3,c+3)]==player) return true;
    }
    return false;
}

bool check_alignment_3_exact(int* board, int player) {
    // Horizontale
    for(int r=0; r<ROWS; r++)
        for(int c=0; c<COLS-2; c++)
            if(board[idx(r,c)]==player && board[idx(r,c+1)]==player && board[idx(r,c+2)]==player) return true;
    // Verticale
    for(int c=0; c<COLS; c++)
        for(int r=0; r<ROWS-2; r++)
            if(board[idx(r,c)]==player && board[idx(r+1,c)]==player && board[idx(r+2,c)]==player) return true;
    // Diagonales
    for(int c=0; c<COLS-2; c++) {
        for(int r=0; r<ROWS-2; r++)
            if(board[idx(r,c)]==player && board[idx(r+1,c+1)]==player && board[idx(r+2,c+2)]==player) return true;
        for(int r=2; r<ROWS; r++)
            if(board[idx(r,c)]==player && board[idx(r-1,c+1)]==player && board[idx(r-2,c+2)]==player) return true;
    }
    return false;
}

void apply_kill_and_gravity(int* board, int victim_idx) {
    int c = victim_idx % COLS;
    int r_removed = victim_idx / COLS;
    for (int r = r_removed; r > 0; r--) {
        board[idx(r, c)] = board[idx(r - 1, c)];
    }
    board[idx(0, c)] = 0;
}

// HEURISTIQUE DE SURVIE 
bool can_player_win_now(int* board, int player, int mode) {
    int temp_board[42];
    for(int c=0; c<COLS; c++) {
        if(is_valid_location(board, c)) {
            std::memcpy(temp_board, board, 42 * sizeof(int));
            int row = get_next_open_row(temp_board, c);
            temp_board[idx(row, c)] = player;

            if(check_win(temp_board, player)) return true;

            if(mode == 1 && check_alignment_3_exact(temp_board, player)) {
                int target_victim = (player == AI_PIECE) ? PLAYER_PIECE : AI_PIECE;
                for(int i=0; i<42; i++) {
                    if(temp_board[i] == target_victim) {
                        int kill_board[42];
                        std::memcpy(kill_board, temp_board, 42 * sizeof(int));
                        apply_kill_and_gravity(kill_board, i);
                        if(check_win(kill_board, player)) return true;
                    }
                }
            }
        }
    }
    return false;
}

//  EVALUATION 
int evaluate_window(int window[], int piece, int mode) {
    int score = 0;
    int opp_piece = (piece == AI_PIECE) ? PLAYER_PIECE : AI_PIECE; 
    int count_piece = 0, count_empty = 0, count_opp = 0;
    for (int i = 0; i < 4; i++) {
        if (window[i] == piece) count_piece++;
        else if (window[i] == 0) count_empty++;
        else if (window[i] == opp_piece) count_opp++;
    }
    if (mode == 0) { 
        if (count_piece == 4) score += 100;
        else if (count_piece == 3 && count_empty == 1) score += 5;
        else if (count_piece == 2 && count_empty == 2) score += 2;
        if (count_opp == 3 && count_empty == 1) score -= 4; 
    } else { 
        if (count_piece == 4) score += 10000;
        else if (count_piece == 3 && count_empty == 1) score += 500; 
        if (count_opp == 3 && count_empty == 1) score -= 100000; 
    }
    return score;
}

int score_position(int* board, int piece, int mode) {
    int score = 0;
    for (int r=0; r<ROWS; r++) for (int c=0; c<COLS-3; c++) {
        int w[4] = {board[idx(r,c)], board[idx(r,c+1)], board[idx(r,c+2)], board[idx(r,c+3)]};
        score += evaluate_window(w, piece, mode);
    }
    for (int c=0; c<COLS; c++) for (int r=0; r<ROWS-3; r++) {
        int w[4] = {board[idx(r,c)], board[idx(r+1,c)], board[idx(r+2,c)], board[idx(r+3,c)]};
        score += evaluate_window(w, piece, mode);
    }
    for (int r=0; r<ROWS-3; r++) for (int c=0; c<COLS-3; c++) {
        int w[4] = {board[idx(r,c)], board[idx(r+1,c+1)], board[idx(r+2,c+2)], board[idx(r+3,c+3)]};
        score += evaluate_window(w, piece, mode);
    }
    for (int r=3; r<ROWS; r++) for (int c=0; c<COLS-3; c++) {
        int w[4] = {board[idx(r,c)], board[idx(r-1,c+1)], board[idx(r-2,c+2)], board[idx(r-3,c+3)]};
        score += evaluate_window(w, piece, mode);
    }
    return score;
}

//  MINIMAX 
int minimax(int* board, int depth, int alpha, int beta, bool maximizingPlayer, int mode) {
    if (check_win(board, AI_PIECE)) return WIN_SCORE;
    if (check_win(board, PLAYER_PIECE)) return -WIN_SCORE;
    if (depth == 0) return score_position(board, AI_PIECE, mode);

    int temp_board[42]; 

    if (maximizingPlayer) {
        int maxEval = -numeric_limits<int>::max();
        for (int c = 0; c < COLS; c++) {
            if (is_valid_location(board, c)) {
                std::memcpy(temp_board, board, 42 * sizeof(int));
                int row = get_next_open_row(temp_board, c);
                temp_board[idx(row, c)] = AI_PIECE;

                if (check_win(temp_board, AI_PIECE)) return WIN_SCORE; 

                else if (mode == 1 && check_alignment_3_exact(temp_board, AI_PIECE)) {
                    int best_kill_score = -numeric_limits<int>::max();
                    bool can_kill = false;
                    for(int i=0; i<42; i++) {
                        if(temp_board[i] == PLAYER_PIECE) {
                            can_kill = true;
                            int kill_board[42];
                            std::memcpy(kill_board, temp_board, 42 * sizeof(int));
                            apply_kill_and_gravity(kill_board, i);
                            if (check_win(kill_board, AI_PIECE)) return WIN_SCORE;
                            int eval = minimax(kill_board, depth - 1, alpha, beta, false, mode);
                            if (eval > best_kill_score) best_kill_score = eval;
                        }
                    }
                    if (can_kill) {
                        maxEval = max(maxEval, best_kill_score);
                        alpha = max(alpha, best_kill_score);
                        if (beta <= alpha) break;
                        continue;
                    }
                }
                int eval = minimax(temp_board, depth - 1, alpha, beta, false, mode);
                maxEval = max(maxEval, eval);
                alpha = max(alpha, eval);
                if (beta <= alpha) break;
            }
        }
        return maxEval;
    } else {
        int minEval = numeric_limits<int>::max();
        for (int c = 0; c < COLS; c++) {
            if (is_valid_location(board, c)) {
                std::memcpy(temp_board, board, 42 * sizeof(int));
                int row = get_next_open_row(temp_board, c);
                temp_board[idx(row, c)] = PLAYER_PIECE;

                if (check_win(temp_board, PLAYER_PIECE)) return -WIN_SCORE; 

                else if (mode == 1 && check_alignment_3_exact(temp_board, PLAYER_PIECE)) {
                    int worst_kill_score = numeric_limits<int>::max();
                    bool can_kill = false;
                    for(int i=0; i<42; i++) {
                        if(temp_board[i] == AI_PIECE) {
                            can_kill = true;
                            int kill_board[42];
                            std::memcpy(kill_board, temp_board, 42 * sizeof(int));
                            apply_kill_and_gravity(kill_board, i);
                            if (check_win(kill_board, PLAYER_PIECE)) return -WIN_SCORE;
                            int eval = minimax(kill_board, depth - 1, alpha, beta, true, mode);
                            if (eval < worst_kill_score) worst_kill_score = eval;
                        }
                    }
                    if (can_kill) {
                        minEval = min(minEval, worst_kill_score);
                        beta = min(beta, worst_kill_score);
                        if (beta <= alpha) break;
                        continue;
                    }
                }
                int eval = minimax(temp_board, depth - 1, alpha, beta, true, mode);
                minEval = min(minEval, eval);
                beta = min(beta, eval);
                if (beta <= alpha) break;
            }
        }
        return minEval;
    }
}

extern "C" {
    int get_best_move(int* board, int depth, int mode) {
        
        // Ordre stratégique : on privilégie le centre
        int priority_cols[] = {3, 2, 4, 1, 5, 0, 6}; 

        // 1. Défense forcée
        // Si l'adversaire peut gagner au prochain coup, on pare immédiatement
        for (int i = 0; i < COLS; i++) {
            int c = priority_cols[i];
            if (is_valid_location(board, c)) {
                int temp_board[42];
                std::memcpy(temp_board, board, 42 * sizeof(int));
                int row = get_next_open_row(temp_board, c);
                temp_board[idx(row, c)] = PLAYER_PIECE; 

                // Si l'adversaire gagne ici, on bloque.
                if (check_win(temp_board, PLAYER_PIECE)) return c; 
                
                if (mode == 1 && check_alignment_3_exact(temp_board, PLAYER_PIECE)) {
                    for(int k=0; k<42; k++) {
                        if(temp_board[k] == AI_PIECE) {
                            int k_board[42];
                            std::memcpy(k_board, temp_board, 42 * sizeof(int));
                            apply_kill_and_gravity(k_board, k);
                            if(check_win(k_board, PLAYER_PIECE)) return c;
                        }
                    }
                }
            }
        }

        // 2. Attaque immédiate
        for (int i = 0; i < COLS; i++) {
            int c = priority_cols[i];
            if (is_valid_location(board, c)) {
                int temp_board[42];
                std::memcpy(temp_board, board, 42 * sizeof(int));
                int row = get_next_open_row(temp_board, c);
                temp_board[idx(row, c)] = AI_PIECE;
                
                if(check_win(temp_board, AI_PIECE)) return c;
                if(mode == 1 && check_alignment_3_exact(temp_board, AI_PIECE)) {
                    for(int k=0; k<42; k++) {
                        if(temp_board[k] == PLAYER_PIECE) {
                            int k_board[42];
                            std::memcpy(k_board, temp_board, 42 * sizeof(int));
                            apply_kill_and_gravity(k_board, k);
                            if(check_win(k_board, AI_PIECE)) return c;
                        }
                    }
                }
            }
        }

        // 3. Algorithme Minimax
        int best_col = -1;
        int best_score = -2000000000;

        // On parcourt les colonnes dans l'ordre de priorité (3, 2, 4...)
        for (int i = 0; i < COLS; i++) {
            int c = priority_cols[i];
            
            if (is_valid_location(board, c)) {
                int temp_board[42];
                std::memcpy(temp_board, board, 42 * sizeof(int));
                int row = get_next_open_row(temp_board, c);
                temp_board[idx(row, c)] = AI_PIECE;

                // Vérification "Suicide" : on ne joue pas si cela permet à l'adversaire de gagner au tour suivant
                bool suicide = false;
                if (can_player_win_now(temp_board, PLAYER_PIECE, mode)) {
                    suicide = true;
                } else if (mode == 1 && check_alignment_3_exact(temp_board, AI_PIECE)) {
                    bool safe_kill_found = false;
                    for(int k=0; k<42; k++) {
                        if(temp_board[k] == PLAYER_PIECE) {
                            int kill_board[42];
                            std::memcpy(kill_board, temp_board, 42 * sizeof(int));
                            apply_kill_and_gravity(kill_board, k);
                            if (!can_player_win_now(kill_board, PLAYER_PIECE, mode)) {
                                safe_kill_found = true; 
                                break;
                            }
                        }
                    }
                    if (!safe_kill_found) suicide = true;
                }

                if (suicide) {
                    int s = -WIN_SCORE; 
                    if (s > best_score) { best_score = s; best_col = c; }
                    continue;
                }

                // Minimax
                int score;
                if (mode == 1 && check_alignment_3_exact(temp_board, AI_PIECE)) {
                    int max_kill_score = -2000000000;
                    for(int k=0; k<42; k++) {
                         if(temp_board[k] == PLAYER_PIECE) {
                            int kill_board[42];
                            std::memcpy(kill_board, temp_board, 42 * sizeof(int));
                            apply_kill_and_gravity(kill_board, k);
                            if (!can_player_win_now(kill_board, PLAYER_PIECE, mode)) {
                                int s = minimax(kill_board, depth - 1, -2000000000, 2000000000, false, mode);
                                if (s > max_kill_score) max_kill_score = s;
                            }
                        }
                    }
                    score = max_kill_score;
                } else {
                    score = minimax(temp_board, depth - 1, -2000000000, 2000000000, false, mode);
                }
                
                // Strictement supérieur pour privilégier l'ordre du tableau priority_cols
                if (score > best_score) {
                    best_score = score;
                    best_col = c;
                }
            }
        }
        
        // Sécurité : si aucun coup n'est trouvé, on prend le premier valide
        if (best_col == -1) {
            for (int i = 0; i < COLS; i++) {
                int c = priority_cols[i];
                if(is_valid_location(board, c)) return c;
            }
        }
        return best_col; 
    }
}
