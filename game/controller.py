from gamemanager import variantes, InvalidMove
from graphicinterface import Interface


class Controller:
    def __init__(self):
        self._interface = Interface()
        self._variantes = variantes
        self._gestionnaire = None
        self._in_menu = True
        self._in_game = False

    def start(self):
        while self._interface._running:
            if self._in_menu:
                self.menu_principal()
            elif self._in_game:
                self.game_loop()

    def menu_principal(self):
        # --- ETAPE 1 : Choix de la Variante ---
        choix_variante = self._interface.send_menu(
            "Bienvenue sur Puissance 4",
            [v.name for v in self._variantes]
        )

        if choix_variante is None: 
            return # Fenêtre fermée

        # --- ETAPE 2 : Choix du Nombre de Joueurs ---
        choix_mode = self._interface.send_menu(
            "Choisissez le mode de jeu",
            ["1 Joueur (Contre l'IA)", "2 Joueurs (Local)"]
        )

        if choix_mode is None: 
            return

        # Si choix_mode vaut 0, c'est "1 Joueur". Sinon c'est False.
        mode_solo = (choix_mode == 0)
        difficulty = 4 # Valeur par défaut

        # --- ETAPE 3 : Difficulté (Seulement si mode solo) ---
        if mode_solo:
            choix_diff = self._interface.send_menu(
                "Niveau de difficulté",
                ["Facile", "Moyen", "Difficile"]
            )
            if choix_diff is None: return
            
            # Mapping : Index -> Profondeur
            # Facile=2 (rapide, bête), Moyen=4, Difficile=6 (fort)
            niveaux = [2, 4, 6]
            difficulty = niveaux[choix_diff]

        # --- INITIALISATION ---
        # On crée le jeu avec TOUS les paramètres
        self._gestionnaire = self._variantes[choix_variante](
            mode_solo=mode_solo, 
            difficulty=difficulty
        )
        
        self._in_menu = False
        self._in_game = True

    def game_loop(self):
        # 1. L'Humain joue
        move = self._interface.send_game(
            self._gestionnaire.current_player,
            self._gestionnaire.board
        )

        if move is None:  # Retour Menu
            self._in_game = False
            self._in_menu = True
            self._gestionnaire = None
            return

        try:
            # --- TOUR HUMAIN ---
            self._gestionnaire.play(move)

            # Vérification fin de partie après coup humain
            if self.check_game_end():
                return 

            # --- TOUR IA (Seulement si activé) ---
            # On vérifie si mode_solo est True
            if getattr(self._gestionnaire, 'mode_solo', False):
                
                # A. On met à jour l'écran pour voir le pion de l'humain
                self._interface.refresh_only(self._gestionnaire.current_player, self._gestionnaire.board)
                
                # B. LE DÉLAI (Pause de 0.7 seconde pour le réalisme)
                self._interface.pause(700) 
                
                # C. L'IA réfléchit et joue
                self._gestionnaire.play_ai_turn()

                # Vérification fin de partie après coup IA
                if self.check_game_end():
                    return

        except InvalidMove:
            pass  # On recommence la boucle

    # --- NOUVELLE MÉTHODE UTILITAIRE ---
    # (Pour ne pas copier-coller le bloc de vérification deux fois)
    def check_game_end(self):
        """Regarde si le jeu est fini (Victoire, Egalité, Event). Retourne True si fini."""
        
        if self._gestionnaire.victory:
            self._interface.refresh_only(self._gestionnaire.current_player, self._gestionnaire.board)
            self._interface.notify_victory(self._gestionnaire.current_player)
            self._in_game = False
            self._in_menu = True
            return True

        elif self._gestionnaire.draw:
            self._interface.refresh_only(self._gestionnaire.current_player, self._gestionnaire.board)
            self._interface.notify_draw()
            self._in_game = False
            self._in_menu = True
            return True

        # Gestion des variantes (Event)
        elif getattr(self._gestionnaire, 'event', False):
            self._interface.notify_message(self._gestionnaire.message_event)
            self._interface.refresh_only(self._gestionnaire.current_player, self._gestionnaire.board)
            # Pas forcément fin de partie, donc on retourne False sauf si tu veux que l'event stoppe tout
            return False 

        return False
    
    
