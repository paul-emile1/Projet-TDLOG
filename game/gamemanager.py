import numpy as np
from abc import ABC, abstractmethod
from game.calculateur import AIModel

class InvalidMove(Exception):
    """Exception levée lorsqu'un coup n'est pas valide."""
    pass

class Gestionnaire(ABC):
    """
    Classe de base gérant l'état du plateau et les règles communes.
    Implémente le pattern Template Method pour la gestion des tours.
    """
    name: str = "Jeu"

    def __init__(self, mode_solo=False, difficulty=4):
        self._width = 7
        self._height = 6
        self._board = np.zeros((self._height, self._width), dtype=int)
        
        # 1 = Joueur IA/Rouge, -1 = Joueur Humain/Jaune
        self._current_player = -1  
        
        self._victory = False
        self._draw = False
        
        # Gestion des événements (utilisé par Variante 1)
        self._event = False
        self._message_event = ""

        self.mode_solo = mode_solo
        self.difficulty = difficulty
        self.ai_engine = None
        
        if self.mode_solo:
            try:
                self.ai_engine = AIModel()
                print(f"IA chargée avec succès. Difficulté : {difficulty}")
            except Exception as e:
                print(f"Erreur critique IA : {e}")

    @property
    def board(self): return self._board

    @property
    def current_player(self): return self._current_player

    @property
    def victory(self): return self._victory

    @property
    def draw(self): return self._draw

    @property
    def event(self): return self._event

    @property
    def message_event(self): return self._message_event

    @property
    def width(self): return self._width

    @property
    def height(self): return self._height

    def get_info_status(self, player: int) -> str:
        """
        Renvoie une info textuelle sur l'état du joueur pour l'interface.
        Par défaut: None. Surchargé par les variantes avec ressources.
        """
        return None

    def check_alignment(self, r: int, c: int, player: int, n: int) -> bool:
        """Vérifie si le pion en (r, c) fait partie d'un alignement de n pions."""
        directions = [(0, 1), (1, 0), (1, 1), (1, -1)]
        
        for dr, dc in directions:
            count = 0
            # On scanne dans le sens positif
            for i in range(n):
                nr, nc = r + dr * i, c + dc * i
                if 0 <= nr < self._height and 0 <= nc < self._width and self._board[nr, nc] == player:
                    count += 1
                else:
                    break
            # On scanne dans le sens négatif
            for i in range(1, n):
                nr, nc = r - dr * i, c - dc * i
                if 0 <= nr < self._height and 0 <= nc < self._width and self._board[nr, nc] == player:
                    count += 1
                else:
                    break
            
            if count >= n:
                return True
        return False

    def get_top_row(self, col: int) -> int:
        """Retourne la première ligne vide d'une colonne ou -1 si pleine."""
        if not (0 <= col < self._width):
            return -1
        for r in range(self._height - 1, -1, -1):
            if self._board[r, col] == 0:
                return r
        return -1

    def verify_victory_condition(self):
        """Vérifie si le plateau contient un gagnant (scan global post-gravité)."""
        has_p1_won = False
        has_p2_won = False

        for r in range(self._height):
            for c in range(self._width):
                p = self._board[r, c]
                if p != 0:
                    if self.check_alignment(r, c, p, 4):
                        if p == self._current_player: has_p1_won = True
                        else: has_p2_won = True
        
        if has_p1_won and has_p2_won:
            self._draw = True
        elif has_p1_won:
            self._victory = True
        elif has_p2_won:
            self._victory = True
            self._current_player *= -1 
        elif np.all(self._board != 0):
            self._draw = True

    def play_ai_turn(self):
        """Orchestre le tour de l'IA avec gestion des stocks pour la V2."""
        # On s'assure que l'IA ne joue que si c'est le Joueur 1
        if self._current_player != 1:
            return

        if not self.ai_engine or self._victory or self._draw:
            return

        # On prépare les données pour le C++ (Mode + Stocks)
        mode_int = 0
        stock_ai = 0
        stock_human = 0

        if isinstance(self, Variante_1): 
            mode_int = 1
        elif isinstance(self, Variante_2): 
            mode_int = 2
            stock_ai = self.p1_stock
            stock_human = self.p2_stock

        # On appelle le C++ avec la signature buffer
        move_data = self.ai_engine.get_best_move(
            self._board, 
            self.difficulty, 
            mode_int, 
            p1_stock=stock_ai, 
            p2_stock=stock_human
        )
        
        col = move_data['col']
        kill_target = move_data['kill']

        # On exécute le coup selon la variante active
        if isinstance(self, Variante_2):
             self.play_ai_atomic_v2(col, kill_target)
        elif isinstance(self, Variante_1) and kill_target:
             self.play_ai_atomic_v1(col, kill_target)
        else:
             print(f"[IA] Coup Simple : Col {col}")
             self.play(col)

    @abstractmethod
    def play(self, move):
        """Méthode principale appelée par le contrôleur."""
        pass


class ClassicGame(Gestionnaire):
    name = "Classique"

    def play(self, move):
        if isinstance(move, tuple):
            column = move[1] 
        else:
            column = move    
        
        row = self.get_top_row(column)
        if row == -1:
            raise InvalidMove("Colonne pleine")

        self._board[row, column] = self._current_player
        
        if self.check_alignment(row, column, self._current_player, 4):
            self._victory = True
        elif np.all(self._board != 0):
            self._draw = True
        else:
            self._current_player *= -1


class Variante_1(Gestionnaire):
    name = "3 pour 1"

    def get_info_status(self, player: int) -> str:
        """Guide le joueur pendant la phase de destruction."""
        if self._event and player == self._current_player:
            return "Retirez un pion adverse"
        return None

    def _apply_gravity_column(self, col):
        new_col = [p for p in self._board[:, col] if p != 0]
        padding = [0] * (self._height - len(new_col))
        self._board[:, col] = np.array(padding + new_col)

    def play_ai_atomic_v1(self, col, kill_target):
        """L'IA joue et tue dans la même séquence (Spécifique V1)."""
        row = self.get_top_row(col)
        if row == -1: return 
        self._board[row, col] = self._current_player

        if kill_target:
            k_row, k_col = kill_target
            if self._board[k_row, k_col] != self._current_player and self._board[k_row, k_col] != 0:
                self._board[k_row, k_col] = 0
                self._apply_gravity_column(k_col)

        self.verify_victory_condition()
        if not self._victory and not self._draw:
            self._current_player *= -1

    def play(self, move):
        if isinstance(move, tuple):
            click_r, click_c = move
        else:
            click_r, click_c = -1, move

        # Phase de destruction (si l'événement est actif)
        if self._event:
            target_val = self._board[click_r, click_c]
            
            if target_val == 0:
                raise InvalidMove("Case vide.")
            if target_val == self._current_player:
                raise InvalidMove("Vous ne pouvez pas détruire votre propre pion !")

            self._board[click_r, click_c] = 0
            self._apply_gravity_column(click_c)
            
            self._event = False
            self.verify_victory_condition()
            
            if not self._victory and not self._draw:
                self._current_player *= -1
            return

        # Phase de pose du pion
        col = click_c
        row = self.get_top_row(col)
        if row == -1: raise InvalidMove("Colonne pleine")
        
        self._board[row, col] = self._current_player

        if self.check_alignment(row, col, self._current_player, 4):
            self._victory = True
            return

        if self.check_alignment(row, col, self._current_player, 3):
            self._event = True
            self._message_event = "BONUS ! Cliquez sur un pion ADVERSE pour le détruire."
            return

        if np.all(self._board != 0): self._draw = True
        else: self._current_player *= -1


class Variante_2(Gestionnaire):
    name = "3 pour 1 v2"

    def __init__(self, mode_solo=False, difficulty=4):
        super().__init__(mode_solo, difficulty)
        self.p1_stock = 0 # Stock Joueur IA/Rouge (1)
        self.p2_stock = 0 # Stock Joueur Humain/Jaune (-1)

    def get_info_status(self, player: int) -> str:
        """Affiche le stock de coups spéciaux."""
        stock = self.p1_stock if player == 1 else self.p2_stock
        return f"Coups Spéciaux : {stock}"

    def _apply_gravity_column(self, col):
        """Identique à V1 : Gestion de la gravité."""
        new_col = [p for p in self._board[:, col] if p != 0]
        padding = [0] * (self._height - len(new_col))
        self._board[:, col] = np.array(padding + new_col)

    def play_ai_atomic_v2(self, col, kill_target):
        """
        L'IA choisit soit de poser (col != -1), soit de tuer (kill_target != None).
        Elle ne fait pas les deux en même temps dans cette variante.
        """
        # Cas 1 : Destruction (L'IA utilise son stock)
        if kill_target and self.p1_stock > 0:
            k_row, k_col = kill_target
            target = self._board[k_row, k_col]
            
            if target == -1: 
                print(f"[IA-V2] Utilise un coup spécial en ({k_row}, {k_col})")
                self._board[k_row, k_col] = 0
                self._apply_gravity_column(k_col)
                self.p1_stock -= 1
                
                self.verify_victory_condition()
                if not self._victory and not self._draw:
                    self._current_player *= -1
                return
        
        # Cas 2 : Pose (L'IA pose un pion, faute de kill ou par choix)
        final_col = col if col != -1 else 0 # Fallback sécurité
        row = self.get_top_row(final_col)
        
        if row != -1:
            self._board[row, final_col] = self._current_player
            
            # Gain de stock si alignement de 3 sans victoire
            if self.check_alignment(row, final_col, self._current_player, 3):
                if not self.check_alignment(row, final_col, self._current_player, 4):
                    print("[IA-V2] Gagne +1 Stock")
                    self.p1_stock += 1

        self.verify_victory_condition()
        if not self._victory and not self._draw:
            self._current_player *= -1

    def play(self, move):
        """
        Tour Humain :
        - Clic sur case vide (colonne) -> Pose de pion.
        - Clic sur pion adverse -> Tentative de destruction (si stock > 0).
        """
        if isinstance(move, tuple):
            click_r, click_c = move
        else:
            # Cas rare (IA vs IA simulé ou test), on suppose une pose
            click_r, click_c = -1, move

        target_val = 0
        if click_r != -1:
            target_val = self._board[click_r, click_c]

        # Option A : Destruction (Clic sur Ennemi)
        if target_val != 0:
            if target_val == self._current_player:
                raise InvalidMove("C'est votre pion.")
            
            current_stock = self.p1_stock if self._current_player == 1 else self.p2_stock
            if current_stock <= 0:
                raise InvalidMove("Pas de coup spécial en stock (Alignez 3 pour en gagner).")

            self._board[click_r, click_c] = 0
            self._apply_gravity_column(click_c)
            
            if self._current_player == 1: self.p1_stock -= 1
            else: self.p2_stock -= 1

            self.verify_victory_condition()
            if not self._victory and not self._draw:
                self._current_player *= -1
            return

        # Option B : Pose (Clic sur Vide / Colonne)
        col = click_c
        row = self.get_top_row(col)
        if row == -1: raise InvalidMove("Colonne pleine")

        self._board[row, col] = self._current_player

        if self.check_alignment(row, col, self._current_player, 4):
            self._victory = True
            return

        # On ne gagne du stock que si on n'a pas gagné la partie
        if self.check_alignment(row, col, self._current_player, 3):
            if self._current_player == 1: self.p1_stock += 1
            else: self.p2_stock += 1
            self._message_event = "Alignement de 3 ! +1 Coup Spécial."
        
        if np.all(self._board != 0): self._draw = True
        else: self._current_player *= -1

variantes = [ClassicGame, Variante_1, Variante_2]
