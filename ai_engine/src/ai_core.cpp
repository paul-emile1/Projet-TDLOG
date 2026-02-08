/**
 * @file ai_core.cpp
 * @brief Moteur d'IA Puissance 4 - Architecture Buffer & Multi-Variantes
 */

#include <iostream>
#include <vector>
#include <limits>
#include <cstring>
#include <algorithm>
#include <memory>
#include <cmath>

// =========================================================
// CONSTANTES GLOBALES
// =========================================================

const int ROWS = 6;
const int COLS = 7;
const int AI_PIECE = 1;      // Joueur Maximisant (Rouge)
const int PLAYER_PIECE = -1; // Joueur Minimisant (Jaune)
const int EMPTY = 0;

// Pondérations Heuristiques
const int SCORE_WIN = 100000;
const int SCORE_3_THREAT = 100;
const int SCORE_2_BUILD = 5;
const int SCORE_BLOCK_3 = -80; 
const int SCORE_STOCK_UNIT = 50; // Valeur d'une munition (V2)
const int CENTER_BONUS = 3;      // Points par pion au centre

// =========================================================
// STRUCTURES DE DONNÉES
// =========================================================

// Structure pour manipuler le coup en interne
struct Move {
    int col;        // 0-6 ou -1 (si action spéciale sans pose)
    int kill_row;   // -1 si pas de kill
    int kill_col;   // -1 si pas de kill
    
    Move(int c) : col(c), kill_row(-1), kill_col(-1) {}
    Move(int c, int kr, int kc) : col(c), kill_row(kr), kill_col(kc) {}
    
    // Constructeur spécial V2 (Destruction pure)
    static Move create_kill(int r, int c) {
        return Move(-1, r, c);
    }
    
    // Conversion vers le format Buffer de sortie
    std::vector<int> to_buffer() const {
        std::vector<int> buf;
        buf.push_back(col);
        if (kill_row != -1) {
            buf.push_back(kill_row);
            buf.push_back(kill_col);
        }
        return buf;
    }
};

// État du jeu unifié (Buffer Wrapper)
struct GameState {
    int cells[ROWS * COLS];
    int p1_stock; // IA
    int p2_stock; // Humain
    
    GameState() : p1_stock(0), p2_stock(0) {
        std::memset(cells, 0, sizeof(cells));
    }

    int get(int r, int c) const { return cells[r * COLS + c]; }
    void set(int r, int c, int val) { cells[r * COLS + c] = val; }
    
    // Helpers Gravité
    void apply_gravity_col(int c) {
        std::vector<int> col_pieces;
        for (int r = ROWS - 1; r >= 0; --r) {
            if (get(r, c) != 0) col_pieces.push_back(get(r, c));
        }
        // Réécriture
        for (int r = ROWS - 1; r >= 0; --r) {
            if (col_pieces.empty()) {
                set(r, c, 0);
            } else {
                set(r, c, col_pieces.front());
                col_pieces.erase(col_pieces.begin());
            }
        }
    }
};

// =========================================================
// LOGIQUE D'ÉVALUATION (HEURISTIQUE)
// =========================================================

int evaluate_window(const std::vector<int>& window, int piece) {
    int score = 0;
    int opp_piece = (piece == AI_PIECE) ? PLAYER_PIECE : AI_PIECE;

    int count_piece = 0;
    int count_empty = 0;
    int count_opp = 0;

    for (int cell : window) {
        if (cell == piece) count_piece++;
        else if (cell == EMPTY) count_empty++;
        else count_opp++;
    }

    if (count_piece == 4) return SCORE_WIN;
    if (count_piece == 3 && count_empty == 1) return SCORE_3_THREAT;
    if (count_piece == 2 && count_empty == 2) return SCORE_2_BUILD;
    
    // Blocage agressif
    if (count_opp == 3 && count_empty == 1) return SCORE_BLOCK_3;

    return 0;
}

int evaluate_board_position(const GameState& state, int player) {
    int score = 0;
    
    // On évalue le contrôle du centre
    for (int r = 0; r < ROWS; r++) {
        if (state.get(r, 3) == player) score += CENTER_BONUS;
    }

    // On scanne les fenêtres glissantes
    // Horizontal
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS - 3; c++) {
            std::vector<int> window = {state.get(r,c), state.get(r,c+1), state.get(r,c+2), state.get(r,c+3)};
            score += evaluate_window(window, player);
        }
    }
    // Vertical
    for (int c = 0; c < COLS; c++) {
        for (int r = 0; r < ROWS - 3; r++) {
            std::vector<int> window = {state.get(r,c), state.get(r+1,c), state.get(r+2,c), state.get(r+3,c)};
            score += evaluate_window(window, player);
        }
    }
    // Diagonale positive
    for (int r = 0; r < ROWS - 3; r++) {
        for (int c = 0; c < COLS - 3; c++) {
            std::vector<int> window = {state.get(r,c), state.get(r+1,c+1), state.get(r+2,c+2), state.get(r+3,c+3)};
            score += evaluate_window(window, player);
        }
    }
    // Diagonale négative
    for (int r = 0; r < ROWS - 3; r++) {
        for (int c = 3; c < COLS; c++) {
            std::vector<int> window = {state.get(r+3,c-3), state.get(r+2,c-2), state.get(r+1,c-1), state.get(r,c)};
            score += evaluate_window(window, player);
        }
    }

    return score;
}

// Helper pour alignment strict
bool check_win(const GameState& s, int player) {
    // Horizontal
    for (int r=0; r<ROWS; r++)
        for (int c=0; c<COLS-3; c++)
            if (s.get(r,c)==player && s.get(r,c+1)==player && s.get(r,c+2)==player && s.get(r,c+3)==player) return true;
    // Vertical
    for (int r=0; r<ROWS-3; r++)
        for (int c=0; c<COLS; c++)
            if (s.get(r,c)==player && s.get(r+1,c)==player && s.get(r+2,c)==player && s.get(r+3,c)==player) return true;
    // Diags
    for (int r=0; r<ROWS-3; r++)
        for (int c=0; c<COLS-3; c++)
            if (s.get(r,c)==player && s.get(r+1,c+1)==player && s.get(r+2,c+2)==player && s.get(r+3,c+3)==player) return true;
    for (int r=3; r<ROWS; r++)
        for (int c=0; c<COLS-3; c++)
            if (s.get(r,c)==player && s.get(r-1,c+1)==player && s.get(r-2,c+2)==player && s.get(r-3,c+3)==player) return true;
    return false;
}

bool check_alignment_3(const GameState& s, int player) {
    // Même logique pour 3 pions
    for (int r=0; r<ROWS; r++)
        for (int c=0; c<COLS-2; c++)
            if (s.get(r,c)==player && s.get(r,c+1)==player && s.get(r,c+2)==player) return true;
    for (int r=0; r<ROWS-2; r++)
        for (int c=0; c<COLS; c++)
            if (s.get(r+1,c)==player && s.get(r+1,c)==player && s.get(r+2,c)==player) return true; 
    return false; 
}

// Helper colonne valide
int get_top_row(const GameState& s, int c) {
    for (int r = ROWS - 1; r >= 0; --r) {
        if (s.get(r, c) == 0) return r;
    }
    return -1;
}

// =========================================================
// RÈGLES DU JEU (INTERFACE)
// =========================================================

class GameRules {
public:
    virtual ~GameRules() = default;
    virtual std::vector<Move> get_possible_moves(const GameState& state, int player) = 0;
    virtual GameState apply_move(const GameState& state, const Move& move, int player) = 0;
    virtual int evaluate(const GameState& state, int player) = 0;
    virtual bool is_game_over(const GameState& state) = 0;
};

// ---------------------------------------------------------
// VARIANTE 0 : CLASSIQUE
// ---------------------------------------------------------
class ClassicRules : public GameRules {
public:
    std::vector<Move> get_possible_moves(const GameState& state, int player) override {
        std::vector<Move> moves;
        // On privilégie le centre : Centre -> Extérieur
        int order[] = {3, 2, 4, 1, 5, 0, 6};
        for (int c : order) {
            if (state.get(0, c) == 0) moves.emplace_back(c);
        }
        return moves;
    }

    GameState apply_move(const GameState& state, const Move& move, int player) override {
        GameState next = state;
        int r = get_top_row(next, move.col);
        if (r != -1) next.set(r, move.col, player);
        return next;
    }

    int evaluate(const GameState& state, int player) override {
        if (check_win(state, AI_PIECE)) return SCORE_WIN * 10;
        if (check_win(state, PLAYER_PIECE)) return -SCORE_WIN * 10;
        return evaluate_board_position(state, AI_PIECE);
    }

    bool is_game_over(const GameState& state) override {
        return check_win(state, AI_PIECE) || check_win(state, PLAYER_PIECE);
    }
};

// ---------------------------------------------------------
// VARIANTE 1 : 3 pour 1 (Ancienne)
// ---------------------------------------------------------
class Variant1Rules : public GameRules {
    // ... (Logique V1 identique à avant, adaptée à GameState) ...
public:
    std::vector<Move> get_possible_moves(const GameState& state, int player) override {
         // Réutilisation de la logique Classic pour simplifier dans ce buffer
         ClassicRules cr;
         return cr.get_possible_moves(state, player);
    }
    GameState apply_move(const GameState& state, const Move& move, int player) override {
        ClassicRules cr;
        return cr.apply_move(state, move, player);
    }
    int evaluate(const GameState& state, int player) override {
        ClassicRules cr;
        return cr.evaluate(state, player);
    }
    bool is_game_over(const GameState& state) override {
        ClassicRules cr;
        return cr.is_game_over(state);
    }
};

// ---------------------------------------------------------
// VARIANTE 2 : STOCK (3 pour 1 v2)
// ---------------------------------------------------------
class Variant2Rules : public GameRules {
private:
    // Vérification locale rapide d'alignement de 3 autour de (r,c)
    bool causes_alignment_3(const GameState& s, int r, int c, int player) {
        // Horizontal
        int count = 0;
        for (int i=std::max(0, c-2); i<=std::min(COLS-1, c+2); i++) {
            if (s.get(r, i) == player) count++;
            else count = 0;
            if (count >= 3) return true;
        }
        // Vertical
        count = 0;
        for (int i=std::max(0, r-2); i<=std::min(ROWS-1, r+2); i++) {
            if (s.get(i, c) == player) count++;
            else count = 0;
            if (count >= 3) return true;
        }
        return false;
    }

public:
    std::vector<Move> get_possible_moves(const GameState& state, int player) override {
        std::vector<Move> moves;
        
        // On vérifie les coups gagnants immédiats (Optimisation)
        for (int c = 0; c < COLS; c++) {
            if (state.get(0, c) == 0) {
                GameState temp = apply_move(state, Move(c), player);
                if (check_win(temp, player)) {
                    moves.clear();
                    moves.emplace_back(c);
                    return moves; 
                }
            }
        }

        // Coups de Pose
        int order[] = {3, 2, 4, 1, 5, 0, 6};
        for (int c : order) {
            if (state.get(0, c) == 0) moves.emplace_back(c);
        }

        // Coups de Destruction (Si Stock > 0)
        int stock = (player == AI_PIECE) ? state.p1_stock : state.p2_stock;
        
        if (stock > 0) {
            int opp = (player == AI_PIECE) ? PLAYER_PIECE : AI_PIECE;
            // On cherche les pions adverses utiles
            for (int r = 0; r < ROWS; r++) {
                for (int c = 0; c < COLS; c++) {
                    if (state.get(r, c) == opp) {
                        moves.push_back(Move::create_kill(r, c));
                    }
                }
            }
        }

        return moves;
    }

    GameState apply_move(const GameState& state, const Move& move, int player) override {
        GameState next = state;

        if (move.col != -1) {
            // Cas : Pose de pion
            int r = get_top_row(next, move.col);
            if (r != -1) {
                next.set(r, move.col, player);
                
                // Vérification victoire prioritaire
                if (check_win(next, player)) return next;

                // Vérification Bonus 3 (+1 Stock)
                if (evaluate_window({next.get(r, move.col), next.get(r, std::max(0, move.col-1)), next.get(r, std::max(0, move.col-2))}, player) >= SCORE_3_THREAT) { 
                }
                
                if (causes_alignment_3(next, r, move.col, player)) {
                    if (player == AI_PIECE) next.p1_stock++;
                    else next.p2_stock++;
                }
            }
        } else {
            // Cas : Destruction
            if (next.get(move.kill_row, move.kill_col) != 0) {
                next.set(move.kill_row, move.kill_col, 0);
                next.apply_gravity_col(move.kill_col);
                
                // Coût du stock
                if (player == AI_PIECE) next.p1_stock--;
                else next.p2_stock--;
            }
        }
        return next;
    }

    int evaluate(const GameState& state, int player) override {
        // Base : Victoire/Défaite
        if (check_win(state, AI_PIECE)) return SCORE_WIN * 10;
        if (check_win(state, PLAYER_PIECE)) return -SCORE_WIN * 10;

        // Heuristique Positionnelle
        int score = evaluate_board_position(state, AI_PIECE);

        // Heuristique Matérielle (Stock)
        score += (state.p1_stock * SCORE_STOCK_UNIT);
        score -= (state.p2_stock * SCORE_STOCK_UNIT);

        return score;
    }

    bool is_game_over(const GameState& state) override {
        return check_win(state, AI_PIECE) || check_win(state, PLAYER_PIECE);
    }
};

// =========================================================
// MOTEUR MINIMAX
// =========================================================

int minimax(GameState state, int depth, int alpha, int beta, bool maximizing, GameRules* rules) {
    if (depth == 0 || rules->is_game_over(state)) {
        return rules->evaluate(state, AI_PIECE);
    }

    int current_player = maximizing ? AI_PIECE : PLAYER_PIECE;
    std::vector<Move> moves = rules->get_possible_moves(state, current_player);

    if (moves.empty()) return 0;

    if (maximizing) {
        int max_eval = -std::numeric_limits<int>::max();
        for (const auto& move : moves) {
            GameState next_state = rules->apply_move(state, move, AI_PIECE);
            int eval = minimax(next_state, depth - 1, alpha, beta, false, rules);
            max_eval = std::max(max_eval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) break;
        }
        return max_eval;
    } else {
        int min_eval = std::numeric_limits<int>::max();
        for (const auto& move : moves) {
            GameState next_state = rules->apply_move(state, move, PLAYER_PIECE);
            int eval = minimax(next_state, depth - 1, alpha, beta, true, rules);
            min_eval = std::min(min_eval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) break;
        }
        return min_eval;
    }
}

// =========================================================
// INTERFACE EXTERNE (BUFFER C)
// =========================================================

static std::vector<int> output_buffer;

extern "C" {
    #ifdef _WIN32
    __declspec(dllexport)
    #endif
    // Signature : input_buffer (Board + Context), depth, mode, out_size (ptr vers int)
    int* get_best_move_buffer(int* input_buffer, int depth, int mode, int* out_size) {
        
        // On décode l'entrée (Architecture Buffer)
        GameState root_state;
        std::memcpy(root_state.cells, input_buffer, sizeof(int) * ROWS * COLS);
        
        // Lecture du Contexte
        if (mode == 2) {
            root_state.p1_stock = input_buffer[42];
            root_state.p2_stock = input_buffer[43];
        }

        // On sélectionne les règles
        std::unique_ptr<GameRules> rules;
        if (mode == 2) rules.reset(new Variant2Rules());
        else if (mode == 1) rules.reset(new Variant1Rules());
        else rules.reset(new ClassicRules());

        // Minimax
        std::vector<Move> moves = rules->get_possible_moves(root_state, AI_PIECE);
        Move best_move(0);
        int best_val = -std::numeric_limits<int>::max();

        if (moves.size() == 1) {
            best_move = moves[0];
        } else {
            for (const auto& move : moves) {
                GameState next = rules->apply_move(root_state, move, AI_PIECE);
                
                // Glouton Victoire
                if (rules->is_game_over(next) && rules->evaluate(next, AI_PIECE) > 90000) {
                    best_move = move;
                    break;
                }

                int val = minimax(next, depth - 1, -std::numeric_limits<int>::max(), std::numeric_limits<int>::max(), false, rules.get());
                
                if (val > best_val) {
                    best_val = val;
                    best_move = move;
                }
            }
        }

        // On encode la sortie
        output_buffer = best_move.to_buffer();
        
        if (out_size) *out_size = static_cast<int>(output_buffer.size());
        return output_buffer.data();
    }
}
