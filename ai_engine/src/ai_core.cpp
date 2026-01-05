#include "ai_core.h"
#include <iostream>
#include <algorithm> // Pour max et min
#include <limits>    // Pour numeric_limits
using namespace std;


// Constantes 
const int ROWS = 6;
const int COLS = 7;
const int WIN_SCORE = 1000000000; 
const int AI_PIECE = 1; // Convention
const int PLAYER_PIECE = -1;


int idx(int r, int c) {
    return r * COLS + c;
}

bool is_valid_location(int* board, int col) {
    // Renvoie Vrai si la case tout en haut de la colonne (ligne 0) est vide (0).
    return board[idx(0,col)] == 0; 
}

int get_next_open_row(int* board, int col) {
    // Parcourt la colonne de bas en haut (de ROWS-1 jusqu'à 0).
    // Renvoie le premier numéro de ligne où la case est vide (0).
    int i = ROWS - 1;
    while(i>=0 && board[idx(i,col)] != 0){
        i -= 1;
    }
    return i;
}


bool check_win(int* board, int player) {
    
    // 1. HORIZONTALE (Ta méthode avec compteur)
    for(int r = 0; r < ROWS; r++){
        int count = 0; // Remise à zéro au début de chaque ligne
        for(int c = 0; c < COLS; c++){
            if(board[idx(r, c)] == player){
                count++;
            } else {
                count = 0;
            }
            if(count >= 4) return true;
        }
    }

    // 2. VERTICALE 
    for(int c = 0; c < COLS; c++){
        int count = 0; // Remise à zéro au début de chaque colonne
        for(int r = 0; r < ROWS; r++){
            if(board[idx(r, c)] == player){
                count++;
            } else {
                count = 0;
            }
            if(count >= 4) return true;
        }
    }

    // 3. DIAGONALE POSITIVE 
    for (int c = 0; c < COLS - 3; c++) {
        for (int r = 0; r < ROWS - 3; r++) {
            if (board[idx(r, c)] == player && 
                board[idx(r+1, c+1)] == player && 
                board[idx(r+2, c+2)] == player && 
                board[idx(r+3, c+3)] == player)
                return true;
        }
    }

    // 4. DIAGONALE NÉGATIVE
    for (int c = 0; c < COLS - 3; c++) {
        for (int r = 3; r < ROWS; r++) { // On part de la ligne 3 car on remonte
            if (board[idx(r, c)] == player && 
                board[idx(r-1, c+1)] == player && 
                board[idx(r-2, c+2)] == player && 
                board[idx(r-3, c+3)] == player)
                return true;
        }
    }

    return false;
}

int evaluate_window(int window[], int piece) {
    int score = 0;
    // On définit qui est l'adversaire
    // Si je suis 1, l'autre est -1. Si je suis -1, l'autre est 1.
    int opp_piece = (piece == 1) ? -1 : 1; 

    int count_piece = 0;
    int count_empty = 0;
    int count_opp = 0;

    // 1. On compte
    for (int i = 0; i < 4; i++) {
        if (window[i] == piece) {
            count_piece++;
        } else if (window[i] == 0) { // IMPORTANT : 0 c'est vide
            count_empty++;
        } else if (window[i] == opp_piece) {
            count_opp++;
        }
    }

    if (count_piece == 4) {
        score += 100;
    } else if (count_piece == 3 && count_empty == 1) {
        score += 5;
    } else if (count_piece == 2 && count_empty == 2) {
        score += 2;
    }

    if (count_opp == 3 && count_empty == 1) {
        score -= 4; // On pénalise cette situation
    }

    return score;
}

int score_position(int* board, int piece) {
    int score = 0;

    // --- BONUS : PRÉFÉRENCE POUR LE CENTRE ---
    // Au Puissance 4, contrôler le centre est stratégique.
    // On donne des points bonus pour chaque pion dans la colonne centrale (colonne 3).
    int center_col = COLS / 2; // 3
    int center_count = 0;
    for (int r = 0; r < ROWS; r++) {
        if (board[idx(r, center_col)] == piece) {
            center_count++;
        }
    }
    score += center_count * 3; // +3 points par pion au centre

    // 1. HORIZONTALE
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < COLS - 3; c++) {
            // On crée la fenêtre
            int window[4] = {
                board[idx(r, c)], 
                board[idx(r, c+1)], 
                board[idx(r, c+2)], 
                board[idx(r, c+3)]
            };
            score += evaluate_window(window, piece);
        }
    }

    // 2. VERTICALE
    for (int c = 0; c < COLS; c++) {
        for (int r = 0; r < ROWS - 3; r++) {
            int window[4] = {
                board[idx(r, c)], 
                board[idx(r+1, c)], 
                board[idx(r+2, c)], 
                board[idx(r+3, c)]
            };
            score += evaluate_window(window, piece);
        }
    }

    // 3. DIAGONALE POSITIVE (/)
    for (int r = 0; r < ROWS - 3; r++) {
        for (int c = 0; c < COLS - 3; c++) {
            int window[4] = {
                board[idx(r, c)], 
                board[idx(r+1, c+1)], 
                board[idx(r+2, c+2)], 
                board[idx(r+3, c+3)]
            };
            score += evaluate_window(window, piece);
        }
    }

    // 4. DIAGONALE NÉGATIVE (\)
    for (int r = 3; r < ROWS; r++) {
        for (int c = 0; c < COLS - 3; c++) {
            int window[4] = {
                board[idx(r, c)], 
                board[idx(r-1, c+1)], 
                board[idx(r-2, c+2)], 
                board[idx(r-3, c+3)]
            };
            score += evaluate_window(window, piece);
        }
    }

    return score;
}



int minimax(int* board, int depth, int alpha, int beta, bool maximizingPlayer) {
    
    // ---  CONDITIONS D'ARRÊT  ---

    //  Est-ce que l'IA a gagné au coup précédent ?
    if (check_win(board, AI_PIECE)) {
        return WIN_SCORE;
    }
    //  Est-ce que l'Humain a gagné au coup précédent ?
    if (check_win(board, PLAYER_PIECE)) {
        return -WIN_SCORE;
    }
    //  Est-ce qu'on est au bout de la réflexion (Profondeur 0) ?
    if (depth == 0) {
        // On retourne la note heuristique du plateau
        return score_position(board, AI_PIECE);
    }
    
    // (Optionnel : Ajouter une vérification de match nul si le plateau est plein)

    // ---  JOUEUR MAXIMISANT (C'est le tour de l'IA) ---
    if (maximizingPlayer) {
        int maxEval = -numeric_limits<int>::max(); // -Infini
        
        // On teste toutes les colonnes
        for (int c = 0; c < COLS; c++) {
            if (is_valid_location(board, c)) {
                // On récupère la ligne où le pion tombe
                int row = get_next_open_row(board, c);
                
                // ON JOUE (Simulation)
                board[idx(row, c)] = AI_PIECE;
                
                // APPEL RÉCURSIF (On passe la main au joueur Min)
                // depth - 1 on descend dans l'arbre et maximizingPlayer devient false c'est à l'autre de jouer
                int eval = minimax(board, depth - 1, alpha, beta, false);
                
                //  ON ANNULE (Backtracking - Très important !)
                board[idx(row, c)] = 0;
                
                //  Mises à jour
                maxEval = max(maxEval, eval); // Garde le meilleur score
                alpha = max(alpha, eval);     // Met à jour le seuil Alpha
                
                //  Coupure Alpha-Beta
                if (beta <= alpha) {
                    break; // L'adversaire ne nous laissera pas jouer ce coup
                }
            }
        }
        return maxEval;
    }

    // --- JOUEUR MINIMISANT (C'est le tour de l'Humain) ---
    else {
        int minEval = numeric_limits<int>::max(); // +Infini
        
        for (int c = 0; c < COLS; c++) {
            if (is_valid_location(board, c)) {
                int row = get_next_open_row(board, c);
                
                // a. L'adversaire JOUE
                board[idx(row, c)] = PLAYER_PIECE;
                
                // b. Récursion (On passe la main à l'IA)
                // Note : maximizingPlayer devient true
                int eval = minimax(board, depth - 1, alpha, beta, true);
                
                // c. Annulation
                board[idx(row, c)] = 0;
                
                // d. Mises à jour (On cherche le MINIMUM)
                minEval = min(minEval, eval);
                beta = min(beta, eval);      // Met à jour le seuil Beta
                
                // e. Coupure Alpha-Beta
                if (beta <= alpha) {
                    break; // L'IA ne laissera pas l'adversaire jouer ce coup
                }
            }
        }
        return minEval;
    }
}


// --- INTERFACE PYTHON (Le Driver) ---

extern "C" {
    /**
     * Cette fonction est celle que Python appelle.
     * Elle teste chaque colonne, lance le minimax, et retourne l'INDEX de la meilleure colonne.
     */
    int get_best_move(int* board, int depth) {
        
        int best_col = -1;
        int best_score = -numeric_limits<int>::max(); // Commence à -Infini

        // On parcourt toutes les colonnes possibles
        for (int c = 0; c < COLS; c++) {
            
            if (is_valid_location(board, c)) {
                
                // 1. On simule le coup
                int row = get_next_open_row(board, c);
                board[idx(row, c)] = AI_PIECE;

                // 2. On demande au Minimax : "Combien vaut ce coup ?"
                // depth-1 car on vient de jouer un coup
                // alpha/beta à l'infini pour commencer
                // false car c'est maintenant au tour de l'adversaire
                int score = minimax(board, depth - 1, -numeric_limits<int>::max(), numeric_limits<int>::max(), false);

                // 3. On annule (Backtracking)
                board[idx(row, c)] = 0;

                // 4. Est-ce que c'est le meilleur coup trouvé jusqu'ici ?
                if (score > best_score) {
                    best_score = score;
                    best_col = c;
                }
            }
        }
        
        return best_col; 
    }
}
